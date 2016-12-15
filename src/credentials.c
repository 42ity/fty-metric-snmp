/*  =========================================================================
    credentials - list of snmp credentials

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
    credentials - list of snmp credentials
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

//  Structure of our class

struct _credentials_t {
    zhash_t *hosts;     //  Declare class properties here
};

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
    self->hosts = zhash_new();
    return self;
}

void
credentials_set (credentials_t *self, const char *host, int version, const char*community)
{
    if (!self || !host || !community) return;
    zhash_delete (self->hosts, host);
    snmp_credentials_t *sc = (snmp_credentials_t *)malloc (sizeof (snmp_credentials_t));
    assert(sc);
    sc->version = version;
    sc->community = strdup (community);
    zhash_insert (self->hosts, host, sc);
    zhash_freefn (self->hosts, host, free_snmp_credentials);
}

const snmp_credentials_t*
credentials_get (credentials_t *self, const char *host)
{
    if (!self || !host) return NULL;
    return (snmp_credentials_t *)zhash_lookup(self->hosts, host);
}

//  --------------------------------------------------------------------------
//  Destroy the credentials

void
credentials_destroy (credentials_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        credentials_t *self = *self_p;
        zhash_destroy (&self->hosts);
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

    //  @selftest
    //  Simple create/destroy test
    credentials_t *self = credentials_new ();
    assert (self);
    credentials_destroy (&self);
    //  @end
    printf ("OK\n");
}
