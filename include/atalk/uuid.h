/*
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

#ifndef AFP_UUID_H
#define AFP_UUID_H

#define UUID_BINSIZE 16
#define UUID_STRINGSIZE 36

typedef unsigned char *uuidp_t;
typedef unsigned char atalk_uuid_t[UUID_BINSIZE];

typedef enum {UUID_USER   = 1,
              UUID_GROUP  = 2,
              UUID_ENOENT = 4} /* used as bit flag */
    uuidtype_t;
#define UUIDTYPESTR_MASK 3
extern char *uuidtype[];

/* afp_options.c needs these. defined in libatalk/ldap.c */
extern char *ldap_host;
extern int  ldap_auth_method;
extern char *ldap_auth_dn;
extern char *ldap_auth_pw;
extern char *ldap_userbase;
extern char *ldap_groupbase;
extern char *ldap_uuid_attr;
extern char *ldap_name_attr;
extern char *ldap_group_attr;
extern char *ldap_uid_attr;

/******************************************************** 
 * Interface
 ********************************************************/

extern int getuuidfromname( const char *name, uuidtype_t type, uuidp_t uuid);
extern int getnamefromuuid( const uuidp_t uuidp, char **name, uuidtype_t *type);

extern void localuuid_from_id(unsigned char *buf, uuidtype_t type, unsigned int id);
extern const char *uuid_bin2string(unsigned char *uuid);
extern void uuid_string2bin( const char *uuidstring, uuidp_t uuid);
extern void uuidcache_dump(void);
#endif /* AFP_UUID_H */
