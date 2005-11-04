/* $Id: actions.c,v 1.3 2005/11/02 13:27:06 andrew Exp $ */
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

#include <portability.h>

#include <sys/param.h>
#include <crm/crm.h>
#include <crm/cib.h>
#include <crm/msg_xml.h>
#include <crm/common/msg.h>
#include <crm/common/xml.h>
#include <tengine.h>
#include <heartbeat.h>
#include <clplumbing/Gmain_timeout.h>
#include <lrm/lrm_api.h>

gboolean initiate_action(action_t *action);
void cib_action_updated(
	const HA_Message *msg, int call_id, int rc,
	crm_data_t *output, void *user_data);



static void
cib_fencing_updated(const HA_Message *msg, int call_id, int rc,
		    crm_data_t *output, void *user_data)
{
	if(rc < cib_ok) {
		crm_err("CIB update failed: %s", cib_error2string(rc));
		crm_log_xml_warn(msg, "[Failed Update]");
	}
	check_for_completion();
}

void
send_stonith_update(stonith_ops_t * op)
{
	enum cib_errors rc = cib_ok;
	const char *target = op->node_name;
	const char *uuid   = op->node_uuid;
	
	/* zero out the node-status & remove all LRM status info */
	crm_data_t *update = NULL;
	crm_data_t *node_state = create_xml_node(NULL, XML_CIB_TAG_STATE);
	
	CRM_DEV_ASSERT(op->node_name != NULL);
	CRM_DEV_ASSERT(op->node_uuid != NULL);
	
	crm_xml_add(node_state, XML_ATTR_UUID,  uuid);
	crm_xml_add(node_state, XML_ATTR_UNAME, target);
	crm_xml_add(node_state, XML_CIB_ATTR_HASTATE,   DEADSTATUS);
	crm_xml_add(node_state, XML_CIB_ATTR_INCCM,     XML_BOOLEAN_NO);
	crm_xml_add(node_state, XML_CIB_ATTR_CRMDSTATE, OFFLINESTATUS);
	crm_xml_add(node_state, XML_CIB_ATTR_JOINSTATE, CRMD_JOINSTATE_DOWN);
	crm_xml_add(node_state, XML_CIB_ATTR_EXPSTATE,  CRMD_JOINSTATE_DOWN);
	crm_xml_add(node_state, XML_CIB_ATTR_REPLACE,   XML_CIB_TAG_LRM);
	create_xml_node(node_state, XML_CIB_TAG_LRM);
	
	update = create_cib_fragment(node_state, XML_CIB_TAG_STATUS);
	
	rc = te_cib_conn->cmds->update(
		te_cib_conn, XML_CIB_TAG_STATUS, update, NULL,
		cib_quorum_override);	
	
	if(rc < cib_ok) {
		const char *fail_text = "Couldnt update CIB after stonith";
		crm_err("CIB update failed: %s", cib_error2string(rc));
		send_complete(fail_text, update, te_failed, i_cancel);
		
	} else {
		/* delay processing the trigger until the update completes */
		add_cib_op_callback(rc, FALSE, NULL, cib_fencing_updated);
	}
	
	free_xml(node_state);
	free_xml(update);
	return;
}

static gboolean
te_fence_node(action_t *action)
{
	char *key = NULL;
	const char *id = NULL;
	const char *uuid = NULL;
	const char *target = NULL;
	stonith_ops_t * st_op = NULL;
	
	id = ID(action->xml);
	target = crm_element_value(action->xml, XML_LRM_ATTR_TARGET);
	uuid = crm_element_value(action->xml, XML_LRM_ATTR_TARGET_UUID);
	
	CRM_DEV_ASSERT(id != NULL);
	CRM_DEV_ASSERT(uuid != NULL);
	CRM_DEV_ASSERT(target != NULL);
	if(crm_assert_failed) {
		/* error */
		te_log_action(LOG_ERR, "Corrupted command (id=%s): no node",
			      crm_str(id));
		return FALSE;
	}
	
	te_log_action(LOG_INFO,
		      "Executing fencing operation (%s) on %s (timeout=%d)",
		      id, target, transition_idle_timeout / 2);
#ifdef TESTING
	action->complete = TRUE;
	process_trigger(action->id);
	return TRUE;
#endif

	crm_malloc0(st_op, sizeof(stonith_ops_t));
	st_op->optype = RESET;
	st_op->timeout = transition_idle_timeout / 2;
	st_op->node_name = crm_strdup(target);
	st_op->node_uuid = crm_strdup(uuid);
	
	key = generate_transition_key(transition_counter, te_uuid);
	st_op->private_data = crm_concat(id, key, ';');
	crm_free(key);
	
	if(stonithd_input_IPC_channel() == NULL) {
		crm_err("Cannot fence %s: stonith not available", target);
		return FALSE;
		
	} else if (ST_OK != stonithd_node_fence( st_op )) {
		crm_err("Cannot fence %s: stonithd_node_fence() call failed ",
			target);
		return FALSE;
	}
	stop_te_timer(transition_timer);
	start_te_timer(transition_timer);
	return TRUE;
}

static gboolean
te_crm_command(action_t *action)
{
	char *value = NULL;
	char *counter = NULL;
	HA_Message *cmd = NULL;		

	const char *id = NULL;
	const char *task = NULL;
	const char *on_node = NULL;

	gboolean ret = TRUE;

	id      = ID(action->xml);
	task    = crm_element_value(action->xml, XML_LRM_ATTR_TASK);
	on_node = crm_element_value(action->xml, XML_LRM_ATTR_TARGET);

	CRM_DEV_ASSERT(on_node != NULL && strlen(on_node) != 0);
	if(crm_assert_failed) {
		/* error */
		te_log_action(LOG_ERR, "Corrupted command (id=%s) %s: no node",
			      crm_str(id), crm_str(task));
		return FALSE;
	}
	
	te_log_action(LOG_INFO, "Executing crm-event (%s): %s on %s",
		      crm_str(id), crm_str(task), on_node);
	
#ifdef TESTING
	action->complete = TRUE;
	process_trigger(action->id);
	return TRUE;
#endif
	
	cmd = create_request(task, NULL, on_node, CRM_SYSTEM_CRMD,
			     CRM_SYSTEM_TENGINE, NULL);
	
	counter = generate_transition_key(transition_counter, te_uuid);
	crm_xml_add(cmd, XML_ATTR_TRANSITION_KEY, counter);
	ret = send_ipc_message(crm_ch, cmd);
	crm_free(counter);
	
	value = g_hash_table_lookup(action->params, XML_ATTR_TE_NOWAIT);
	if(ret == FALSE) {
		crm_err("Action %d failed: send", action->id);
		return FALSE;
		
	} else if(crm_is_true(value)) {
		crm_info("Skipping wait for %d", action->id);
		action->complete = TRUE;
		process_trigger(action->id);
		
	} else if(ret && action->timeout > 0) {
		crm_debug_3("Setting timer for action %d",action->id);
		action->timer->reason = timeout_action_warn;
		start_te_timer(action->timer);
	}
	return TRUE;
}

static gboolean
te_rsc_command(action_t *action) 
{
	/* never overwrite stop actions in the CIB with
	 *   anything other than completed results
	 *
	 * Writing pending stops makes it look like the
	 *   resource is running again
	 */
	const char *task = NULL;
	const char *on_node = NULL;
	action->invoked = FALSE;

	on_node  = crm_element_value(action->xml, XML_LRM_ATTR_TARGET);
	CRM_DEV_ASSERT(on_node != NULL && strlen(on_node) != 0);
	if(crm_assert_failed) {
		/* error */
		te_log_action(LOG_ERR, "Corrupted command(id=%s) %s: no node",
			      ID(action->xml), crm_str(task));
		return FALSE;
	}	
	
#ifdef TESTING
	cib_action_update(action, LRM_OP_DONE);
	return TRUE;
#endif
	
	task = crm_element_value(action->xml, XML_LRM_ATTR_TASK);
	if(safe_str_eq(task, CRMD_ACTION_START)
	   || safe_str_eq(task, CRMD_ACTION_PROMOTE)) {
		cib_action_update(action, LRM_OP_PENDING);
		
	} else {
		cib_action_updated(NULL, 0, cib_ok, NULL, action);
	}
	return TRUE;
}

gboolean
initiate_action(action_t *action) 
{
	const char *id = NULL;

	id = crm_element_value(action->xml, XML_ATTR_ID);

	if(id == NULL) {
		/* error */
		te_log_action(LOG_ERR, "Corrupted command %s: no ID",
			      crm_element_name(action->xml));
		crm_log_xml_err(action->xml, "[corrupt cmd]");
		
	} else if(action->type == action_type_pseudo){
		te_log_action(LOG_INFO, "Executing pseudo-event: %d",
			      action->id);
		
		action->complete = TRUE;
		process_trigger(action->id);
		return TRUE;

	} else if(action->type == action_type_rsc) {
		return te_rsc_command(action);

	} else if(action->type == action_type_crm) {
		const char *task = NULL;
		task = crm_element_value(action->xml, XML_LRM_ATTR_TASK);
		CRM_DEV_ASSERT(task != NULL);

		if(safe_str_eq(task, CRM_OP_FENCE)) {
			return te_fence_node(action);

		} else {
			return te_crm_command(action);
		}
		
	} else {
		te_log_action(LOG_ERR,
			      "Failed on unsupported command type: %s (id=%s)",
			      crm_element_name(action->xml), id);
	}

	return FALSE;
}

gboolean
cib_action_update(action_t *action, int status)
{
	char *code = NULL;
	crm_data_t *fragment = NULL;
	crm_data_t *state    = NULL;
	crm_data_t *rsc      = NULL;
	crm_data_t *xml_op   = NULL;
	char *op_id = NULL;

	enum cib_errors rc = cib_ok;
	const char *task   = crm_element_value(action->xml, XML_LRM_ATTR_TASK);
	const char *rsc_id = crm_element_value(action->xml, XML_LRM_ATTR_RSCID);
	const char *target = crm_element_value(action->xml, XML_LRM_ATTR_TARGET);
	const char *task_uuid = crm_element_value(
		action->xml, XML_LRM_ATTR_TASK_KEY);
	
	const char *target_uuid =
		crm_element_value(action->xml, XML_LRM_ATTR_TARGET_UUID);

	int call_options = cib_quorum_override;

	if(status == LRM_OP_TIMEOUT) {
		if(crm_element_value(action->xml, XML_LRM_ATTR_RSCID) != NULL) {
			crm_warn("%s: %s %s on %s timed out",
				 crm_element_name(action->xml), task_uuid, rsc_id, target);
		} else {
			crm_warn("%s: %s on %s timed out",
				 crm_element_name(action->xml), task_uuid, target);
		}
	}
	code = crm_itoa(status);
	
/*
  update the CIB

<node_state id="hadev">
      <lrm>
        <lrm_resources>
          <lrm_resource id="rsc2" last_op="start" op_code="0" target="hadev"/>
*/

	fragment = NULL;
	state    = create_xml_node(NULL, XML_CIB_TAG_STATE);

	crm_xml_add(state, XML_ATTR_UUID,  target_uuid);
	crm_xml_add(state, XML_ATTR_UNAME, target);
	
	rsc = create_xml_node(state, XML_CIB_TAG_LRM);
	rsc = create_xml_node(rsc,   XML_LRM_TAG_RESOURCES);
	rsc = create_xml_node(rsc,   XML_LRM_TAG_RESOURCE);

	xml_op = create_xml_node(rsc,XML_LRM_TAG_RSC_OP);
	
	crm_xml_add(rsc,    XML_ATTR_ID, rsc_id);
	crm_xml_add(xml_op, XML_ATTR_ID, task);
	
	op_id = generate_op_key(rsc_id, task, action->interval);
	crm_xml_add(xml_op, XML_ATTR_ID, op_id);
	crm_free(op_id);
	
	crm_xml_add(xml_op, XML_LRM_ATTR_TASK, task);
	crm_xml_add(rsc, XML_LRM_ATTR_RSCSTATE,
			      get_rsc_state(task, status));
	
	crm_xml_add(xml_op, XML_LRM_ATTR_OPSTATUS, code);
	crm_xml_add(xml_op, XML_LRM_ATTR_CALLID, "-1");
	crm_xml_add(xml_op, XML_LRM_ATTR_RC, code);
	crm_xml_add(xml_op, "origin", __FUNCTION__);

	crm_free(code);

	code = generate_transition_key(transition_counter, te_uuid);
	crm_xml_add(xml_op, XML_ATTR_TRANSITION_KEY, code);
	crm_free(code);

	code = generate_transition_magic(
		crm_element_value(xml_op, XML_ATTR_TRANSITION_KEY), status, status);
	crm_xml_add(xml_op,  XML_ATTR_TRANSITION_MAGIC, code);
	crm_free(code);
	
	set_node_tstamp(xml_op);

	fragment = create_cib_fragment(state, XML_CIB_TAG_STATUS);
	
	crm_debug_3("Updating CIB with \"%s\" (%s): %s %s on %s",
		  status<0?"new action":XML_ATTR_TIMEOUT,
		  crm_element_name(action->xml), crm_str(task), rsc_id, target);
	
#ifndef TESTING
	rc = te_cib_conn->cmds->update(
		te_cib_conn, XML_CIB_TAG_STATUS, fragment, NULL, call_options);

	crm_debug("Updating CIB with %s action %d: %s %s on %s (call_id=%d)",
		  op_status2text(status), action->id, task_uuid, rsc_id, target, rc);

	if(status == LRM_OP_PENDING) {
		crm_debug_2("Waiting for callback id: %d", rc);
		add_cib_op_callback(rc, FALSE, action, cib_action_updated);
	}
#else
	te_log_action(LOG_INFO, "Initiating action %d: %s %s on %s",
		      action->id, task_uuid, rsc_id, target);
	call_options = 0;
	{
		HA_Message *cmd = ha_msg_new(11);
		ha_msg_add(cmd, F_TYPE,		T_CRM);
		ha_msg_add(cmd, F_CRM_VERSION,	CRM_FEATURE_SET);
		ha_msg_add(cmd, F_CRM_MSG_TYPE, XML_ATTR_REQUEST);
		ha_msg_add(cmd, F_CRM_TASK,	CRM_OP_EVENTCC);
		ha_msg_add(cmd, F_CRM_SYS_TO,   CRM_SYSTEM_TENGINE);
		ha_msg_add(cmd, F_CRM_SYS_FROM, CRM_SYSTEM_TENGINE);
		ha_msg_addstruct(cmd, crm_element_name(state), state);
		send_ipc_message(crm_ch, cmd);
	}
#endif
	free_xml(fragment);
	free_xml(state);

	action->sent_update = TRUE;
	
	if(rc < cib_ok) {
		return FALSE;
	}

	return TRUE;
}


void
cib_action_updated(
	const HA_Message *msg, int call_id, int rc, crm_data_t *output, void *user_data)
{
	HA_Message *cmd = NULL;
	crm_data_t *rsc_op  = NULL;
	action_t *action = user_data;
	char *counter = crm_itoa(transition_counter);

	const char *task    = NULL;
	const char *value   = NULL;
	const char *rsc_id  = NULL;
	const char *on_node = NULL;
	const char *task_uuid = NULL;

	CRM_DEV_ASSERT(action != NULL);      if(crm_assert_failed) { return; }
	CRM_DEV_ASSERT(action->xml != NULL); if(crm_assert_failed) { return; }

	rsc_op  = action->xml;
	task    = crm_element_value(rsc_op, XML_LRM_ATTR_TASK);
	task_uuid = crm_element_value(action->xml, XML_LRM_ATTR_TASK_KEY);
	rsc_id  = crm_element_value(rsc_op, XML_LRM_ATTR_RSCID);
	on_node = crm_element_value(rsc_op, XML_LRM_ATTR_TARGET);
	counter = generate_transition_key(transition_counter, te_uuid);
	crm_xml_add(rsc_op, XML_ATTR_TRANSITION_KEY, counter);
	crm_free(counter);
	
	if(rc < cib_ok) {
		crm_err("Update for action %d: %s %s on %s FAILED",
			action->id, task_uuid, rsc_id, on_node);
		send_complete(cib_error2string(rc), output, te_failed, i_cancel);
		return;
	}

	if(te_fsa_state != s_in_transition) {
		int pending_updates = num_cib_op_callbacks();
		if(pending_updates == 0) {
			send_complete("CIB update queue empty", output,
				      te_done, i_cib_complete);
		} else {
			crm_debug("Still waiting on %d callbacks",
				pending_updates);
		}
		crm_debug("Not executing action: Not in a transition: %d",
			  te_fsa_state);
		return;
	}
	
	crm_info("Initiating action %d: %s %s on %s",
		 action->id, task_uuid, rsc_id, on_node);
	
	if(rsc_op != NULL) {
		crm_log_xml_debug_2(rsc_op, "Performing");
	}
	cmd = create_request(CRM_OP_INVOKE_LRM, rsc_op, on_node,
			     CRM_SYSTEM_LRMD, CRM_SYSTEM_TENGINE, NULL);
	

#ifndef TESTING
	send_ipc_message(crm_ch, cmd);
#else
	crm_log_message(LOG_INFO, cmd);
#endif
	
	action->invoked = TRUE;
	value = g_hash_table_lookup(action->params, XML_ATTR_TE_NOWAIT);
	if(crm_is_true(value)) {
		crm_info("Skipping wait for %d", action->id);
		action->complete = TRUE;
		process_trigger(action->id);

	} else if(action->timeout > 0) {
		crm_debug_3("Setting timer for action %d",action->id);
		action->timer->reason = timeout_action_warn;
		start_te_timer(action->timer);
	}
}
	