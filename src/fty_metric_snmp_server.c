/*  =========================================================================
    fty_metric_snmp_server - Main actor

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
    fty_metric_snmp_server - Main actor
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

//  Structure of our class

struct _fty_metric_snmp_server_t {
    mlm_client_t *mlm;
    zlist_t *rules;
    zhash_t *host_actors;
    zpoller_t *poller;
    credentials_t *credentials;
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

    self->host_actors = zhash_new();
    assert (self->host_actors);

    self->credentials = credentials_new();
    assert (self->credentials);
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
        zpoller_destroy (&self->poller);
        credentials_destroy (&self->credentials);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Add one json-encoded rule to list of rules

void
fty_metric_snmp_server_add_rule (fty_metric_snmp_server_t *self, const char *json)
{
    if (!self || !json) return;

    rule_t *rule = rule_new();
    if (rule_parse (rule, json) == 0) {
        zlist_append (self->rules, rule);
        zlist_freefn (self->rules, rule, rule_freefn, true);
    } else {
        rule_destroy (&rule);
    }
}

//  --------------------------------------------------------------------------
//  Load all rules in directory. Rule MUST have ".json" extension.

void
fty_metric_snmp_server_load_rules (fty_metric_snmp_server_t *self, const char *path)
{
    if (!self || !path) return;
    char fullpath [PATH_MAX];

    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry -> d_type == DT_LNK || entry -> d_type == DT_REG || entry -> d_type == 0) {
            // file or link
            int l = strlen (entry -> d_name);
            if ( l > 5 && streq (&(entry -> d_name[l - 6]), ".rule")) {
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

//  --------------------------------------------------------------------------
//  Function returns true if function should be evaluated for particular asset.
//  This is decided by asset name (json "assets": []) or group (json "groups":[])

int
is_rule_for_this_asset (rule_t *rule, fty_proto_t *ftymsg)
{
    if (!rule || !ftymsg) return 0;

    char *asset = (char *)fty_proto_name (ftymsg);
    if (zlist_exists (rule_assets(rule), asset)) return 1;

    zhash_t *ext = fty_proto_ext (ftymsg);
    zlist_t *keys = zhash_keys (ext);
    char *key = (char *)zlist_first (keys);
    while (key) {
        if (strncmp ("group.", key, 6) == 0) {
            // this is group
            char * grp = (char *)zhash_lookup (ext, key);
            if (zlist_exists (rule_groups (rule), grp)) {
                zlist_destroy (&keys);
                return 1;
            }
        }
        key = (char *)zlist_next (keys);
    }
    zlist_destroy (&keys);

    const char *model = fty_proto_ext_string (ftymsg, "model", NULL);
    if (model && zlist_exists (rule_models (rule), (void *) model)) return 1;
    model = fty_proto_ext_string (ftymsg, "device.part", NULL);
    if (model && zlist_exists (rule_models (rule), (void *) model)) return 1;

    return 0;
}

//  --------------------------------------------------------------------------
//  Try SNMP credentials with this host and return first working.

const snmp_credentials_t *
fty_metric_snmp_server_detect_credentials (fty_metric_snmp_server_t *self, const char *ip)
{
    const snmp_credentials_t *cr = credentials_first (self->credentials);
    const char *startoid = ".1";
    char *oid, *value;

    while (cr) {
        ftysnmp_getnext (
            ip,
            startoid,
            cr,
            &oid,
            &value
        );
        if (oid && value) {
            free (oid);
            free (value);
            return cr;
        }
        cr = credentials_next (self->credentials);
    }
    return NULL;
}

//  --------------------------------------------------------------------------
//  Update poller to have all existing sockets, pipes and actors

void
fty_metric_snmp_server_update_poller (fty_metric_snmp_server_t *self, zsock_t *pipe)
{
    if (!self || !pipe ) return;
    zpoller_destroy (&self -> poller);
    self -> poller = zpoller_new (pipe, mlm_client_msgpipe (self -> mlm), NULL);
    zactor_t *a = (zactor_t *) zhash_first (self -> host_actors);
    while (a) {
        zpoller_add (self -> poller, a);
        a = (zactor_t *) zhash_next (self -> host_actors);
    }
}


//  --------------------------------------------------------------------------
//  When asset message comes, function creates new host_actor if not exists.

zactor_t *
fty_metric_snmp_server_asset (fty_metric_snmp_server_t *self, fty_proto_t *ftymsg, zsock_t *pipe)
{
    if (!self || !ftymsg) return NULL;

    const char *operation = fty_proto_operation (ftymsg);
    const char *assetname = fty_proto_name (ftymsg);

    if (streq (operation, "delete")) {
        if (zhash_lookup (self->host_actors, assetname)) {
            zhash_delete (self->host_actors, assetname);
            fty_metric_snmp_server_update_poller (self, pipe);
        }
        return NULL;
    }
    if (streq (operation, "inventory") && streq (mlm_client_sender (self -> mlm), "asset-autoupdate")) {
        zhash_t *ext = fty_proto_ext (ftymsg);
        const char *ip = (char *)zhash_lookup (ext, "ip.1");
        if (!ip) return NULL;
        zactor_t *host = (zactor_t *) zhash_lookup (self->host_actors, assetname);
        if (host) zstr_send (host, "DROPLUA");

        rule_t *rule = (rule_t *)zlist_first (self->rules);
        bool haverule = false;
        while (rule) {
            if (is_rule_for_this_asset (rule, ftymsg)) {
                haverule = true;
                if (!host) {
                    zsys_debug ("deploying actor for %s", assetname);
                    host = zactor_new(host_actor, NULL);
                    assert (host);
                    zhash_insert (self->host_actors, assetname, host);
                    zhash_freefn (self->host_actors, assetname, host_actor_freefn);
                    zstr_sendx (host, "ASSETNAME", assetname, NULL);
                    fty_metric_snmp_server_update_poller (self, pipe);
                }
                zsys_debug ("function '%s' send to '%s' actor", rule_name (rule), assetname);
                zstr_sendx (host, "LUA", rule_name (rule), rule_evaluation (rule), NULL);
            }
            rule = (rule_t *)zlist_next (self->rules);
        }
        if (!haverule) {
            zsys_debug ("no rule for %s", assetname);
            if (host) {
                zactor_destroy (&host);
                fty_metric_snmp_server_update_poller (self, pipe);
            }
            return NULL;
        }
        zstr_sendx (host, "IP", ip, NULL);
        const snmp_credentials_t *cr = fty_metric_snmp_server_detect_credentials (self, ip);
        if (cr) {
            char *versionstr = zsys_sprintf ("%i", cr->version);
            zstr_sendx (host, "CREDENTIALS", versionstr, cr->community, NULL);
            zstr_free (&versionstr);
        } else {
            zsys_error ("Can't detect SNMP credentials for %s", assetname);
            zstr_sendx (host, "CREDENTIALS", "0", "", NULL);
        }
        return host;
    }
    return NULL;
}

//  --------------------------------------------------------------------------
//  Main fty_metric_snmp_server actor
void
fty_metric_snmp_server_actor_main_loop (fty_metric_snmp_server_t *self, zsock_t *pipe)
{
    if (!self || !pipe) return;

    int ttl = 60;
    fty_metric_snmp_server_update_poller (self, pipe);
    zsock_signal (pipe, 0);
    while (!zsys_interrupted) {
        zsock_t *which = (zsock_t *) zpoller_wait (self -> poller, -1);
        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            if (msg) {
                char *cmd = zmsg_popstr (msg);
                zsys_debug ("pipe command %s", cmd);
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
                    else if (streq (cmd, "LOADCREDENTIALS")) {
                        char *path = zmsg_popstr (msg);
                        assert (path);
                        credentials_load (self->credentials, path);
                        zstr_free (&path);
                    }
                    else if (streq (cmd, "TTL")) {
                        char *ttlstr = zmsg_popstr (msg);
                        assert (ttlstr);
                        ttl = atoi (ttlstr);
                        zstr_free (&ttlstr);
                    }
                    else if (streq (cmd, "RULE")) {
                        char *json = zmsg_popstr (msg);
                        assert (json);
                        fty_metric_snmp_server_add_rule (self, json);
                        zstr_free (&json);
                    }
                    else if (streq (cmd, "WAKEUP")) {
                        zactor_t *a = (zactor_t *) zhash_first (self -> host_actors);
                        while (a) {
                            zstr_send (a, "WAKEUP");
                            a = (zactor_t *) zhash_next (self -> host_actors);
                        }
                    }
                    zstr_free (&cmd);
                }
                zmsg_destroy (&msg);
            }
        }
        else if (which == mlm_client_msgpipe (self->mlm)) {
            // got malamute message, probably an asset
            zmsg_t *msg = mlm_client_recv (self->mlm);
            if (msg && is_fty_proto (msg)) {
                fty_proto_t *ftymsg = fty_proto_decode (&msg);
                if (fty_proto_id (ftymsg) == FTY_PROTO_ASSET) {
                    fty_metric_snmp_server_asset (self, ftymsg, pipe);
                }
                fty_proto_destroy (&ftymsg);
            }
            zmsg_destroy (&msg);
        }
        else if (which != NULL) {
            // must be host_actor then
            zsys_debug ("got host actor message");
            zmsg_t *msg = zmsg_recv (which);
            char *cmd = zmsg_popstr (msg);
            if (cmd && streq (cmd, "METRIC")) {
                char *element = zmsg_popstr (msg);
                char *type = zmsg_popstr (msg);
                char *value = zmsg_popstr (msg);
                char *units = zmsg_popstr (msg);
                char *pollfreq = zmsg_popstr (msg);
                char *desc = zmsg_popstr (msg);
                if (type && element && value && units && pollfreq) {
                    int freq = atoi (pollfreq);
                    char *topic = zsys_sprintf ("%s@%s", type, element);
                    zhash_t *aux = zhash_new ();
                    zhash_autofree (aux);
                    assert (aux);
                    if (desc && strlen (desc)) {
                        zhash_insert (aux, "description", desc);
                    }
                    zmsg_t *metric = fty_proto_encode_metric (aux, time(NULL), ttl*freq, type, element, value, units);
                    mlm_client_send (self->mlm, topic, &metric);
                    zmsg_destroy (&metric);
                    zhash_destroy (&aux);
                    zstr_free (&topic);
                }
                zstr_free (&type);
                zstr_free (&element);
                zstr_free (&value);
                zstr_free (&units);
                zstr_free (&pollfreq);
                zstr_free (&desc);
            }
            zstr_free (&cmd);
            zmsg_destroy (&msg);
        }
    }
}

//  --------------------------------------------------------------------------
//  Zactor interface

void
fty_metric_snmp_server_actor(zsock_t *pipe, void *args)
{
    fty_metric_snmp_server_t *self = fty_metric_snmp_server_new();
    assert (self);
    fty_metric_snmp_server_actor_main_loop (self, pipe);
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

    // actor test
    static const char *endpoint = "inproc://fty-metric-snmp";
    zactor_t *malamute = zactor_new (mlm_server, (void*) "Malamute");
    zstr_sendx (malamute, "BIND", endpoint, NULL);
    if (verbose) zstr_send (malamute, "VERBOSE");

    zactor_t *server = zactor_new (fty_metric_snmp_server_actor, NULL);
    assert (server);
    zstr_sendx (server, "BIND", endpoint, "me", NULL);
    zstr_sendx (server, "PRODUCER", FTY_PROTO_STREAM_METRICS, NULL);
    zstr_sendx (server, "CONSUMER", FTY_PROTO_STREAM_ASSETS, ".*", NULL);
    zstr_sendx (server, "TTL", "100", NULL);

    static const char *rule =
        "{"
        " \"name\" : \"testrule\","
        " \"description\" : \"Rule for testing\","
        " \"assets\" : [\"mydevice\"],"
        " \"groups\" : [\"mygroup\"],"
        " \"evaluation\" : \""
        "   function main (host)"
        "     return { 'temperature', 10, 'C', 'nice tempereture' }"
        "   end"
        " \""
        "}";
    zstr_sendx (server, "RULE", rule, NULL);
    zclock_sleep (1000);

    mlm_client_t *asset = mlm_client_new ();
    mlm_client_connect (asset, endpoint, 5000, "asset-autoupdate");
    mlm_client_set_producer (asset, FTY_PROTO_STREAM_ASSETS);
    mlm_client_set_consumer (asset, FTY_PROTO_STREAM_METRICS, ".*");

    zhash_t *ext = zhash_new();
    zhash_autofree (ext);
    zhash_insert (ext, "ip.1", "127.0.0.1");
    zmsg_t *assetmsg = fty_proto_encode_asset (
        NULL,
        "mydevice",
        "inventory",
        ext
    );
    zhash_destroy (&ext);
    mlm_client_send (asset, "myasset", &assetmsg);
    zmsg_destroy (&assetmsg);

    ext = zhash_new();
    zhash_autofree (ext);
    zhash_insert (ext, "ip.1", "127.0.0.1");
    zhash_insert (ext, "group.1", "mygroup");
    assetmsg = fty_proto_encode_asset (
        NULL,
        "somedev",
        "inventory",
        ext
    );
    zhash_destroy (&ext);
    mlm_client_send (asset, "myasset", &assetmsg);
    zmsg_destroy (&assetmsg);

    zclock_sleep (1000);

    zstr_send (server, "WAKEUP");
    zclock_sleep (1000);

    {
        zmsg_t *received = mlm_client_recv (asset);
        assert (received);
        fty_proto_t *metric = fty_proto_decode (&received);
        fty_proto_print (metric);
        fty_proto_destroy (&metric);
        zmsg_destroy (&received);
    }
    {
        zmsg_t *received = mlm_client_recv (asset);
        assert (received);
        fty_proto_t *metric = fty_proto_decode (&received);
        fty_proto_print (metric);
        fty_proto_destroy (&metric);
        zmsg_destroy (&received);
    }

    mlm_client_destroy (&asset);
    zactor_destroy (&server);
    zactor_destroy (&malamute);
    //  @end
    printf ("OK\n");
}
