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

#ifndef RULE_H_INCLUDED
#define RULE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rule_t rule_t;

//  @interface
//  Create a new rule
FTY_METRIC_SNMP_EXPORT rule_t *
    rule_new (void);

//  Destroy the rule
FTY_METRIC_SNMP_EXPORT void
    rule_destroy (rule_t **self_p);

//  Self test of this class
FTY_METRIC_SNMP_EXPORT void
    rule_test (bool verbose);

int rule_parse (rule_t *self, const char *json);

int rule_load (rule_t *self, const char *path);

void rule_freefn (void *self);
//  @end

#ifdef __cplusplus
}
#endif

#endif
