/*  =========================================================================
    luasnmp - lua snmp extension

    Copyright (C) 2014 - 2015 Eaton                                        
                                                                           
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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

//#define DISABLE_MIB_LOADING 1


//#include <net-snmp/library/oid.h>
#include <stdio.h>
#include <stdbool.h>


typedef u_long myoid;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

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

int enum_to_snmp_version (int version)
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

char *oid_to_sring (myoid anOID[], int len)
{
    char buffer[1024];
    
    netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_OID_OUTPUT_FORMAT, NETSNMP_OID_OUTPUT_NUMERIC);
    memset (buffer, 0, sizeof (buffer));
    if(snprint_objid(buffer, sizeof(buffer)-1, anOID, len) == -1) return NULL;
    return strdup (buffer);
}

char *var_to_sring (const netsnmp_variable_list *variable)
{
    char buffer[1024];
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, true);
    memset (buffer, 0, sizeof (buffer));
    if(snprint_value(buffer, sizeof(buffer)-1, variable->name, variable->name_length, variable) == -1) return NULL;
    return strdup (buffer);
}



char* snmp_get_v12 (const char* host, const char *oid, const char* community, int version)
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
    session.version = version = version;
    session.community = (unsigned char *)community;
    session.community_len = strlen (community);
   
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

void snmp_getnext_v12 (const char* host, const char *oid, const char* community, int version, char **resultoid, char **resultvalue)
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
    session.version = version = version;
    session.community = (unsigned char *)community;
    session.community_len = strlen (community);
   
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


void luasnmp_init()
{
    init_snmp("fty-snmp-client");
}

static int snmp_get(lua_State *L)
{
	char* host = lua_tostring(L, 1);
	char* oid = lua_tostring(L, 2);

    if (!host || !oid ) {
        return 0;
    }
    
    // get credentials/snmpversion for host
    lua_getglobal(L, "SNMP_VERSION");
    int version = -1;
    char *versionstr = lua_tostring (L, -1);
    if (versionstr) version = snmp_version_to_enum(atoi (versionstr));
    
    lua_getglobal(L, "SNMP_COMMUNITY_NAME");
    char *community = lua_tostring (L, -1);
    if (! community || (version == -1)) {
        return 0;
    }
    
    char *result = snmp_get_v12 (host, oid, community, version);
    if (result) {
        lua_pushstring (L, result);
        free (result);
        return 1;
    } else {
        return 0;
    }
}

static int snmp_getnext(lua_State *L)
{
	char* host = lua_tostring(L, 1);
	char* oid = lua_tostring(L, 2);
    if (!host || !oid ) {
        return 0;
    }
    
    // get credentials/snmpversion for host
    lua_getglobal(L, "SNMP_VERSION");
    int version = -1;
    char *versionstr = lua_tostring (L, -1);
    if (versionstr) version = snmp_version_to_enum (atoi (versionstr));
    
    lua_getglobal(L, "SNMP_COMMUNITY_NAME");
    char *community = lua_tostring (L, -1);
    if (! community || (version == -1)) {
        return 0;
    }
    
    char *nextoid, *nextvalue;
    snmp_getnext_v12 (host, oid, community, version, &nextoid, &nextvalue);
    if (nextoid && nextvalue) {
        lua_pushstring (L, nextoid);
        lua_pushstring (L, nextvalue);
        free (nextoid);
        free (nextvalue);
        return 2;
    } else {
        if (nextoid) free (nextoid);
        if (nextvalue) free (nextvalue);
        return 0;
    }
}


void extend_lua_of_snmp(lua_State *L)
{
    lua_register (L, "snmp_get", snmp_get);
    lua_register (L, "snmp_getnext", snmp_getnext);
}

lua_State *luasnmp_new (void)
{
#if LUA_VERSION_NUM > 501
    lua_State *l = luaL_newstate();   
#else
    lua_State *l = lua_open();
#endif
    if (!l) return NULL;
    luaL_openlibs(l); // get functions like print();
    extend_lua_of_snmp (l); //extend of snmp
    return l;
}

void luasnmp_destroy (lua_State **self_p)
{
    if (!self_p || !*self_p) return;
    lua_close (*self_p);
    *self_p = NULL;
}

void luasnmp_test (bool verbose)
{
    //@ test is empty 
}

