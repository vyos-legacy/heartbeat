#
# heartbeat: Linux-HA heartbeat code
#
# Copyright (C) 2001 Michael Moerz
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
MAINTAINERCLEANFILES    = Makefile.in

INCLUDES                = -I$(top_builddir)/include -I$(top_srcdir)/include \
			  -I$(top_builddir)/linux-ha -I$(top_srcdir)/linux-ha \
			  -I$(top_builddir)/libltdl -I$(top_srcdir)/libltdl

EXTRA_DIST		= $(hanoarch_DATA) $(sbin_SCRIPTS)

apigid			= @HA_APIGID@
habindir		= @bindir@
halibdir		= $(libdir)/@HB_PKG@
hanoarchdir		= @HA_NOARCHDATAHBDIR@
gliblib			= @GLIBLIB@

habin_PROGRAMS		= cl_status cl_respawn 
hanoarch_DATA		= utillib.sh
sbin_SCRIPTS		= hb_report ocf-tester

cl_status_SOURCES	= cl_status.c
cl_status_LDADD		= $(top_builddir)/lib/hbclient/libhbclient.la	\
			  $(top_builddir)/lib/clplumbing/libplumb.la	\
			  $(gliblib)					\
			  $(top_builddir)/replace/libreplace.la

cl_respawn_SOURCES	= cl_respawn.c
cl_respawn_LDADD	= $(top_builddir)/lib/clplumbing/libplumb.la	\
			  $(top_builddir)/lib/apphb/libapphb.la 	\
			  $(gliblib)					\
			  $(top_builddir)/replace/libreplace.la

if USE_LIBNET
halib_PROGRAMS		= send_arp
send_arp_SOURCES	= send_arp.libnet.c
send_arp_CFLAGS		= @LIBNETDEFINES@
send_arp_LDADD	 	= $(top_builddir)/lib/clplumbing/libplumb.la	\
			  $(gliblib) @LIBNETLIBS@
else

if SENDARP_LINUX
halib_PROGRAMS		= send_arp
send_arp_SOURCES	= send_arp.linux.c
endif

endif

install-data-hook:    # install-exec-hook doesn't work (!)
	-chgrp $(apigid) $(DESTDIR)/$(habindir)/cl_status
	-chmod g+s,a-w $(DESTDIR)/$(habindir)/cl_status

.PHONY: install-exec-hook
