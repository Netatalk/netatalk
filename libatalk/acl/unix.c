/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

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

#ifdef HAVE_SOLARIS_ACLS

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/acl.h>

#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/acl.h>

/* Get ACL. Allocates storage as needed. Caller must free.
 * Returns no of ACEs or -1 on error.  */
int get_nfsv4_acl(const char *name, ace_t **retAces)
{
    int ace_count = -1;
    ace_t *aces;
    struct stat st;

    *retAces = NULL;
    /* Only call acl() for regular files and directories, otherwise just return 0 */
    if (lstat(name, &st) != 0) {
        LOG(log_warning, logtype_afpd, "get_nfsv4_acl(\"%s/%s\"): %s", getcwdpath(), name, strerror(errno));
        return -1;
    }

    if (S_ISLNK(st.st_mode))
        /* sorry, no ACLs for symlinks */
        return 0;

    if ( ! (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) {
        LOG(log_warning, logtype_afpd, "get_nfsv4_acl(\"%s/%s\"): special", getcwdpath(), name);
        return 0;
    }

    if ((ace_count = acl(name, ACE_GETACLCNT, 0, NULL)) == 0) {
        LOG(log_warning, logtype_afpd, "get_nfsv4_acl(\"%s/%s\"): 0 ACEs", getcwdpath(), name);
        return 0;
    }

    if (ace_count == -1) {
        LOG(log_error, logtype_afpd, "get_nfsv4_acl: acl('%s/%s', ACE_GETACLCNT): ace_count %i, error: %s",
            getcwdpath(), name, ace_count, strerror(errno));
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

/*
  Concatenate ACEs
*/
ace_t *concat_aces(ace_t *aces1, int ace1count, ace_t *aces2, int ace2count)
{
    ace_t *new_aces;
    int i, j;

    /* malloc buffer for new ACL */
    if ((new_aces = malloc((ace1count + ace2count) * sizeof(ace_t))) == NULL) {
        LOG(log_error, logtype_afpd, "combine_aces: malloc %s", strerror(errno));
        return NULL;
    }

    /* Copy ACEs from buf1 */
    for (i=0; i < ace1count; ) {
        memcpy(&new_aces[i], &aces1[i], sizeof(ace_t));
        i++;
    }

    j = i;

    /* Copy ACEs from buf2 */
    for (i=0; i < ace2count; ) {
        memcpy(&new_aces[j], &aces2[i], sizeof(ace_t));
        i++;
        j++;
    }
    return new_aces;
}

/*
  Remove any trivial ACE "in-place". Returns no of non-trivial ACEs
*/
int strip_trivial_aces(ace_t **saces, int sacecount)
{
    int i,j;
    int nontrivaces = 0;
    ace_t *aces = *saces;
    ace_t *new_aces;

    if (aces == NULL || sacecount <= 0)
        return 0;

    /* Count non-trivial ACEs */
    for (i=0; i < sacecount; ) {
        if ( ! (aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)))
            nontrivaces++;
        i++;
    }
    /* malloc buffer for new ACL */
    if ((new_aces = malloc(nontrivaces * sizeof(ace_t))) == NULL) {
        LOG(log_error, logtype_afpd, "strip_trivial_aces: malloc %s", strerror(errno));
        return -1;
    }

    /* Copy non-trivial ACEs */
    for (i=0, j=0; i < sacecount; ) {
        if ( ! (aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE))) {
            memcpy(&new_aces[j], &aces[i], sizeof(ace_t));
            j++;
        }
        i++;
    }

    free(aces);
    *saces = new_aces;

    LOG(log_debug7, logtype_afpd, "strip_trivial_aces: non-trivial ACEs: %d", nontrivaces);

    return nontrivaces;
}

/*
  Remove non-trivial ACEs "in-place". Returns no of trivial ACEs.
*/
int strip_nontrivial_aces(ace_t **saces, int sacecount)
{
    int i,j;
    int trivaces = 0;
    ace_t *aces = *saces;
    ace_t *new_aces;

    /* Count trivial ACEs */
    for (i=0; i < sacecount; ) {
        if ((aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)))
            trivaces++;
        i++;
    }
    /* malloc buffer for new ACL */
    if ((new_aces = malloc(trivaces * sizeof(ace_t))) == NULL) {
        LOG(log_error, logtype_afpd, "strip_nontrivial_aces: malloc %s", strerror(errno));
        return -1;
    }

    /* Copy trivial ACEs */
    for (i=0, j=0; i < sacecount; ) {
        if ((aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE))) {
            memcpy(&new_aces[j], &aces[i], sizeof(ace_t));
            j++;
        }
        i++;
    }
    /* Free old ACEs */
    free(aces);
    *saces = new_aces;

    LOG(log_debug7, logtype_afpd, "strip_nontrivial_aces: trivial ACEs: %d", trivaces);

    return trivaces;
}

/*!
 * Change mode of file preserving existing explicit ACEs
 *
 * nfsv4_chmod
 * (1) reads objects ACL (acl1)
 * (2) removes all trivial ACEs from the ACL by calling strip_trivial_aces(), possibly
 *     leaving 0 ACEs in the ACL if there were only trivial ACEs as mapped from the mode
 * (3) calls chmod() with mode
 * (4) reads the changed ACL (acl2) which
 *     a) might still contain explicit ACEs (up to onnv132)
 *     b) will have any explicit ACE removed (starting with onnv145/Openindiana)
 * (5) strip any explicit ACE from acl2 using strip_nontrivial_aces()
 * (6) merge acl2 and acl2
 * (7) set the ACL merged ACL on the object
 */
int nfsv4_chmod(char *name, mode_t mode)
{
    int ret = -1;
    int noaces, nnaces;
    ace_t *oacl = NULL, *nacl = NULL, *cacl = NULL;

    LOG(log_debug, logtype_afpd, "nfsv4_chmod(\"%s/%s\", %04o)",
        getcwdpath(), name, mode);

    if ((noaces = get_nfsv4_acl(name, &oacl)) == -1) /* (1) */
        goto exit;
    if ((noaces = strip_trivial_aces(&oacl, noaces)) == -1) /* (2) */
        goto exit;

#ifdef chmod
#undef chmod
#endif
    if (chmod(name, mode) != 0) /* (3) */
        goto exit;

    if ((nnaces = get_nfsv4_acl(name, &nacl)) == -1) /* (4) */
        goto exit;
    if ((nnaces = strip_nontrivial_aces(&nacl, nnaces)) == -1) /* (5) */
        goto exit;

    if ((cacl = concat_aces(oacl, noaces, nacl, nnaces)) == NULL) /* (6) */
        goto exit;

    if ((ret = acl(name, ACE_SETACL, noaces + nnaces, cacl)) != 0) {
        LOG(log_error, logtype_afpd, "nfsv4_chmod: error setting acl: %s", strerror(errno));
        goto exit;
    }

exit:
    if (oacl) free(oacl);
    if (nacl) free(nacl);
    if (cacl) free(cacl);

    LOG(log_debug, logtype_afpd, "nfsv4_chmod(\"%s/%s\", %04o): result: %u",
        getcwdpath(), name, mode, ret);

    return ret;
}

#endif /* HAVE_SOLARIS_ACLS */
