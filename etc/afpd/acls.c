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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <sys/acl.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/uuid.h>
#include <atalk/acl.h>

#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"

#include "acls.h"
#include "acl_mappings.h"

/* for map_acl() */
#define SOLARIS_2_DARWIN 1
#define DARWIN_2_SOLARIS 2

/********************************************************
 * Basic and helper funcs
 ********************************************************/

/*
  Takes a users name, uid and primary gid and checks if user is member of any group
  Returns -1 if no or error, 0 if yes
*/
static int check_group(char *name, uid_t uid, gid_t pgid, gid_t path_gid)
{
    int i;
    struct group *grp;

    if (pgid == path_gid)
        return 0;

    grp = getgrgid(path_gid);
    if (!grp)
        return -1;

    i = 0;
    while (grp->gr_mem[i] != NULL) {
        if ( (strcmp(grp->gr_mem[i], name)) == 0 ) {
            LOG(log_debug, logtype_afpd, "check_group: requested user:%s is member of: %s", name, grp->gr_name);
            return 0;
        }
        i++;
    }

    return -1;
}

/*
  Maps ACE array from Solaris to Darwin. Darwin ACEs are stored in network byte order.
  Return numer of mapped ACEs or -1 on error.
  All errors while mapping (e.g. getting UUIDs from LDAP) are fatal.
*/
static int map_aces_solaris_to_darwin(ace_t *aces, darwin_ace_t *darwin_aces, int ace_count)
{
    int i, count = 0;
    uint32_t flags;
    uint32_t rights;
    struct passwd *pwd = NULL;
    struct group *grp = NULL;

    LOG(log_debug7, logtype_afpd, "map_aces_solaris_to_darwin: parsing %d ACES", ace_count);

    while(ace_count--) {
        LOG(log_debug7, logtype_afpd, "map_aces_solaris_to_darwin: parsing ACE No. %d", ace_count + 1);
        /* if its a ACE resulting from nfsv4 mode mapping, discard it */
        if (aces->a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)) {
            LOG(log_debug, logtype_afpd, "map_aces_solaris_to_darwin: trivial ACE");
            aces++;
            continue;
        }

        if ( ! (aces->a_flags & ACE_IDENTIFIER_GROUP) ) {
            /* its a user ace */
            LOG(log_debug, logtype_afpd, "map_aces_solaris_to_darwin: found user ACE with uid: %d", aces->a_who);
            pwd = getpwuid(aces->a_who);
            if (!pwd) {
                LOG(log_error, logtype_afpd, "map_aces_solaris_to_darwin: getpwuid error: %s", strerror(errno));
                return -1;
            }
            LOG(log_debug, logtype_afpd, "map_aces_solaris_to_darwin: uid: %d -> name: %s", aces->a_who, pwd->pw_name);
            if ( (getuuidfromname(pwd->pw_name, UUID_USER, darwin_aces->darwin_ace_uuid)) != 0) {
                LOG(log_error, logtype_afpd, "map_aces_solaris_to_darwin: getuuidfromname error");
                return -1;
            }
        } else {
            /* its a group ace */
            LOG(log_debug, logtype_afpd, "map_aces_solaris_to_darwin: found group ACE with gid: %d", aces->a_who);
            grp = getgrgid(aces->a_who);
            if (!grp) {
                LOG(log_error, logtype_afpd, "map_aces_solaris_to_darwin: getgrgid error: %s", strerror(errno));
                return -1;
            }
            LOG(log_debug, logtype_afpd, "map_aces_solaris_to_darwin: gid: %d -> name: %s", aces->a_who, grp->gr_name);
            if ( (getuuidfromname(grp->gr_name, UUID_GROUP, darwin_aces->darwin_ace_uuid)) != 0) {
                LOG(log_error, logtype_afpd, "map_aces_solaris_to_darwin: getuuidfromname error");
                return -1;
            }
        }

        /* map flags */
        if (aces->a_type == ACE_ACCESS_ALLOWED_ACE_TYPE)
            flags = DARWIN_ACE_FLAGS_PERMIT;
        else if (aces->a_type == ACE_ACCESS_DENIED_ACE_TYPE)
            flags = DARWIN_ACE_FLAGS_DENY;
        else {          /* unsupported type */
            aces++;
            continue;
        }
        for(i=0; nfsv4_to_darwin_flags[i].from != 0; i++) {
            if (aces->a_flags & nfsv4_to_darwin_flags[i].from)
                flags |= nfsv4_to_darwin_flags[i].to;
        }
        darwin_aces->darwin_ace_flags = htonl(flags);

        /* map rights */
        rights = 0;
        for (i=0; nfsv4_to_darwin_rights[i].from != 0; i++) {
            if (aces->a_access_mask & nfsv4_to_darwin_rights[i].from)
                rights |= nfsv4_to_darwin_rights[i].to;
        }
        darwin_aces->darwin_ace_rights = htonl(rights);

        count++;
        aces++;
        darwin_aces++;
    }

    return count;
}

/*
  Maps ACE array from Darwin to Solaris. Darwin ACEs are expected in network byte order.
  Return numer of mapped ACEs or -1 on error.
  All errors while mapping (e.g. getting UUIDs from LDAP) are fatal.
*/
int map_aces_darwin_to_solaris(darwin_ace_t *darwin_aces, ace_t *nfsv4_aces, int ace_count)
{
    int i, mapped_aces = 0;
    uint32_t darwin_ace_flags;
    uint32_t darwin_ace_rights;
    uint16_t nfsv4_ace_flags;
    uint32_t nfsv4_ace_rights;
    char *name;
    uuidtype_t uuidtype;
    struct passwd *pwd;
    struct group *grp;

    while(ace_count--) {
        nfsv4_ace_flags = 0;
        nfsv4_ace_rights = 0;

        /* uid/gid first */
        if ( (getnamefromuuid(darwin_aces->darwin_ace_uuid, &name, &uuidtype)) != 0)
            return -1;
        if (uuidtype == UUID_USER) {
            pwd = getpwnam(name);
            if (!pwd) {
                LOG(log_error, logtype_afpd, "map_aces_darwin_to_solaris: getpwnam: %s", strerror(errno));
                free(name);
                return -1;
            }
            nfsv4_aces->a_who = pwd->pw_uid;
        } else { /* hopefully UUID_GROUP*/
            grp = getgrnam(name);
            if (!grp) {
                LOG(log_error, logtype_afpd, "map_aces_darwin_to_solaris: getgrnam: %s", strerror(errno));
                free(name);
                return -1;
            }
            nfsv4_aces->a_who = (uid_t)(grp->gr_gid);
            nfsv4_ace_flags |= ACE_IDENTIFIER_GROUP;
        }
        free(name);
        name = NULL;

        /* now type: allow/deny */
        darwin_ace_flags = ntohl(darwin_aces->darwin_ace_flags);
        if (darwin_ace_flags & DARWIN_ACE_FLAGS_PERMIT)
            nfsv4_aces->a_type = ACE_ACCESS_ALLOWED_ACE_TYPE;
        else if (darwin_ace_flags & DARWIN_ACE_FLAGS_DENY)
            nfsv4_aces->a_type = ACE_ACCESS_DENIED_ACE_TYPE;
        else { /* unsupported type */
            darwin_aces++;
            continue;
        }
        /* map flags */
        for(i=0; darwin_to_nfsv4_flags[i].from != 0; i++) {
            if (darwin_ace_flags & darwin_to_nfsv4_flags[i].from)
                nfsv4_ace_flags |= darwin_to_nfsv4_flags[i].to;
        }

        /* map rights */
        darwin_ace_rights = ntohl(darwin_aces->darwin_ace_rights);
        for (i=0; darwin_to_nfsv4_rights[i].from != 0; i++) {
            if (darwin_ace_rights & darwin_to_nfsv4_rights[i].from)
                nfsv4_ace_rights |= darwin_to_nfsv4_rights[i].to;
        }

        LOG(log_debug9, logtype_afpd, "map_aces_darwin_to_solaris: ACE flags: Darwin:%08x -> NFSv4:%04x", darwin_ace_flags, nfsv4_ace_flags);
        LOG(log_debug9, logtype_afpd, "map_aces_darwin_to_solaris: ACE rights: Darwin:%08x -> NFSv4:%08x", darwin_ace_rights, nfsv4_ace_rights);

        nfsv4_aces->a_flags = nfsv4_ace_flags;
        nfsv4_aces->a_access_mask = nfsv4_ace_rights;

        mapped_aces++;
        darwin_aces++;
        nfsv4_aces++;
    }

    return mapped_aces;
}

/********************************************************
 * 2nd level funcs
 ********************************************************/

/*  Map between ACL styles (SOLARIS_2_DARWIN, DARWIN_2_SOLARIS).
    Reads from 'aces' buffer, writes to 'rbuf' buffer.
    Caller must provide buffer.
    Darwin ACEs are read and written in network byte order.
    Needs to know how many ACEs are in the ACL (ace_count). Ignores trivial ACEs.
    Return no of mapped ACEs or -1 on error. */
static int map_acl(int type, ace_t *nfsv4_aces, darwin_ace_t *buf, int ace_count)
{
    int mapped_aces;

    LOG(log_debug9, logtype_afpd, "map_acl: BEGIN");

    switch (type) {
    case SOLARIS_2_DARWIN:
        mapped_aces = map_aces_solaris_to_darwin( nfsv4_aces, buf, ace_count);
        break;

    case DARWIN_2_SOLARIS:
        mapped_aces = map_aces_darwin_to_solaris( buf, nfsv4_aces, ace_count);
        break;

    default:
        mapped_aces = -1;
        break;
    }

    LOG(log_debug9, logtype_afpd, "map_acl: END");
    return mapped_aces;
}

/********************************************************
 * 1st level funcs
 ********************************************************/


/* Get ACL from object omitting trivial ACEs. Map to Darwin ACL style and
   store Darwin ACL at rbuf. Add length of ACL written to rbuf to *rbuflen.
   Returns 0 on success, -1 on error. */
static int get_and_map_acl(char *name, char *rbuf, size_t *rbuflen)
{
    int ace_count, mapped_aces, err;
    ace_t *aces;
    uint32_t *darwin_ace_count = (u_int32_t *)rbuf;

    LOG(log_debug9, logtype_afpd, "get_and_map_acl: BEGIN");

    /* Skip length and flags */
    rbuf += 4;
    *rbuf = 0;
    rbuf += 4;

    if ( (ace_count = get_nfsv4_acl(name, &aces)) == -1) {
        LOG(log_error, logtype_afpd, "get_and_map_acl: couldnt get ACL");
        return -1;
    }

    if ( (mapped_aces = map_acl(SOLARIS_2_DARWIN, aces, (darwin_ace_t *)rbuf, ace_count)) == -1) {
        err = -1;
        goto cleanup;
    }
    LOG(log_debug, logtype_afpd, "get_and_map_acl: mapped %d ACEs", mapped_aces);

    err = 0;
    *darwin_ace_count = htonl(mapped_aces);
    *rbuflen += sizeof(darwin_acl_header_t) + (mapped_aces * sizeof(darwin_ace_t));

cleanup:
    free(aces);

    LOG(log_debug9, logtype_afpd, "get_and_map_acl: END");
    return err;
}

/* Removes all non-trivial ACLs from object. Returns full AFPERR code. */
static int remove_acl_vfs(const struct vol *vol,const char *path, int dir)
{
    int ret;

    /* Ressource etc. first */
    if ((ret = vol->vfs->vfs_remove_acl(vol, path, dir)) != AFP_OK)
        return ret;
    /* now the data fork or dir */
    return (remove_acl(path));
}

/*
  Set ACL. Subtleties:
  - the client sends a complete list of ACEs, not only new ones. So we dont need to do
  any combination business (one exception being 'kFileSec_Inherit': see next)
  - client might request that we add inherited ACEs via 'kFileSec_Inherit'.
  We will store inherited ACEs first, which is Darwins canonical order.
  - returns AFPerror code
*/
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

/*
  Checks if a given UUID has requested_rights(type darwin_ace_rights) for path.
  Note: this gets called frequently and is a good place for optimizations !
*/
static int check_acl_access(const char *path, const uuidp_t uuid, uint32_t requested_darwin_rights)
{
    int                 ret, i, ace_count, dir, checkgroup;
    char                *username = NULL; /* might be group too */
    uuidtype_t          uuidtype;
    uid_t               uid;
    gid_t               pgid;
    uint32_t            requested_rights = 0, allowed_rights = 0, denied_rights = 0;
    ace_t               *aces;
    struct passwd       *pwd;
    struct stat         st;
    int                 check_user_trivace = 0, check_group_trivace = 0;
    uid_t               who;
    uint16_t            flags;
    uint16_t            type;
    uint32_t            rights;

#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "check_access: BEGIN. Request: %08x", requested_darwin_rights);
#endif
    /* Get uid or gid from UUID */
    if ( (getnamefromuuid(uuid, &username, &uuidtype)) != 0) {
        LOG(log_error, logtype_afpd, "check_access: error getting name from UUID");
        return AFPERR_PARAM;
    }

    /* File or dir */
    if ((lstat(path, &st)) != 0) {
        LOG(log_error, logtype_afpd, "check_access: stat: %s", strerror(errno));
        ret = AFPERR_PARAM;
        goto exit;
    }
    dir = S_ISDIR(st.st_mode);

    if (uuidtype == UUID_USER) {
        pwd = getpwnam(username);
        if (!pwd) {
            LOG(log_error, logtype_afpd, "check_access: getpwnam: %s", strerror(errno));
            ret = AFPERR_MISC;
            goto exit;
        }
        uid = pwd->pw_uid;
        pgid = pwd->pw_gid;

        /* If user is file/dir owner we must check the user trivial ACE */
        if (uid == st.st_uid) {
            LOG(log_debug, logtype_afpd, "check_access: user: %s is files owner. Must check trivial user ACE", username);
            check_user_trivace = 1;
        }

        /* Now check if requested user is files owning group. If yes we must check the group trivial ACE */
        if ( (check_group(username, uid, pgid, st.st_gid)) == 0) {
            LOG(log_debug, logtype_afpd, "check_access: user: %s is in group: %d. Must check trivial group ACE", username, st.st_gid);
            check_group_trivace = 1;
        }
    } else { /* hopefully UUID_GROUP*/
        LOG(log_error, logtype_afpd, "check_access: afp_access for UUID of groups not supported!");
#if 0
        grp = getgrnam(username);
        if (!grp) {
            LOG(log_error, logtype_afpd, "check_access: getgrnam: %s", strerror(errno));
            return -1;
        }
        if (st.st_gid == grp->gr_gid )
            check_group_trivace = 1;
#endif
    }

    /* Map requested rights to Solaris style. */
    for (i=0; darwin_to_nfsv4_rights[i].from != 0; i++) {
        if (requested_darwin_rights & darwin_to_nfsv4_rights[i].from)
            requested_rights |= darwin_to_nfsv4_rights[i].to;
    }

    /* Get ACL from file/dir */
    if ( (ace_count = get_nfsv4_acl(path, &aces)) == -1) {
        LOG(log_error, logtype_afpd, "check_access: error getting ACEs");
        ret = AFPERR_MISC;
        goto exit;
    }
    if (ace_count == 0) {
        LOG(log_debug, logtype_afpd, "check_access: 0 ACEs from get_nfsv4_acl");
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Now check requested rights */
    ret = AFPERR_ACCESS;
    i = 0;
    do { /* Loop through ACEs */
        who = aces[i].a_who;
        flags = aces[i].a_flags;
        type = aces[i].a_type;
        rights = aces[i].a_access_mask;

        if (flags & ACE_INHERIT_ONLY_ACE)
            continue;

        /* Check if its a group ACE and set checkgroup to 1 if yes */
        checkgroup = 0;
        if ( (flags & ACE_IDENTIFIER_GROUP) && !(flags & ACE_GROUP) ) {
            if ( (check_group(username, uid, pgid, who)) == 0)
                checkgroup = 1;
            else
                continue;
        }

        /* Now the tricky part: decide if ACE effects our user. I'll explain:
           if its a dedicated (non trivial) ACE for the user
           OR
           if its a ACE for a group we're member of
           OR
           if its a trivial ACE_OWNER ACE and requested UUID is the owner
           OR
           if its a trivial ACE_GROUP ACE and requested UUID is group
           OR
           if its a trivial ACE_EVERYONE ACE
           THEN
           process ACE */
        if (
            ( (who == uid) && !(flags & (ACE_TRIVIAL|ACE_IDENTIFIER_GROUP)) ) ||
            (checkgroup) ||
            ( (flags & ACE_OWNER) && check_user_trivace ) ||
            ( (flags & ACE_GROUP) && check_group_trivace ) ||
            ( flags & ACE_EVERYONE )
            ) {
            /* Found an applicable ACE */
            if (type == ACE_ACCESS_ALLOWED_ACE_TYPE)
                allowed_rights |= rights;
            else if (type == ACE_ACCESS_DENIED_ACE_TYPE)
                /* Only or to denied rights if not previously allowed !! */
                denied_rights |= ((!allowed_rights) & rights);
        }
    } while (++i < ace_count);


    /* Darwin likes to ask for "delete_child" on dir,
       "write_data" is actually the same, so we add that for dirs */
    if (dir && (allowed_rights & ACE_WRITE_DATA))
        allowed_rights |= ACE_DELETE_CHILD;

    if (requested_rights & denied_rights) {
        LOG(log_debug, logtype_afpd, "check_access: some requested right was denied:");
        ret = AFPERR_ACCESS;
    } else if ((requested_rights & allowed_rights) != requested_rights) {
        LOG(log_debug, logtype_afpd, "check_access: some requested right wasn't allowed:");
        ret = AFPERR_ACCESS;
    } else {
        LOG(log_debug, logtype_afpd, "check_access: all requested rights are allowed:");
        ret = AFP_OK;
    }

    LOG(log_debug, logtype_afpd, "check_access: Requested rights: %08x, allowed_rights: %08x, denied_rights: %08x, Result: %d",
        requested_rights, allowed_rights, denied_rights, ret);

exit:
    free(aces);
    free(username);
#ifdef DEBUG
    LOG(log_debug9, logtype_afpd, "check_access: END");
#endif
    return ret;
}

/********************************************************
 * Interface
 ********************************************************/

int afp_access(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    int         ret;
    struct vol      *vol;
    struct dir      *dir;
    uint32_t            did, darwin_ace_rights;
    uint16_t        vid;
    struct path         *s_path;
    uuidp_t             uuid;

    LOG(log_debug9, logtype_afpd, "afp_access: BEGIN");

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid ))) {
        LOG(log_error, logtype_afpd, "afp_access: error getting volid:%d", vid);
        return AFPERR_NOOBJ;
    }

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did ))) {
        LOG(log_error, logtype_afpd, "afp_access: error getting did:%d", did);
        return afp_errno;
    }

    /* Skip bitmap */
    ibuf += 2;

    /* Store UUID address */
    uuid = (uuidp_t)ibuf;
    ibuf += UUID_BINSIZE;

    /* Store ACE rights */
    memcpy(&darwin_ace_rights, ibuf, 4);
    darwin_ace_rights = ntohl(darwin_ace_rights);
    ibuf += 4;

    /* get full path and handle file/dir subtleties in netatalk code*/
    if (NULL == ( s_path = cname( vol, dir, &ibuf ))) {
        LOG(log_error, logtype_afpd, "afp_getacl: cname got an error!");
        return AFPERR_NOOBJ;
    }
    if (!s_path->st_valid)
        of_statdir(vol, s_path);
    if ( s_path->st_errno != 0 ) {
        LOG(log_error, logtype_afpd, "afp_getacl: cant stat");
        return AFPERR_NOOBJ;
    }

    ret = check_acl_access(s_path->u_name, uuid, darwin_ace_rights);

    LOG(log_debug9, logtype_afpd, "afp_access: END");
    return ret;
}

int afp_getacl(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *dir;
    int         ret;
    uint32_t           did;
    uint16_t        vid, bitmap;
    struct path         *s_path;
    struct passwd       *pw;
    struct group        *gr;

    LOG(log_debug9, logtype_afpd, "afp_getacl: BEGIN");
    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid ))) {
        LOG(log_error, logtype_afpd, "afp_getacl: error getting volid:%d", vid);
        return AFPERR_NOOBJ;
    }

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did ))) {
        LOG(log_error, logtype_afpd, "afp_getacl: error getting did:%d", did);
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    memcpy(rbuf, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );
    rbuf += sizeof( bitmap );
    *rbuflen += sizeof( bitmap );

    /* skip maxreplysize */
    ibuf += 4;

    /* get full path and handle file/dir subtleties in netatalk code*/
    if (NULL == ( s_path = cname( vol, dir, &ibuf ))) {
        LOG(log_error, logtype_afpd, "afp_getacl: cname got an error!");
        return AFPERR_NOOBJ;
    }
    if (!s_path->st_valid)
        of_statdir(vol, s_path);
    if ( s_path->st_errno != 0 ) {
        LOG(log_error, logtype_afpd, "afp_getacl: cant stat");
        return AFPERR_NOOBJ;
    }

    /* Shall we return owner UUID ? */
    if (bitmap & kFileSec_UUID) {
        LOG(log_debug, logtype_afpd, "afp_getacl: client requested files owner user UUID");
        if (NULL == (pw = getpwuid(s_path->st.st_uid))) {
            LOG(log_debug, logtype_afpd, "afp_getacl: local uid: %u", s_path->st.st_uid);
            localuuid_from_id(rbuf, UUID_USER, s_path->st.st_uid);
        } else {
            LOG(log_debug, logtype_afpd, "afp_getacl: got uid: %d, name: %s", s_path->st.st_uid, pw->pw_name);
            if ((ret = getuuidfromname(pw->pw_name, UUID_USER, rbuf)) != 0)
                return AFPERR_MISC;
        }
        rbuf += UUID_BINSIZE;
        *rbuflen += UUID_BINSIZE;
    }

    /* Shall we return group UUID ? */
    if (bitmap & kFileSec_GRPUUID) {
        LOG(log_debug, logtype_afpd, "afp_getacl: client requested files owner group UUID");
        if (NULL == (gr = getgrgid(s_path->st.st_gid))) {
            LOG(log_debug, logtype_afpd, "afp_getacl: local gid: %u", s_path->st.st_gid);
            localuuid_from_id(rbuf, UUID_GROUP, s_path->st.st_gid);
        } else {
            LOG(log_debug, logtype_afpd, "afp_getacl: got gid: %d, name: %s", s_path->st.st_gid, gr->gr_name);
            if ((ret = getuuidfromname(gr->gr_name, UUID_GROUP, rbuf)) != 0)
                return AFPERR_MISC;
        }
        rbuf += UUID_BINSIZE;
        *rbuflen += UUID_BINSIZE;
    }

    /* Shall we return ACL ? */
    if (bitmap & kFileSec_ACL) {
        LOG(log_debug, logtype_afpd, "afp_getacl: client requested files ACL");
        get_and_map_acl(s_path->u_name, rbuf, rbuflen);
    }

    LOG(log_debug9, logtype_afpd, "afp_getacl: END");
    return AFP_OK;
}

int afp_setacl(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *dir;
    int         ret;
    uint32_t            did;
    uint16_t        vid, bitmap;
    struct path         *s_path;

    LOG(log_debug9, logtype_afpd, "afp_setacl: BEGIN");
    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof(vid);
    if (NULL == ( vol = getvolbyvid( vid ))) {
        LOG(log_error, logtype_afpd, "afp_setacl: error getting volid:%d", vid);
        return AFPERR_NOOBJ;
    }

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did ))) {
        LOG(log_error, logtype_afpd, "afp_setacl: error getting did:%d", did);
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    /* get full path and handle file/dir subtleties in netatalk code*/
    if (NULL == ( s_path = cname( vol, dir, &ibuf ))) {
        LOG(log_error, logtype_afpd, "afp_setacl: cname got an error!");
        return AFPERR_NOOBJ;
    }
    if (!s_path->st_valid)
        of_statdir(vol, s_path);
    if ( s_path->st_errno != 0 ) {
        LOG(log_error, logtype_afpd, "afp_setacl: cant stat");
        return AFPERR_NOOBJ;
    }
    LOG(log_debug, logtype_afpd, "afp_setacl: unixname: %s", s_path->u_name);

    /* Padding? */
    if ((unsigned long)ibuf & 1)
        ibuf++;

    /* Start processing request */

    /* Change owner: dont even try */
    if (bitmap & kFileSec_UUID) {
        LOG(log_debug, logtype_afpd, "afp_setacl: change owner request. discarded.");
        ret = AFPERR_ACCESS;
        ibuf += UUID_BINSIZE;
    }

    /* Change group: certain changes might be allowed, so try it. FIXME: not implemented yet. */
    if (bitmap & kFileSec_UUID) {
        LOG(log_debug, logtype_afpd, "afp_setacl: change group request. not supported this time.");
        ret = AFPERR_PARAM;
        ibuf += UUID_BINSIZE;
    }

    /* Remove ACL ? */
    if (bitmap & kFileSec_REMOVEACL) {
        LOG(log_debug, logtype_afpd, "afp_setacl: Remove ACL request.");
        if ((ret = remove_acl_vfs(vol, s_path->u_name, S_ISDIR(s_path->st.st_mode))) != AFP_OK)
            LOG(log_error, logtype_afpd, "afp_setacl: error from remove_acl");
    }

    /* Change ACL ? */
    if (bitmap & kFileSec_ACL) {
        LOG(log_debug, logtype_afpd, "afp_setacl: Change ACL request.");

        /* Check if its our job to preserve inherited ACEs */
        if (bitmap & kFileSec_Inherit)
            ret = set_acl_vfs(vol, s_path->u_name, 1, ibuf);
        else
            ret = set_acl_vfs(vol, s_path->u_name, 0, ibuf);
        if (ret == 0)
            ret = AFP_OK;
        else {
            LOG(log_warning, logtype_afpd, "afp_setacl(\"%s/%s\"): error",
                getcwdpath(), s_path->u_name);
            ret = AFPERR_MISC;
        }
    }

    LOG(log_debug9, logtype_afpd, "afp_setacl: END");
    return ret;
}

/*
  unix.c/accessmode calls this: map ACL to OS 9 mode
*/
void acltoownermode(char *path, struct stat *st, uid_t uid, struct maccess *ma)
{
    struct passwd *pw;
    uuid_t uuid;
    int dir, r_ok, w_ok, x_ok;

    if ( ! (AFPobj->options.flags & OPTION_UUID))
        return;

    LOG(log_maxdebug, logtype_afpd, "acltoownermode('%s')", path);

    if ((pw = getpwuid(uid)) == NULL) {
        LOG(log_error, logtype_afpd, "acltoownermode: %s", strerror(errno));
        return;
    }

    /* We need the UUID for check_acl_access */
    if ((getuuidfromname(pw->pw_name, UUID_USER, uuid)) != 0)
        return;

    /* These work for files and dirs */
    r_ok = check_acl_access(path, uuid, DARWIN_ACE_READ_DATA);
    w_ok = check_acl_access(path, uuid, (DARWIN_ACE_WRITE_DATA|DARWIN_ACE_APPEND_DATA));
    x_ok = check_acl_access(path, uuid, DARWIN_ACE_EXECUTE);

    LOG(log_debug7, logtype_afpd, "acltoownermode: ma_user before: %04o",ma->ma_user);
    if (r_ok == 0)
        ma->ma_user |= AR_UREAD;
    if (w_ok == 0)
        ma->ma_user |= AR_UWRITE;
    if (x_ok == 0)
        ma->ma_user |= AR_USEARCH;
    LOG(log_debug7, logtype_afpd, "acltoownermode: ma_user after: %04o", ma->ma_user);

    return;
}

/*
  We're being called at the end of afp_createdir. We're (hopefully) inside dir
  and ".AppleDouble" and ".AppleDouble/.Parent" should have already been created.
  We then inherit any explicit ACE from "." to ".AppleDouble" and ".AppleDouble/.Parent".
  FIXME: add to VFS layer ?
*/
void addir_inherit_acl(const struct vol *vol)
{
    ace_t *diraces = NULL, *adaces = NULL, *combinedaces = NULL;
    int diracecount, adacecount;

    LOG(log_debug9, logtype_afpd, "addir_inherit_acl: BEGIN");

    /* Check if ACLs are enabled for the volume */
    if (vol->v_flags & AFPVOL_ACLS) {

        if ((diracecount = get_nfsv4_acl(".", &diraces)) <= 0)
            goto cleanup;
        /* Remove any trivial ACE from "." */
        if ((diracecount = strip_trivial_aces(&diraces, diracecount)) <= 0)
            goto cleanup;

        /*
          Inherit to ".AppleDouble"
        */

        if ((adacecount = get_nfsv4_acl(".AppleDouble", &adaces)) <= 0)
            goto cleanup;
        /* Remove any non-trivial ACE from ".AppleDouble" */
        if ((adacecount = strip_nontrivial_aces(&adaces, adacecount)) <= 0)
            goto cleanup;

        /* Combine ACEs */
        if ((combinedaces = concat_aces(diraces, diracecount, adaces, adacecount)) == NULL)
            goto cleanup;

        /* Now set new acl */
        if ((acl(".AppleDouble", ACE_SETACL, diracecount + adacecount, combinedaces)) != 0)
            LOG(log_error, logtype_afpd, "addir_inherit_acl: acl: %s", strerror(errno));

        free(adaces);
        adaces = NULL;
        free(combinedaces);
        combinedaces = NULL;

        /*
          Inherit to ".AppleDouble/.Parent"
        */

        if ((adacecount = get_nfsv4_acl(".AppleDouble/.Parent", &adaces)) <= 0)
            goto cleanup;
        if ((adacecount = strip_nontrivial_aces(&adaces, adacecount)) <= 0)
            goto cleanup;

        /* Combine ACEs */
        if ((combinedaces = concat_aces(diraces, diracecount, adaces, adacecount)) == NULL)
            goto cleanup;

        /* Now set new acl */
        if ((acl(".AppleDouble/.Parent", ACE_SETACL, diracecount + adacecount, combinedaces)) != 0)
            LOG(log_error, logtype_afpd, "addir_inherit_acl: acl: %s", strerror(errno));


    }

cleanup:
    LOG(log_debug9, logtype_afpd, "addir_inherit_acl: END");

    free(diraces);
    free(adaces);
    free(combinedaces);
}
