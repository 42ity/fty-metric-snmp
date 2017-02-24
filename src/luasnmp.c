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


#include "luasnmp.h"

#include <stdio.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>


//  --------------------------------------------------------------------------
//  Init net-snmp library

void luasnmp_init()
{
    init_snmp("fty-snmp-client");
}

//  --------------------------------------------------------------------------
//  SNMP get lua binding

static int lua_snmp_get(lua_State *L)
{
    const char* host = lua_tostring(L, 1);
    const char* oid = lua_tostring(L, 2);

    if (!host || !oid ) {
        return 0;
    }

    // get credentials/snmpversion for host
    snmp_credentials_t credentials;
    lua_getglobal(L, "SNMP_VERSION");
    credentials.version = -1;
    const char *versionstr = lua_tostring (L, -1);
    if (versionstr)
        credentials.version = atoi (versionstr);
    lua_getglobal(L, "SNMP_COMMUNITY_NAME");
    credentials.community = (char *)lua_tostring (L, -1);

    if (! credentials.community || (credentials.version < 1)) {
        return 0;
    }
    char *result = ftysnmp_get (host, oid, &credentials);
    if (result) {
        lua_pushstring (L, result);
        free (result);
        return 1;
    } else {
        return 0;
    }
}

//  --------------------------------------------------------------------------
//  SNMP get-next lua binding

static int lua_snmp_getnext(lua_State *L)
{
    const char *host = lua_tostring(L, 1);
    const char *oid = lua_tostring(L, 2);
    if (!host || !oid ) {
        return 0;
    }

    // get credentials/snmpversion for host
    snmp_credentials_t credentials;
    lua_getglobal(L, "SNMP_VERSION");
    credentials.version = -1;
    const char *versionstr = lua_tostring (L, -1);
    if (versionstr)
        credentials.version = atoi (versionstr);

    lua_getglobal(L, "SNMP_COMMUNITY_NAME");
    credentials.community = (char *)lua_tostring (L, -1);
    if (! credentials.community || (credentials.version < 1)) {
        return 0;
    }

    char *nextoid, *nextvalue;
    ftysnmp_getnext (host, oid, &credentials, &nextoid, &nextvalue);
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


//  --------------------------------------------------------------------------
//  Register SNMP functions in lua

void extend_lua_of_snmp(lua_State *L)
{
    lua_register (L, "snmp_get", lua_snmp_get);
    lua_register (L, "snmp_getnext", lua_snmp_getnext);
}

//  --------------------------------------------------------------------------
//  Create a new lua state with SNMP support

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

//  --------------------------------------------------------------------------
//  Destroy luasnmp

void luasnmp_destroy (lua_State **self_p)
{
    if (!self_p || !*self_p) return;
    lua_close (*self_p);
    *self_p = NULL;
}

//  --------------------------------------------------------------------------
//  Selftest is empty

void luasnmp_test (bool verbose)
{
    //@ test is empty
}

