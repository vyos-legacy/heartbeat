/* 
 * Copyright (C) 2004 Andrew Beekhof <andrew@beekhof.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <sys/param.h>
#include <crm/crm.h>
#include <string.h>
#include <crmd_fsa.h>
#include <libxml/tree.h>

#include <heartbeat.h>

#include <hb_api.h>
#include <crm/msg_xml.h>
#include <crm/common/xml.h>
#include <crm/common/msg.h>
#include <crm/cib.h>

#include <crmd.h>
#include <crmd_messages.h>
#include <crmd_callbacks.h>

#include <crm/dmalloc_wrapper.h>

FILE *msg_in_strm = NULL;
FILE *msg_ipc_strm = NULL;

xmlNodePtr find_xml_in_hamessage(const struct ha_msg* msg);
gboolean crmd_ha_input_dispatch(int fd, gpointer user_data);
void crmd_ha_input_destroy(gpointer user_data);

void
crmd_ha_input_callback(const struct ha_msg* msg, void* private_data)
{
	const char *to = NULL;
	char *xml_text = NULL;
	xmlNodePtr root_xml_node = NULL;
	const char *from = ha_msg_value(msg, F_ORIG);
	const char *seq  = ha_msg_value(msg, F_SEQ);

#ifdef MSG_LOG
	if(msg_in_strm == NULL) {
		msg_in_strm = fopen(DEVEL_DIR"/inbound.log", "w");
	}
#endif

	if(safe_str_eq(from, fsa_our_uname)) {
#ifdef MSG_LOG
		fprintf(msg_in_strm,
			"Discarded message [F_SEQ=%s] from ourselves.\n", seq);
		fflush(msg_in_strm);
#endif
		return;
	} else if(from == NULL) {
		crm_err("Value of %s was NULL", F_ORIG);
	}
	
	root_xml_node = find_xml_in_hamessage(msg);
	to = xmlGetProp(root_xml_node, XML_ATTR_HOSTTO);

#ifdef MSG_LOG
	xml_text = dump_xml_formatted(root_xml_node);
	fprintf(msg_in_strm, "[%s (%s:%s)]\t%s\n", crm_str(from),
		seq,
		ha_msg_value(msg, F_TYPE),
		xml_text
		);
	fflush(msg_in_strm);
	crm_free(xml_text);
#endif
	
	if(to != NULL && strlen(to) > 0 && strcmp(to, fsa_our_uname) != 0) {
#ifdef MSG_LOG
		fprintf(msg_in_strm,
			"Discarding message [F_SEQ=%s] for someone else", seq);
#endif
		return;
	}

	set_xml_property_copy(root_xml_node, XML_ATTR_HOSTFROM, from);
	set_xml_property_copy(root_xml_node, "F_SEQ", seq);
	s_crmd_fsa(C_HA_MESSAGE, I_ROUTER, root_xml_node);

	free_xml(root_xml_node);

	return;
}

/*
 * Apparently returning TRUE means "stay connected, keep doing stuff".
 * Returning FALSE means "we're all done, close the connection"
 */
gboolean
crmd_ipc_input_callback(IPC_Channel *client, gpointer user_data)
{
	int lpc = 0;
	char *buffer = NULL;
	IPC_Message *msg = NULL;
	gboolean hack_return_good = TRUE;
	xmlNodePtr root_xml_node;
	crmd_client_t *curr_client = (crmd_client_t*)user_data;

	
	crm_verbose("Processing IPC message from %s",
		   curr_client->table_key);

#ifdef MSG_LOG
	if(msg_ipc_strm == NULL) {
		msg_ipc_strm = fopen(DEVEL_DIR"/inbound.ipc.log", "w");
	}
#endif

	while(client->ops->is_message_pending(client)) {
		if (client->ch_status == IPC_DISCONNECT) {
			/* The message which was pending for us is that
			 * the IPC status is now IPC_DISCONNECT */
			break;
		}
		if (client->ops->recv(client, &msg) != IPC_OK) {
			perror("Receive failure:");
#ifdef MSG_LOG
			fprintf(msg_ipc_strm, "[%s] [receive failure]\n",
				curr_client->table_key);
			fflush(msg_in_strm);
#endif
			return !hack_return_good;
		}
		if (msg == NULL) {
#ifdef MSG_LOG
			fprintf(msg_ipc_strm, "[%s] [__nothing__]\n",
				curr_client->table_key);
			fflush(msg_in_strm);
#endif
			crm_err("No message this time");
			continue;
		}

		lpc++;
		buffer = (char*)msg->msg_body;
		crm_verbose("Processing xml from %s [text=%s]\n",
			   curr_client->table_key, buffer);
	
#ifdef MSG_LOG
		fprintf(msg_ipc_strm, "[%s] [text=%s]\n",
			curr_client->table_key, buffer);
		fflush(msg_in_strm);
#endif

		root_xml_node = find_xml_in_ipcmessage(msg, FALSE);
	
		if (root_xml_node != NULL) {
			if (crmd_authorize_message(
				    root_xml_node, msg, curr_client)) {
				s_crmd_fsa(
					C_IPC_MESSAGE,I_ROUTER,root_xml_node);
			}

		} else {
			crm_info("IPC Message was not valid... discarding.");
		}
		free_xml(root_xml_node);
		msg->msg_done(msg);
		
		msg = NULL;
		buffer = NULL;
		root_xml_node = NULL;
	}

	crm_verbose("Processed %d messages", lpc);
    
	if (client->ch_status == IPC_DISCONNECT)
	{
		crm_info("received HUP from %s",
			 curr_client->table_key);
		if (curr_client != NULL) {
			struct crm_subsystem_s *the_subsystem = NULL;
			
			if (curr_client->sub_sys == NULL) {
				crm_warn("Client hadn't registered with us yet");

			} else if (strcmp(CRM_SYSTEM_PENGINE,
					  curr_client->sub_sys) == 0) {
				the_subsystem = pe_subsystem;

			} else if (strcmp(CRM_SYSTEM_TENGINE,
					  curr_client->sub_sys) == 0) {
				the_subsystem = te_subsystem;

			} else if (strcmp(CRM_SYSTEM_CIB,
					  curr_client->sub_sys) == 0){
				the_subsystem = cib_subsystem;
			}
			
			if(the_subsystem != NULL) {
				cleanup_subsystem(the_subsystem);
			} /* else that was a transient client */
			
			if (curr_client->table_key != NULL) {
				/*
				 * Key is destroyed below:
				 *	curr_client->table_key
				 * Value is cleaned up by:
				 *	G_main_del_IPC_Channel
				 */
				g_hash_table_remove(
					ipc_clients, curr_client->table_key);
			}


			if(curr_client->client_source != NULL) {
				gboolean det = G_main_del_IPC_Channel(
					curr_client->client_source);
			
				crm_verbose("crm_client was %s detached",
					   det?"successfully":"not");
			}
			
			crm_free(curr_client->table_key);
			crm_free(curr_client->sub_sys);
			crm_free(curr_client->uuid);
			crm_free(curr_client);
		}
		return !hack_return_good;
	}
    
	return hack_return_good;
}


void
lrm_op_callback (lrm_op_t* op)
{
	s_crmd_fsa(C_LRM_OP_CALLBACK, I_LRM_EVENT, op);
}

void
crmd_client_status_callback(const char * node, const char * client,
		 const char * status, void * private)
{
	const char    *join = NULL;
	const char   *extra = NULL;
	xmlNodePtr   update = NULL;
	xmlNodePtr fragment = NULL;
	xmlNodePtr msg_options = NULL;
	xmlNodePtr request = NULL;

	if(safe_str_eq(status, JOINSTATUS)){
		status = ONLINESTATUS;
		extra  = XML_CIB_ATTR_CLEAR_SHUTDOWN;

	} else if(safe_str_eq(status, LEAVESTATUS)){
		status = OFFLINESTATUS;
		join   = CRMD_JOINSTATE_DOWN;
		extra  = XML_CIB_ATTR_CLEAR_SHUTDOWN;
	}
	
	crm_notice("Status update: Client %s/%s now has status [%s]\n",
		   node, client, status);

	if(AM_I_DC == FALSE) {
		crm_debug("Got client status callback in non-DC mode");

	} else {
		update = create_node_state(node, node, NULL, status, join);

		set_xml_property_copy(update, extra, XML_BOOLEAN_TRUE);
		
		fragment = create_cib_fragment(update, NULL);

		msg_options = set_xml_attr(
			NULL, XML_TAG_OPTIONS, XML_ATTR_OP, CRM_OP_UPDATE, TRUE);
		
		request = create_request(
			msg_options, fragment, NULL,
			CRM_SYSTEM_DCIB, CRM_SYSTEM_DC, NULL, NULL);

		crm_xml_debug(fragment, "Client status update");
		
		free_xml(fragment);
		free_xml(update);

		s_crmd_fsa(C_CRMD_STATUS_CALLBACK, I_CIB_OP, request);
		
	}
}


xmlNodePtr
find_xml_in_hamessage(const struct ha_msg* msg)
{
	const char *xml;
   	xmlDocPtr doc;
	xmlNodePtr root;

	
	if (msg == NULL) {
		crm_info("**** ha_crm_msg_callback called on a NULL message");
		return NULL;
	}

#if 0
	crm_debug("[F_TYPE=%s]", ha_msg_value(msg, F_TYPE));
	crm_debug("[F_ORIG=%s]", ha_msg_value(msg, F_ORIG));
	crm_debug("[F_TO=%s]", ha_msg_value(msg, F_TO));
	crm_debug("[F_COMMENT=%s]", ha_msg_value(msg, F_COMMENT));
	crm_debug("[F_XML=%s]", ha_msg_value(msg, "xml"));
/*    crm_debug("[F_=%s]", ha_msg_value(ha_msg, F_)); */
#endif
	
	if (strcmp("CRM", ha_msg_value(msg, F_TYPE)) != 0) {
		crm_info("Received a (%s) message by mistake.",
		       ha_msg_value(msg, F_TYPE));
		return NULL;
	}
	xml = ha_msg_value(msg, "xml");
	if (xml == NULL) {
		crm_info("No XML attached to this message.");
		return NULL;
	}
	doc = xmlParseMemory(xml, strlen(xml));
	if (doc == NULL) {
		crm_info("XML Buffer was not valid.");
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		crm_info("Root node was NULL.");
		return NULL;
	}
	return root;
}

gboolean lrm_dispatch(int fd, gpointer user_data)
{
	ll_lrm_t *lrm = (ll_lrm_t*)user_data;
	lrm->lrm_ops->rcvmsg(lrm, FALSE);
	return TRUE;
}

#define MAX_EMPTY_CALLBACKS 20
int empty_callbacks = 0;

gboolean
crmd_ha_input_dispatch(int fd, gpointer user_data)
{
	int lpc = 0;
	ll_cluster_t *hb_cluster = (ll_cluster_t*)user_data;
    
	while(hb_cluster->llc_ops->msgready(hb_cluster)) {
		lpc++;
		empty_callbacks = 0;
		/* invoke the callbacks but dont block */
		hb_cluster->llc_ops->rcvmsg(hb_cluster, 0);
	}

	if(lpc == 0) {
		/* hey what happened?? */
		crm_warn("We were called but no message was ready."
		       "  Likely the connection to Heartbeat failed,"
			" check the logs.");

		if(empty_callbacks++ > MAX_EMPTY_CALLBACKS) {
			crm_err("%d empty callbacks received..."
				" considering heartbeat dead",
				MAX_EMPTY_CALLBACKS);

			/* s_crmd_fsa(C_HA_DISCONNECT, I_ERROR, NULL); */

			return FALSE;
		}
	}
	
    
	return TRUE;
}

void
crmd_ha_input_destroy(gpointer user_data)
{
	crm_crit("Heartbeat has left us");
	/* this is always an error */
	/* feed this back into the FSA */
	s_crmd_fsa(C_HA_DISCONNECT, I_ERROR, NULL);
}


gboolean
crmd_client_connect(IPC_Channel *client_channel, gpointer user_data)
{
	if (client_channel == NULL) {
		crm_err("Channel was NULL");

	} else if (client_channel->ch_status == IPC_DISCONNECT) {
		crm_err("Channel was disconnected");

	} else {
		crmd_client_t *blank_client = NULL;
		crm_malloc(blank_client, sizeof(crmd_client_t));
	
		if (blank_client == NULL) {
			return FALSE;
		}
		
		client_channel->ops->set_recv_qlen(client_channel, 100);
		client_channel->ops->set_send_qlen(client_channel, 100);
	
		blank_client->client_channel = client_channel;
		blank_client->sub_sys   = NULL;
		blank_client->uuid      = NULL;
		blank_client->table_key = NULL;
	
		blank_client->client_source =
			G_main_add_IPC_Channel(
				G_PRIORITY_LOW, client_channel,
				FALSE,  crmd_ipc_input_callback,
				blank_client, default_ipc_input_destroy);
	}
    
	return TRUE;
}
