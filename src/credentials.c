/*  =========================================================================
    credentials - list of snmp credentials

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
    credentials - list of snmp credentials
@discuss
    This class holds all possible credentials readed from config file.
    Class is used to iterate trough credentials when new host appears
    to find valid credentials for this particular host

    TODO: support SNMPv3
@end
*/

#include "fty_metric_snmp_classes.h"

//  Structure of credential list
struct _credentials_t {
    zlist_t *credentials;
};

//  --------------------------------------------------------------------------
// Free snmp credentials
void free_snmp_credentials(void *c) {
    if (!c) return;
    snmp_credentials_t *sc = (snmp_credentials_t *)c;
    if (sc->community) free (sc->community);
    free (c);
}

//  --------------------------------------------------------------------------
//  Create a new credentials
credentials_t *
credentials_new (void)
{
    credentials_t *self = (credentials_t *) zmalloc (sizeof (credentials_t));
    assert (self);
    self->credentials = zlist_new();
    return self;
}

//  --------------------------------------------------------------------------
//  Add new credentials for SNMP version 1 and 2c
void
credentials_set (credentials_t *self, int version, const char*community)
{
    if (!self || !community) return;

    snmp_credentials_t *sc = (snmp_credentials_t *)malloc (sizeof (snmp_credentials_t));
    assert(sc);
    sc->version = version;
    sc->community = strdup (community);
    zlist_append (self->credentials, sc);
    zlist_freefn (self->credentials, sc, free_snmp_credentials, true);
}

//  --------------------------------------------------------------------------
//  Get first credentials in list or NULL if empty.
const snmp_credentials_t*
credentials_first (credentials_t *self)
{
    if (!self || !self->credentials) return NULL;
    return (snmp_credentials_t *)zlist_first(self->credentials);
}

//  --------------------------------------------------------------------------
//  Get next credentials in list or NULL if we reached end of list.
const snmp_credentials_t*
credentials_next (credentials_t *self)
{
    if (!self || !self->credentials) return NULL;
    return (snmp_credentials_t *)zlist_next(self->credentials);
}

//  --------------------------------------------------------------------------
//  Load credentials from zconfig file.
void
credentials_load (credentials_t *self, char *path)
{
    zconfig_t *cfg = zconfig_load (path);
    if (!cfg) return;

    zconfig_t *item = zconfig_locate (cfg, "snmp/community");
    if (item) {
        zconfig_t *child = zconfig_child (item);
        while (child) {
            if (!streq (zconfig_value (child), "")) {
                credentials_set (self, 1, zconfig_value (child));
                credentials_set (self, 2, zconfig_value (child));
            }
            child = zconfig_next (child);
        }
    }
    zconfig_destroy (&cfg);
}

//  --------------------------------------------------------------------------
//  Destroy the credentials class
void
credentials_destroy (credentials_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        credentials_t *self = *self_p;
        zlist_destroy (&self->credentials);
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class
void
credentials_test (bool verbose)
{
    printf (" * credentials: ");

    // Note: If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw. They are defined below along with a
    // usecase (asert) to make compilers happy.
    const char *SELFTEST_DIR_RO = "src/selftest-ro";
    const char *SELFTEST_DIR_RW = "src/selftest-rw";
    assert (SELFTEST_DIR_RO);
    assert (SELFTEST_DIR_RW);
    // std::string str_SELFTEST_DIR_RO = std::string(SELFTEST_DIR_RO);
    // std::string str_SELFTEST_DIR_RW = std::string(SELFTEST_DIR_RW);

    //  @selftest
    //  Simple create/destroy/load test
    credentials_t *self = credentials_new ();
    assert (self);
    char *cfg_file = zsys_sprintf ("%s/rules/communities.cfg", SELFTEST_DIR_RO);
    assert (cfg_file);
    credentials_load (self, cfg_file);
    zstr_free (&cfg_file);
    const snmp_credentials_t *cr = credentials_first (self);
    int cnt = 0;
    while (cr) {
        ++cnt;
        cr = credentials_next (self);
    }
    assert (cnt == 4);
    credentials_destroy (&self);
    //  @end
    printf ("OK\n");
}
