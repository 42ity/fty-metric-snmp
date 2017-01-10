/*  =========================================================================
    rule - rule

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
    rule - rule
@discuss
@end
*/

#include "fty_metric_snmp_classes.h"

//  Structure of our class

struct _rule_t {
    char *name;
    char *description;
    zlist_t *assets;
    zlist_t *groups;
    char *evaluation;
};


//  --------------------------------------------------------------------------
//  Create a new rule

rule_t *
rule_new (void)
{
    rule_t *self = (rule_t *) zmalloc (sizeof (rule_t));
    assert (self);
    self -> assets = zlist_new ();
    zlist_autofree (self -> assets);
    self -> groups = zlist_new ();
    zlist_autofree (self -> groups);
    return self;
}

//  --------------------------------------------------------------------------
//  Parse JSON into rule
int rule_json_callback (const char *locator, const char *value, void *data)
{
    if (!data) return 1;
    
    rule_t *self = (rule_t *) data;
    
    if (streq (locator, "name")) {
        self -> name = vsjson_decode_string (value);
    }
    else if (streq (locator, "description")) {
        self -> description = vsjson_decode_string (value);
    }
    else if (strncmp (locator, "assets/", 7) == 0) {
        char *asset = vsjson_decode_string (value);
        zlist_append (self -> assets, asset);
        zlist_freefn (self -> assets, asset, free, true);
    }
    else if (strncmp (locator, "groups/", 7) == 0) {
        char *group = vsjson_decode_string (value);
        zlist_append (self -> groups, group);
        zlist_freefn (self -> groups, group, free, true);
    }
    else if (streq (locator, "evaluation")) {
        self -> evaluation = vsjson_decode_string (value);
    }
    return 0;
}

int rule_parse (rule_t *self, const char *json)
{
    return vsjson_parse (json, rule_json_callback, self);
}

int rule_load (rule_t *self, const char *path)
{
    int fd = open (path, O_RDONLY);
    if (fd == -1) return -1;

    struct stat rstat;
    fstat (fd, &rstat);

    int capacity = rstat.st_size + 1;
    char *buffer = (char *) malloc (capacity);
    assert (buffer);
    memset (buffer, 0, capacity);

    read (fd, buffer, capacity);
    close (fd);
    return rule_parse (self, buffer);
}

//  --------------------------------------------------------------------------
//  Destroy the rule

void
rule_destroy (rule_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        rule_t *self = *self_p;
        //  Free class properties here
        zlist_destroy (&self->assets);
        zlist_destroy (&self->groups);
        zstr_free (&self->name);
        zstr_free (&self->description);
        zstr_free (&self->evaluation);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

void
rule_freefn (void *self)
{
    if (!self) return;
    rule_t *rulep = (rule_t *)self;
    rule_destroy (&rulep);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
rule_test (bool verbose)
{
    printf (" * rule: ");

    //  @selftest
    //  Simple create/destroy test
    rule_t *self = rule_new ();
    assert (self);
    rule_load (self, "rules/linuxload.json");
    rule_destroy (&self);
    //  @end
    printf ("OK\n");
}

void
vsjson_test (bool verbose)
{
    printf (" * vsjson: ");

    //  @selftest
    //  @end
    printf ("OK\n");
}

