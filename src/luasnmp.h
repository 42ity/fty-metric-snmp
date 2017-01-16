/*  =========================================================================
    luasnmp - lua snmp extension

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

#ifndef LUASNMP_H_INCLUDED
#define LUASNMP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new luasnmp
FTY_METRIC_SNMP_EXPORT lua_State *
    luasnmp_new (void);

// Destroy luasnmp
FTY_METRIC_SNMP_EXPORT void
    luasnmp_destroy (lua_State **self_p);

//  Self test of this class
FTY_METRIC_SNMP_EXPORT void
    luasnmp_test (bool verbose);

int snmp_version_to_enum (int version);

int enum_to_snmp_version (int version);

//  @end

#ifdef __cplusplus
}
#endif

#endif
