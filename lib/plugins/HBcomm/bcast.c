static const char _bcast_Id [] = "$Id: bcast.c,v 1.29 2003/01/31 10:02:09 lars Exp $";
/*
 * bcast.c: UDP/IP broadcast-based communication code for heartbeat.
 *
 * Copyright (C) 1999, 2000,2001 Alan Robertson <alanr@unix.sh>
 *
 * About 150 lines of the code in this file originally borrowed in
 * 1999 from Tom Vogt's "Heart" program, and significantly mangled by
 *	Alan Robertson <alanr@unix.sh>
 *	
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <portability.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <heartbeat.h>
#include <HBcomm.h>

#if defined(SO_BINDTODEVICE)
#	include <net/if.h>
#endif

#define PIL_PLUGINTYPE          HB_COMM_TYPE
#define PIL_PLUGINTYPE_S        HB_COMM_TYPE_S
#define PIL_PLUGIN              bcast
#define PIL_PLUGIN_S            "bcast"
#define PIL_PLUGINLICENSE 	LICENSE_LGPL
#define PIL_PLUGINLICENSEURL 	URL_LGPL
#include <pils/plugin.h>

struct ip_private {
        char *  interface;      /* Interface name */
	struct in_addr bcast;   /* Broadcast address */
        struct sockaddr_in      addr;   /* Broadcast addr */
        int     port;
        int     rsocket;        /* Read-socket */
        int     wsocket;        /* Write-socket */
};


static int		bcast_init(void);
struct hb_media*	bcast_new(const char* interface);
static int		bcast_open(struct hb_media* mp);
static int		bcast_close(struct hb_media* mp);
struct ha_msg*		bcast_read(struct hb_media* mp);
static int		bcast_write(struct hb_media* mp, struct ha_msg* msg);
static int		bcast_make_receive_sock(struct hb_media* ei);
static int		bcast_make_send_sock(struct hb_media * mp);
static struct ip_private *
			new_ip_interface(const char * ifn, int port);
static int		bcast_descr(char** buffer);
static int		bcast_mtype(char** buffer);
static int		bcast_isping(void);
static int		localudpport = -1;


int if_get_broadaddr(const char *ifn, struct in_addr *broadaddr);

/*
 * bcastclose is called as part of unloading the bcast HBcomm plugin.
 * If there was any global data allocated, or file descriptors opened, etc.
 * which is associated with the plugin, and not a single interface
 * in particular, here's our chance to clean it up.
 */

static void
bcastclosepi(PILPlugin*pi)
{
}


/*
 * bcastcloseintf called as part of shutting down the bcast HBcomm interface.
 * If there was any global data allocated, or file descriptors opened, etc.
 * which is associated with the bcast implementation, here's our chance
 * to clean it up.
 */
static PIL_rc
bcastcloseintf(PILInterface* pi, void* pd)
{
	return PIL_OK;
}

static struct hb_media_fns bcastOps ={
	bcast_new,	/* Create single object function */
	NULL,		/* whole-line parse function */
	bcast_open,
	bcast_close,
	bcast_read,
	bcast_write,
	bcast_mtype,
	bcast_descr,
	bcast_isping,
};

PIL_PLUGIN_BOILERPLATE("1.0", Debug, bcastclosepi);
static const PILPluginImports*  PluginImports;
static PILPlugin*               OurPlugin;
static PILInterface*		OurInterface;
static struct hb_media_imports*	OurImports;
static void*			interfprivate;

#define LOG	PluginImports->log
#define MALLOC	PluginImports->alloc
#define FREE	PluginImports->mfree

PIL_rc
PIL_PLUGIN_INIT(PILPlugin*us, const PILPluginImports* imports);

PIL_rc
PIL_PLUGIN_INIT(PILPlugin*us, const PILPluginImports* imports)
{
	/* Force the compiler to do a little type checking */
	(void)(PILPluginInitFun)PIL_PLUGIN_INIT;

	PluginImports = imports;
	OurPlugin = us;

	/* Register ourself as a plugin */
	imports->register_plugin(us, &OurPIExports);  

	/*  Register our interface implementation */
 	return imports->register_interface(us, PIL_PLUGINTYPE_S
	,	PIL_PLUGIN_S
	,	&bcastOps
	,	bcastcloseintf		/*close */
	,	&OurInterface
	,	(void*)&OurImports
	,	interfprivate); 
}

#define		ISBCASTOBJECT(mp) ((mp) && ((mp)->vf == (void*)&bcastOps))
#define		BCASTASSERT(mp)	g_assert(ISBCASTOBJECT(mp))

static int 
bcast_mtype(char** buffer) { 
	
	*buffer = MALLOC(sizeof(PIL_PLUGIN_S));

	strcpy(*buffer, PIL_PLUGIN_S);

	return STRLEN(PIL_PLUGIN_S);
}

static int
bcast_descr(char **buffer) { 

	const char* str = "UDP/IP broadcast";	

	*buffer = MALLOC((strlen(str) * sizeof(char)) + 1);

	strcpy(*buffer, str);

	return strlen(str);
}

static int
bcast_isping(void) {
    return 0;
}

static int
bcast_init(void)
{
	struct servent*	service;

	(void)_heartbeat_h_Id;
	(void)_bcast_Id;
	(void)_ha_msg_h_Id;

	g_assert(OurImports != NULL);

	if (localudpport <= 0) {
		const char *	chport;
		if ((chport  = OurImports->ParamValue("udpport")) != NULL) {
			sscanf(chport, "%d", &localudpport);
			if (localudpport <= 0) {
				PILCallLog(LOG, PIL_CRIT
				,	"bad port number %s"
				,	chport);
				return HA_FAIL;
			}
		}
	}

	/* No port specified in the configuration... */

	if (localudpport <= 0) {
		/* If our service name is in /etc/services, then use it */
		if ((service=getservbyname(HA_SERVICENAME, "udp")) != NULL){
			localudpport = ntohs(service->s_port);
		}else{
			localudpport = UDPPORT;
		}
	}
	return(HA_OK);
}

/*
 *	Create new UDP/IP broadcast heartbeat object 
 *	Name of interface is passed as a parameter
 */
struct hb_media *
bcast_new(const char * intf)
{
	struct ip_private*	ipi;
	struct hb_media *	ret;

	bcast_init();
	ipi = new_ip_interface(intf, localudpport);
	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG, "bcast_new: attempting to open %s:%d", intf
		,	localudpport);
	}

	if (ipi == NULL) {
		PILCallLog(LOG, PIL_CRIT, "IP interface [%s] does not exist"
		,	intf);
		return(NULL);
	}
	ret = (struct hb_media*) MALLOC(sizeof(struct hb_media));
	if (ret != NULL) {
		char * name;
		memset(ret, 0, sizeof(*ret));
		ret->pd = (void*)ipi;
		name = MALLOC(strlen(intf)+1);
		strcpy(name, intf);
		ret->name = name;
		if (DEBUGPKT) {
			PILCallLog(LOG, PIL_DEBUG, "bcast_new: returning ret (%s)", ret->name);
		}
	}else{
		FREE(ipi->interface);
		FREE(ipi);
		if (DEBUGPKT) {
			PILCallLog(LOG, PIL_DEBUG, "bcast_new: ret was NULL");
		}
	}
	return(ret);
}

/*
 *	Open UDP/IP broadcast heartbeat interface
 */
static int
bcast_open(struct hb_media* mp)
{
	struct ip_private * ei;

	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;

	if ((ei->wsocket = bcast_make_send_sock(mp)) < 0) {
		return(HA_FAIL);
	}
	if ((ei->rsocket = bcast_make_receive_sock(mp)) < 0) {
		bcast_close(mp);
		return(HA_FAIL);
	}
	PILCallLog(LOG, PIL_INFO
	,	"UDP Broadcast heartbeat started on port %d (%d) interface %s"
	,	localudpport, ei->port, mp->name);

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_open : Socket %d opened for reading"
		", socket %d opened for writing."
		,	ei->rsocket, ei->wsocket);
	}

	return(HA_OK);
}

/*
 *	Close UDP/IP broadcast heartbeat interface
 */
static int
bcast_close(struct hb_media* mp)
{
	struct ip_private * ei;
	int	rc = HA_OK;

	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;

	if (ei->rsocket >= 0) {
		if (close(ei->rsocket) < 0) {
			rc = HA_FAIL;
		}
	}
	if (ei->wsocket >= 0) {
		if (close(ei->wsocket) < 0) {
			rc = HA_FAIL;
		}
	}
	PILCallLog(LOG, PIL_INFO
	, "UDP Broadcast heartbeat closed on port %d interface %s - Status: %d"
	,	localudpport, mp->name, rc);

	return(rc);
}
/*
 * Receive a heartbeat broadcast packet from BCAST interface
 */

struct ha_msg *
bcast_read(struct hb_media* mp)
{
	struct ip_private *	ei;
	char			buf[MAXLINE];
	int			addr_len = sizeof(struct sockaddr);
   	struct sockaddr_in	their_addr; /* connector's addr information */
	int	numbytes;

	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_read : reading from socket %d (writing to socket %d)"
		,	ei->rsocket, ei->wsocket);
	}

	if ((numbytes=recvfrom(ei->rsocket, buf, MAXLINE-1, MSG_WAITALL
	,	(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		if (errno != EINTR) {
			PILCallLog(LOG, PIL_CRIT
			,	"Error receiving from socket: %s"
			,	strerror(errno));
		}
		return NULL;
	}
	/* Avoid possible buffer overruns */
	buf[numbytes] = EOS;

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG, "got %d byte packet from %s"
		,	numbytes, inet_ntoa(their_addr.sin_addr));
	}
	if (DEBUGPKTCONT && numbytes > 0) {
		PILCallLog(LOG, PIL_DEBUG, "%s", buf);
	}
	return(string2msg(buf, sizeof(buf)));
}

/*
 * Send a heartbeat packet over broadcast UDP/IP interface
 */

static int
bcast_write(struct hb_media* mp, struct ha_msg * msgptr)
{
	struct ip_private *	ei;
	int			rc;
	char*			pkt;
	int			size;

	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;

	if ((pkt = msg2string(msgptr)) == NULL)  {
		return(HA_FAIL);
	}
	size = strlen(pkt)+1;

	if ((rc=sendto(ei->wsocket, pkt, size, 0
	,	(struct sockaddr *)&ei->addr
	,	sizeof(struct sockaddr))) != size) {
		PILCallLog(LOG, PIL_CRIT, "Error sending packet: %s"
		,	strerror(errno));
		ha_free(pkt);
		return(HA_FAIL);
	}

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_write : writing %d bytes to %s (socket %d)"
		,	rc, inet_ntoa(ei->addr.sin_addr), ei->wsocket);
   	}

	if (DEBUGPKTCONT) {
		PILCallLog(LOG, PIL_DEBUG, pkt);
   	}
	ha_free(pkt);
	return(HA_OK);
}

/*
 * Set up socket for sending broadcast UDP heartbeats
 */

static int
bcast_make_send_sock(struct hb_media * mp)
{
	int sockfd, one = 1;
	struct ip_private * ei;
	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		PILCallLog(LOG, PIL_CRIT
		,	"Error getting socket: %s", strerror(errno));
		return(sockfd);
   	}

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_make_send_sock: Opened socket %d", sockfd);
	}

	/* Warn that we're going to broadcast */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const void *) &one, sizeof(one))==-1){
		PILCallLog(LOG, PIL_CRIT
		,	"Error setting socket option SO_BROADCAST: %s"
		,	strerror(errno));
		close(sockfd);
		return(-1);
	}

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_make_send_sock: Modified %d"
		" Added option SO_BROADCAST."
			, sockfd);
	}

#if defined(SO_DONTROUTE) && !defined(USE_ROUTING)
	/* usually, we don't want to be subject to routing. */
	if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE,(const void *) &one,sizeof(int))==-1) {
		PILCallLog(LOG, PIL_CRIT
		,	"Error setting socket option SO_DONTROUTE: %s"
		,	strerror(errno));
		close(sockfd);
		return(-1);
	}

	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG, "bcast_make_send_sock:"
		" Modified %d Added option SO_DONTROUTE."
			, sockfd);
	}

#endif
#if defined(SO_BINDTODEVICE)
	{
		/*
		 *  We want to send out this particular interface
		 *
		 * This is so we can have redundant NICs, and heartbeat on both
		 */
		struct ifreq i;
		strcpy(i.ifr_name,  mp->name);

		if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE
		,	(const void *) &i, sizeof(i)) == -1) {
			PILCallLog(LOG, PIL_CRIT
			,	"Error setting socket option SO_BINDTODEVICE"
			": %s"
			,	strerror(errno));
			close(sockfd);
			return(-1);
		}

		if (DEBUGPKT) {
			PILCallLog(LOG, PIL_DEBUG
			, "bcast_make_send_sock: Modified %d"
			" Added option SO_BINDTODEVICE."
			,	sockfd);
		}

	}
#endif
	if (fcntl(sockfd,F_SETFD, FD_CLOEXEC)) {
		PILCallLog(LOG, PIL_CRIT
		,	"Error setting close-on-exec flag: %s"
		,	strerror(errno));
	}
	return(sockfd);
}

/*
 * Set up socket for listening to heartbeats (UDP broadcasts)
 */

#define	MAXBINDTRIES	10
static int
bcast_make_receive_sock(struct hb_media * mp) {

	struct ip_private * ei;
	struct sockaddr_in my_addr;    /* my address information */
	int	sockfd;
	int	bindtries;
	int	boundyet=0;
	int	j;

	BCASTASSERT(mp);
	ei = (struct ip_private *) mp->pd;
	memset(&(my_addr), 0, sizeof(my_addr));	/* zero my address struct */
	my_addr.sin_family = AF_INET;		/* host byte order */
	my_addr.sin_port = htons(ei->port);	/* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY;	/* auto-fill with my IP */

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		PILCallLog(LOG, PIL_CRIT, "Error getting socket: %s"
		,	strerror(errno));
		return(-1);
	}
	/* 
 	 * Set SO_REUSEADDR on the server socket s. Variable j is used
 	 * as a scratch varable.
 	 *
 	 * 16th February 2000
 	 * Added by Horms <horms@vergenet.net>
 	 * with thanks to Clinton Work <work@scripty.com>
 	 */
	j = 1;
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, (const void *)&j, sizeof j) <0){
		/* Ignore it.  It will almost always be OK anyway. */
		PILCallLog(LOG, PIL_CRIT
		,	"Error setting socket option SO_REUSEADDR: %s"
		,	strerror(errno));
	}        

        if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_make_receive_sock: Modified %d Added option SO_REUSEADDR."
		, sockfd);
	}

#if defined(SO_BINDTODEVICE)
	{
		/*
		 *  We want to receive packets only from this interface...
		 */
		struct ifreq i;
		strcpy(i.ifr_name,  ei->interface);

		if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE
		,	(const void *)&i, sizeof(i)) == -1) {
			PILCallLog(LOG, PIL_CRIT
			,	"Error setting socket option"
			" SO_BINDTODEVICE(r) on %s: %s"
			,	i.ifr_name, strerror(errno));
			close(sockfd);
			return(-1);
		}
		if (ANYDEBUG) {
			PILCallLog(LOG, PIL_DEBUG
			,	"SO_BINDTODEVICE(r) set for device %s"
			,	i.ifr_name);
		}
	}
#endif

	/* Try binding a few times before giving up */
	/* Sometimes a process with it open is exiting right now */

	for(bindtries=0; !boundyet && bindtries < MAXBINDTRIES; ++bindtries) {
		if (bind(sockfd, (struct sockaddr *)&my_addr
		,	sizeof(struct sockaddr)) < 0) {
			PILCallLog(LOG, PIL_CRIT
			,	"Error binding socket (%s). Retrying."
			,	strerror(errno));
			sleep(1);
		}else{
			boundyet = 1;
		}
	}
	if (!boundyet) {
#if !defined(SO_BINDTODEVICE)
		if (errno == EADDRINUSE) {
			/* This happens with multiple bcast or ppp interfaces */
			PILCallLog(LOG, PIL_INFO
			,	"Someone already listening on port %d [%s]"
			,	ei->port
			,	ei->interface);
			PILCallLog(LOG, PIL_INFO, "BCAST read process exiting");
			close(sockfd);
			cleanexit(0);
		}
#else
		PILCallLog(LOG, PIL_CRIT
		,	"Unable to bind socket (%s). Giving up."
		,	strerror(errno));
		close(sockfd);
		return(-1);
#endif
	}
	if (fcntl(sockfd,F_SETFD, FD_CLOEXEC)) {
		PILCallLog(LOG, PIL_CRIT
		,	"Error setting the close-on-exec flag: %s"
		,	strerror(errno));
	}
	if (DEBUGPKT) {
		PILCallLog(LOG, PIL_DEBUG
		,	"bcast_make_receive_sock: Returning %d", sockfd);
	}
	return(sockfd);
}



static struct ip_private *
new_ip_interface(const char * ifn, int port)
{
	struct ip_private * ep;
	struct in_addr broadaddr;

	/* Fetch the broadcast address for this interface */
	if (if_get_broadaddr(ifn, &broadaddr) < 0) {
		/* this function whines about problems... */
		return (NULL);
	}
	
	/*
	 * We now have all the information we need.  Populate our
	 * structure with the information we've gotten.
	 */

	ep = (struct ip_private *)MALLOC(sizeof(struct ip_private));
	if (ep == NULL)  {
		return(NULL);
	}

	ep->bcast = broadaddr;

	ep->interface = (char *)MALLOC(strlen(ifn)+1);
	if(ep->interface == NULL) {
		FREE(ep);
		return(NULL);
	}
	strcpy(ep->interface, ifn);
	
	memset(&ep->addr, 0, sizeof(ep->addr));	/* zero the struct */
	ep->addr.sin_family = AF_INET;		/* host byte order */
	ep->addr.sin_port = htons(port);	/* short, network byte order */
	ep->port = port;
	ep->wsocket = -1;
	ep->rsocket = -1;
	ep->addr.sin_addr = ep->bcast;
	return(ep);
}


/*
 * ha_if.c - code that extracts information about a network interface
 *
 * See the linux ifconfig source code for more examples.
 *
 * Works on HP_UX 10.20, freebsd, linux rh6.2
 * Works on solaris or Unixware (SVR4) with:
 *   gcc -DBSD_COMP -c ha_if.c
 * Doesn't seem to work at all on Digital Unix (???)
 *
 * Author: Eric Z. Ayers <eric.ayers@compgen.com>
 *
 * Copyright (C) 2000 Computer Generation Incorporated
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */



#include <sys/ioctl.h>


#ifdef HAVE_SYS_SOCKIO_H
#	include <sys/sockio.h>
#endif


/*
  if_get_broadaddr
     Retrieve the ipv4 broadcast address for the specified network interface.

  Inputs:  ifn - the name of the network interface:
                 e.g. eth0, eth1, ppp0, plip0, plusb0 ...
  Outputs: broadaddr - returned broadcast address.

  Returns: 0 on success
           -1 on failure - sets errno.
 */
int
if_get_broadaddr(const char *ifn, struct in_addr *broadaddr)
{
	int		return_val;
	int		fd = -1;
	struct ifreq ifr; /* points to one interface returned from ioctl */

	/* get rid of compiler warnings about unreferenced variables */
	(void)_heartbeat_h_Id;
	(void)_ha_msg_h_Id;
	
	fd = socket (PF_INET, SOCK_DGRAM, 0);
	
	if (fd < 0) {
		PILCallLog(LOG, PIL_CRIT
		,	"Error opening socket for interface %s: %s"
		,	ifn, strerror(errno));
		return -1;
	}
	
	strncpy (ifr.ifr_name, ifn, sizeof(ifr.ifr_name));

	/* Fetch the broadcast address of this interface by calling ioctl() */
	return_val = ioctl(fd,SIOCGIFBRDADDR, &ifr);
	
	if (return_val == 0 ) {
		if (ifr.ifr_broadaddr.sa_family == AF_INET) {
			struct sockaddr_in sin_ptr;
			
			memcpy(&sin_ptr, &ifr.ifr_broadaddr,
				sizeof(sin_ptr));
			
			memcpy(broadaddr, &sin_ptr.sin_addr,
				sizeof(*broadaddr));
			
			/* leave return_val set to 0 to return success! */
		}else{
			PILCallLog(LOG, PIL_CRIT
			,	"Wrong family for broadcast interface %s: %s"
			,	ifn, strerror(errno));
			return_val = -1;
		}
		
	}else{
		PILCallLog(LOG, PIL_CRIT
		,	"Get broadcast for interface %s failed: %s"
		,	ifn, strerror(errno));
		return_val = -1;
	}
	
	close (fd);

	return return_val;
	
} /* end if_get_broadaddr() */

/*
 * $Log: bcast.c,v $
 * Revision 1.29  2003/01/31 10:02:09  lars
 * Various small code cleanups:
 * - Lots of "signed vs unsigned" comparison fixes
 * - time_t globally replaced with TIME_T
 * - All seqnos moved to "seqno_t", which defaults to unsigned long
 * - DIMOF() definition centralized to portability.h and typecast to int
 * - EOS define moved to portability.h
 * - dropped inclusion of signal.h from stonith.h, so that sigignore is
 *   properly defined
 *
 * Revision 1.28  2003/01/08 21:14:08  msoffen
 * Fixed problems that prevented compiling with -Wtraditional
 *
 * Revision 1.27  2002/10/22 17:41:58  alan
 * Added some documentation about deadtime, etc.
 * Switched one of the sets of FIFOs to IPC channels.
 * Added msg_from_IPC to ha_msg.c make that easier.
 * Fixed a few compile errors that were introduced earlier.
 * Moved hb_api_core.h out of the global include directory,
 * and back into a local directory.  I also make sure it doesn't get
 * installed.  This *shouldn't* cause problems.
 * Added a ipc_waitin() function to the IPC code to allow you to wait for
 * input synchronously if you really want to.
 * Changes the STONITH test to default to enabled.
 *
 * Revision 1.26  2002/10/02 13:40:26  alan
 * Fixed some potential holes pointed out by Nathan Wallwork.
 *
 * Revision 1.25  2002/09/19 22:40:17  alan
 * Changed a few error return checks to not print anything and return
 * if an error was encountered.
 * Changed a few debug messages to only print if a strictly positive number
 * of chars was received.
 *
 * Revision 1.24  2002/09/12 03:52:07  alan
 * Fixed up a comment :-(.
 *
 * Revision 1.23  2002/09/12 03:39:45  alan
 * Fixed some logging level names in the code and also
 * fixed an error in the license chosen for a file.
 *
 * Revision 1.22  2002/09/11 07:11:23  alan
 * Updated a good bit of code to use logging functions more uniformly.
 * Some used printfs, and some used ha_log, and some used the PILS
 * function pointers (which sidesteps the printf argument checking).
 *
 * Revision 1.21  2002/08/07 18:20:33  msoffen
 * Cleaned up many warning messages from FreeBSD and Solaris.
 * Cleaned up signal calls with SIG_IGN for Solaris to use sigignore function (to remove some warnings).
 *
 * Revision 1.20  2002/07/16 17:10:32  msoffen
 * Changed so that Lars shouldn't have the issue with the struct copy.
 *
 * Revision 1.19  2002/07/16 16:18:15  msoffen
 * -Re-added in LArs's comments.
 *
 * Revision 1.18  2002/07/16 16:16:06  msoffen
 * Changed to NOT assume that the address is 1st.
 *
 * Revision 1.17  2002/07/16 11:47:53 lars
 * Type and alignment fixes for IA64, x86_64, sparc, s390 and PPC(64).
 *
 * Revision 1.16  2002/06/16 06:11:26  alan
 * Put in a couple of changes to the PILS interfaces
 *  - exported license information (name, URL)
 *  - imported malloc/free
 *
 * Revision 1.15  2002/06/16 03:42:53  alan
 * Incorporated code from St�phane Billiart to add a unicast heartbeat capability
 * to heartbeat.
 * Fixed a small bug in bcast.c where the "old" logging function was called
 * instead of the new one.
 *
 * Revision 1.14  2002/05/01 23:50:35  alan
 * Put in some comments about how the code avoids potential buffer overruns.
 *
 * Revision 1.13  2002/04/13 22:35:08  alan
 * Changed ha_msg_add_nv to take an end pointer to make it safer.
 * Added a length parameter to string2msg so it would be safer.
 * Changed the various networking plugins to use the new string2msg().
 *
 * Revision 1.12  2002/04/09 12:45:36  alan
 * Put in changes to the bcast, mcast and serial code such that
 * interrupted system calls in reads are ignored.
 *
 * Revision 1.11  2002/03/25 23:54:52  alan
 * Put in fixes to two bugs noted by Gamid Isayev of netzilla networks.
 * One of the two fixes is also due to him.
 *
 * Revision 1.10  2002/03/25 23:19:36  horms
 * Patches from Gamid Isayev to
 *
 * 1) Avoid shadowing the udpport global, hence this
 *    configuration option should now work.
 *
 * 2) Call getservbyname with a vaild protocol (udp), this should
 *    also allow the udpport configuration option to work.
 *
 * --
 * Horms
 *
 * Revision 1.9  2001/10/03 05:33:28  alan
 * Added a little more error checking for the broadcast port number...
 *
 * Revision 1.8  2001/10/02 20:15:41  alan
 * Debug code, etc. from Matt Soffen...
 *
 * Revision 1.7  2001/10/02 15:52:20  alan
 * Replaced bzero with memset...  This is a minor portability issue...
 *
 * Revision 1.6  2001/10/02 05:12:19  alan
 * Various portability fixes (make warnings go away) for Solaris.
 *
 * Revision 1.5  2001/09/07 16:18:17  alan
 * Updated ping.c to conform to the new plugin loading system.
 * Changed log messages in bcast, mcast, ping and serial to use the
 * new logging function.
 *
 * Revision 1.4  2001/08/15 16:17:12  alan
 * Fixed the code so that serial comm plugins build/load/work.
 *
 * Revision 1.3  2001/08/15 02:14:49  alan
 * Put in #include net/if.h needed for FreeBSD and solaris
 *
 * Revision 1.2  2001/08/11 01:40:54  alan
 * Removed the old copy of the heartbeat.h file, and removed a
 * blank line from a file... ;-)
 *
 * Revision 1.1  2001/08/10 17:16:44  alan
 * New code for the new plugin loading system.
 *
 * Revision 1.20  2001/06/19 13:56:28  alan
 * FreeBSD portability patch from Matt Soffen.
 * Mainly added #include "portability.h" to lots of files.
 * Also added a library to Makefile.am
 */
