/*
  $Id: ldap.c,v 1.7 2010-04-23 11:37:06 franklahm Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <ldap.h>

#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/uuid.h>
#include <atalk/ldapconfig.h>   /* For struct ldap_pref */

typedef enum {
    KEEPALIVE = 1
} ldapcon_t;

/********************************************************
 * LDAP config stuff. Filled by libatalk/acl/ldap_config.c
 ********************************************************/
int ldap_config_valid;

char *ldap_server;
int  ldap_auth_method;
char *ldap_auth_dn;
char *ldap_auth_pw;
char *ldap_userbase;
int  ldap_userscope;
char *ldap_groupbase;
int  ldap_groupscope;
char *ldap_uuid_attr;
char *ldap_name_attr;
char *ldap_group_attr;
char *ldap_uid_attr;

struct ldap_pref ldap_prefs[] = {
    {&ldap_server,     "ldap_server",      0, 0, -1},
    {&ldap_auth_method,"ldap_auth_method", 1, 1, -1},
    {&ldap_auth_dn,    "ldap_auth_dn",     0, 0,  0},
    {&ldap_auth_pw,    "ldap_auth_pw",     0, 0,  0},
    {&ldap_userbase,   "ldap_userbase",    0, 0, -1},
    {&ldap_userscope,  "ldap_userscope",   1 ,1, -1},
    {&ldap_groupbase,  "ldap_groupbase",   0, 0, -1},
    {&ldap_groupscope, "ldap_groupscope",  1 ,1, -1},
    {&ldap_uuid_attr,  "ldap_uuid_attr",   0, 0, -1},
    {&ldap_name_attr,  "ldap_name_attr",   0, 0, -1},
    {&ldap_group_attr, "ldap_group_attr",  0, 0, -1},
    {&ldap_uid_attr,   "ldap_uid_attr",    0, 0,  0},
    {NULL,             NULL,               0, 0, -1}
};

struct pref_array prefs_array[] = {
    {"ldap_auth_method", "none",   LDAP_AUTH_NONE},
    {"ldap_auth_method", "simple", LDAP_AUTH_SIMPLE},
    {"ldap_auth_method", "sasl",   LDAP_AUTH_SASL},
    {"ldap_userscope",   "base",   LDAP_SCOPE_BASE},
    {"ldap_userscope",   "one",    LDAP_SCOPE_ONELEVEL},
    {"ldap_userscope",   "sub",    LDAP_SCOPE_SUBTREE},
    {"ldap_groupscope",  "base",   LDAP_SCOPE_BASE},
    {"ldap_groupscope",  "one",    LDAP_SCOPE_ONELEVEL},
    {"ldap_groupscope",  "sub",    LDAP_SCOPE_SUBTREE},
    {NULL,               NULL,     0}
};

/********************************************************
 * Static helper function
 ********************************************************/

/*
 * ldap_getattr_fromfilter_withbase_scope():
 *   conflags: KEEPALIVE
 *   scope: LDAP_SCOPE_BASE, LDAP_SCOPE_ONELEVEL, LDAP_SCOPE_SUBTREE
 *   result: return unique search result here, allocated here, caller must free
 *
 * All connection managment to the LDAP server is done here. Just set KEEPALIVE if you know
 * you will be dispatching more than one search in a row, then don't set it with the last search.
 * You MUST dispatch the queries timely, otherwise the LDAP handle might timeout.
 */
static int ldap_getattr_fromfilter_withbase_scope( const char *searchbase,
                                                   const char *filter,
                                                   char *attributes[],
                                                   int scope,
                                                   ldapcon_t conflags,
                                                   char **result) {
    int ret;
    int ldaperr;
    int retrycount = 0;
    int desired_version  = LDAP_VERSION3;
    static int ldapconnected = 0;
    static LDAP *ld     = NULL;
    LDAPMessage* msg    = NULL;
    LDAPMessage* entry  = NULL;

    char **attribute_values;
    struct timeval timeout;

    LOG(log_maxdebug, logtype_afpd,"ldap_getattr_fromfilter_withbase_scope: BEGIN");

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    /* init LDAP if necessary */
retry:
    ret = 0;

    if (!ldapconnected) {
        LOG(log_maxdebug, logtype_default, "ldap_getattr_fromfilter_withbase_scope: LDAP server: \"%s\"",
            ldap_server);
        if ((ld = ldap_init(ldap_server, LDAP_PORT)) == NULL ) {
            LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_init error");
            return -1;
        }
        if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &desired_version) != 0) {
            /* LDAP_OPT_SUCCESS is not in the proposed standard, so we check for 0
               http://tools.ietf.org/id/draft-ietf-ldapext-ldap-c-api-05.txt */
            LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_set_option failed!");
            ret = -1;
            goto cleanup;
        }
    }

    /* connect */
    if (!ldapconnected) {
        if (LDAP_AUTH_NONE == ldap_auth_method) {
            if (ldap_bind_s(ld, "", "", LDAP_AUTH_SIMPLE) != LDAP_SUCCESS ) {
                LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_bind failed!");
                LOG(log_error, logtype_default, "ldap_auth_method: \'%d\'", ldap_auth_method);
                return -1;
            }
            ldapconnected = 1;

        } else if (LDAP_AUTH_SIMPLE == ldap_auth_method) {
            if (ldap_bind_s(ld, ldap_auth_dn, ldap_auth_pw, ldap_auth_method) != LDAP_SUCCESS ) {
                LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_bind failed!");
                LOG(log_error, logtype_default, "ldap_auth_dn: \'%s\', ldap_auth_pw: \'%s\', ldap_auth_method: \'%d\'",
                    ldap_auth_dn, ldap_auth_pw, ldap_auth_method);
                return -1;
            }
            ldapconnected = 1;
        }
    }

    LOG(log_maxdebug, logtype_afpd, "LDAP start search: base: %s, filter: %s, attr: %s",
        searchbase, filter, attributes[0]);

    /* start LDAP search */
    ldaperr = ldap_search_st(ld, searchbase, scope, filter, attributes, 0, &timeout, &msg);
    LOG(log_maxdebug, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_search_st returned: %s, %u",
        ldap_err2string(ldaperr), ldaperr);
    if (ldaperr != LDAP_SUCCESS) {
        if (retrycount ==1)
            LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: ldap_search_st failed: %s", ldap_err2string(ldaperr));
        ret = -1;
        goto cleanup;
    }

    /* parse search result */
    LOG(log_maxdebug, logtype_default, "ldap_getuuidfromname: got %d entries from ldap search",
        ldap_count_entries(ld, msg));
    if (ldap_count_entries(ld, msg) != 1) {
        ret = -1;
        goto cleanup;
    }

    entry = ldap_first_entry(ld, msg);
    if (entry == NULL) {
        LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: error in ldap_first_entry");
        ret = -1;
        goto cleanup;
    }
    attribute_values = ldap_get_values(ld, entry, attributes[0]);
    if (attribute_values == NULL) {
        LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: error in ldap_get_values");
        ret = -1;
        goto cleanup;
    }

    LOG(log_maxdebug, logtype_afpd,"LDAP Search result: %s: %s",
        attributes[0], attribute_values[0]);

    /* allocate place for uuid as string */
    *result = calloc( 1, strlen(attribute_values[0]) + 1);
    if (*result == NULL) {
        LOG(log_error, logtype_default, "ldap_getattr_fromfilter_withbase_scope: %s: error calloc'ing",strerror(errno));
        ret = -1;
        goto cleanup;
    }
    /* get value */
    strcpy( *result, attribute_values[0]);
    ldap_value_free(attribute_values);

    /* FIXME: is there another way to free entry ? */
    while (entry != NULL)
        entry = ldap_next_entry(ld, entry);

cleanup:
    if (msg)
        ldap_msgfree(msg);
    if (ld) {
        if (ldapconnected
            && ( !(conflags & KEEPALIVE)
                 ||
                 ((ret == -1) && (ldaperr != LDAP_SUCCESS))) /* ie ldapsearch got 0 results */
            ) {

            ldapconnected = 0;  /* regardless of unbind errors */
            LOG(log_maxdebug, logtype_default,"LDAP unbind");
            if (ldap_unbind_s(ld) != 0) {
                LOG(log_error, logtype_default, "ldap_unbind_s: %s\n", ldap_err2string(ldaperr));
                return -1;
            }
            retrycount++;
            if (retrycount < 2)
                goto retry;
        }
    }
    return ret;
}

/********************************************************
 * Interface
 ********************************************************/

int ldap_getuuidfromname( const char *name, uuidtype_t type, char **uuid_string) {
    int ret;
    int len;
    char filter[256];           /* this should really be enough. we dont want to malloc everything! */
    char *attributes[]  = { ldap_uuid_attr, NULL};
    char *ldap_attr;

    /* make filter */
    if (type == UUID_GROUP)
        ldap_attr = ldap_group_attr;
    else /* type hopefully == UUID_USER */
        ldap_attr = ldap_name_attr;
    len = snprintf( filter, 256, "%s=%s", ldap_attr, name);
    if (len >= 256 || len == -1) {
        LOG(log_error, logtype_default, "ldap_getnamefromuuid: filter error:%d, \"%s\"", len, filter);
        return -1;
    }

    if (type == UUID_GROUP) {
        ret = ldap_getattr_fromfilter_withbase_scope( ldap_groupbase, filter, attributes, ldap_groupscope, KEEPALIVE, uuid_string);
    } else  { /* type hopefully == UUID_USER */
        ret = ldap_getattr_fromfilter_withbase_scope( ldap_userbase, filter, attributes, ldap_userscope, KEEPALIVE, uuid_string);
    }
    return ret;
}

int ldap_getnamefromuuid( char *uuidstr, char **name, uuidtype_t *type) {
    int ret;
    int len;
    char filter[256];       /* this should really be enough. we dont want to malloc everything! */
    char *attributes[]  = { NULL, NULL};

    /* make filter */
    len = snprintf( filter, 256, "%s=%s", ldap_uuid_attr, uuidstr);
    if (len >= 256 || len == -1) {
        LOG(log_error, logtype_default, "ldap_getnamefromuuid: filter overflow:%d, \"%s\"", len, filter);
        return -1;
    }
    /* search groups first. group acls are probably used more often */
    attributes[0] = ldap_group_attr;
    ret = ldap_getattr_fromfilter_withbase_scope( ldap_groupbase, filter, attributes, ldap_groupscope, KEEPALIVE, name);
    if (ret == 0) {
        *type = UUID_GROUP;
        return 0;
    }
    attributes[0] = ldap_name_attr;
    ret = ldap_getattr_fromfilter_withbase_scope( ldap_userbase, filter, attributes, ldap_userscope, KEEPALIVE, name);
    if (ret == 0) {
        *type = UUID_USER;
        return 0;
    }

    return ret;
}
