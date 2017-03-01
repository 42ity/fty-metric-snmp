/*  =========================================================================
    fty_metric_snmp - agent for getting measurements using LUA and SNMP

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
    fty_metric_snmp - agent for getting measurements using LUA and SNMP
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

static const char *ACTOR_NAME = "fty-metric-smtp";
static const char *ENDPOINT = "ipc://@/malamute";
static const char *RULES_DIR = "./rules";
static const char *SNMP_CONFIG_FILE = "/etc/sysconfig/fty.cfg";
static int POLLING = 60;

static int
s_wakeup_event (zloop_t *loop, int timer_id, void *output)
{
    zstr_send (output, "WAKEUP");
    return 0;
}

int main (int argc, char *argv [])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        const char *param = NULL;
        if (argn < argc - 1) param = argv [argn+1];
        
        if (streq (argv [argn], "--help") ||  streq (argv [argn], "-h")) {
            puts ("fty-metric-snmp [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            puts ("  --endpoint / -e        malamute endpoint [ipc://@/malamute]");
            puts ("  --snmpconfig / -c      config file with SNMP communities [/etc/sysconfig/fty.cfg]");
            puts ("  --rules / -r           directory with rules [./rules]");
            puts ("  --polling / -p         polling interval in seconds [60]");
            return 0;
        }
        else if (streq (argv [argn], "--verbose") ||  streq (argv [argn], "-v")) {
            verbose = true;
        }
        else if (streq (argv [argn], "--endpoint") || streq (argv [argn], "-e")) {
            if (param) ENDPOINT = param;
            ++argn;
        }
        else if (streq (argv [argn], "--snmpconfig") || streq (argv [argn], "-c")) {
            if (param) SNMP_CONFIG_FILE = param;
            ++argn;
        }
        else if (streq (argv [argn], "--polling") || streq (argv [argn], "-p")) {
            if (param) {
                errno = 0;
                long int i = strtol (param, NULL, 10);
                if (errno) {
                    zsys_error ("Invalid polling interval %s", param);
                } else {
                    POLLING = i;
                }
            }
            ++argn;
        }
        else if (streq (argv [argn], "--rules") || streq (argv [argn], "-r")) {
            if (param) RULES_DIR = param;
            ++argn;
        }
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    if (verbose)
        zsys_info ("fty_metric_snmp - started");
    zactor_t *server = zactor_new (fty_metric_snmp_server_actor, NULL);
    assert (server);
    zstr_sendx (server, "BIND", ENDPOINT, ACTOR_NAME, NULL);
    zstr_sendx (server, "PRODUCER", FTY_PROTO_STREAM_METRICS, NULL);
    zstr_sendx (server, "CONSUMER", FTY_PROTO_STREAM_ASSETS, ".*", NULL);
    zstr_sendx (server, "LOADRULES", RULES_DIR, NULL);
    zstr_sendx (server, "LOADCREDENTIALS", SNMP_CONFIG_FILE, NULL);
    // ttl = 2.5 * POLLING
    char *ttl = zsys_sprintf ("%i", POLLING * 5 / 2);
    if (ttl) {
        zstr_sendx (server, "TTL", ttl, NULL);
        zstr_free (&ttl);
    }
    
    zloop_t *wakeup = zloop_new();
    zloop_timer (wakeup, POLLING * 1000, 0, s_wakeup_event, server);
    zloop_start (wakeup);
    
    while (!zsys_interrupted) {
        zmsg_t *msg = zactor_recv (server);
        zmsg_destroy (&msg);
    }
    
    zloop_destroy (&wakeup);
    zactor_destroy (&server);
    if (verbose)
        zsys_info ("fty_metric_snmp - exited");
    return 0;
}
