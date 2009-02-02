/*
   $Id: aclldap.h,v 1.1 2009-02-02 11:55:01 franklahm Exp $
   Copyright (c) 2008,2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#ifndef ACLLDAP_H
#define ACLLDAP_H

#include <atalk/uuid.h>		/* just for uuidtype_t*/

/******************************************************** 
 * Interface
 ********************************************************/

/* 
 *   name: give me his name
 *   type: and type of USER or GROUP
 *   uuid_string: returns pointer to allocated string
 * returns 0 on success !=0 on errror  
 */
extern int ldap_getuuidfromname( const char *name, uuidtype_t type, char **uuid_string);

/* 
 *   uuipd: give me his uuid
 *   name:  returns pointer to allocated string
 *   type:  returns type: USER or GROUP
 * returns 0 on success !=0 on errror
 */
extern int ldap_getnamefromuuid( uuidp_t uuidp, char **name, uuidtype_t *type); 

#endif /* ACLLDAP_H */
