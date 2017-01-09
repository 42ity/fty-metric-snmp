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
    int filler;     //  Declare class properties here
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
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Main fty_metric_snmp_server actor

void
fty_metric_snmp_server_actor (zsock_t *pipe, void *args)
{
    fty_metric_snmp_server_t *self = fty_metric_snmp_server_new ();
    assert (self);
    zpoller_t *poller = zpoller_new (pipe, NULL);
    // TODO: read list of communities (zconfig)
    // TODO: connect to malamute
    // TODO: read rules
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
                    zstr_free (&cmd);
                }
                zmsg_destroy (&msg);
            }
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
