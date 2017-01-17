/*  =========================================================================
    luasnmp - lua snmp extension

    Copyright (C) 2016 - 2017 Tomas Halman                                 
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

#include "fty_metric_snmp_classes.h"
//#include "ftysnmp.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <stdio.h>
#include <stdbool.h>

typedef u_long myoid;

//  --------------------------------------------------------------------------
//  Convert SNMP version (1, 2, 3) to net-snmp enums

int snmp_version_to_enum (int version)
{
    switch (version) {
    case 1:
        return SNMP_VERSION_1;
    case 2:
        return SNMP_VERSION_2c;
    case 3:
        return SNMP_VERSION_3;
    default:
        return -1;
    }
}

//  --------------------------------------------------------------------------
//  Convert net-snmp version enums to number (1, 2, 3)

int snmp_enum_to_version (int version)
{
    switch (version) {
    case SNMP_VERSION_1:
        return 1;
    case SNMP_VERSION_2c:
        return 2;
    case SNMP_VERSION_3:
        return 3;
    default:
        return -1;
    }
}

//  --------------------------------------------------------------------------
//  Convert net-snmp oid to char * string like ".1.3.4.6"

char *oid_to_sring (myoid anOID[], int len)
{
    char buffer[1024];
    
    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_OID_OUTPUT_FORMAT, NETSNMP_OID_OUTPUT_NUMERIC);
    memset (buffer, 0, sizeof (buffer));
    if(snprint_objid(buffer, sizeof(buffer)-1, anOID, len) == -1) return NULL;
    return strdup (buffer);
}

//  --------------------------------------------------------------------------
//  Converts net-snmp value into string

char *var_to_sring (const netsnmp_variable_list *variable)
{
    char buffer[1024];
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, true);
    memset (buffer, 0, sizeof (buffer));
    if(snprint_value(buffer, sizeof(buffer)-1, variable->name, variable->name_length, variable) == -1) return NULL;
    return strdup (buffer);
}

//  --------------------------------------------------------------------------
//  snmp get version 1 and 2c

char* ftysnmp_get_v12 (const char* host, const char *oid, const snmp_credentials_t* credentials)
{
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    char *result = NULL;
    myoid anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    
    struct variable_list *vars;
    int status;

    snmp_sess_init (&session);
    session.peername = (char *)host;
    session.version = snmp_version_to_enum (credentials->version);
    session.community = (unsigned char *) credentials->community;
    session.community_len = strlen (credentials->community);
   
    ss = snmp_open (&session);
    if (!ss) return NULL;
    
    pdu = snmp_pdu_create (SNMP_MSG_GET);
    read_objid (oid, anOID, &anOID_len);
    snmp_add_null_var(pdu, anOID, anOID_len);
   
    status = snmp_synch_response (ss, pdu, &response);
   
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        vars = response->variables;
        result = var_to_sring (vars);
    }
    if (response) snmp_free_pdu (response);
    snmp_close(ss);
    return result;
}

//  --------------------------------------------------------------------------
//  snmp get-next version 1 and 2c

void ftysnmp_getnext_v12 (const char* host, const char *oid, const snmp_credentials_t* credentials, char **resultoid, char **resultvalue)
{
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu;
    struct snmp_pdu *response;
    unsigned long int anOID[MAX_OID_LEN];
    size_t anOID_len = MAX_OID_LEN;
    char *nextoid = NULL;
    char *nextvalue = NULL; 
    struct variable_list *vars;
    int status;

    *resultoid = NULL;
    *resultvalue = NULL;        

    snmp_sess_init (&session);
    session.peername = (char *)host;
    session.version = snmp_version_to_enum (credentials->version);
    session.community = (unsigned char *) credentials->community;
    session.community_len = strlen (credentials->community);
   
    ss = snmp_open (&session);
    if (!ss) return;
    
    pdu = snmp_pdu_create (SNMP_MSG_GETNEXT);
    read_objid (oid, anOID, &anOID_len);
    snmp_add_null_var(pdu, anOID, anOID_len);
   
    status = snmp_synch_response (ss, pdu, &response);
   
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        
        vars = response->variables; // we should have just one variable
        nextoid = oid_to_sring (vars->name, vars->name_length);
        nextvalue = var_to_sring (vars);
    }
    if (nextoid && nextvalue) {
        *resultoid = nextoid;
        *resultvalue = nextvalue;
    } else {
        if (resultoid) free (resultoid);
        if (resultvalue) free (resultvalue);
        *resultoid = NULL;
        *resultvalue = NULL;        
    }
    if (response) snmp_free_pdu (response);
    snmp_close(ss);
}

//  --------------------------------------------------------------------------
//  snmp get function

char *ftysnmp_get (const char* host, const char *oid, const snmp_credentials_t *credentials)
{
    if (!host || !oid || !credentials) return NULL;
    if (credentials->version == 3) {
        // Not supported yet
        // TODO: SNMPv3 support
        return NULL;
    }
    return ftysnmp_get_v12 (host, oid, credentials);
}

//  --------------------------------------------------------------------------
//  snmp getnext function

void ftysnmp_getnext (const char* host, const char *oid, const snmp_credentials_t *credentials, char **resultoid, char **resultvalue)
{
    if (!host || !oid || !credentials || !resultoid || !resultvalue) {
        if (resultoid) *resultoid = NULL;
        if (resultvalue) *resultvalue = NULL;
    }
    if (credentials->version == 3) {
        // Not supported yet
        // TODO: SNMPv3 support
        *resultoid = NULL;
        *resultvalue = NULL;
    }
    ftysnmp_getnext_v12 (host, oid, credentials, resultoid, resultvalue);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void ftysnmp_test (bool verbose)
{
    //@ test is empty 
}

