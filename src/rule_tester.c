/*  =========================================================================
    rule_tester - Class for testing rule file

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

/*
@header
    rule_tester - Class for testing rule file
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

#include <lualib.h>
#include <lauxlib.h>

//  --------------------------------------------------------------------------
//  rule tester function

int
rule_tester (
    const char *file,
    int snmpversion,
    const char *community,
    const char *addr)
{
    if (!file) {
        puts ("Rule not specified!");
        return 1;
    }
    
    int result = 0;
    int returnedvalues = 0;
    rule_t *rule = rule_new ();
    lua_State *lua = luasnmp_new();
    if (rule_load (rule, file)) {
        puts ("Error: can't parse rule file!");
        result = 2;
        goto cleanup;
    }
    if (luaL_dostring (lua, rule_evaluation (rule)) != 0) {
        puts ("Error: lua syntax error");
        result = 3;
        goto cleanup;
    }
    lua_pushstring(lua, addr);
    lua_setglobal(lua, "NAME");
    lua_getglobal (lua, "main");
    if (!lua_isfunction (lua, -1)) {
        puts ("Error: main function not found");
        result = 4;
        goto cleanup;
    }
    lua_settop (lua, 0);

    // TODO: set credentials to lua

    lua_getglobal (lua, "main");
    lua_pushstring (lua, addr);
    if (lua_pcall(lua, 1, 1, 0) == 0) {
        // check if result is an array
        if (! lua_istable (lua, -1)) {
            zsys_error ("function did not returned array");
            result = 5;
            goto cleanup;
        }
        int i = 1;
        while (true) {
            const char *type = NULL;
            const char *value = NULL;
            const char *units = NULL;
            const char *desc = NULL;

            lua_pushnumber(lua, i++);
            lua_gettable(lua, -2);
            if (lua_isstring (lua, -1)) type = lua_tostring(lua,-1);
            lua_pop (lua, 1);

            lua_pushnumber(lua, i++);
            lua_gettable(lua, -2);
            if (lua_isstring (lua, -1)) value = lua_tostring(lua,-1);
            lua_pop (lua, 1);

            lua_pushnumber(lua, i++);
            lua_gettable(lua, -2);
            if (lua_isstring (lua, -1)) units = lua_tostring(lua,-1);
            lua_pop (lua, 1);

            lua_pushnumber(lua, i++);
            lua_gettable(lua, -2);
            if (lua_isstring (lua, -1)) desc = lua_tostring(lua,-1);
            lua_pop (lua, 1);

            if (type || value || units) {
                ++returnedvalues;
                printf ("got METRIC/%s/%s/%s/%s/%s\n",
                        addr,
                        type ? type : "(null)",
                        value ? value : "(null)",
                        units ? units : "(null)",
                        desc);
            } else {
                break;
            }
        }
    } else {
        lua_error (lua);
    }
 cleanup:
    luasnmp_destroy (&lua);
    rule_destroy (&rule);
    if (result == 0) {
        printf ("Seems OK, %i values returned.\n", returnedvalues);
    }
    return result;
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
rule_tester_test (bool verbose)
{
    printf (" * rule_tester: ");
    //  @selftest
    //  @end
    printf ("OK\n");
}
