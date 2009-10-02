/*
  $Id: acl.c,v 1.1 2009-10-02 09:32:41 franklahm Exp $
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

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

#include <errno.h>
#include <sys/acl.h>

#include <atalk/util.h>
#include <atalk/logger.h>

/* Get ACL. Allocates storage as needed. Caller must free.
 * Returns no of ACEs or -1 on error.  */
int get_nfsv4_acl(const char *name, ace_t **retAces)
{
    int ace_count = -1;
    ace_t *aces;

    *retAces = NULL;
    ace_count = acl(name, ACE_GETACLCNT, 0, NULL);
    if (ace_count <= 0) {
	LOG(log_error, logtype_afpd, "get_nfsv4_acl: acl(ACE_GETACLCNT) error");
        return -1;
    }

    aces = malloc(ace_count * sizeof(ace_t));
    if (aces == NULL) {
	LOG(log_error, logtype_afpd, "get_nfsv4_acl: malloc error");
	return -1;
    }

    if ( (acl(name, ACE_GETACL, ace_count, aces)) == -1 ) {
	LOG(log_error, logtype_afpd, "get_nfsv4_acl: acl(ACE_GETACL) error");
	free(aces);
	return -1;
    }

    LOG(log_debug9, logtype_afpd, "get_nfsv4_acl: file: %s -> No. of ACEs: %d", name, ace_count);
    *retAces = aces;

    return ace_count;
}

/* Removes all non-trivial ACLs from object. Returns full AFPERR code. */
int remove_acl(const char *name)
{
    int ret,i, ace_count, trivial_aces, new_aces_count;
    ace_t *old_aces = NULL;
    ace_t *new_aces = NULL;

    LOG(log_debug9, logtype_afpd, "remove_acl: BEGIN");

    /* Get existing ACL and count trivial ACEs */
    if ((ace_count = get_nfsv4_acl(name, &old_aces)) == -1)
        return AFPERR_MISC;
    trivial_aces = 0;
    for ( i=0; i < ace_count; i++) {
        if (old_aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE))
            trivial_aces++;
    }

    /* malloc buffer for new ACL */
    if ((new_aces = malloc(trivial_aces * sizeof(ace_t))) == NULL) {
        LOG(log_error, logtype_afpd, "remove_acl: malloc %s", strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Now copy the trivial ACEs */
    new_aces_count = 0;
    for (i=0; i < ace_count; i++) {
        if (old_aces[i].a_flags  & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)) {
            memcpy(&new_aces[new_aces_count], &old_aces[i], sizeof(ace_t));
            new_aces_count++;
        }
    }

    if ( (acl(name, ACE_SETACL, trivial_aces, new_aces)) == 0)
        ret = AFP_OK;
    else {
        LOG(log_error, logtype_afpd, "set_acl: error setting acl: %s", strerror(errno));
        if (errno == (EACCES | EPERM))
            ret = AFPERR_ACCESS;
        else if (errno == ENOENT)
            ret = AFPERR_NOITEM;
        else
            ret = AFPERR_MISC;
    }

exit:
    free(old_aces);
    free(new_aces);

    LOG(log_debug9, logtype_afpd, "remove_acl: END");
    return ret;
}
