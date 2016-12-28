/*  =========================================================================
    host_actor - Actor testing one host

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

/*
@header
    host_actor - Actor testing one host
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"
#include "host_actor.h"
#include "credentials.h"
#include "luasnmp.h"

#include <lualib.h>
#include <lauxlib.h>

#include <zactor.h>
#include <zhash.h>
#include <zmsg.h>
//  --------------------------------------------------------------------------
//  free function for zhash

void zhash_lua_free (void *data) {
    if (!data) return;
    lua_State *l = (lua_State *)data;
    luasnmp_destroy (&l);
}

//  --------------------------------------------------------------------------
//  compile lua function and add it to list
void host_actor_add_lua_function (zhash_t *functions, char *name, char *func, const snmp_credentials_t *cred)
{
    lua_State *l = luasnmp_new ();
    luaL_dostring (l, func);
    // TODO: check whether it compiles
    // TODO: check whether main(host) exists
    // TODO: push snmp credentials
    zhash_insert (functions, name, func);
    zhash_freefn (functions, name, zhash_lua_free);
}

//  --------------------------------------------------------------------------
//  actor function

void
host_actor (zsock_t *pipe, void *args)
{
    char *host = NULL;
    // snmp credentials?
    snmp_credentials_t credentials = {0, NULL};
    zhash_t *functions = zhash_new ();
    zhash_autofree (functions);
    
    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        zmsg_t *msg = zmsg_recv (pipe);
        if (msg) {
            char *cmd = zmsg_popstr (msg);
            if (cmd) {
                if (streq (cmd, "$TERM")) {
                    zstr_free (&cmd);
                    zmsg_destroy (&msg);
                    goto cleanup;
                }
                else if (streq (cmd, "POLL")) {
                    // TODO
                }
                else if (streq (cmd, "LUA")) {
                    char *name = zmsg_popstr (msg);
                    char *func = zmsg_popstr (msg);
                    if (name && func) {
                        host_actor_add_lua_function (functions, name, func, &credentials);
                    }
                    zstr_free (&name);
                    zstr_free (&func);
                }
                else if (streq (cmd, "CREDENTIALS")) {
                    char *version = zmsg_popstr (msg);
                    char *community = zmsg_popstr (msg);
                    if (version && community) {
                        zstr_free (&credentials.community);
                        credentials.version = atoi (version);
                        credentials.community = community;
                        community = NULL;
                    }
                    zstr_free (&version);
                    zstr_free (&community);
                }
                else if (streq (cmd, "HOST")) {
                    zstr_free (&host);
                    host = zmsg_popstr (msg);
                }
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
        }
    }

 cleanup:
    zhash_destroy (&functions);
    zstr_free (&host);
    zstr_free (&credentials.community);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
host_actor_test (bool verbose)
{
    printf (" * host_actor: ");
    //  @selftest
    zactor_t *actor = zactor_new (host_actor, NULL);
    assert (actor);
    zstr_sendx (actor, "HOST", "127.0.0.1", NULL);
    zstr_sendx (actor, "CREDENTIALS", "1", "public", NULL);
    zstr_sendx (actor, "FUNCTION", "load", "function main(host) return { 'load', 15, '%' } end", NULL);
    // zmsg_sendx (actor, "POLL", NULL);
    // zmsg_t *msg = zmsg_recv (actor); // get metric
    zactor_destroy (&actor);    
    //  @end
    printf ("OK\n");
}
