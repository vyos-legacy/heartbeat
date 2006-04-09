/* $Id: attrd_updater.c,v 1.1 2006/04/09 12:50:04 andrew Exp $ */
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
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>
#include <crm/common/ipc.h>
#include <clplumbing/lsb_exitcodes.h>
#include <attrd.h>

#define OPTARGS      "hVn:v:d:s:S:"

const char* crm_system_name = "attrd_updater";

const char *attr_name = NULL;
const char *attr_value = NULL;
const char *attr_set = NULL;
const char *attr_section = NULL;
const char *attr_dampen = NULL;

void usage(const char* cmd, int exit_status);

static gboolean
process_attrd_message(
	HA_Message *msg, crm_data_t *xml_data, IPC_Channel *sender)
{
	crm_err("Why did we get a message?");
	crm_log_message_adv(LOG_WARNING, "attrd:msg", msg);
	return TRUE;
}

int
main(int argc, char ** argv)
{
	HA_Message *update = NULL;
	IPC_Channel *attrd = NULL;
	int argerr = 0;
	int flag;
	
	crm_log_init(crm_system_name);
	crm_debug_3("Begining option processing");

	while ((flag = getopt(argc, argv, OPTARGS)) != EOF) {
		switch(flag) {
			case 'V':
				alter_debug(DEBUG_INC);
				break;
			case 'h':		/* Help message */
				usage(crm_system_name, LSB_EXIT_OK);
				break;
			case 'n':
				attr_name = crm_strdup(optarg);
				break;
			case 'v':
				attr_value = crm_strdup(optarg);
				break;
			case 's':
				attr_set = crm_strdup(optarg);
				break;
			case 'd':
				attr_dampen = crm_strdup(optarg);
				break;
			case 'S':
				attr_section = crm_strdup(optarg);
				break;
			default:
				++argerr;
				break;
		}
	}
    
	crm_debug_3("Option processing complete");

	if (optind > argc) {
		++argerr;
	}
    
	if (argerr) {
		usage(crm_system_name, LSB_EXIT_GENERIC);
	}
    
	/* read local config file */
    
	init_client_ipc_comms(T_ATTRD, subsystem_msg_dispatch,
			      (void*)process_attrd_message, &attrd);

	if(attrd == NULL) {
		crm_err("Could not connect to "T_ATTRD);
		return 1;
	}

	update = ha_msg_new(4);
	ha_msg_add(update, F_TYPE, T_ATTRD);
	ha_msg_add(update, F_ORIG, crm_system_name);
	ha_msg_add(update, F_ATTRD_TASK, "update");
	ha_msg_add(update, F_ATTRD_ATTRIBUTE, attr_name);
	if(attr_value != NULL) {
		ha_msg_add(update, F_ATTRD_VALUE,   attr_value);
	}
	if(attr_set != NULL) {
		ha_msg_add(update, F_ATTRD_SET,     attr_set);
	}
	if(attr_section != NULL) {
		ha_msg_add(update, F_ATTRD_SECTION, attr_section);
	}
	if(attr_dampen != NULL) {
		ha_msg_add(update, F_ATTRD_DAMPEN, attr_dampen);
	}
	
	if(send_ipc_message(attrd, update) == FALSE) {
		crm_err("Update failed");
		return 1;
	}
	return 0;
}

void
usage(const char* cmd, int exit_status)
{
	FILE* stream;

	stream = exit_status ? stderr : stdout;

	fprintf(stream, "usage: %s [-srkh]"
		"[-c configure file]\n", cmd);
/* 	fprintf(stream, "\t-d\tsets debug level\n"); */
/* 	fprintf(stream, "\t-s\tgets daemon status\n"); */
/* 	fprintf(stream, "\t-r\trestarts daemon\n"); */
/* 	fprintf(stream, "\t-k\tstops daemon\n"); */
/* 	fprintf(stream, "\t-h\thelp message\n"); */
	fflush(stream);

	exit(exit_status);
}
