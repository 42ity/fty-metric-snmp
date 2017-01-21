/*  =========================================================================
    fty_metric_snmp_rule - description

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
    fty_metric_snmp_rule - 
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

int main (int argc, char *argv [])
{
    int argn;
    const char *file = NULL;
    int snmpversion = 1;
    const char *community = "public";
    const char *host = "localhost";
    
    for (argn = 1; argn < argc; argn++) {
        char *param = NULL;
        if (argn < argc - 1) param = argv [argn + 1];
        if (streq (argv [argn], "--help") ||  streq (argv [argn], "-h")) {
            puts ("fty-metric-snmp-rule [options] ...");
            puts ("  --help / -h            this information");
            puts ("  --rule / -r            rule file");
            puts ("  --snmp-version / -s    snmp version [1], (1 or 2)");
            puts ("  --community / -c       snmp community name [public]");
            puts ("  --host / -H            server to test with [localhost]");
            return 0;
        }
        else if (streq (argv [argn], "--rule") ||  streq (argv [argn], "-r")) {
            if (param) file = param;
            ++argn;
        }
        else if (streq (argv [argn], "--snmp-version") ||  streq (argv [argn], "-s")) {
            if (param) {
                errno = 0;
                snmpversion = strtol (param, NULL, 10);
                if (errno || snmpversion < 1 || snmpversion > 2) {
                    printf ("Invalid SNMP version '%s'\n", param);
                    return 1;
                }
            }
            ++argn;
        }
        else if (streq (argv [argn], "--community") ||  streq (argv [argn], "-c")) {
            if (param) community = param;
            ++argn;
        }
        else if (streq (argv [argn], "--host") ||  streq (argv [argn], "-H")) {
            if (param) host = param;
            ++argn;
        }
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    return rule_tester (
        file,
        1,
        community,
        host
    );
}
