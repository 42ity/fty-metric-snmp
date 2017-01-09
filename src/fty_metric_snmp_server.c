/*  =========================================================================
    fty_metric_snmp_server - Actor

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
    fty_metric_snmp_server - Actor
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

//  Structure of our class

struct _fty_metric_snmp_server_t {
    mlm_client_t *mlm;
    zlist_t *rules;
    zhash_t *host_actors;
};


//  --------------------------------------------------------------------------
//  Create a new fty_metric_snmp_server

fty_metric_snmp_server_t *
fty_metric_snmp_server_new (void)
{
    fty_metric_snmp_server_t *self = (fty_metric_snmp_server_t *) zmalloc (sizeof (fty_metric_snmp_server_t));
    assert (self);
    
    self->mlm = mlm_client_new();
    assert (self->mlm);
    
    self->rules = zlist_new();
    assert (self->rules);
    zlist_autofree (self->rules);

    self->host_actors = zhash_new();
    assert (self->host_actors);
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the fty_metric_snmp_server

void
fty_metric_snmp_server_destroy (fty_metric_snmp_server_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        fty_metric_snmp_server_t *self = *self_p;
        //  Free class properties here
        mlm_client_destroy (&self->mlm);
        zlist_destroy (&self->rules);
        zhash_destroy (&self->host_actors);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

void
fty_metric_snmp_server_load_rules (fty_metric_snmp_server_t *self, const char *path)
{
    if (!self || !path) return;
    char fullpath [PATH_MAX];
    
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry -> d_type == DT_LNK || entry -> d_type == DT_REG) {
            // file or link
            int l = strlen (entry -> d_name);
            if ( l > 5 && streq (&(entry -> d_name[l - 6]), ".json")) {
                // json file
                rule_t *rule = rule_new();
                snprintf (fullpath, PATH_MAX, "%s/%s", path, entry -> d_name);
                if (rule_load (rule, fullpath) == 0) {
                    zlist_append (self->rules, rule);
                    zlist_freefn (self->rules, rule, rule_freefn, true);
                } else {
                    rule_destroy (&rule);
                }
            }
        }
    }
    closedir(dir);
}

zactor_t *
fty_metric_snmp_server_asset_update (fty_metric_snmp_server_t *self, fty_proto_t *ftymsg, zpoller_t *poller)
{
    return NULL;
}

zactor_t *
fty_metric_snmp_server_asset_delete (fty_metric_snmp_server_t *self, fty_proto_t *ftymsg, zpoller_t *poller)
{
    return NULL;
}

//  --------------------------------------------------------------------------
//  Main fty_metric_snmp_server actor

void
fty_metric_snmp_server_actor (zsock_t *pipe, void *args)
{
    fty_metric_snmp_server_t *self = fty_metric_snmp_server_new ();
    assert (self);
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (self->mlm), NULL);
    // TODO: read list of communities (zconfig)
    // TODO: react on asset from stream
    // TODO: react on metric from host_actor
    // TODO: send POLL event to host_actors (zloop)?
    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        zsock_t *which = (zsock_t *)zpoller_wait(poller, 30);
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            if (msg) {
                char *cmd = zmsg_popstr (msg);
                if (cmd) {
                    if (streq (cmd, "$TERM")) {
                        zstr_free (&cmd);
                        zmsg_destroy (&msg);
                        break;
                    }
                    else if (streq (cmd, "BIND")) {
                        char *endpoint = zmsg_popstr (msg);
                        char *myname = zmsg_popstr (msg);
                        assert (endpoint && myname);
                        mlm_client_connect (self->mlm, endpoint, 5000, myname);
                        zstr_free (&endpoint);
                        zstr_free (&myname);
                    }
                    else if (streq (cmd, "PRODUCER")) {
                        char *stream = zmsg_popstr (msg);
                        assert (stream);
                        mlm_client_set_producer (self->mlm, stream);
                        zstr_free (&stream);
                    }
                    else if (streq (cmd, "CONSUMER")) {
                        char *stream = zmsg_popstr (msg);
                        char *pattern = zmsg_popstr (msg);
                        assert (stream && pattern);
                        mlm_client_set_consumer (self->mlm, stream, pattern);
                        zstr_free (&stream);
                        zstr_free (&pattern);
                    }
                    else if (streq (cmd, "LOADRULES")) {
                        char *path = zmsg_popstr (msg);
                        assert (path);
                        fty_metric_snmp_server_load_rules (self, path);
                        zstr_free (&path);
                    }
                    zstr_free (&cmd);
                }
                zmsg_destroy (&msg);
            }
        }
        else if (which == mlm_client_msgpipe (self->mlm)) {
            // got malamute message, probably an asset
            zmsg_t *msg = zmsg_recv (which); 
            if (msg && is_fty_proto (msg)) {
                fty_proto_t *ftymsg = fty_proto_decode (&msg);
                if (fty_proto_id (ftymsg) == FTY_PROTO_ASSET) {
                    if (
                        streq (fty_proto_operation (ftymsg), "update") ||
                        streq (fty_proto_operation (ftymsg), "create")
                    ) {
                        fty_metric_snmp_server_asset_update (self, ftymsg, poller);
                    }
                    if (streq (fty_proto_operation (ftymsg), "delete")) {
                        fty_metric_snmp_server_asset_delete (self, ftymsg, poller);
                    }
                }
                fty_proto_destroy (&ftymsg);
            }
            zmsg_destroy (&msg);
        }
        else if (which == NULL) {
            if (!zsys_interrupted) {
                // this is polling
            }
        }
        else {
            // must be host_actor then
            zmsg_t *msg = zmsg_recv (which);
            char *cmd = zmsg_popstr (msg);
            if (cmd && streq (cmd, "METRIC")) {
                char *type = zmsg_popstr (msg);
                char *element = zmsg_popstr (msg);
                char *value = zmsg_popstr (msg);
                char *units = zmsg_popstr (msg);
                zmsg_t *metric = fty_proto_encode_metric (NULL, type, element, value, units, 60);
                //size_t s = strlen(type) + strlen(element) + 2
                char *topic;
                asprintf(&topic, "%s@%s", type, element);
                mlm_client_send (self->mlm, topic, &metric);
                zmsg_destroy (&metric);
                zstr_free (&topic);
                zstr_free (&type);
                zstr_free (&topic);
                zstr_free (&value);
                zstr_free (&units);
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
        }
    }
    zpoller_destroy (&poller);
    fty_metric_snmp_server_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
fty_metric_snmp_server_test (bool verbose)
{
    printf (" * fty_metric_snmp_server: ");

    //  @selftest
    //  Simple create/destroy test
    fty_metric_snmp_server_t *self = fty_metric_snmp_server_new ();
    assert (self);
    fty_metric_snmp_server_destroy (&self);
    //  @end
    printf ("OK\n");
}
