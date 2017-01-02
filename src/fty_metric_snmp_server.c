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
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new fty_metric_snmp_server

fty_metric_snmp_server_t *
fty_metric_snmp_server_new (void)
{
    fty_metric_snmp_server_t *self = (fty_metric_snmp_server_t *) zmalloc (sizeof (fty_metric_snmp_server_t));
    assert (self);
    //  Initialize class properties here
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
    zpoller_t *poller = zpoller_new (pipe, NULL);
    
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
                    zstr_free (&cmd);
                }
                zmsg_free (msg);
            }
        }
    }
    
    zpoller_destroy (&poller);
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
