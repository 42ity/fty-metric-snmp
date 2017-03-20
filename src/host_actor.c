/*  =========================================================================
    host_actor - Actor testing one host

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

struct _host_actor_t {
    char *asset;
    char *ip;
    snmp_credentials_t credentials;
    zhash_t *functions;
    unsigned int counter;
};


//  --------------------------------------------------------------------------
//  private polling function class

typedef struct {
    unsigned int polling;
    lua_State *lua;
} polling_function_t;

polling_function_t *pf_new ()
{
    polling_function_t *self = (polling_function_t *) zmalloc (sizeof (polling_function_t));
    assert (self);
    return self;
}

void pf_destroy (polling_function_t **self_p)
{
    if (!self_p || !*self_p) return;

    polling_function_t *self = *self_p;
    if (self -> lua) luasnmp_destroy (&self -> lua);
    free (self);
    *self_p = NULL;
}

void pf_set_polling (polling_function_t *self, unsigned int polling)
{
    if (!self) return;
    self -> polling = polling;
}

void pf_set_lua (polling_function_t *self, lua_State **lua)
{
    if (!self) return;

    if (self->lua) luasnmp_destroy (&self -> lua);
    self -> lua = *lua;
    *lua = NULL;
}

unsigned int pf_polling (polling_function_t *self)
{
    if (!self) return 1;
    return self->polling;
}

lua_State *pf_lua (polling_function_t *self)
{
    if (!self) return NULL;
    return self->lua;
}

void pf_freefn (void *self)
{
    if (!self) return;
    polling_function_t *pf = (polling_function_t *)self;
    pf_destroy (&pf);
}

//  --------------------------------------------------------------------------
//  Create a new host actor

host_actor_t *
host_actor_new ()
{
    host_actor_t *self = (host_actor_t *) zmalloc (sizeof (host_actor_t));
    assert (self);
    self -> functions = zhash_new ();
    return self;
}

//  --------------------------------------------------------------------------
//  Destroy a host actor

void
host_actor_destroy (host_actor_t **self_p)
{
    if (!self_p || !*self_p) return;
    host_actor_t *self = *self_p;

    zstr_free (&self->asset);
    zstr_free (&self->ip);
    zstr_free (&self->credentials.community);
    host_actor_remove_functions (self);
    zhash_destroy (&self->functions);
    free (self);
    *self_p = NULL;
}

//  --------------------------------------------------------------------------
//  Remove lua function

void host_actor_remove_function (host_actor_t *self, const char *name)
{
    if (!self || ! name) return;
    zhash_delete (self->functions, name);
}

//  --------------------------------------------------------------------------
//  Remove lua function

void host_actor_remove_functions (host_actor_t *self)
{
    if (!self) return;
    zlist_t *keys = zhash_keys (self->functions);
    char *key = (char *) zlist_first (keys);
    while (key) {
        host_actor_remove_function (self, key);
        key = (char *) zlist_next (keys);
    }
    zlist_destroy (&keys);
}

//  --------------------------------------------------------------------------
//  set credentials in all lua functions

void host_actor_set_credentials_to_lua (host_actor_t *self)
{
    if (!self) return;

    polling_function_t *pf = (polling_function_t *) zhash_first (self->functions);
    while (pf) {
        lua_State *l = pf_lua (pf);
        lua_pushnumber (l, self->credentials.version);
        lua_setglobal (l, "SNMP_VERSION");
        if (self -> credentials.community) {
            lua_pushstring (l, self -> credentials.community);
            lua_setglobal (l, "SNMP_COMMUNITY_NAME");
        }
        pf = (polling_function_t *) zhash_next (self->functions);
    }
}

//  --------------------------------------------------------------------------
//  compile lua function and add it to list

void host_actor_add_lua_function (host_actor_t *self, const char *name, const char *func, unsigned int polling)
{
    if (!self) return;

    zsys_debug ("adding lua func");
    lua_State *l = luasnmp_new ();
    if (luaL_dostring (l, func) != 0) {
        zsys_error ("rule %s has an error", name);
        luasnmp_destroy (&l);
        return;
    }
    lua_getglobal (l, "main");
    if (!lua_isfunction (l, -1)) {
        zsys_error ("main function not found in rule %s", name);
        luasnmp_destroy (&l);
        return;
    }
    lua_pushnumber (l, self -> credentials.version);
    lua_setglobal (l, "SNMP_VERSION");
    if (self -> credentials.community) {
        lua_pushstring (l, self -> credentials.community);
        lua_setglobal (l, "SNMP_COMMUNITY_NAME");
    }
    lua_settop (l, 0);

    polling_function_t *pf = pf_new ();
    pf_set_polling (pf, polling);
    pf_set_lua (pf, &l);

    zhash_insert (self -> functions, name, pf);
    zhash_freefn (self -> functions, name, pf_freefn);
    zsys_debug ("New function '%s' created", name);
}

//  --------------------------------------------------------------------------
//  evaluate one function and send metric messages

void host_actor_evaluate (polling_function_t *pf, const char *name, const char *ip, zsock_t *pipe)
{
    lua_State *l = pf_lua (pf);
    lua_settop (l, 0);
    lua_pushstring (l, name);
    lua_setglobal (l, "NAME");
    lua_getglobal (l, "main");
    lua_pushstring (l, ip);

    zsys_debug ("lua called for %s", name);
    if (lua_pcall(l, 1, 1, 0) == 0) {
        // check if result is an array
        if (! lua_istable (l, -1)) {
            zsys_error ("function did not returned array");
            return;
        }
        char *pollfreq = zsys_sprintf("%i", pf_polling (pf));
        int i = 1;
        while (true) {
            const char *type = NULL;
            const char *value = NULL;
            const char *units = NULL;
            const char *description = "";

            lua_pushnumber(l, i++);
            lua_gettable(l, -2);
            if (lua_isstring (l, -1)) type = lua_tostring(l,-1);
            lua_pop (l, 1);

            lua_pushnumber(l, i++);
            lua_gettable(l, -2);
            if (lua_isstring (l, -1)) value = lua_tostring(l,-1);
            lua_pop (l, 1);

            lua_pushnumber(l, i++);
            lua_gettable(l, -2);
            if (lua_isstring (l, -1)) units = lua_tostring(l,-1);
            lua_pop (l, 1);

            lua_pushnumber(l, i++);
            lua_gettable(l, -2);
            if (lua_isstring (l, -1)) description = lua_tostring(l,-1);
            lua_pop (l, 1);

            if (type && value && units) {
                zsys_debug ("sending METRIC/%s/%s/%s/%s/%s/%s", name, type, value, units, pollfreq, description);
                zstr_sendx (pipe, "METRIC", name, type, value, units, pollfreq, description, NULL);
            } else {
                break;
            }
        }
        zstr_free (&pollfreq);
    }
}

//  --------------------------------------------------------------------------
//  actor main loop

void
host_actor_main_loop (host_actor_t *self, zsock_t *pipe)
{
    if (! self) {
        zsock_signal (pipe, -1);
        return;
    }

    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        zmsg_t *msg = zmsg_recv (pipe);
        if (msg) {
            char *cmd = zmsg_popstr (msg);
            if (cmd) {
                if (streq (cmd, "$TERM")) {
                    zstr_free (&cmd);
                    zmsg_destroy (&msg);
                    break; //goto cleanup;
                }
                else if (streq (cmd, "WAKEUP")) {
                    zsys_debug ("actor for '%s' received WAKEUP command, (%s)", self->asset, self->ip);
                    if (self->ip) {
                        polling_function_t *pf = (polling_function_t *) zhash_first (self->functions);
                        if (!pf) zsys_error ("asset '%s' has no defined function", self->asset);
                        while(pf) {
                            if (self -> counter % pf_polling (pf) == 0) {
                                host_actor_evaluate (pf, self->asset, self->ip, pipe);
                            }
                            pf = (polling_function_t *) zhash_next (self->functions);
                        }
                    }
                    ++ self -> counter;
                    zsys_debug ("counter: %i", self->counter);
                }
                else if (streq (cmd, "LUA")) {
                    char *name = zmsg_popstr (msg);
                    char *func = zmsg_popstr (msg);
                    char *polling = zmsg_popstr (msg);
                    if (name && func) {
                        unsigned int ipolling = polling ? atoi (polling) : 1;
                        host_actor_add_lua_function (self, name, func, ipolling);
                    }
                    zstr_free (&name);
                    zstr_free (&func);
                    zstr_free (&polling);
                }
                else if (streq (cmd, "DROPLUA")) {
                    host_actor_remove_functions (self);
                }
                else if (streq (cmd, "CREDENTIALS")) {
                    char *version = zmsg_popstr (msg);
                    char *community = zmsg_popstr (msg);
                    if (version && community) {
                        zstr_free (&self->credentials.community);
                        self -> credentials.version = atoi (version);
                        self -> credentials.community = community;
                        host_actor_set_credentials_to_lua (self);
                        community = NULL;
                    }
                    zstr_free (&version);
                    zstr_free (&community);
                }
                else if (streq (cmd, "ASSETNAME")) {
                    zstr_free (&self -> asset);
                    self -> asset = zmsg_popstr (msg);
                }
                else if (streq (cmd, "IP")) {
                    zstr_free (&self -> ip);
                    self -> ip = zmsg_popstr (msg);
                }
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
        }
    }
}

//  --------------------------------------------------------------------------
//  actor function

void host_actor (zsock_t *pipe, void *args)
{
    host_actor_t *self = host_actor_new();
    assert (self);
    host_actor_main_loop (self, pipe);
    host_actor_destroy (&self);
}

//  --------------------------------------------------------------------------
//  actor freefn for zhash/zlist

void host_actor_freefn (void *self)
{
    if (!self) return;
    zactor_t *ha = (zactor_t *) self;
    zactor_destroy (&ha);
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
    zstr_sendx (actor, "ASSETNAME", "localhost", NULL);
    zstr_sendx (actor, "IP", "127.0.0.1", NULL);
    zstr_sendx (actor, "CREDENTIALS", "1", "public", NULL);
    zstr_sendx (actor, "LUA", "load", "function main(host) return { 'load', 15, '%' } end", "1", NULL);

    zstr_sendx (actor, "WAKEUP", NULL);
    zmsg_t *msg = zmsg_recv (actor);
    char *c = zmsg_popstr (msg);
    assert (c);
    assert (streq (c, "METRIC"));
    zstr_free (&c);
    c = zmsg_popstr (msg);
    assert (c);
    assert (streq (c, "localhost"));
    zstr_free (&c);
    c = zmsg_popstr (msg);
    assert (c);
    assert (streq (c, "load"));
    zstr_free (&c);
    c = zmsg_popstr (msg);
    assert (c);
    assert (streq (c, "15"));
    zstr_free (&c);
    c = zmsg_popstr (msg);
    assert (c);
    assert (streq (c, "%"));
    zstr_free (&c);
    zmsg_destroy (&msg);

    zactor_destroy (&actor);
    //  @end
    printf ("OK\n");
}
