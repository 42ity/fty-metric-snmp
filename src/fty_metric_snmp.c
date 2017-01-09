/*  =========================================================================
    fty_metric_snmp - description

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
    fty_metric_snmp - 
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

static const char *ACTOR_NAME = "fty-metric-smtp";
static const char *ENDPOINT = "ipc://@/malamute";
static const char *RULES_DIR = "./rules";

int main (int argc, char *argv [])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("fty-metric-snmp [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
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
    while (!zsys_interrupted) {
        zmsg_t *msg = zactor_recv (server);
        zmsg_destroy (&msg);
    }
    zactor_destroy (&server);
    if (verbose)
        zsys_info ("fty_metric_snmp - exited");
    return 0;
}
