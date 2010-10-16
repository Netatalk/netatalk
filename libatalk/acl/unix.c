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

#ifdef HAVE_NFSv4_ACLS

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
    if (lstat(name, &st) != 0)
        return -1;
    if ( ! (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)))
        return 0;

    if ((ace_count = acl(name, ACE_GETACLCNT, 0, NULL)) == 0)
        return 0;

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
  Remove any trivial ACE "in-place". Returns no of non-trivial ACEs
*/
int strip_trivial_aces(ace_t **saces, int sacecount)
{
    int i,j;
    int nontrivaces = 0;
    ace_t *aces = *saces;
    ace_t *new_aces;

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

}

#if 0
static int set_acl_vfs(const struct vol *vol, char *name, int inherit, char *ibuf)
{
    int ret, i, nfsv4_ace_count, tocopy_aces_count = 0, new_aces_count = 0, trivial_ace_count = 0;
    ace_t *old_aces, *new_aces = NULL;
    uint16_t flags;
    uint32_t ace_count;

    LOG(log_debug9, logtype_afpd, "set_acl: BEGIN");

    /*  Get no of ACEs the client put on the wire */
    ace_count = htonl(*((uint32_t *)ibuf));
    ibuf += 8;      /* skip ACL flags (see acls.h) */

    if (inherit)
        /* inherited + trivial ACEs */
        flags = ACE_INHERITED_ACE | ACE_OWNER | ACE_GROUP | ACE_EVERYONE;
    else
        /* only trivial ACEs */
        flags = ACE_OWNER | ACE_GROUP | ACE_EVERYONE;

    /* Get existing ACL and count ACEs which have to be copied */
    if ((nfsv4_ace_count = get_nfsv4_acl(name, &old_aces)) == -1)
        return AFPERR_MISC;
    for ( i=0; i < nfsv4_ace_count; i++) {
        if (old_aces[i].a_flags & flags)
            tocopy_aces_count++;
    }

    /* Now malloc buffer exactly sized to fit all new ACEs */
    new_aces = malloc( (ace_count + tocopy_aces_count) * sizeof(ace_t) );
    if (new_aces == NULL) {
        LOG(log_error, logtype_afpd, "set_acl: malloc %s", strerror(errno));
        ret = AFPERR_MISC;
        goto cleanup;
    }

    /* Start building new ACL */

    /* Copy local inherited ACEs. Therefore we have 'Darwin canonical order' (see chmod there):
       inherited ACEs first. */
    if (inherit) {
        for (i=0; i < nfsv4_ace_count; i++) {
            if (old_aces[i].a_flags & ACE_INHERITED_ACE) {
                memcpy(&new_aces[new_aces_count], &old_aces[i], sizeof(ace_t));
                new_aces_count++;
            }
        }
    }
    LOG(log_debug7, logtype_afpd, "set_acl: copied %d inherited ACEs", new_aces_count);

    /* Now the ACEs from the client */
    ret = map_acl(DARWIN_2_SOLARIS, &new_aces[new_aces_count], (darwin_ace_t *)ibuf, ace_count);
    if (ret == -1) {
        ret = AFPERR_PARAM;
        goto cleanup;
    }
    new_aces_count += ace_count;
    LOG(log_debug7, logtype_afpd, "set_acl: mapped %d ACEs from client", ace_count);

    /* Now copy the trivial ACEs */
    for (i=0; i < nfsv4_ace_count; i++) {
        if (old_aces[i].a_flags  & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)) {
            memcpy(&new_aces[new_aces_count], &old_aces[i], sizeof(ace_t));
            new_aces_count++;
            trivial_ace_count++;
        }
    }
    LOG(log_debug7, logtype_afpd, "set_acl: copied %d trivial ACEs", trivial_ace_count);

    /* Ressourcefork first.
       Note: for dirs we set the same ACL on the .AppleDouble/.Parent _file_. This
       might be strange for ACE_DELETE_CHILD and for inheritance flags. */
    if ( (ret = vol->vfs->vfs_acl(vol, name, ACE_SETACL, new_aces_count, new_aces)) != 0) {
        LOG(log_error, logtype_afpd, "set_acl: error setting acl: %s", strerror(errno));
        if (errno == (EACCES | EPERM))
            ret = AFPERR_ACCESS;
        else if (errno == ENOENT)
            ret = AFPERR_NOITEM;
        else
            ret = AFPERR_MISC;
        goto cleanup;
    }
    if ( (ret = acl(name, ACE_SETACL, new_aces_count, new_aces)) != 0) {
        LOG(log_error, logtype_afpd, "set_acl: error setting acl: %s", strerror(errno));
        if (errno == (EACCES | EPERM))
            ret = AFPERR_ACCESS;
        else if (errno == ENOENT)
            ret = AFPERR_NOITEM;
        else
            ret = AFPERR_MISC;
        goto cleanup;
    }

    ret = AFP_OK;

cleanup:
    free(old_aces);
    free(new_aces);

    LOG(log_debug9, logtype_afpd, "set_acl: END");
    return ret;
}
#endif  /* 0 */

#endif /* HAVE_NFSv4_ACLS */
