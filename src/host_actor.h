/*  =========================================================================
    host_actor - actor testing one host

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

#ifndef HOST_ACTOR_H_INCLUDED
#define HOST_ACTOR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _host_actor_t host_actor_t;

//  @interface
//  host actor function
FTY_METRIC_SNMP_EXPORT void
    host_actor (zsock_t *pipe, void *args);

//  Self test of this class
FTY_METRIC_SNMP_EXPORT void
    host_actor_test (bool verbose);

FTY_METRIC_SNMP_EXPORT host_actor_t *
    host_actor_new ();

FTY_METRIC_SNMP_EXPORT void
    host_actor_destroy (host_actor_t **self_p);

FTY_METRIC_SNMP_EXPORT void
    host_actor_remove_function (host_actor_t *self, const char *name);

FTY_METRIC_SNMP_EXPORT void
    host_actor_remove_functions (host_actor_t *self);

FTY_METRIC_SNMP_EXPORT void
    host_actor_freefn (void *self);

//  @end

#ifdef __cplusplus
}
#endif

#endif
