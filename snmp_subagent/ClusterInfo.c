/* This file was generated by mib2c and is intended for use as a mib module
   for the ucd-snmp snmpd agent. */


#ifdef IN_UCD_SNMP_SOURCE
/* If we're compiling this file inside the ucd-snmp source tree */

/* This should always be included first before anything else */
#include <config.h>


/* minimal include directives */
#include "mibincl.h"
#include "util_funcs.h"


#else /* !IN_UCD_SNMP_SOURCE */

#include "config.h" /* linux-ha/config.h */

#include <ucd-snmp/ucd-snmp-config.h>
#include <ucd-snmp/ucd-snmp-includes.h>
#include <ucd-snmp/ucd-snmp-agent-includes.h>

#ifdef HAVE_NET_SNMP
	#ifdef HAVE_NET_SNMP_UTIL_FUNCS_H
		#include <ucd-snmp/util_funcs.h>
	#else /* !HAVE_NET_SNMP_UTIL_FUNCS_H */
		#include "ucd_util_funcs.h"
	#endif /* HAVE_NET_SNMP_UTIL_FUNCS_H */ 
#else /* !HAVE_NET_SNMP */
	#include <ucd-snmp/util_funcs.h>
#endif /* HAVE_NET_SNMP */ 

#endif /* !IN_UCD_SNMP_SOURCE */


#include "ClusterInfo.h"
#include <clplumbing/cl_log.h>
#include "haclient.h"


/* 
 * ClusterInfo_variables_oid:
 *   this is the top level oid that we want to register under.  This
 *   is essentially a prefix, with the suffix appearing in the
 *   variable below.
 */


oid ClusterInfo_variables_oid[] = { 1,3,6,1,4,1,4682,1 };


/* 
 * variable2 ClusterInfo_variables:
 *   this variable defines function callbacks and type return information 
 *   for the ClusterInfo mib section 
 */


struct variable2 ClusterInfo_variables[] = {
/*  magic number        , variable type , ro/rw , callback fn  , L, oidsuffix */
#define   NODECOUNT             1
  { NODECOUNT           , ASN_COUNTER   , RONLY , var_ClusterInfo, 1, { 1 } },

};
/*    (L = length of the oidsuffix) */


/*
 * init_ClusterInfo():
 *   Initialization routine.  This is called when the agent starts up.
 *   At a minimum, registration of your variables should take place here.
 */
void init_ClusterInfo(void) {


	/* register ourselves with the agent to handle our mib tree */
	REGISTER_MIB("ClusterInfo", ClusterInfo_variables, variable2,
			ClusterInfo_variables_oid);


	/* place any other initialization junk you need here */
	(void) _ha_msg_h_Id;
}


/*
 * var_ClusterInfo():
 *   This function is called every time the agent gets a request for
 *   a scalar variable that might be found within your mib section
 *   registered above.  It is up to you to do the right thing and
 *   return the correct value.
 *     You should also correct the value of "var_len" if necessary.
 *
 *   Please see the documentation for more information about writing
 *   module extensions, and check out the examples in the examples
 *   and mibII directories.
 */
unsigned char *
var_ClusterInfo(struct variable *vp, 
                oid     *name, 
                size_t  *length, 
                int     exact, 
                size_t  *var_len, 
                WriteMethod **write_method)
{


	/* variables we may use later */
	static long long_ret;
	
	if (header_generic(vp,name,length,exact,var_len,write_method)
	                                == MATCH_FAILED )
		return NULL;
	
	
	/* 
	 * this is where we do the value assignments for the mib results.
	 */
	switch(vp->magic) {
	
		case NODECOUNT:
			long_ret = 0;
			if (get_node_count(&long_ret) != HA_OK) 
				return NULL;
			return (unsigned char *) &long_ret;
	
		default:
			ERROR_MSG("");
	}
	return NULL;
}

