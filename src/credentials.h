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

#ifndef CREDENTIALS_H_INCLUDED
#define CREDENTIALS_H_INCLUDED

#include "fty_metric_snmp_classes.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _credentials_t credentials_t;

//  @interface
//  Create a new credentials
FTY_METRIC_SNMP_PRIVATE credentials_t *
    credentials_new (void);

//  Destroy the credentials
FTY_METRIC_SNMP_PRIVATE void
    credentials_destroy (credentials_t **self_p);

//  Add new credentials for SNMP version 1 and 2c
FTY_METRIC_SNMP_PRIVATE void
    credentials_set (credentials_t *self, int version, const char*community);

//  Get first credentials in list or NULL if empty.
FTY_METRIC_SNMP_PRIVATE const snmp_credentials_t *
    credentials_first (credentials_t *self);

//  Get next credentials in list or NULL if we reached end of list.
FTY_METRIC_SNMP_PRIVATE const snmp_credentials_t *
    credentials_next (credentials_t *self);

//  Load credentials from zconfig file.
FTY_METRIC_SNMP_PRIVATE void
    credentials_load (credentials_t *self, char *path);
//  @end

#ifdef __cplusplus
}
#endif

#endif
