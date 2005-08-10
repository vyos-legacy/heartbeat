/* $Id: group.c,v 1.32 2005/08/10 08:55:03 andrew Exp $ */
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

#include <pengine.h>
#include <pe_utils.h>
#include <crm/msg_xml.h>

extern gboolean rsc_colocation_new(
	const char *id, enum con_strength strength,
	resource_t *rsc_lh, resource_t *rsc_rh);

typedef struct group_variant_data_s
{
		int num_children;
		GListPtr child_list; /* resource_t* */
		resource_t *self;
		resource_t *first_child;
		resource_t *last_child;

		gboolean child_starting;
		gboolean child_stopping;
		
} group_variant_data_t;


#define get_group_variant_data(data, rsc)				\
	CRM_ASSERT(rsc->variant == pe_group);				\
	CRM_ASSERT(rsc->variant_opaque != NULL);			\
	data = (group_variant_data_t *)rsc->variant_opaque;		\

void group_unpack(resource_t *rsc, pe_working_set_t *data_set)
{
	crm_data_t *xml_obj = rsc->xml;
	crm_data_t *xml_self = create_xml_node(NULL, XML_CIB_TAG_RESOURCE);
	group_variant_data_t *group_data = NULL;
	resource_t *self = NULL;

	crm_debug_3("Processing resource %s...", rsc->id);

	crm_malloc0(group_data, sizeof(group_variant_data_t));
	group_data->num_children = 0;
	group_data->self	 = NULL;
	group_data->child_list   = NULL;
	group_data->first_child  = NULL;
	group_data->last_child   = NULL;

	/* this is a bit of a hack - but simplifies everything else */
	copy_in_properties(xml_self, xml_obj);
	if(common_unpack(xml_self, &self, data_set)) {
		group_data->self = self;
		self->restart_type = pe_restart_restart;

	} else {
		crm_log_xml_err(xml_self, "Couldnt unpack dummy child");
		return;
	}

	xml_child_iter(
		xml_obj, xml_native_rsc, XML_CIB_TAG_RESOURCE,

		resource_t *new_rsc = NULL;
		set_id(xml_native_rsc, rsc->id, -1);
		inherit_parent_attributes(xml_self, xml_native_rsc, TRUE);

		if(common_unpack(xml_native_rsc, &new_rsc, data_set)) {
			group_data->num_children++;
			group_data->child_list = g_list_append(
				group_data->child_list, new_rsc);
			
			group_data->last_child = new_rsc;

			if(group_data->first_child == NULL) {
				group_data->first_child = new_rsc;
			}
			
			rsc_colocation_new("pe_group_internal_colo", pecs_must,
					   group_data->self, new_rsc);
		
			crm_action_debug_3(
				print_resource("Added", new_rsc, FALSE));
		} else {
			pe_err("Failed unpacking resource %s",
				crm_element_value(xml_obj, XML_ATTR_ID));
		}
		);
	crm_debug_3("Added %d children to resource %s...",
		    group_data->num_children, rsc->id);
	
	rsc->variant_opaque = group_data;
}


resource_t *
group_find_child(resource_t *rsc, const char *id)
{
	group_variant_data_t *group_data = NULL;
	if(rsc->variant == pe_group) {
		group_data = (group_variant_data_t *)rsc->variant_opaque;
	} else {
		pe_err("Resource %s was not a \"group\" variant", rsc->id);
		return NULL;
	}
	return pe_find_resource(group_data->child_list, id);
}

int group_num_allowed_nodes(resource_t *rsc)
{
	group_variant_data_t *group_data = NULL;
	if(rsc->variant == pe_native) {
		group_data = (group_variant_data_t *)rsc->variant_opaque;
	} else {
		pe_err("Resource %s was not a \"native\" variant",
			rsc->id);
		return 0;
	}
	if(group_data->self == NULL) {
		return 0;
	}
 	return group_data->self->fns->num_allowed_nodes(group_data->self);
}

void group_color(resource_t *rsc, pe_working_set_t *data_set)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	if(group_data->self == NULL) {
		return;
	}

 	group_data->self->fns->color(group_data->self, data_set);
}

void group_update_pseudo_status(resource_t *parent, resource_t *child);

void group_create_actions(resource_t *rsc, pe_working_set_t *data_set)
{
	action_t *op = NULL;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		child_rsc->fns->create_actions(child_rsc, data_set);
		group_update_pseudo_status(rsc, child_rsc);
		);

	op = start_action(group_data->self, NULL, !group_data->child_starting);
	op->pseudo   = TRUE;

	op = custom_action(group_data->self, started_key(rsc),
			   CRMD_ACTION_STARTED, NULL,
			   !group_data->child_starting, data_set);
	op->pseudo   = TRUE;

	op = stop_action(group_data->self, NULL, !group_data->child_stopping);
	op->pseudo   = TRUE;
	
	op = custom_action(group_data->self, stopped_key(rsc),
			   CRMD_ACTION_STOPPED, NULL,
			   !group_data->child_stopping, data_set);
	op->pseudo   = TRUE;
}

void
group_update_pseudo_status(resource_t *parent, resource_t *child) 
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, parent);

	if(group_data->child_stopping && group_data->child_starting) {
		return;
	}
	slist_iter(
		action, action_t, child->actions, lpc,

		if(action->optional) {
			continue;
		}
		if(safe_str_eq(CRMD_ACTION_STOP, action->task)) {
			group_data->child_stopping = TRUE;
		} else if(safe_str_eq(CRMD_ACTION_START, action->task)) {
			group_data->child_starting = TRUE;
		}
		
		);

}

void group_internal_constraints(resource_t *rsc, pe_working_set_t *data_set)
{
	resource_t *last_rsc = NULL;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	custom_action_order(
		group_data->self, stopped_key(group_data->self), NULL,
		group_data->self, start_key(group_data->self), NULL,
		pe_ordering_optional, data_set);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,

		order_restart(child_rsc);

		if(last_rsc != NULL) {
			order_start_start(
				last_rsc, child_rsc, pe_ordering_optional);
			order_stop_stop(
				child_rsc, last_rsc, pe_ordering_optional);

		} else {
			custom_action_order(
				child_rsc, stop_key(child_rsc), NULL,
				group_data->self, stopped_key(group_data->self), NULL,
				pe_ordering_optional, data_set);

			order_start_start(group_data->self, child_rsc,
					  pe_ordering_optional);
		}
		
		last_rsc = child_rsc;
		);

	if(last_rsc != NULL) {
		custom_action_order(
			last_rsc, start_key(last_rsc), NULL,
			group_data->self, started_key(group_data->self), NULL,
			pe_ordering_optional, data_set);

		order_stop_stop(
			group_data->self, last_rsc, pe_ordering_optional);
	}
		
}


void group_rsc_colocation_lh(rsc_colocation_t *constraint)
{
	resource_t *rsc = constraint->rsc_lh;
	group_variant_data_t *group_data = NULL;
	
	if(rsc == NULL) {
		pe_err("rsc_lh was NULL for %s", constraint->id);
		return;

	} else if(constraint->rsc_rh == NULL) {
		pe_err("rsc_rh was NULL for %s", constraint->id);
		return;
		
	} else {
		crm_debug_4("Processing constraints from %s", rsc->id);
	}

	get_group_variant_data(group_data, rsc);
	if(group_data->self == NULL) {
		return;
	}

	group_data->self->fns->rsc_colocation_rh(group_data->self, constraint);
	
}

void group_rsc_colocation_rh(resource_t *rsc, rsc_colocation_t *constraint)
{
	resource_t *rsc_lh = rsc;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Processing RH of constraint %s", constraint->id);
	crm_action_debug_3(print_resource("LHS", rsc_lh, TRUE));

	if(group_data->self == NULL) {
		return;
	}
	
	group_data->self->fns->rsc_colocation_rh(group_data->self, constraint);
}


void group_rsc_order_lh(resource_t *rsc, order_constraint_t *order)
{
	char *stop_id = NULL;
	char *start_id = NULL;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Processing LH of ordering constraint %d", order->id);

	if(group_data->self == NULL) {
		return;
	}

	stop_id = stop_key(rsc);
	start_id = start_key(rsc);
	
	if(safe_str_eq(order->lh_action_task, start_id)) {
		crm_free(order->lh_action_task);
		order->lh_action_task = started_key(rsc);

	} else if(safe_str_eq(order->lh_action_task, stop_id)) {
		crm_free(order->lh_action_task);
		order->lh_action_task = stopped_key(rsc);
	}

	crm_free(start_id);
	crm_free(stop_id);
	
	group_data->self->fns->rsc_order_lh(group_data->self, order);
}

void group_rsc_order_rh(
	action_t *lh_action, resource_t *rsc, order_constraint_t *order)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Processing RH of ordering constraint %d", order->id);

	if(group_data->self == NULL) {
		return;
	}

	group_data->self->fns->rsc_order_rh(lh_action, group_data->self, order);
}

void group_rsc_location(resource_t *rsc, rsc_to_node_t *constraint)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Processing actions from %s", rsc->id);

	if(group_data->self != NULL) {
		group_data->self->fns->rsc_location(group_data->self, constraint);
	}

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,

		child_rsc->fns->rsc_location(child_rsc, constraint);
		);
}

void group_expand(resource_t *rsc, pe_working_set_t *data_set)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Processing actions from %s", rsc->id);

	group_data->self->fns->expand(group_data->self, data_set);

	if(group_data->self == NULL) {
		return;
	}
	
	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,

		child_rsc->fns->expand(child_rsc, data_set);
		);

}

gboolean group_active(resource_t *rsc, gboolean all)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		gboolean child_active = child_rsc->fns->active(child_rsc, all);
		if(all == FALSE && child_active) {
			return TRUE;
		} else if(child_active == FALSE) {
			return FALSE;
		}
		);
	if(all) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void group_printw(resource_t *rsc, const char *pre_text, int *index)
{
#if CURSES_ENABLED
	const char *child_text = NULL;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);
	if(pre_text != NULL) {
		child_text = "        ";
	} else {
		child_text = "    ";
	}
	
	move(*index, 0);
	printw("Resource Group: %s\n", rsc->id);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		(*index)++;
		child_rsc->fns->printw(child_rsc, child_text, index);
		);
#else
	crm_err("printw support requires ncurses to be available during configure");
#endif
}

void group_html(resource_t *rsc, const char *pre_text, FILE *stream)
{
	const char *child_text = NULL;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);
	if(pre_text != NULL) {
		child_text = "        ";
	} else {
		child_text = "    ";
	}
	
	fprintf(stream, "Resource Group: %s\n", rsc->id);
	fprintf(stream, "<ul>\n");

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		fprintf(stream, "<li>\n");
		child_rsc->fns->html(child_rsc, child_text, stream);
		fprintf(stream, "</li>\n");
		);
	fprintf(stream, "</ul>\n");
}

void group_dump(resource_t *rsc, const char *pre_text, gboolean details)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	if(group_data->self == NULL) {
		return;
	}

	common_dump(rsc, pre_text, details);
	
	group_data->self->fns->dump(group_data->self, pre_text, details);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		child_rsc->fns->dump(child_rsc, pre_text, details);
		);
}

void group_free(resource_t *rsc)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	crm_debug_3("Freeing %s", rsc->id);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,

		crm_debug_3("Freeing child %s", child_rsc->id);
		child_rsc->fns->free(child_rsc);
		);

	crm_debug_3("Freeing child list");
	pe_free_shallow_adv(group_data->child_list, FALSE);

	free_xml(group_data->self->xml);
	if(group_data->self != NULL) {
		group_data->self->fns->free(group_data->self);
	}

	common_free(rsc);
}


void
group_agent_constraints(resource_t *rsc)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		child_rsc->fns->agent_constraints(child_rsc);
		);
}

rsc_state_t
group_resource_state(resource_t *rsc)
{
	rsc_state_t state = rsc_state_unknown;
	rsc_state_t last_state = rsc_state_unknown;
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		last_state = child_rsc->fns->state(child_rsc);
		switch(last_state) {
			case rsc_state_unknown:
			case rsc_state_move:
				return last_state;
				break;
			case rsc_state_restart:
				CRM_DEV_ASSERT(state != rsc_state_stopping);
				CRM_DEV_ASSERT(state != rsc_state_stopped);
/* 				CRM_DEV_ASSERT(state != rsc_state_active); */
				state = last_state;
				break;
			case rsc_state_active:
/* 				CRM_DEV_ASSERT(state != rsc_state_restart); */
				CRM_DEV_ASSERT(state != rsc_state_stopping);
				if(last_state == rsc_state_unknown) {
					state = last_state;
				}
				break;
			case rsc_state_stopped:
				CRM_DEV_ASSERT(state != rsc_state_restart);
				CRM_DEV_ASSERT(state != rsc_state_starting);
				if(last_state == rsc_state_unknown) {
					state = last_state;
				}
				break;
			case rsc_state_starting:
				CRM_DEV_ASSERT(state != rsc_state_stopped);
				if(state != rsc_state_restart) {
					state = last_state;
				}
				break;				
			case rsc_state_stopping:
				CRM_DEV_ASSERT(state != rsc_state_active);
				if(state != rsc_state_restart) {
					state = last_state;
				}
				break;		
		}
		);
	return state;
}

void
group_create_notify_element(resource_t *rsc, action_t *op, crm_data_t *parent,
			    pe_working_set_t *data_set)
{
	group_variant_data_t *group_data = NULL;
	get_group_variant_data(group_data, rsc);

	slist_iter(
		child_rsc, resource_t, group_data->child_list, lpc,
		
		child_rsc->fns->create_notify_element(
			child_rsc, op, parent, data_set);
		);
}
