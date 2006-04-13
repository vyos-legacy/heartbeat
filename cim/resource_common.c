/*
 * resource_common.c: common functions for resource providers
 *
 * Author: Jia Ming Pan <jmltc@cn.ibm.com>
 * Copyright (c) 2005 International Business Machines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <portability.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>
#include "cluster_info.h"
#include "resource_common.h"
#include "cmpi_utils.h"

static char 	SystemName         [] = "LinuxHACluster";
static char 	SystemClassName    [] = "HA_Cluster";
static char 	PrimitiveClassName [] = "HA_PrimitiveResource";
static char 	GroupClassName     [] = "HA_ResourceGroup";
static char 	CloneClassName     [] = "HA_ResourceClone";
static char 	MasterClassName    [] = "HA_MasterSlaveResource";

static void     primitive_set_instance(CMPIBroker * broker, 
			CMPIInstance * ci, struct ha_msg *, CMPIStatus *);
static void     group_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
			struct ha_msg *, CMPIStatus *);
static void     clone_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
			struct ha_msg *, CMPIStatus *);
static void     master_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
			struct ha_msg *, CMPIStatus *);
static CMPIInstance *   make_instance(CMPIBroker * broker, char * classname, 
					CMPIObjectPath * op, struct ha_msg *info, 
					uint32_t type, CMPIStatus * rc);
static CMPIInstance *   make_instance_byid(CMPIBroker * broker, 
					CMPIObjectPath * ref, char * rscid, 
					uint32_t type, CMPIStatus * rc);
static CMPIObjectPath * make_objectpath_byid(CMPIBroker * broker, 
					CMPIObjectPath * ref, char * rscid, 
					uint32_t type, CMPIStatus * rc);
#if 0
static void     
attributes2inst(CMPIBroker * broker, CMPIInstance * ci,
	const char * rscid, struct ha_msg *resource, CMPIStatus * rc)
{
	struct ha_msg *attributes = NULL;
	CMPIArray * array = NULL;
        int len = 0, i = 0;
	
	if ((attributes = cim_get_rscattrs(rscid)) == NULL ) {
		return;
	}

        len = cim_msg_children_count(attributes);
	cim_debug2(LOG_INFO, "attirbutes len: %d", len);

        if ( ( array = CMNewArray(broker, len, CMPI_chars, rc)) == NULL ) {
                cl_log(LOG_ERR, "Resource attribute: can't make CMPIArray.");
		ha_msg_del(attributes);
                return;
        }

        for ( i = 0; i < len; i++ ) {
                char buf[MAXLEN];
                const char *id, *name, *value, *p;
		struct ha_msg *attribute;
		
                attribute = cim_msg_child_index(attributes, i);
                if ( attribute == NULL ) {
                        continue;
                }

                id    = cl_get_string(attribute, "id");
                name  = cl_get_string(attribute, "name");
                value = cl_get_string(attribute, "value");
                snprintf(buf, MAXLEN, "%s,%s=%s", id, name, value);

                if ( (p = cim_strdup(buf)) ){
                        CMSetArrayElementAt(array, i, &p, CMPI_chars);
                }
        }
	CMSetProperty(ci, "InstanceAttributes", &array, CMPI_charsA);

}

static struct ha_msg*
inst2attributes(CMPIObjectPath* cop, CMPIInstance* ci, CMPIStatus* rc)
{
        char *id, *rscid, *rsc_crname;
        CMPIArray * array;
        int i, len;
        struct ha_msg *attributes;

        DEBUG_ENTER();
        id 	   = CMGetKeyString(cop, "Id", rc);
        rscid 	   = CMGetKeyString(cop, "SystemName", rc);
        rsc_crname = CMGetKeyString(cop, "SystemCreationClassName", rc);

	cim_debug2(LOG_INFO, "%s: id, rscid, crname = %s, %s, %s", 
			__FUNCTION__, id, rscid, rsc_crname);
        array = CMGetProperty(ci, "InstanceAttributes", rc).value.array;
        if ( array == NULL ) {
		cl_log(LOG_ERR, "get_attributes: attributes missing.");
		return NULL;
        }
        len = CMGetArrayCount(array, rc);
        if ( rc->rc != CMPI_RC_OK) {
                cl_log(LOG_ERR, "get_attributes: can't get array length.");
                return NULL;
        }

        if ((attributes = ha_msg_new(16)) == NULL ) {
		cl_log(LOG_ERR, "get_attributes: failed to alloc array.");
                return NULL;
        }

        for(i = 0; i < len; i++) {
                CMPIData data;
		struct ha_msg *attribute;
                char **s, *v;
                int len;

                data = CMGetArrayElementAt(array, i, rc);
                if ( rc->rc != CMPI_RC_OK ) {
                        continue;
                }
                v = CMGetCharPtr(data.value.string);

                if((attribute = ha_msg_new(3)) == NULL ) {
                        continue;
                }
		
		/* parse attributes, get key and value */
                s = split_string(v, &len, " ,=");
                if ( len == 3 ) {
			ha_msg_add(attribute, "id", s[0]);
                        ha_msg_add(attribute, "name", s[1]);
                        ha_msg_add(attribute, "value", s[2]);
                }

		cim_msg_add_child(attributes, s[0], attribute);
                free_2d_array(s, len, cim_free);
        }

        DEBUG_LEAVE();
	return attributes;
}

#endif

static void
primitive_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
		struct ha_msg *info, CMPIStatus * rc)
{
        const char *const_host= NULL, *id;
	char * host = NULL;
	struct ha_msg *msg = NULL;

	cmpi_msg2inst(broker, ci, HA_PRIMITIVE_RESOURCE, info, rc);
	id = cl_get_string(info, "id");
	
	if ( !cim_is_rsc_disabled(id) ) {
		msg = cim_query_dispatch(GET_RSC_HOST, id, NULL);
	}
        /* get hosting node */
        if ( msg && (const_host = cl_get_string(msg, "host")) != NULL ){
		host = cim_strdup(const_host);
                cl_log(LOG_INFO, "Hosting node is %s", host);
                CMSetProperty(ci, "HostingNode", host, CMPI_chars);
        } else {
               /* OpenWBEM will segment fault in HostedResource provider 
                  if "HostingNode" not set */
                host = cim_strdup ("Unknown");
                CMSetProperty(ci, "HostingNode", host, CMPI_chars);
        }
        cim_free(host);
}
static void 
group_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
		struct ha_msg *info, CMPIStatus * rc)
{
	cmpi_msg2inst(broker, ci, HA_RESOURCE_GROUP, info, rc);
}

static void 
clone_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
		struct ha_msg *clone, CMPIStatus * rc)
{
	cmpi_msg2inst(broker, ci, HA_RESOURCE_CLONE, clone, rc);
}

static void 
master_set_instance(CMPIBroker * broker, CMPIInstance * ci, 
		struct ha_msg *master, CMPIStatus * rc)
{
	cmpi_msg2inst(broker, ci, HA_MASTERSLAVE_RESOURCE, master, rc);
}

static CMPIInstance *
make_instance(CMPIBroker * broker, char * classname, CMPIObjectPath * op, 
              struct ha_msg* info, uint32_t type, CMPIStatus * rc) 
{
        CMPIInstance * ci = NULL;
        char *id, *status;
	const char *constid, *conststatus;
        char caption [MAXLEN];
	struct ha_msg *msg;	

        ci = CMNewInstance(broker, op, rc);
        if ( CMIsNullObject(ci) ) {
                cl_log(LOG_ERR, "%s: Can't create instance.", __FUNCTION__);
	        CMSetStatusWithChars(broker, rc, 
		       CMPI_RC_ERR_FAILED, "Can't get create instance");
        	return NULL;
	}

	constid = cl_get_string(info, "id");
	id = cim_strdup(constid);

        snprintf(caption, MAXLEN, "Resource.%s", id);
        /* set other key properties inherited from super classes */
        CMSetProperty(ci, "SystemCreationClassName", 
			      SystemClassName, CMPI_chars);
        CMSetProperty(ci, "SystemName", SystemName, CMPI_chars);
        CMSetProperty(ci, "CreationClassName", classname, CMPI_chars);      
        CMSetProperty(ci, "Id", id, CMPI_chars);
	
        /* set Caption */
        CMSetProperty(ci, "Caption", caption, CMPI_chars);

	if ((msg = cim_query_dispatch(GET_RSC_STATUS, id, NULL)) ){
	        if (( conststatus = cl_get_string(msg, "status")) ) {
			status = cim_strdup(conststatus);
                	CMSetProperty(ci, "ResourceStatus", status, CMPI_chars);
	                cim_free(status);
        	}
	}
	ha_msg_del(msg);

        if (type == TID_RES_PRIMITIVE) {
		/* attributes2inst(broker, ci, id, info, rc); */
		primitive_set_instance(broker, ci, info, rc);
	} else if ( type == TID_RES_GROUP){
		group_set_instance(broker, ci, info, rc);
	} else if ( type == TID_RES_CLONE){
		clone_set_instance(broker, ci, info, rc);
	} else if ( type == TID_RES_MASTER) {
		master_set_instance(broker, ci, info, rc);
        }
	cim_free(id);

        return ci;
}

static CMPIInstance *
make_instance_byid(CMPIBroker * broker, CMPIObjectPath * ref, 
                    char * rscid, uint32_t type, CMPIStatus * rc) 
{
        CMPIInstance * ci = NULL;
        char * namespace, * classname;
        CMPIObjectPath * op;
	struct ha_msg *info, *msg;
	int i, len;

        namespace = CMGetCharPtr(CMGetNameSpace(ref, rc));
        switch(type) {
        case TID_RES_PRIMITIVE:
                classname = PrimitiveClassName;
                break;
        case TID_RES_GROUP:
                classname = GroupClassName;
                break;
        case TID_RES_CLONE:
                classname = CloneClassName;
                break;
        case TID_RES_MASTER:
                classname = MasterClassName;
                break;
        default:
             return NULL;
        }

	if ((info = cim_find_rsc(type, rscid)) == NULL ) {
		cl_log(LOG_ERR, "%s: failed to get resource", __FUNCTION__);
		return NULL;
	}
	
	if ( (msg = cim_get_disabled_rsc_list()) == NULL ) {
		ha_msg_add(info, "enabled", "true");
	} else {
		len = cim_list_length(msg);
		for(i = 0; i < len; i++) {
			char *rsc = NULL;
			if ((rsc = cim_list_index(msg, i)) == NULL ) {
				continue;
			}
			if ( strncmp(rsc, rscid, MAXLEN) == 0){
				break;
			}
		}
		if(i==len) {	/* not found */
			ha_msg_add(info, "enabled", "true");
		}else {
			ha_msg_add(info, "enabled", "false");
		}
	}
	
        op = CMNewObjectPath(broker, namespace, classname, rc);
        if ( CMIsNullObject(op) ) {
		cl_log(LOG_ERR, "%s: can't create objectpath", __FUNCTION__);
                return NULL;
        }

        ci = make_instance(broker, classname, op, info, type, rc);
	ha_msg_del(info);
	CMRelease(op);
        return ci;
}


static CMPIObjectPath *
make_objectpath_byid(CMPIBroker * broker, CMPIObjectPath * ref, 
                  char * rscid, uint32_t type, CMPIStatus * rc)
{
        char *namespace, *classname;
        CMPIObjectPath * op;

	DEBUG_ENTER();
        if ( rscid == NULL )  { return NULL; }
        namespace = CMGetCharPtr(CMGetNameSpace(ref, rc));

        switch(type) {
        case TID_RES_PRIMITIVE:
                classname = PrimitiveClassName;
                break;
        case TID_RES_GROUP:
                classname = GroupClassName;
                break;
        case TID_RES_CLONE:
                classname = CloneClassName;
                break;
        case TID_RES_MASTER:
                classname = MasterClassName;
                break;
        default:
             return NULL;
        }
        
        op = CMNewObjectPath(broker, namespace, classname, rc);
        if ( CMIsNullObject(op) ) {
                return NULL;
        }

        CMAddKey(op, "Id", rscid, CMPI_chars);
        CMAddKey(op, "SystemName", SystemName, CMPI_chars);
        CMAddKey(op, "SystemCreationClassName", SystemClassName, CMPI_chars);
        CMAddKey(op, "CreationClassName", classname, CMPI_chars);

	DEBUG_LEAVE();
        return op;
}

int
resource_get_inst(CMPIBroker * broker, char * classname, CMPIContext * ctx, 
                  CMPIResult * rslt, CMPIObjectPath * ref,
                  char ** properties, uint32_t type, CMPIStatus * rc)
{
        CMPIInstance * 	ci = NULL;
        CMPIData 	data_name;
        char * 		rscid = NULL;
        uint32_t 	this_type;

	if ( cim_get_hb_status() != HB_RUNNING ) {
		rc->rc = CMPI_RC_ERR_FAILED;
		cl_log(LOG_WARNING, "Heartbeat not running.");
		return HA_FAIL;
	}

        data_name = CMGetKey(ref, "Id", rc);
        if ( data_name.value.string == NULL ) {
                cl_log(LOG_WARNING, "key %s is NULL", "Id");
                return HA_FAIL;
        }

        rscid = CMGetCharPtr(data_name.value.string);
        this_type = (type == 0) 
		? cim_get_rsctype(rscid)	: type;

        ci = make_instance_byid(broker, ref, rscid, this_type, rc);
        if ( CMIsNullObject(ci) ) {
                cl_log(LOG_WARNING, "Can not create resource instance.");
		return HA_FAIL;
        }

        CMReturnInstance(rslt, ci);
        CMReturnDone(rslt);
	return HA_OK;
}

/* should return primitives in group for primitive provider? no */
int 
resource_enum_insts(CMPIBroker * broker, char * classname, CMPIContext * ctx, 
                   CMPIResult * rslt, CMPIObjectPath * ref, int need_inst,
                   uint32_t type, CMPIStatus * rc)
{
	struct ha_msg* names;
        int i, len;

	DEBUG_ENTER();
	if ( cim_get_hb_status() != HB_RUNNING ) {
		rc->rc = CMPI_RC_ERR_FAILED;
		cl_log(LOG_WARNING, "Heartbeat not running.");
		return HA_FAIL;
	}

        if ( ( names = cim_get_rsc_list()) == NULL ) {
		rc->rc = CMPI_RC_ERR_FAILED;
		cl_log(LOG_WARNING, "Get resource list failed.");
                return HA_FAIL;
        }

	len = cim_list_length(names);
        for ( i = 0; i < len; i++) {
		char *rsc = NULL;
		if ((rsc = cim_list_index(names, i)) == NULL) {
			continue;
		}	
                if ( type != cim_get_rsctype(rsc)){
                        continue;
                }
		/* should we return all sub resource of group/clone/master
		   for the primitive resources enumeration operation? */
                if ( need_inst ) {
                        CMPIInstance * ci = NULL;
                        ci = make_instance_byid(broker, ref, rsc, type, rc);
			if( ci ) {
                        	CMReturnInstance(rslt, ci);
			}
                } else {
                        CMPIObjectPath * op;
                        op = make_objectpath_byid(broker, ref, rsc, type, rc);
			if (op) {
                        	/* add object to rslt */
	                        CMReturnObjectPath(rslt, op);
			}
                }
        }
        CMReturnDone(rslt);
	ha_msg_del(names);
	DEBUG_LEAVE();
        return HA_OK;
}

int
resource_cleanup(CMPIBroker * broker, char * classname, CMPIInstanceMI * mi, 
                 CMPIContext * ctx, uint32_t type, CMPIStatus * rc)
{
        return HA_OK;
}

int
resource_del_inst(CMPIBroker * broker, char * classname, CMPIContext * ctx,
                CMPIResult * rslt, CMPIObjectPath * ref, CMPIStatus * rc)
{
        char * rscid;
        CMPIString * string;
	int ret = HA_FAIL;
        if ((string = CMGetKey(ref, "Id", rc).value.string) == NULL ) {
		cl_log(LOG_WARNING, "Resource id missing.");
                return HA_FAIL;
        }
        rscid = CMGetCharPtr(string);
	ret = cim_remove_rsc(rscid);
	rc->rc = (ret==HA_OK)? CMPI_RC_OK : CMPI_RC_ERR_FAILED;
	return ret;
}

#if 0
static int
resource_update_instattrs(const char *rscid, struct ha_msg * attributes)
{
	int i, len;
	struct ha_msg * old = cim_get_rscattrs(rscid);
	
	/* delete nvpair */
	len = (old) ? cim_msg_children_count(old): 0;
	for (i = 0; i < len; i++) {
		const char * id = cim_msg_child_name(old, i);
		if (id && cl_get_struct(attributes, id) == NULL ) {
			cim_remove_attrnvpair(rscid, id);
		}
	}
	len = cim_msg_children_count(attributes);
	for (i = 0; i < len; i++) {
		struct ha_msg * nvpair = cim_msg_child_index(attributes, i);
		if ( nvpair ) {
			const char * id = cim_msg_child_name(attributes, i);
			cim_update_attrnvpair(rscid, id, nvpair);
		} 
	}

	return HA_OK;
}

#endif

int 
resource_update_inst(CMPIBroker * broker, char * classname, CMPIContext * ctx,
                CMPIResult * rslt, CMPIObjectPath * cop, CMPIInstance * ci,
                char ** properties, uint32_t type, CMPIStatus * rc)
{
	struct ha_msg *resource = NULL /*, *attributes = NULL*/;
	char * rscid;
	int ret = 0;
	DEBUG_ENTER();
	/* get resource id */
        if ((rscid = CMGetKeyString(cop, "Id", rc)) == NULL ) {
		return HA_FAIL;
	}

	/* get original values, and set new values */
	resource = cim_find_rsc(type, rscid);

	if ( type == TID_RES_PRIMITIVE ) {
		cl_msg_remove(resource, CIM_MSG_ATTR);
		cmpi_inst2msg(ci, HA_PRIMITIVE_RESOURCE, resource, rc);
		/* attributes = inst2attributes(cop, ci, rc); */
	} else if (type == TID_RES_MASTER) {
		cmpi_inst2msg(ci, HA_MASTERSLAVE_RESOURCE, resource, rc);
	} else if (type == TID_RES_CLONE ) {
		cmpi_inst2msg(ci, HA_RESOURCE_CLONE, resource, rc);
	}

        /* update resource */
	ret = cim_update_rsc(type, rscid, resource);
	if ( type != TID_RES_PRIMITIVE) {
		goto done;
	}

	/* update its attributes */
	/*
	ret = resource_update_instattrs(rscid, attributes);
	ha_msg_del(attributes);
	*/
done:
	ha_msg_del(resource);
	rc->rc = (ret==HA_OK) ? CMPI_RC_OK: CMPI_RC_ERR_FAILED;
	DEBUG_LEAVE();
	return ret;
}

int
resource_create_inst(CMPIBroker * broker, char * classname, CMPIContext * ctx,
                CMPIResult * rslt, CMPIObjectPath * cop, CMPIInstance * ci,
                uint32_t type, CMPIStatus * rc)
{
	struct ha_msg *resource /*, *attributes*/;
	int ret = HA_FAIL;
	char * rscid = NULL;
	const char * rsctype = NULL;

	DEBUG_ENTER();
	if((resource = ha_msg_new(16)) == NULL ) {
		return HA_FAIL;
	}

	/* get resource id */
        if ((rscid = CMGetKeyString(cop, "Id", rc)) == NULL ) {
		ha_msg_del(resource);
		return HA_FAIL;
	}

	switch(type){
	case TID_RES_PRIMITIVE:
		/* get resource attributes */
		/* attributes = inst2attributes(cop, ci, rc); */
		ret = cmpi_inst2msg(ci, HA_PRIMITIVE_RESOURCE, resource, rc);
		rsctype = "native";
		break;
	case TID_RES_CLONE:
		ret = cmpi_inst2msg(ci, HA_RESOURCE_CLONE, resource, rc);
		ha_msg_add(resource, "advance", "clone");
		rsctype = "clone";
		break;
	case TID_RES_MASTER:
		ret = cmpi_inst2msg(ci, HA_MASTERSLAVE_RESOURCE, resource, rc);
		ha_msg_add(resource, "advance", "master");
		rsctype = "master";
		break;
	case TID_RES_GROUP:
		rsctype = "group";
		break;
	}

	ha_msg_add(resource, "enabled", "false");

	/* add to list */	
	ret = cim_update_disabled_rsc_list(1, rscid);

	/* write to disk */
	cim_add_rsctype(rscid, rsctype);
	cim_store_rsc(type, rscid, resource);
	/*
	if ( type == TID_RES_PRIMITIVE) {
		resource_update_instattrs(rscid, attributes);
		ha_msg_del(attributes);
	}
	*/
	rc->rc = (ret==HA_OK) ? CMPI_RC_OK: CMPI_RC_ERR_FAILED;
	ha_msg_del(resource);
	DEBUG_LEAVE();
	return ret;

}
#if 0
/* add a operation to resource */
int 
resource_add_operation(CMPIBroker * broker, char * classname, CMPIContext * ctx,
                CMPIResult * rslt, CMPIObjectPath * ref, uint32_t type,
		CMPIArgs *in, CMPIArgs *out, CMPIStatus * rc)
{
	CMPIObjectPath * opop;
	const char * key[] = {"Id", "SystemName", "SystemCreationClassName"};
	char *id, *sysname, *syscrname, *namespace;
	struct ha_msg *msg;

	if((opop = CMGetArg(in, "Operation", rc).value.ref) == NULL ) {
		cl_log(LOG_ERR, "%s: can't get Operation ObjectPath.", 
				__FUNCTION__);
		return HA_FAIL;
	}
	
	id 	  = CMGetKeyString(opop, key[0], rc);
        sysname   = CMGetKeyString(opop, key[1], rc);
        syscrname = CMGetKeyString(opop, key[2], rc);

        namespace = CMGetCharPtr(CMGetNameSpace(opop, rc));
	if ((msg = cim_load_operation(sysname, id)) == NULL ) {
		cl_log(LOG_ERR, "%s: can't find instance for %s:%s:%s:%s.",
			__FUNCTION__, namespace, id, sysname, syscrname);
		return HA_FAIL;
	}

	cim_add_rscop(sysname, msg);
	ha_msg_del(msg);	

	return HA_OK;
}
#endif

/* add sub resource to group/clone/master-slave */
int 
resource_add_subrsc(CMPIBroker * broker, char * classname, CMPIContext * ctx,
                CMPIResult * rslt, CMPIObjectPath * ref, uint32_t type,
		CMPIArgs *in, CMPIArgs *out, CMPIStatus * rc)
{
	CMPIObjectPath * rscop;
	char *subrscid, *rscid;
	struct ha_msg *subrsc, *resource;
	int ret ;

	rscid = CMGetKeyString(ref, "Id", rc);
	resource = cim_find_rsc(type, rscid);

	if((rscop = CMGetArg(in, "Resource", rc).value.ref) == NULL ) {
		cl_log(LOG_ERR, "%s: can't get Resource ObjectPath.",
			__FUNCTION__);
		return HA_FAIL;
	}
	
	subrscid = CMGetKeyString(rscop, "Id", rc);
	subrsc = cim_find_rsc(TID_RES_PRIMITIVE, subrscid);
	if (subrsc == NULL ) {
		cl_log(LOG_ERR, "%s: resource %s not exist.", 
				__FUNCTION__, subrscid);
		return HA_FAIL;
	}


	ret = cim_add_subrsc(resource, subrsc);
	ha_msg_del(subrsc);
	ha_msg_del(resource);
	
	rc->rc = (ret == HA_OK) ? CMPI_RC_OK : CMPI_RC_ERR_FAILED;
	return ret;
}

