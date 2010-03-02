/*
 * $Id: directory.c,v 1.139 2010-03-02 18:07:13 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * 19 jan 2000 implemented red-black trees for directory lookups
 * (asun@cobalt.com).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <grp.h>
#include <pwd.h>
#include <sys/param.h>
#include <errno.h>
#include <utime.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/uuid.h>
#include <atalk/unix.h>

#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"
#include "file.h"
#include "filedir.h"
#include "globals.h"
#include "unix.h"
#include "mangle.h"
#include "hash.h"

#ifdef HAVE_NFSv4_ACLS
extern void addir_inherit_acl(const struct vol *vol);
#endif

/* 
 * Directory caches
 * ================
 *
 * There are currently two cache structures where afpd caches directory information
 * a) a DID/dirname cache in a hashtable 
 * b) a (red-black) tree with CNIDs as key
 *
 * a) is for searching by DID/dirname
 * b) is for searching by CNID
 *
 * Through additional parent, child, previous and next pointers, b) is also used to
 * represent the on-disk layout of the filesystem. parent and child point to parent
 * and child directory respectively, linking 2 or more subdirectories in one
 * directory with previous and next pointers.
 *
 * Usage examples, highlighting the main functions:
 * 
 * a) is eg used in enumerate():
 * if IS_DIR
 *     dir = dirsearch_byname() // search in cache
 *     if (dir == NULL)         // not found
 *         dir = adddir()       // add to cache
 *      getdirparams()
 *
 * b) is eg used in afp_getfildirparams()
 * dirlookup()             // wrapper for cache and db search
 *   => dir = dirsearch()  // search in cache
 *      if (dir)           // found
 *          return
 *      else               // not found,
 *          cnid_resolve() // resolve with CNID database
 *      cname()            // add to cache
 */

struct dir  *curdir;
int             afp_errno;

#define SENTINEL (&sentinel)
static struct dir sentinel = { SENTINEL, SENTINEL, NULL, /* left, right, back */
                               DIRTREE_COLOR_BLACK,      /* color */
                               NULL, NULL,               /* parent, child */
                               NULL, NULL,               /* previous, next */
                               NULL, 0, 0,               /* oforks, did, flags */
                               0, 0,                     /* ctime, offcnt */
                               NULL, NULL, NULL};        /* mname, uname, ucs2-name */
static struct dir rootpar  = { SENTINEL, SENTINEL, NULL,
                               0,
                               NULL, NULL,
                               NULL, NULL,
                               NULL, 0, 0,
                               0, 0,
                               NULL, NULL, NULL};

/* (from IM: Toolbox Essentials)
 * dirFinderInfo (DInfo) fields:
 * field        bytes
 * frRect       8    folder's window rectangle
 * frFlags      2    flags
 * frLocation   4    folder's location in window
 * frView       2    folder's view (default == closedView (256))
 *
 * extended dirFinderInfo (DXInfo) fields:
 * frScroll     4    scroll position
 * frOpenChain: 4    directory ID chain of open folders
 * frScript:    1    script flag and code
 * frXFlags:    1    reserved
 * frComment:   2    comment ID
 * frPutAway:   4    home directory ID
 */

static struct dir *
vol_tree_root(const struct vol *vol, u_int32_t did)
{
    struct dir *dir;

    if (vol->v_curdir && vol->v_curdir->d_did == did) {
        dir = vol->v_curdir;
    }
    else {
        dir = vol->v_root;
    }
    return dir;
}

/*
 * redid did assignment for directories. now we use red-black trees.
 * how exciting.
 */
struct dir *
dirsearch(const struct vol *vol, u_int32_t did)
{
    struct dir  *dir;


    /* check for 0 did */
    if (!did) {
        afp_errno = AFPERR_PARAM;
        return NULL;
    }
    if ( did == DIRDID_ROOT_PARENT ) {
        if (!rootpar.d_did)
            rootpar.d_did = DIRDID_ROOT_PARENT;
        rootpar.d_child = vol->v_dir;
        return( &rootpar );
    }

    dir = vol_tree_root(vol, did);

    afp_errno = AFPERR_NOOBJ;
    while ( dir != SENTINEL ) {
        if (dir->d_did == did)
            return dir->d_m_name ? dir : NULL;
        dir = (dir->d_did > did) ? dir->d_left : dir->d_right;
    }
    return NULL;
}

/* ------------------- */
int get_afp_errno(const int param)
{
    if (afp_errno != AFPERR_DID1)
        return afp_errno;
    return param;
}

/* ------------------- */
struct dir *
dirsearch_byname( const struct vol *vol, struct dir *cdir, char *name)
{
    struct dir *dir = NULL;

    if ((cdir->d_did != DIRDID_ROOT_PARENT) && (cdir->d_child)) {
        struct dir key;
        hnode_t *hn;

        key.d_parent = cdir;
        key.d_u_name = name;
        hn = hash_lookup(vol->v_hash, &key);
        if (hn) {
            dir = hnode_get(hn);
        }
    }
    return dir;
}

/* -----------------------------------------
 * if did is not in the cache resolve it with cnid
 *
 * FIXME
 * OSX call it with bogus id, ie file ID not folder ID,
 * and we are really bad in this case.
 */
struct dir *
dirlookup( struct vol *vol, u_int32_t did)
{
    struct dir   *ret;
    char     *upath;
    cnid_t       id, cnid;
    static char  path[MAXPATHLEN + 1];
    size_t len,  pathlen;
    char         *ptr;
    static char  buffer[12 + MAXPATHLEN + 1];
    int          buflen = 12 + MAXPATHLEN + 1;
    char         *mpath;
    int          utf8;
    size_t       maxpath;

    ret = dirsearch(vol, did);
    if (ret != NULL || afp_errno == AFPERR_PARAM)
        return ret;

    utf8 = utf8_encoding();
    maxpath = (utf8)?MAXPATHLEN -7:255;
    id = did;
    if (NULL == (upath = cnid_resolve(vol->v_cdb, &id, buffer, buflen)) ) {
        afp_errno = AFPERR_NOOBJ;
        return NULL;
    }
    ptr = path + MAXPATHLEN;
    if (NULL == ( mpath = utompath(vol, upath, did, utf8) ) ) {
        afp_errno = AFPERR_NOOBJ;
        return NULL;
    }
    len = strlen(mpath);
    pathlen = len;          /* no 0 in the last part */
    len++;
    strcpy(ptr - len, mpath);
    ptr -= len;
    while (1) {
        ret = dirsearch(vol,id);
        if (ret != NULL) {
            break;
        }
        cnid = id;
        if ( NULL == (upath = cnid_resolve(vol->v_cdb, &id, buffer, buflen))
             ||
             NULL == (mpath = utompath(vol, upath, cnid, utf8))
            ) {
            afp_errno = AFPERR_NOOBJ;
            return NULL;
        }

        len = strlen(mpath) + 1;
        pathlen += len;
        if (pathlen > maxpath) {
            afp_errno = AFPERR_PARAM;
            return NULL;
        }
        strcpy(ptr - len, mpath);
        ptr -= len;
    }

    /* fill the cache, another place where we know about the path type */
    if (utf8) {
        u_int16_t temp16;
        u_int32_t temp;

        ptr -= 2;
        temp16 = htons(pathlen);
        memcpy(ptr, &temp16, sizeof(temp16));

        temp = htonl(kTextEncodingUTF8);
        ptr -= 4;
        memcpy(ptr, &temp, sizeof(temp));
        ptr--;
        *ptr = 3;
    }
    else {
        ptr--;
        *ptr = (unsigned char)pathlen;
        ptr--;
        *ptr = 2;
    }
    /* cname is not efficient */
    if (cname( vol, ret, &ptr ) == NULL )
        return NULL;

    return dirsearch(vol, did);
}

/* child addition/removal */
static void dirchildadd(const struct vol *vol, struct dir *a, struct dir *b)
{
    if (!a->d_child)
        a->d_child = b;
    else {
        b->d_next = a->d_child;
        b->d_prev = b->d_next->d_prev;
        b->d_next->d_prev = b;
        b->d_prev->d_next = b;
    }
    if (!hash_alloc_insert(vol->v_hash, b, b)) {
        LOG(log_error, logtype_afpd, "dirchildadd: can't hash %s", b->d_u_name);
    }
}

static void dirchildremove(struct dir *a,struct dir *b)
{
    if (a->d_child == b)
        a->d_child = (b == b->d_next) ? NULL : b->d_next;
    b->d_next->d_prev = b->d_prev;
    b->d_prev->d_next = b->d_next;
    b->d_next = b->d_prev = b;
}

/* --------------------------- */
/* rotate the tree to the left */
static void dir_leftrotate(struct vol *vol, struct dir *dir)
{
    struct dir *right = dir->d_right;

    /* whee. move the right's left tree into dir's right tree */
    dir->d_right = right->d_left;
    if (right->d_left != SENTINEL)
        right->d_left->d_back = dir;

    if (right != SENTINEL) {
        right->d_back = dir->d_back;
        right->d_left = dir;
    }

    if (!dir->d_back) /* no parent. move the right tree to the top. */
        vol->v_root = right;
    else if (dir == dir->d_back->d_left) /* we were on the left */
        dir->d_back->d_left = right;
    else
        dir->d_back->d_right = right; /* we were on the right */

    /* re-insert dir on the left tree */
    if (dir != SENTINEL)
        dir->d_back = right;
}



/* rotate the tree to the right */
static void dir_rightrotate(struct vol *vol, struct dir *dir)
{
    struct dir *left = dir->d_left;

    /* whee. move the left's right tree into dir's left tree */
    dir->d_left = left->d_right;
    if (left->d_right != SENTINEL)
        left->d_right->d_back = dir;

    if (left != SENTINEL) {
        left->d_back = dir->d_back;
        left->d_right = dir;
    }

    if (!dir->d_back) /* no parent. move the left tree to the top. */
        vol->v_root = left;
    else if (dir == dir->d_back->d_right) /* we were on the right */
        dir->d_back->d_right = left;
    else
        dir->d_back->d_left = left; /* we were on the left */

    /* re-insert dir on the right tree */
    if (dir != SENTINEL)
        dir->d_back = left;
}

#if 0
/* recolor after a removal */
static struct dir *dir_rmrecolor(struct vol *vol, struct dir *dir)
{
    struct dir *leaf;

    while ((dir != vol->v_root) && (dir->d_color == DIRTREE_COLOR_BLACK)) {
        /* are we on the left tree? */
        if (dir == dir->d_back->d_left) {
            leaf = dir->d_back->d_right; /* get right side */
            if (leaf->d_color == DIRTREE_COLOR_RED) {
                /* we're red. we need to change to black. */
                leaf->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_color = DIRTREE_COLOR_RED;
                dir_leftrotate(vol, dir->d_back);
                leaf = dir->d_back->d_right;
            }

            /* right leaf has black end nodes */
            if ((leaf->d_left->d_color == DIRTREE_COLOR_BLACK) &&
                (leaf->d_right->d_color = DIRTREE_COLOR_BLACK)) {
                leaf->d_color = DIRTREE_COLOR_RED; /* recolor leaf as red */
                dir = dir->d_back; /* ascend */
            } else {
                if (leaf->d_right->d_color == DIRTREE_COLOR_BLACK) {
                    leaf->d_left->d_color = DIRTREE_COLOR_BLACK;
                    leaf->d_color = DIRTREE_COLOR_RED;
                    dir_rightrotate(vol, leaf);
                    leaf = dir->d_back->d_right;
                }
                leaf->d_color = dir->d_back->d_color;
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                leaf->d_right->d_color = DIRTREE_COLOR_BLACK;
                dir_leftrotate(vol, dir->d_back);
                dir = vol->v_root;
            }
        } else { /* right tree */
            leaf = dir->d_back->d_left; /* left tree */
            if (leaf->d_color == DIRTREE_COLOR_RED) {
                leaf->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_color = DIRTREE_COLOR_RED;
                dir_rightrotate(vol, dir->d_back);
                leaf = dir->d_back->d_left;
            }

            /* left leaf has black end nodes */
            if ((leaf->d_right->d_color == DIRTREE_COLOR_BLACK) &&
                (leaf->d_left->d_color = DIRTREE_COLOR_BLACK)) {
                leaf->d_color = DIRTREE_COLOR_RED; /* recolor leaf as red */
                dir = dir->d_back; /* ascend */
            } else {
                if (leaf->d_left->d_color == DIRTREE_COLOR_BLACK) {
                    leaf->d_right->d_color = DIRTREE_COLOR_BLACK;
                    leaf->d_color = DIRTREE_COLOR_RED;
                    dir_leftrotate(vol, leaf);
                    leaf = dir->d_back->d_left;
                }
                leaf->d_color = dir->d_back->d_color;
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                leaf->d_left->d_color = DIRTREE_COLOR_BLACK;
                dir_rightrotate(vol, dir->d_back);
                dir = vol->v_root;
            }
        }
    }
    dir->d_color = DIRTREE_COLOR_BLACK;

    return dir;
}
#endif /* 0 */

/* --------------------- */
static void dir_hash_del(const struct vol *vol, struct dir *dir)
{
    hnode_t *hn;

    hn = hash_lookup(vol->v_hash, dir);
    if (!hn) {
        LOG(log_error, logtype_afpd, "dir_hash_del: %s not hashed", dir->d_u_name);
    }
    else {
        hash_delete(vol->v_hash, hn);
    }
}

/* remove the node from the tree. this is just like insertion, but
 * different. actually, it has to worry about a bunch of things that
 * insertion doesn't care about. */

static void dir_remove( struct vol *vol, struct dir *dir)
{
#ifdef REMOVE_NODES
    struct ofork *of, *last;
    struct dir *node, *leaf;
#endif /* REMOVE_NODES */

    if (!dir || (dir == SENTINEL))
        return;

    /* i'm not sure if it really helps to delete stuff. */
    dir_hash_del(vol, dir);
    vol->v_curdir = NULL;
#ifndef REMOVE_NODES
    dirfreename(dir);
    dir->d_m_name = NULL;
    dir->d_u_name = NULL;
    dir->d_m_name_ucs2 = NULL;
#else /* ! REMOVE_NODES */

    /* go searching for a node with at most one child */
    if ((dir->d_left == SENTINEL) || (dir->d_right == SENTINEL)) {
        node = dir;
    } else {
        node = dir->d_right;
        while (node->d_left != SENTINEL)
            node = node->d_left;
    }

    /* get that child */
    leaf = (node->d_left != SENTINEL) ? node->d_left : node->d_right;

    /* detach node */
    leaf->d_back = node->d_back;
    if (!node->d_back) {
        vol->v_root = leaf;
    } else if (node == node->d_back->d_left) { /* left tree */
        node->d_back->d_left = leaf;
    } else {
        node->d_back->d_right = leaf;
    }

    /* we want to free node, but we also want to free the data in dir.
     * currently, that's d_name and the directory traversal bits.
     * we just copy the necessary bits and then fix up all the
     * various pointers to the directory. needless to say, there are
     * a bunch of places that store the directory struct. */
    if (node != dir) {
        struct dir save, *tmp;

        memcpy(&save, dir, sizeof(save));
        memcpy(dir, node, sizeof(struct dir));

        /* restore the red-black bits */
        dir->d_left = save.d_left;
        dir->d_right = save.d_right;
        dir->d_back = save.d_back;
        dir->d_color = save.d_color;

        if (node == vol->v_dir) {/* we may need to fix up this pointer */
            vol->v_dir = dir;
            rootpar.d_child = vol->v_dir;
        } else {
            /* if we aren't the root directory, we have parents and
             * siblings to worry about */
            if (dir->d_parent->d_child == node)
                dir->d_parent->d_child = dir;
            dir->d_next->d_prev = dir;
            dir->d_prev->d_next = dir;
        }

        /* fix up children. */
        tmp = dir->d_child;
        while (tmp) {
            tmp->d_parent = dir;
            tmp = (tmp == dir->d_child->d_prev) ? NULL : tmp->d_next;
        }

        if (node == curdir) /* another pointer to fixup */
            curdir = dir;

        /* we also need to fix up oforks. bleah */
        if ((of = dir->d_ofork)) {
            last = of->of_d_prev;
            while (of) {
                of->of_dir = dir;
                of = (last == of) ? NULL : of->of_d_next;
            }
        }

        /* set the node's d_name */
        node->d_m_name = save.d_m_name;
        node->d_u_name = save.d_u_name;
        node->d_m_name_ucs2 = save.d_m_name_ucs2;
    }

    if (node->d_color == DIRTREE_COLOR_BLACK)
        dir_rmrecolor(vol, leaf);

    if (node->d_m_name_ucs2)
        free(node->d_u_name_ucs2);
    if (node->d_u_name != node->d_m_name) {
        free(node->d_u_name);
    }
    free(node->d_m_name);
    free(node);
#endif /* ! REMOVE_NODES */
}

/* ---------------------------------------
 * remove the node and its childs from the tree
 *
 * FIXME what about opened forks with refs to it?
 * it's an afp specs violation because you can't delete
 * an opened forks. Now afpd doesn't care about forks opened by other
 * process. It's fixable within afpd if fnctl_lock, doable with smb and
 * next to impossible for nfs and local filesystem access.
 */
static void dir_invalidate( struct vol *vol, struct dir *dir)
{
    if (curdir == dir) {
        /* v_root can't be deleted */
        if (movecwd(vol, vol->v_root) < 0) {
            LOG(log_error, logtype_afpd, "cname can't chdir to : %s", vol->v_root);
        }
    }
    /* FIXME */
    dirchildremove(dir->d_parent, dir);
    dir_remove( vol, dir );
}

/* ------------------------------------ */
static struct dir *dir_insert(const struct vol *vol, struct dir *dir)
{
    struct dir  *pdir;

    pdir = vol_tree_root(vol, dir->d_did);
    while (pdir->d_did != dir->d_did ) {
        if ( pdir->d_did > dir->d_did ) {
            if ( pdir->d_left == SENTINEL ) {
                pdir->d_left = dir;
                dir->d_back = pdir;
                return NULL;
            }
            pdir = pdir->d_left;
        } else {
            if ( pdir->d_right == SENTINEL ) {
                pdir->d_right = dir;
                dir->d_back = pdir;
                return NULL;
            }
            pdir = pdir->d_right;
        }
    }
    return pdir;
}

#define ENUMVETO "./../Network Trash Folder/TheVolumeSettingsFolder/TheFindByContentFolder/:2eDS_Store/Contents/Desktop Folder/Trash/Benutzer/"

int
caseenumerate(const struct vol *vol, struct path *path, struct dir *dir)
{
    DIR               *dp;
    struct dirent     *de;
    int               ret;
    static u_int32_t  did = 0;
    static char       cname[MAXPATHLEN];
    static char       lname[MAXPATHLEN];
    ucs2_t        u2_path[MAXPATHLEN];
    ucs2_t        u2_dename[MAXPATHLEN];
    char          *tmp, *savepath;

    if (!(vol->v_flags & AFPVOL_CASEINSEN))
        return -1;

    if (veto_file(ENUMVETO, path->u_name))
        return -1;

    savepath = path->u_name;

    /* very simple cache */
    if ( dir->d_did == did && strcmp(lname, path->u_name) == 0) {
        path->u_name = cname;
        path->d_dir = NULL;
        if (of_stat( path ) == 0 ) {
            return 0;
        }
        /* something changed, we cannot stat ... */
        did = 0;
    }

    if (NULL == ( dp = opendir( "." )) ) {
        LOG(log_debug, logtype_afpd, "caseenumerate: opendir failed: %s", dir->d_u_name);
        return -1;
    }


    /* LOG(log_debug, logtype_afpd, "caseenumerate: for %s", path->u_name); */
    if ((size_t) -1 == convert_string(vol->v_volcharset, CH_UCS2, path->u_name, -1, u2_path, sizeof(u2_path)) )
        LOG(log_debug, logtype_afpd, "caseenumerate: conversion failed for %s", path->u_name);

    /*LOG(log_debug, logtype_afpd, "caseenumerate: dir: %s, path: %s", dir->d_u_name, path->u_name); */
    ret = -1;
    for ( de = readdir( dp ); de != NULL; de = readdir( dp )) {
        if (NULL == check_dirent(vol, de->d_name))
            continue;

        if ((size_t) -1 == convert_string(vol->v_volcharset, CH_UCS2, de->d_name, -1, u2_dename, sizeof(u2_dename)) )
            continue;

        if (strcasecmp_w( u2_path, u2_dename) == 0) {
            tmp = path->u_name;
            strlcpy(cname, de->d_name, sizeof(cname));
            path->u_name = cname;
            path->d_dir = NULL;
            if (of_stat( path ) == 0 ) {
                LOG(log_debug, logtype_afpd, "caseenumerate: using dir: %s, path: %s", de->d_name, path->u_name);
                strlcpy(lname, tmp, sizeof(lname));
                did = dir->d_did;
                ret = 0;
                break;
            }
            else
                path->u_name = tmp;
        }

    }
    closedir(dp);

    if (ret) {
        /* invalidate cache */
        cname[0] = 0;
        did = 0;
        path->u_name = savepath;
    }
    /* LOG(log_debug, logtype_afpd, "caseenumerate: path on ret: %s", path->u_name); */
    return ret;
}


/*
 * attempt to extend the current dir. tree to include path
 * as a side-effect, movecwd to that point and return the new dir
 */
static struct dir *
extenddir(struct vol *vol, struct dir *dir, struct path *path)
{
    path->d_dir = NULL;

    if ( path->u_name == NULL) {
        afp_errno = AFPERR_PARAM;
        return NULL;
    }

    if (check_name(vol, path->u_name)) {
        /* the name is illegal */
        LOG(log_info, logtype_afpd, "extenddir: illegal path: '%s'", path->u_name);
        path->u_name = NULL;
        afp_errno = AFPERR_PARAM;
        return NULL;
    }

    if (of_stat( path ) != 0 ) {
        if (!(vol->v_flags & AFPVOL_CASEINSEN))
            return NULL;
        else if(caseenumerate(vol, path, dir) != 0)
            return(NULL);
    }

    if (!S_ISDIR(path->st.st_mode)) {
        return( NULL );
    }

    /* mac name is always with the right encoding (from cname()) */
    if (( dir = adddir( vol, dir, path)) == NULL ) {
        return( NULL );
    }

    path->d_dir = dir;
    if ( movecwd( vol, dir ) < 0 ) {
        return( NULL );
    }

    return( dir );
}

/* -------------------------
   appledouble mkdir afp error code.
*/
static int netatalk_mkdir(const char *name)
{
    if (ad_mkdir(name, DIRBITS | 0777) < 0) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EROFS :
            return( AFPERR_VLOCK );
        case EPERM:
        case EACCES :
            return( AFPERR_ACCESS );
        case EEXIST :
            return( AFPERR_EXIST );
        case ENOSPC :
        case EDQUOT :
            return( AFPERR_DFULL );
        default :
            return( AFPERR_PARAM );
        }
    }
    return AFP_OK;
}

/* ------------------- */
static int deletedir(char *dir)
{
    char path[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent   *de;
    struct stat st;
    size_t len;
    int err = AFP_OK;
    size_t remain;

    if ((len = strlen(dir)) +2 > sizeof(path))
        return AFPERR_PARAM;

    /* already gone */
    if ((dp = opendir(dir)) == NULL)
        return AFP_OK;

    strcpy(path, dir);
    strcat(path, "/");
    len++;
    remain = sizeof(path) -len -1;
    while ((de = readdir(dp)) && err == AFP_OK) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        if (strlen(de->d_name) > remain) {
            err = AFPERR_PARAM;
            break;
        }
        strcpy(path + len, de->d_name);
        if (lstat(path, &st)) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            err = deletedir(path);
        } else {
            err = netatalk_unlink(path);
        }
    }
    closedir(dp);

    /* okay. the directory is empty. delete it. note: we already got rid
       of .AppleDouble.  */
    if (err == AFP_OK) {
        err = netatalk_rmdir(dir);
    }
    return err;
}

/* do a recursive copy. */
static int copydir(const struct vol *vol, char *src, char *dst)
{
    char spath[MAXPATHLEN + 1], dpath[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent   *de;
    struct stat st;
    struct utimbuf      ut;
    size_t slen, dlen;
    size_t srem, drem;
    int err;

    /* doesn't exist or the path is too long. */
    if (((slen = strlen(src)) > sizeof(spath) - 2) ||
        ((dlen = strlen(dst)) > sizeof(dpath) - 2) ||
        ((dp = opendir(src)) == NULL))
        return AFPERR_PARAM;

    /* try to create the destination directory */
    if (AFP_OK != (err = netatalk_mkdir(dst)) ) {
        closedir(dp);
        return err;
    }

    /* set things up to copy */
    strcpy(spath, src);
    strcat(spath, "/");
    slen++;
    srem = sizeof(spath) - slen -1;

    strcpy(dpath, dst);
    strcat(dpath, "/");
    dlen++;
    drem = sizeof(dpath) - dlen -1;

    err = AFP_OK;
    while ((de = readdir(dp))) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        if (strlen(de->d_name) > srem) {
            err = AFPERR_PARAM;
            break;
        }
        strcpy(spath + slen, de->d_name);

        if (lstat(spath, &st) == 0) {
            if (strlen(de->d_name) > drem) {
                err = AFPERR_PARAM;
                break;
            }
            strcpy(dpath + dlen, de->d_name);

            if (S_ISDIR(st.st_mode)) {
                if (AFP_OK != (err = copydir(vol, spath, dpath)))
                    goto copydir_done;
            } else if (AFP_OK != (err = copyfile(vol, vol, spath, dpath, NULL, NULL))) {
                goto copydir_done;

            } else {
                /* keep the same time stamp. */
                ut.actime = ut.modtime = st.st_mtime;
                utime(dpath, &ut);
            }
        }
    }

    /* keep the same time stamp. */
    if (lstat(src, &st) == 0) {
        ut.actime = ut.modtime = st.st_mtime;
        utime(dst, &ut);
    }

copydir_done:
    closedir(dp);
    return err;
}


/* --- public functions follow --- */

/* NOTE: we start off with at least one node (the root directory). */
static struct dir *dirinsert(struct vol *vol, struct dir *dir)
{
    struct dir *node;

    if ((node = dir_insert(vol, dir)))
        return node;

    /* recolor the tree. the current node is red. */
    dir->d_color = DIRTREE_COLOR_RED;

    /* parent of this node has to be black. if the parent node
     * is red, then we have a grandparent. */
    while ((dir != vol->v_root) &&
           (dir->d_back->d_color == DIRTREE_COLOR_RED)) {
        /* are we on the left tree? */
        if (dir->d_back == dir->d_back->d_back->d_left) {
            node = dir->d_back->d_back->d_right;  /* get the right node */
            if (node->d_color == DIRTREE_COLOR_RED) {
                /* we're red. we need to change to black. */
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                node->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_back->d_color = DIRTREE_COLOR_RED;
                dir = dir->d_back->d_back; /* finished. go up. */
            } else {
                if (dir == dir->d_back->d_right) {
                    dir = dir->d_back;
                    dir_leftrotate(vol, dir);
                }
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_back->d_color = DIRTREE_COLOR_RED;
                dir_rightrotate(vol, dir->d_back->d_back);
            }
        } else {
            node = dir->d_back->d_back->d_left;
            if (node->d_color == DIRTREE_COLOR_RED) {
                /* we're red. we need to change to black. */
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                node->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_back->d_color = DIRTREE_COLOR_RED;
                dir = dir->d_back->d_back; /* finished. ascend */
            } else {
                if (dir == dir->d_back->d_left) {
                    dir = dir->d_back;
                    dir_rightrotate(vol, dir);
                }
                dir->d_back->d_color = DIRTREE_COLOR_BLACK;
                dir->d_back->d_back->d_color = DIRTREE_COLOR_RED;
                dir_leftrotate(vol, dir->d_back->d_back);
            }
        }
    }

    vol->v_root->d_color = DIRTREE_COLOR_BLACK;
    return NULL;
}

/* ---------------------------- */
struct dir *
adddir(struct vol *vol, struct dir *dir, struct path *path)
{
    struct dir  *cdir, *edir;
    int     upathlen;
    char        *name;
    char        *upath;
    struct stat *st;
    int         deleted;
    struct adouble  ad;
    struct adouble *adp = NULL;
    cnid_t      id;

    upath = path->u_name;
    st    = &path->st;
    upathlen = strlen(upath);

    /* get_id needs adp for reading CNID from adouble file */
    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    if ((ad_open_metadata(upath, ADFLAGS_DIR, 0, &ad)) == 0)
        adp = &ad;

    id = get_id(vol, adp, st, dir->d_did, upath, upathlen);

    if (adp)
        ad_close_metadata(adp);

    if (id == 0) {
        return NULL;
    }
    if (!path->m_name && !(path->m_name = utompath(vol, upath, id , utf8_encoding()))) {
        return NULL;
    }
    name  = path->m_name;
    if ((cdir = dirnew(name, upath)) == NULL) {
        LOG(log_error, logtype_afpd, "adddir: malloc: %s", strerror(errno) );
        return NULL;
    }
    if ((size_t)-1 == convert_string_allocate((utf8_encoding())?CH_UTF8_MAC:vol->v_maccharset, CH_UCS2, path->m_name, -1, (char **)&cdir->d_m_name_ucs2)) {
        LOG(log_error, logtype_afpd, "Couldn't set UCS2 name for %s", name);
        cdir->d_m_name_ucs2 = NULL;
    }

    cdir->d_did = id;

    if ((edir = dirinsert( vol, cdir ))) {
        /* it's not possible with LASTDID
           for CNID:
           - someone else have moved the directory.
           - it's a symlink inside the share.
           - it's an ID reused, the old directory was deleted but not
           the cnid record and the server've reused the inode for
           the new dir.
           for HASH (we should get ride of HASH)
           - someone else have moved the directory.
           - it's an ID reused as above
           - it's a hash duplicate and we are in big trouble
        */
        deleted = (edir->d_m_name == NULL);
        if (!deleted)
            dir_hash_del(vol, edir);
        dirfreename(edir);
        edir->d_m_name = cdir->d_m_name;
        edir->d_u_name = cdir->d_u_name;
        edir->d_m_name_ucs2 = cdir->d_m_name_ucs2;
        free(cdir);
        cdir = edir;
        LOG(log_error, logtype_afpd, "adddir: insert %s", edir->d_m_name);
        if (!cdir->d_parent || (cdir->d_parent == dir && !deleted)) {
            hash_alloc_insert(vol->v_hash, cdir, cdir);
            return cdir;
        }
        /* the old was not in the same folder */
        if (!deleted)
            dirchildremove(cdir->d_parent, cdir);
    }

    /* parent/child directories */
    cdir->d_parent = dir;
    dirchildadd(vol, dir, cdir);
    return( cdir );
}

/* --- public functions follow --- */
/* free everything down. we don't bother to recolor as this is only
 * called to free the entire tree */
void dirfreename(struct dir *dir)
{
    if (dir->d_u_name != dir->d_m_name) {
        free(dir->d_u_name);
    }
    if (dir->d_m_name_ucs2)
        free(dir->d_m_name_ucs2);
    free(dir->d_m_name);
}

void dirfree(struct dir *dir)
{
    if (!dir || (dir == SENTINEL))
        return;

    if ( dir->d_left != SENTINEL ) {
        dirfree( dir->d_left );
    }
    if ( dir->d_right != SENTINEL ) {
        dirfree( dir->d_right );
    }

    if (dir != SENTINEL) {
        dirfreename(dir);
        free( dir );
    }
}

/* --------------------------------------------
 * most of the time mac name and unix name are the same
 */
struct dir *dirnew(const char *m_name, const char *u_name)
{
    struct dir *dir;

    dir = (struct dir *) calloc(1, sizeof( struct dir ));
    if (!dir)
        return NULL;

    if ((dir->d_m_name = strdup(m_name)) == NULL) {
        free(dir);
        return NULL;
    }

    if (m_name == u_name || !strcmp(m_name, u_name)) {
        dir->d_u_name = dir->d_m_name;
    }
    else if ((dir->d_u_name = strdup(u_name)) == NULL) {
        free(dir->d_m_name);
        free(dir);
        return NULL;
    }

    dir->d_m_name_ucs2 = NULL;
    dir->d_left = dir->d_right = SENTINEL;
    dir->d_next = dir->d_prev = dir;
    return dir;
}

/* ------------------ */
static hash_val_t hash_fun_dir(const void *key)
{
    const struct dir *k = key;

    static unsigned long randbox[] = {
        0x49848f1bU, 0xe6255dbaU, 0x36da5bdcU, 0x47bf94e9U,
        0x8cbcce22U, 0x559fc06aU, 0xd268f536U, 0xe10af79aU,
        0xc1af4d69U, 0x1d2917b5U, 0xec4c304dU, 0x9ee5016cU,
        0x69232f74U, 0xfead7bb3U, 0xe9089ab6U, 0xf012f6aeU,
    };

    const unsigned char *str = (unsigned char *)(k->d_u_name);
    hash_val_t acc = k->d_parent->d_did;

    while (*str) {
        acc ^= randbox[(*str + acc) & 0xf];
        acc = (acc << 1) | (acc >> 31);
        acc &= 0xffffffffU;
        acc ^= randbox[((*str++ >> 4) + acc) & 0xf];
        acc = (acc << 2) | (acc >> 30);
        acc &= 0xffffffffU;
    }
    return acc;
}

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)    \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)    \
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static hash_val_t hash_fun2_dir(const void *key)
{
    const struct dir *k = key;
    const char *data = k->d_u_name;
    int len = strlen(k->d_u_name);
    hash_val_t hash = k->d_parent->d_did, tmp;

    int rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3: hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= data[sizeof (uint16_t)] << 18;
        hash += hash >> 11;
        break;
    case 2: hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/* ---------------- */
static int hash_comp_dir(const void *key1, const void *key2)
{
    const struct dir *k1 = key1;
    const struct dir *k2 = key2;

    return !(k1->d_parent->d_did == k2->d_parent->d_did && !strcmp(k1->d_u_name, k2->d_u_name));
}

/* ---------------- */
hash_t *
dirhash(void)
{
    return hash_create(HASHCOUNT_T_MAX, hash_comp_dir, hash_fun2_dir);
}

/* ------------------ */
static struct path *invalidate (struct vol *vol, struct dir *dir, struct path *ret)
{
    /* it's tricky:
       movecwd failed some of dir path are not there anymore.
       FIXME Is it true with other errors?
       so we remove dir from the cache
    */
    if (dir->d_did == DIRDID_ROOT_PARENT)
        return NULL;
    if (afp_errno == AFPERR_ACCESS) {
        if ( movecwd( vol, dir->d_parent ) < 0 ) {
            return NULL;
        }
        /* FIXME should we set these?, don't need to call stat() after:
           ret->st_valid = 1;
           ret->st_errno = EACCES;
        */
        ret->m_name = dir->d_m_name;
        ret->u_name = dir->d_u_name;
        ret->d_dir = dir;
        return ret;
    } else if (afp_errno == AFPERR_NOOBJ) {
        if ( movecwd( vol, dir->d_parent ) < 0 ) {
            return NULL;
        }
        strcpy(ret->m_name, dir->d_m_name);
        if (dir->d_m_name == dir->d_u_name) {
            ret->u_name = ret->m_name;
        }
        else {
            size_t tp = strlen(ret->m_name)+1;

            ret->u_name =  ret->m_name +tp;
            strcpy(ret->u_name, dir->d_u_name);
        }
        /* FIXME should we set :
           ret->st_valid = 1;
           ret->st_errno = ENOENT;
        */
        dir_invalidate(vol, dir);
        return ret;
    }
    dir_invalidate(vol, dir);
    return NULL;
}

/* -------------------------------------------------- */
/* cname
   return
   if it's a filename:
   in extenddir:
   compute unix name
   stat the file or errno
   return
   filename
   curdir: filename parent directory

   if it's a dirname:
   not in the cache
   in extenddir
   compute unix name
   stat the dir or errno
   return
   if chdir error
   dirname
   curdir: dir parent directory
   sinon
   dirname: ""
   curdir: dir
   in the cache
   return
   if chdir error
   dirname
   curdir: dir parent directory
   else
   dirname: ""
   curdir: dir

*/
struct path *
cname(struct vol *vol, struct dir *dir, char **cpath)
{
    struct dir         *cdir, *scdir=NULL;
    static char        path[ MAXPATHLEN + 1];
    static struct path ret;

    char        *data, *p;
    int         extend = 0;
    int         len;
    u_int32_t   hint;
    u_int16_t   len16;
    int         size = 0;
    char        sep;
    int         toUTF8 = 0;

    data = *cpath;
    afp_errno = AFPERR_NOOBJ;
    memset(&ret, 0, sizeof(ret));
    switch (ret.m_type = *data) { /* path type */
    case 2:
        data++;
        len = (unsigned char) *data++;
        size = 2;
        sep = 0;
        if (afp_version >= 30) {
            ret.m_type = 3;
            toUTF8 = 1;
        }
        break;
    case 3:
        if (afp_version >= 30) {
            data++;
            memcpy(&hint, data, sizeof(hint));
            hint = ntohl(hint);
            data += sizeof(hint);

            memcpy(&len16, data, sizeof(len16));
            len = ntohs(len16);
            data += 2;
            size = 7;
            sep = 0; /* '/';*/
            break;
        }
        /* else it's an error */
    default:
        afp_errno = AFPERR_PARAM;
        return( NULL );
    }
    *cpath += len + size;
    *path = '\0';
    ret.m_name = path;
    for ( ;; ) {
        if ( len == 0 ) {
            if (movecwd( vol, dir ) < 0 ) {
                return invalidate(vol, dir, &ret );
            }
            if (*path == '\0') {
                ret.u_name = ".";
                ret.d_dir = dir;
            }
            return &ret;
        }

        if (*data == sep ) {
            data++;
            len--;
        }
        while (*data == sep && len > 0 ) {
            if ( dir->d_parent == NULL ) {
                return NULL;
            }
            dir = dir->d_parent;
            data++;
            len--;
        }

        /* would this be faster with strlen + strncpy? */
        p = path;
        while ( *data != sep && len > 0 ) {
            *p++ = *data++;
            if (p > &path[ MAXPATHLEN]) {
                afp_errno = AFPERR_PARAM;
                return( NULL );
            }
            len--;
        }

        /* short cut bits by chopping off a trailing \0. this also
           makes the traversal happy w/ filenames at the end of the
           cname. */
        if (len == 1)
            len--;

        *p = '\0';

        if ( p == path ) { /* end of the name parameter */
            continue;
        }
        ret.u_name = NULL;
        if (afp_version >= 30) {
            char *t;
            cnid_t fileid;

            if (toUTF8) {
                static char temp[ MAXPATHLEN + 1];

                if (dir->d_did == DIRDID_ROOT_PARENT) {
                    /*
                      With uft8 volume name is utf8-mac, but requested path may be a mangled longname. See #2611981.
                      So we compare it with the longname from the current volume and if they match
                      we overwrite the requested path with the utf8 volume name so that the following
                      strcmp can match.
                    */
                    ucs2_to_charset(vol->v_maccharset, vol->v_macname, temp, AFPVOL_MACNAMELEN + 1);
                    if (strcasecmp( path, temp) == 0)
                        ucs2_to_charset(CH_UTF8_MAC, vol->v_u8mname, path, AFPVOL_U8MNAMELEN);
                } else {
                    /* toUTF8 */
                    if (mtoUTF8(vol, path, strlen(path), temp, MAXPATHLEN) == (size_t)-1) {
                        afp_errno = AFPERR_PARAM;
                        return( NULL );
                    }
                    strcpy(path, temp);
                }
            }
            /* check for OS X mangled filename :( */

            t = demangle_osx(vol, path, dir->d_did, &fileid);
            if (t != path) {
                ret.u_name = t;
                /* duplicate work but we can't reuse all convert_char we did in demangle_osx
                 * flags weren't the same
                 */
                if ( (t = utompath(vol, ret.u_name, fileid, utf8_encoding())) ) {
                    /* at last got our view of mac name */
                    strcpy(path,t);
                }
            }
        }
        if (ret.u_name == NULL) {
            if (!(ret.u_name = mtoupath(vol, ret.m_name, dir->d_did, utf8_encoding()))) {
                afp_errno = AFPERR_PARAM;
                return NULL;
            }
        }
        if ( !extend ) {
            ucs2_t *tmpname;
            cdir = dir->d_child;
            scdir = NULL;
            if ( cdir && (vol->v_flags & AFPVOL_CASEINSEN) &&
                 (size_t)-1 != convert_string_allocate(((ret.m_type == 3)?CH_UTF8_MAC:vol->v_maccharset),
                                                       CH_UCS2, path, -1, (char **)&tmpname) )
            {
                while (cdir) {
                    if (!cdir->d_m_name_ucs2) {
                        LOG(log_error, logtype_afpd, "cname: no UCS2 name for %s (did %u)!!!", cdir->d_m_name, ntohl(cdir->d_did) );
                        /* this shouldn't happen !!!! */
                        goto noucsfallback;
                    }

                    if ( strcmp_w( cdir->d_m_name_ucs2, tmpname ) == 0 ) {
                        break;
                    }
                    if ( strcasecmp_w( cdir->d_m_name_ucs2, tmpname ) == 0 ) {
                        scdir = cdir;
                    }
                    cdir = (cdir == dir->d_child->d_prev) ? NULL :cdir->d_next;
                }
                free(tmpname);
            }
            else {
            noucsfallback:
                if (dir->d_did == DIRDID_ROOT_PARENT) {
                    /*
                      root parent (did 1) has one child: the volume. Requests for did=1 with some <name>
                      must check against the volume name.
                    */
                    if (!strcmp(vol->v_dir->d_m_name, ret.m_name))
                        cdir = vol->v_dir;
                    else
                        cdir = NULL;
                }
                else {
                    cdir = dirsearch_byname(vol, dir, ret.u_name);
                }
            }

            if (cdir == NULL && scdir != NULL) {
                cdir = scdir;
                /* LOG(log_debug, logtype_afpd, "cname: using casediff for %s, (%s = %s)", fullpathname(cdir->d_u_name), cdir->d_m_name, path ); */
            }

            if ( cdir == NULL ) {
                ++extend;
                /* if dir == curdir it always succeed,
                   even if curdir is deleted.
                   it's not a pb because it will fail in extenddir
                */
                if ( movecwd( vol, dir ) < 0 ) {
                    /* dir is not valid anymore
                       we delete dir from the cache and abort.
                    */
                    if ( dir->d_did == DIRDID_ROOT_PARENT) {
                        afp_errno = AFPERR_NOOBJ;
                        return NULL;
                    }
                    if (afp_errno == AFPERR_ACCESS)
                        return NULL;
                    dir_invalidate(vol, dir);
                    return NULL;
                }
                cdir = extenddir( vol, dir, &ret );
            }

        } else {
            cdir = extenddir( vol, dir, &ret );
        } /* if (!extend) */

        if ( cdir == NULL ) {

            if ( len > 0 || !ret.u_name ) {
                return NULL;
            }

        } else {
            dir = cdir;
            *path = '\0';
        }
    } /* for (;;) */
}

/*
 * Move curdir to dir, with a possible chdir()
 */
int movecwd(struct vol *vol, struct dir *dir)
{
    char path[MAXPATHLEN + 1];
    struct dir  *d;
    char    *p, *u;
    int     n;
    int     ret;

    if ( dir == curdir ) {
        return( 0 );
    }
    if ( dir->d_did == DIRDID_ROOT_PARENT) {
        afp_errno = AFPERR_DID1; /* AFPERR_PARAM;*/
        return( -1 );
    }

    p = path + sizeof(path) - 1;
    *p = '\0';
    for ( d = dir; d->d_parent != NULL && d != curdir; d = d->d_parent ) {
        u = d->d_u_name;
        if (!u) {
            /* parent directory is deleted */
            afp_errno = AFPERR_NOOBJ;
            return -1;
        }
        n = strlen( u );
        if (p -n -1 < path) {
            afp_errno = AFPERR_PARAM;
            return -1;
        }
        *--p = '/';
        p -= n;
        memcpy( p, u, n );
    }
    if ( d != curdir ) {
        n = strlen( vol->v_path );
        if (p -n -1 < path) {
            afp_errno = AFPERR_PARAM;
            return -1;
        }
        *--p = '/';
        p -= n;
        memcpy( p, vol->v_path, n );
    }
    if ( (ret = lchdir(p )) != 0 ) {
        LOG(log_debug, logtype_afpd, "movecwd('%s'): ret:%d, %u/%s", p, ret, errno, strerror(errno));

        if (ret == 1) {
            /* p is a symlink or getcwd failed */
            afp_errno = AFPERR_BADTYPE;
            vol->v_curdir = curdir = vol->v_dir;
            if (chdir(vol->v_path ) < 0) {
                LOG(log_debug, logtype_afpd, "can't chdir back'%s': %s", vol->v_path, strerror(errno));
                /* XXX what do we do here? */
            }
            return -1;
        }
        switch (errno) {
        case EACCES:
        case EPERM:
            afp_errno = AFPERR_ACCESS;
            break;
        default:
            afp_errno = AFPERR_NOOBJ;

        }
        return( -1 );
    }
    vol->v_curdir = curdir = dir;
    return( 0 );
}

/*
 * We can't use unix file's perm to support Apple's inherited protection modes.
 * If we aren't the file's owner we can't change its perms when moving it and smb
 * nfs,... don't even try.
 */
#define AFP_CHECK_ACCESS

int check_access(char *path, int mode)
{
#ifdef AFP_CHECK_ACCESS
    struct maccess ma;
    char *p;

    p = ad_dir(path);
    if (!p)
        return -1;

    accessmode(p, &ma, curdir, NULL);
    if ((mode & OPENACC_WR) && !(ma.ma_user & AR_UWRITE))
        return -1;
    if ((mode & OPENACC_RD) && !(ma.ma_user & AR_UREAD))
        return -1;
#endif
    return 0;
}

/* --------------------- */
int file_access(struct path *path, int mode)
{
    struct maccess ma;

    accessmode(path->u_name, &ma, curdir, &path->st);
    if ((mode & OPENACC_WR) && !(ma.ma_user & AR_UWRITE))
        return -1;
    if ((mode & OPENACC_RD) && !(ma.ma_user & AR_UREAD))
        return -1;
    return 0;

}

/* --------------------- */
void setdiroffcnt(struct dir *dir, struct stat *st,  u_int32_t count)
{
    dir->offcnt = count;
    dir->ctime = st->st_ctime;
    dir->d_flags &= ~DIRF_CNID;
}

/* ---------------------
 * is our cached offspring count valid?
 */

static int diroffcnt(struct dir *dir, struct stat *st)
{
    return st->st_ctime == dir->ctime;
}

/* ---------------------
 * is our cached also for reenumerate id?
 */

int dirreenumerate(struct dir *dir, struct stat *st)
{
    return st->st_ctime == dir->ctime && (dir->d_flags & DIRF_CNID);
}

/* --------------------- */
static int invisible_dots(const struct vol *vol, const char *name)
{
    return vol_inv_dots(vol) && *name  == '.' && strcmp(name, ".") && strcmp(name, "..");
}

/* ------------------------------
   (".", curdir)
   (name, dir) with curdir:name == dir, from afp_enumerate
*/

int getdirparams(const struct vol *vol,
                 u_int16_t bitmap, struct path *s_path,
                 struct dir *dir,
                 char *buf, size_t *buflen )
{
    struct maccess  ma;
    struct adouble  ad;
    char        *data, *l_nameoff = NULL, *utf_nameoff = NULL;
    int         bit = 0, isad = 0;
    u_int32_t           aint;
    u_int16_t       ashort;
    int                 ret;
    u_int32_t           utf8 = 0;
    cnid_t              pdid;
    struct stat *st = &s_path->st;
    char *upath = s_path->u_name;

    if ((bitmap & ((1 << DIRPBIT_ATTR)  |
                   (1 << DIRPBIT_CDATE) |
                   (1 << DIRPBIT_MDATE) |
                   (1 << DIRPBIT_BDATE) |
                   (1 << DIRPBIT_FINFO)))) {

        ad_init(&ad, vol->v_adouble, vol->v_ad_options);
        if ( !ad_metadata( upath, ADFLAGS_CREATE|ADFLAGS_DIR, &ad) ) {
            isad = 1;
            if (ad.ad_md->adf_flags & O_CREAT) {
                /* We just created it */
                ad_setname(&ad, s_path->m_name);
                ad_setid( &ad,
                          s_path->st.st_dev,
                          s_path->st.st_ino,
                          dir->d_did,
                          dir->d_parent->d_did,
                          vol->v_stamp);
                ad_flush( &ad);
            }
        }
    }

    if ( dir->d_did == DIRDID_ROOT) {
        pdid = DIRDID_ROOT_PARENT;
    } else if (dir->d_did == DIRDID_ROOT_PARENT) {
        pdid = 0;
    } else {
        pdid = dir->d_parent->d_did;
    }

    data = buf;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch ( bit ) {
        case DIRPBIT_ATTR :
            if ( isad ) {
                ad_getattr(&ad, &ashort);
            } else if (invisible_dots(vol, dir->d_u_name)) {
                ashort = htons(ATTRBIT_INVISIBLE);
            } else
                ashort = 0;
            ashort |= htons(ATTRBIT_SHARED);
            memcpy( data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case DIRPBIT_PDID :
            memcpy( data, &pdid, sizeof( pdid ));
            data += sizeof( pdid );
            break;

        case DIRPBIT_CDATE :
            if (!isad || (ad_getdate(&ad, AD_DATE_CREATE, &aint) < 0))
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_MDATE :
            aint = AD_DATE_FROM_UNIX(st->st_mtime);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_BDATE :
            if (!isad || (ad_getdate(&ad, AD_DATE_BACKUP, &aint) < 0))
                aint = AD_DATE_START;
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_FINFO :
            if ( isad ) {
                memcpy( data, ad_entry( &ad, ADEID_FINDERI ), 32 );
            } else { /* no appledouble */
                memset( data, 0, 32 );
                /* set default view -- this also gets done in ad_open() */
                ashort = htons(FINDERINFO_CLOSEDVIEW);
                memcpy(data + FINDERINFO_FRVIEWOFF, &ashort, sizeof(ashort));

                /* dot files are by default visible */
                if (invisible_dots(vol, dir->d_u_name)) {
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
                }
            }
            data += 32;
            break;

        case DIRPBIT_LNAME :
            if (dir->d_m_name) /* root of parent can have a null name */
                l_nameoff = data;
            else
                memset(data, 0, sizeof(u_int16_t));
            data += sizeof( u_int16_t );
            break;

        case DIRPBIT_SNAME :
            memset(data, 0, sizeof(u_int16_t));
            data += sizeof( u_int16_t );
            break;

        case DIRPBIT_DID :
            memcpy( data, &dir->d_did, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_OFFCNT :
            ashort = 0;
            /* this needs to handle current directory access rights */
            if (diroffcnt(dir, st)) {
                ashort = (dir->offcnt > 0xffff)?0xffff:dir->offcnt;
            }
            else if ((ret = for_each_dirent(vol, upath, NULL,NULL)) >= 0) {
                setdiroffcnt(dir, st,  ret);
                ashort = (dir->offcnt > 0xffff)?0xffff:dir->offcnt;
            }
            ashort = htons( ashort );
            memcpy( data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case DIRPBIT_UID :
            aint = htonl(st->st_uid);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_GID :
            aint = htonl(st->st_gid);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case DIRPBIT_ACCESS :
            accessmode( upath, &ma, dir , st);

            *data++ = ma.ma_user;
            *data++ = ma.ma_world;
            *data++ = ma.ma_group;
            *data++ = ma.ma_owner;
            break;

            /* Client has requested the ProDOS information block.
               Just pass back the same basic block for all
               directories. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :
            if (afp_version >= 30) { /* UTF8 name */
                utf8 = kTextEncodingUTF8;
                if (dir->d_m_name) /* root of parent can have a null name */
                    utf_nameoff = data;
                else
                    memset(data, 0, sizeof(u_int16_t));
                data += sizeof( u_int16_t );
                aint = 0;
                memcpy(data, &aint, sizeof( aint ));
                data += sizeof( aint );
            }
            else { /* ProDOS Info Block */
                *data++ = 0x0f;
                *data++ = 0;
                ashort = htons( 0x0200 );
                memcpy( data, &ashort, sizeof( ashort ));
                data += sizeof( ashort );
                memset( data, 0, sizeof( ashort ));
                data += sizeof( ashort );
            }
            break;

        case DIRPBIT_UNIXPR :
            aint = htonl(st->st_uid);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
            aint = htonl(st->st_gid);
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );

            aint = st->st_mode;
            aint = htonl ( aint & ~S_ISGID );  /* Remove SGID, OSX doesn't like it ... */
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );

            accessmode( upath, &ma, dir , st);

            *data++ = ma.ma_user;
            *data++ = ma.ma_world;
            *data++ = ma.ma_group;
            *data++ = ma.ma_owner;
            break;

        default :
            if ( isad ) {
                ad_close_metadata( &ad );
            }
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( l_nameoff ) {
        ashort = htons( data - buf );
        memcpy( l_nameoff, &ashort, sizeof( ashort ));
        data = set_name(vol, data, pdid, dir->d_m_name, dir->d_did, 0);
    }
    if ( utf_nameoff ) {
        ashort = htons( data - buf );
        memcpy( utf_nameoff, &ashort, sizeof( ashort ));
        data = set_name(vol, data, pdid, dir->d_m_name, dir->d_did, utf8);
    }
    if ( isad ) {
        ad_close_metadata( &ad );
    }
    *buflen = data - buf;
    return( AFP_OK );
}

/* ----------------------------- */
int path_error(struct path *path, int error)
{
/* - a dir with access error
 * - no error it's a file
 * - file not found
 */
    if (path_isadir(path))
        return afp_errno;
    if (path->st_valid && path->st_errno)
        return error;
    return AFPERR_BADTYPE ;
}

/* ----------------------------- */
int afp_setdirparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol  *vol;
    struct dir  *dir;
    struct path *path;
    u_int16_t   vid, bitmap;
    u_int32_t   did;
    int     rc;

    *rbuflen = 0;
    ibuf += 2;
    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return afp_errno;
    }

    memcpy( &bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (NULL == ( path = cname( vol, dir, &ibuf )) ) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    if ( *path->m_name != '\0' ) {
        rc = path_error(path, AFPERR_NOOBJ);
        /* maybe we are trying to set perms back */
        if (rc != AFPERR_ACCESS)
            return rc;
    }

    /*
     * If ibuf is odd, make it even.
     */
    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    if (AFP_OK == ( rc = setdirparams(vol, path, bitmap, ibuf )) ) {
        setvoltime(obj, vol );
    }
    return( rc );
}

/*
 * cf AFP3.0.pdf page 244 for change_mdate and change_parent_mdate logic
 *
 * assume path == '\0' eg. it's a directory in canonical form
 */

struct path Cur_Path = {
    0,
    "",  /* mac name */
    ".", /* unix name */
    0,   /* id */
    NULL,/* struct dir */
    0,   /* stat is not set */
};

/* ------------------ */
static int set_dir_errors(struct path *path, const char *where, int err)
{
    switch ( err ) {
    case EPERM :
    case EACCES :
        return AFPERR_ACCESS;
    case EROFS :
        return AFPERR_VLOCK;
    }
    LOG(log_error, logtype_afpd, "setdirparam(%s): %s: %s", fullpathname(path->u_name), where, strerror(err) );
    return AFPERR_PARAM;
}

/* ------------------ */
int setdirparams(struct vol *vol,
                 struct path *path, u_int16_t d_bitmap, char *buf )
{
    struct maccess  ma;
    struct adouble  ad;
    struct utimbuf      ut;
    struct timeval      tv;

    char                *upath;
    struct dir          *dir;
    int         bit, isad = 1;
    int                 cdate, bdate;
    int                 owner, group;
    u_int16_t       ashort, bshort, oshort;
    int                 err = AFP_OK;
    int                 change_mdate = 0;
    int                 change_parent_mdate = 0;
    int                 newdate = 0;
    u_int16_t           bitmap = d_bitmap;
    u_char              finder_buf[32];
    u_int32_t       upriv;
    mode_t              mpriv = 0;
    u_int16_t           upriv_bit = 0;

    bit = 0;
    upath = path->u_name;
    dir   = path->d_dir;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch( bit ) {
        case DIRPBIT_ATTR :
            change_mdate = 1;
            memcpy( &ashort, buf, sizeof( ashort ));
            buf += sizeof( ashort );
            break;
        case DIRPBIT_CDATE :
            change_mdate = 1;
            memcpy(&cdate, buf, sizeof(cdate));
            buf += sizeof( cdate );
            break;
        case DIRPBIT_MDATE :
            memcpy(&newdate, buf, sizeof(newdate));
            buf += sizeof( newdate );
            break;
        case DIRPBIT_BDATE :
            change_mdate = 1;
            memcpy(&bdate, buf, sizeof(bdate));
            buf += sizeof( bdate );
            break;
        case DIRPBIT_FINFO :
            change_mdate = 1;
            memcpy( finder_buf, buf, 32 );
            buf += 32;
            break;
        case DIRPBIT_UID :  /* What kind of loser mounts as root? */
            change_parent_mdate = 1;
            memcpy( &owner, buf, sizeof(owner));
            buf += sizeof( owner );
            break;
        case DIRPBIT_GID :
            change_parent_mdate = 1;
            memcpy( &group, buf, sizeof( group ));
            buf += sizeof( group );
            break;
        case DIRPBIT_ACCESS :
            change_mdate = 1;
            change_parent_mdate = 1;
            ma.ma_user = *buf++;
            ma.ma_world = *buf++;
            ma.ma_group = *buf++;
            ma.ma_owner = *buf++;
            mpriv = mtoumode( &ma ) | vol->v_dperm;
            if (dir_rx_set(mpriv) && setdirmode( vol, upath, mpriv) < 0 ) {
                err = set_dir_errors(path, "setdirmode", errno);
                bitmap = 0;
            }
            break;
            /* Ignore what the client thinks we should do to the
               ProDOS information block.  Skip over the data and
               report nothing amiss. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :
            if (afp_version < 30) {
                buf += 6;
            }
            else {
                err = AFPERR_BITMAP;
                bitmap = 0;
            }
            break;
        case DIRPBIT_UNIXPR :
            if (vol_unix_priv(vol)) {
                memcpy( &owner, buf, sizeof(owner)); /* FIXME need to change owner too? */
                buf += sizeof( owner );
                memcpy( &group, buf, sizeof( group ));
                buf += sizeof( group );

                change_mdate = 1;
                change_parent_mdate = 1;
                memcpy( &upriv, buf, sizeof( upriv ));
                buf += sizeof( upriv );
                upriv = ntohl (upriv) | vol->v_dperm;
                if (dir_rx_set(upriv)) {
                    /* maybe we are trying to set perms back */
                    if ( setdirunixmode(vol, upath, upriv) < 0 ) {
                        bitmap = 0;
                        err = set_dir_errors(path, "setdirunixmode", errno);
                    }
                }
                else {
                    /* do it later */
                    upriv_bit = 1;
                }
                break;
            }
            /* fall through */
        default :
            err = AFPERR_BITMAP;
            bitmap = 0;
            break;
        }

        bitmap = bitmap>>1;
        bit++;
    }
    ad_init(&ad, vol->v_adouble, vol->v_ad_options);

    if (ad_open_metadata( upath, ADFLAGS_DIR, O_CREAT, &ad) < 0) {
        /*
         * Check to see what we're trying to set.  If it's anything
         * but ACCESS, UID, or GID, give an error.  If it's any of those
         * three, we don't need the ad to be open, so just continue.
         *
         * note: we also don't need to worry about mdate. also, be quiet
         *       if we're using the noadouble option.
         */
        if (!vol_noadouble(vol) && (d_bitmap &
                                    ~((1<<DIRPBIT_ACCESS)|(1<<DIRPBIT_UNIXPR)|
                                      (1<<DIRPBIT_UID)|(1<<DIRPBIT_GID)|
                                      (1<<DIRPBIT_MDATE)|(1<<DIRPBIT_PDINFO)))) {
            return AFPERR_ACCESS;
        }

        isad = 0;
    } else {
        /*
         * Check to see if a create was necessary. If it was, we'll want
         * to set our name, etc.
         */
        if ( (ad_get_HF_flags( &ad ) & O_CREAT)) {
            ad_setname(&ad, curdir->d_m_name);
        }
    }

    bit = 0;
    bitmap = d_bitmap;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch( bit ) {
        case DIRPBIT_ATTR :
            if (isad) {
                ad_getattr(&ad, &bshort);
                oshort = bshort;
                if ( ntohs( ashort ) & ATTRBIT_SETCLR ) {
                    bshort |= htons( ntohs( ashort ) & ~ATTRBIT_SETCLR );
                } else {
                    bshort &= ~ashort;
                }
                if ((bshort & htons(ATTRBIT_INVISIBLE)) != (oshort & htons(ATTRBIT_INVISIBLE)))
                    change_parent_mdate = 1;
                ad_setattr(&ad, bshort);
            }
            break;
        case DIRPBIT_CDATE :
            if (isad) {
                ad_setdate(&ad, AD_DATE_CREATE, cdate);
            }
            break;
        case DIRPBIT_MDATE :
            break;
        case DIRPBIT_BDATE :
            if (isad) {
                ad_setdate(&ad, AD_DATE_BACKUP, bdate);
            }
            break;
        case DIRPBIT_FINFO :
            if (isad) {
                /* Fixes #2802236 */
                u_int16_t *fflags = (u_int16_t *)(finder_buf + FINDERINFO_FRFLAGOFF);
                *fflags &= htons(~FINDERINFO_ISHARED);
                /* #2802236 end */
                if (  dir->d_did == DIRDID_ROOT ) {
                    /*
                     * Alright, we admit it, this is *really* sick!
                     * The 4 bytes that we don't copy, when we're dealing
                     * with the root of a volume, are the directory's
                     * location information. This eliminates that annoying
                     * behavior one sees when mounting above another mount
                     * point.
                     */
                    memcpy( ad_entry( &ad, ADEID_FINDERI ), finder_buf, 10 );
                    memcpy( ad_entry( &ad, ADEID_FINDERI ) + 14, finder_buf + 14, 18 );
                } else {
                    memcpy( ad_entry( &ad, ADEID_FINDERI ), finder_buf, 32 );
                }
            }
            break;
        case DIRPBIT_UID :  /* What kind of loser mounts as root? */
            if ( (dir->d_did == DIRDID_ROOT) &&
                 (setdeskowner( ntohl(owner), -1 ) < 0)) {
                err = set_dir_errors(path, "setdeskowner", errno);
                if (isad && err == AFPERR_PARAM) {
                    err = AFP_OK; /* ???*/
                }
                else {
                    goto setdirparam_done;
                }
            }
            if ( setdirowner(vol, upath, ntohl(owner), -1 ) < 0 ) {
                err = set_dir_errors(path, "setdirowner", errno);
                goto setdirparam_done;
            }
            break;
        case DIRPBIT_GID :
            if (dir->d_did == DIRDID_ROOT)
                setdeskowner( -1, ntohl(group) );
            if ( setdirowner(vol, upath, -1, ntohl(group) ) < 0 ) {
                err = set_dir_errors(path, "setdirowner", errno);
                goto setdirparam_done;
            }
            break;
        case DIRPBIT_ACCESS :
            if (dir->d_did == DIRDID_ROOT) {
                setdeskmode(mpriv);
                if (!dir_rx_set(mpriv)) {
                    /* we can't remove read and search for owner on volume root */
                    err = AFPERR_ACCESS;
                    goto setdirparam_done;
                }
            }

            if (!dir_rx_set(mpriv) && setdirmode( vol, upath, mpriv) < 0 ) {
                err = set_dir_errors(path, "setdirmode", errno);
                goto setdirparam_done;
            }
            break;
        case DIRPBIT_PDINFO :
            if (afp_version >= 30) {
                err = AFPERR_BITMAP;
                goto setdirparam_done;
            }
            break;
        case DIRPBIT_UNIXPR :
            if (vol_unix_priv(vol)) {
                if (dir->d_did == DIRDID_ROOT) {
                    if (!dir_rx_set(upriv)) {
                        /* we can't remove read and search for owner on volume root */
                        err = AFPERR_ACCESS;
                        goto setdirparam_done;
                    }
                    setdeskowner( -1, ntohl(group) );
                    setdeskmode( upriv );
                }
                if ( setdirowner(vol, upath, -1, ntohl(group) ) < 0 ) {
                    err = set_dir_errors(path, "setdirowner", errno);
                    goto setdirparam_done;
                }

                if ( upriv_bit && setdirunixmode(vol, upath, upriv) < 0 ) {
                    err = set_dir_errors(path, "setdirunixmode", errno);
                    goto setdirparam_done;
                }
            }
            else {
                err = AFPERR_BITMAP;
                goto setdirparam_done;
            }
            break;
        default :
            err = AFPERR_BITMAP;
            goto setdirparam_done;
            break;
        }

        bitmap = bitmap>>1;
        bit++;
    }

setdirparam_done:
    if (change_mdate && newdate == 0 && gettimeofday(&tv, NULL) == 0) {
        newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
    }
    if (newdate) {
        if (isad)
            ad_setdate(&ad, AD_DATE_MODIFY, newdate);
        ut.actime = ut.modtime = AD_DATE_TO_UNIX(newdate);
        utime(upath, &ut);
    }

    if ( isad ) {
        if (path->st_valid && !path->st_errno) {
            struct stat *st = &path->st;

            if (dir && dir->d_parent) {
                ad_setid(&ad, st->st_dev, st->st_ino,  dir->d_did, dir->d_parent->d_did, vol->v_stamp);
            }
        }
        ad_flush( &ad);
        ad_close_metadata( &ad);
    }

    if (change_parent_mdate && dir->d_did != DIRDID_ROOT
        && gettimeofday(&tv, NULL) == 0) {
        if (!movecwd(vol, dir->d_parent)) {
            newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
            /* be careful with bitmap because now dir is null */
            bitmap = 1<<DIRPBIT_MDATE;
            setdirparams(vol, &Cur_Path, bitmap, (char *)&newdate);
            /* should we reset curdir ?*/
        }
    }

    return err;
}

int afp_syncdir(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
#ifdef HAVE_DIRFD
    DIR                  *dp;
#endif
    int                  dfd;
    struct vol           *vol;
    struct dir           *dir;
    u_int32_t            did;
    u_int16_t            vid;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == (vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );

    /*
     * Here's the deal:
     * if it's CNID 2 our only choice to meet the specs is call sync.
     * For any other CNID just sync that dir. To my knowledge the
     * intended use of FPSyncDir is to sync the volume so all we're
     * ever going to see here is probably CNID 2. Anyway, we' prepared.
     */

    if ( ntohl(did) == 2 ) {
        sync();
    } else {
        if (NULL == ( dir = dirlookup( vol, did )) ) {
            return afp_errno; /* was AFPERR_NOOBJ */
        }

        if (movecwd( vol, dir ) < 0 )
            return ( AFPERR_NOOBJ );

        /*
         * Assuming only OSens that have dirfd also may require fsyncing directories
         * in order to flush metadata e.g. Linux.
         */

#ifdef HAVE_DIRFD
        if (NULL == ( dp = opendir( "." )) ) {
            switch( errno ) {
            case ENOENT :
                return( AFPERR_NOOBJ );
            case EACCES :
                return( AFPERR_ACCESS );
            default :
                return( AFPERR_PARAM );
            }
        }

        LOG(log_debug, logtype_afpd, "afp_syncdir: dir: '%s'", dir->d_u_name);

        dfd = dirfd( dp );
        if ( fsync ( dfd ) < 0 )
            LOG(log_error, logtype_afpd, "afp_syncdir(%s):  %s",
                dir->d_u_name, strerror(errno) );
        closedir(dp); /* closes dfd too */
#endif

        if ( -1 == (dfd = open(vol->ad_path(".", ADFLAGS_DIR), O_RDWR))) {
            switch( errno ) {
            case ENOENT:
                return( AFPERR_NOOBJ );
            case EACCES:
                return( AFPERR_ACCESS );
            default:
                return( AFPERR_PARAM );
            }
        }

        LOG(log_debug, logtype_afpd, "afp_syncdir: ad-file: '%s'",
            vol->ad_path(".", ADFLAGS_DIR) );

        if ( fsync(dfd) < 0 )
            LOG(log_error, logtype_afpd, "afp_syncdir(%s): %s",
                vol->ad_path(dir->d_u_name, ADFLAGS_DIR), strerror(errno) );
        close(dfd);
    }

    return ( AFP_OK );
}

int afp_createdir(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct adouble  ad;
    struct vol      *vol;
    struct dir      *dir;
    char        *upath;
    struct path         *s_path;
    u_int32_t       did;
    u_int16_t       vid;
    int                 err;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return afp_errno; /* was AFPERR_NOOBJ */
    }
    /* for concurrent access we need to be sure we are not in the
     * folder we want to create...
     */
    movecwd(vol, dir);

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return get_afp_errno(AFPERR_PARAM);
    }
    /* cname was able to move curdir to it! */
    if (*s_path->m_name == '\0')
        return AFPERR_EXIST;

    upath = s_path->u_name;

    if (AFP_OK != (err = netatalk_mkdir( upath))) {
        return err;
    }

    if (of_stat(s_path) < 0) {
        return AFPERR_MISC;
    }
    curdir->offcnt++;
    if ((dir = adddir( vol, curdir, s_path)) == NULL) {
        return AFPERR_MISC;
    }

    if ( movecwd( vol, dir ) < 0 ) {
        return( AFPERR_PARAM );
    }

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    if (ad_open_metadata( ".", ADFLAGS_DIR, O_CREAT, &ad ) < 0)  {
        if (vol_noadouble(vol))
            goto createdir_done;
        return( AFPERR_ACCESS );
    }
    ad_setname(&ad, s_path->m_name);
    ad_setid( &ad, s_path->st.st_dev, s_path->st.st_ino, dir->d_did, did, vol->v_stamp);

    ad_flush( &ad);
    ad_close_metadata( &ad);

createdir_done:
#ifdef HAVE_NFSv4_ACLS
    /* FIXME: are we really inside the created dir? */
    addir_inherit_acl(vol);
#endif

    memcpy( rbuf, &dir->d_did, sizeof( u_int32_t ));
    *rbuflen = sizeof( u_int32_t );
    setvoltime(obj, vol );
    return( AFP_OK );
}

/*
 * dst       new unix filename (not a pathname)
 * newname   new mac name
 * newparent curdir
 *
 */
int renamedir(const struct vol *vol, char *src, char *dst,
              struct dir *dir,
              struct dir *newparent,
              char *newname)
{
    struct adouble  ad;
    struct dir      *parent;
    char                *buf;
    int         len, err;

    /* existence check moved to afp_moveandrename */
    if ( unix_rename( src, dst ) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        case EINVAL:
            /* tried to move directory into a subdirectory of itself */
            return AFPERR_CANTMOVE;
        case EXDEV:
            /* this needs to copy and delete. bleah. that means we have
             * to deal with entire directory hierarchies. */
            if ((err = copydir(vol, src, dst)) < 0) {
                deletedir(dst);
                return err;
            }
            if ((err = deletedir(src)) < 0)
                return err;
            break;
        default :
            return( AFPERR_PARAM );
        }
    }

    vol->vfs->vfs_renamedir(vol, src, dst);

    len = strlen( newname );
    /* rename() succeeded so we need to update our tree even if we can't open
     * metadata
     */

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);

    if (!ad_open_metadata( dst, ADFLAGS_DIR, 0, &ad)) {
        ad_setname(&ad, newname);
        ad_flush( &ad);
        ad_close_metadata( &ad);
    }

    dir_hash_del(vol, dir);
    if (dir->d_m_name == dir->d_u_name)
        dir->d_u_name = NULL;

    if ((buf = (char *) realloc( dir->d_m_name, len + 1 )) == NULL ) {
        LOG(log_error, logtype_afpd, "renamedir: realloc mac name: %s", strerror(errno) );
        /* FIXME : fatal ? */
        return AFPERR_MISC;
    }
    dir->d_m_name = buf;
    strcpy( dir->d_m_name, newname );

    if (newname == dst) {
        free(dir->d_u_name);
        dir->d_u_name = dir->d_m_name;
    }
    else {
        if ((buf = (char *) realloc( dir->d_u_name, strlen(dst) + 1 )) == NULL ) {
            LOG(log_error, logtype_afpd, "renamedir: realloc unix name: %s", strerror(errno) );
            return AFPERR_MISC;
        }
        dir->d_u_name = buf;
        strcpy( dir->d_u_name, dst );
    }

    if (dir->d_m_name_ucs2)
        free(dir->d_m_name_ucs2);

    dir->d_m_name_ucs2 = NULL;
    if ((size_t)-1 == convert_string_allocate((utf8_encoding())?CH_UTF8_MAC:vol->v_maccharset, CH_UCS2, dir->d_m_name, -1, (char**)&dir->d_m_name_ucs2))
        dir->d_m_name_ucs2 = NULL;

    if (( parent = dir->d_parent ) == NULL ) {
        return( AFP_OK );
    }
    if ( parent == newparent ) {
        hash_alloc_insert(vol->v_hash, dir, dir);
        return( AFP_OK );
    }

    /* detach from old parent and add to new one. */
    dirchildremove(parent, dir);
    dir->d_parent = newparent;
    dirchildadd(vol, newparent, dir);
    return( AFP_OK );
}

/* delete an empty directory */
int deletecurdir(struct vol *vol)
{
    struct dirent *de;
    struct stat st;
    struct dir  *fdir;
    DIR *dp;
    struct adouble  ad;
    u_int16_t       ashort;
    int err;

    if ( curdir->d_parent == NULL ) {
        return( AFPERR_ACCESS );
    }

    fdir = curdir;

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    /* we never want to create a resource fork here, we are going to delete it */
    if ( ad_metadata( ".", ADFLAGS_DIR, &ad) == 0 ) {

        ad_getattr(&ad, &ashort);
        ad_close_metadata(&ad);
        if ((ashort & htons(ATTRBIT_NODELETE))) {
            return  AFPERR_OLOCK;
        }
    }
    err = vol->vfs->vfs_deletecurdir(vol);
    if (err) {
        return err;
    }

    /* now get rid of dangling symlinks */
    if ((dp = opendir("."))) {
        while ((de = readdir(dp))) {
            /* skip this and previous directory */
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            /* bail if it's not a symlink */
            if ((lstat(de->d_name, &st) == 0) && !S_ISLNK(st.st_mode)) {
                closedir(dp);
                return AFPERR_DIRNEMPT;
            }

            if ((err = netatalk_unlink(de->d_name))) {
                closedir(dp);
                return err;
            }
        }
    }

    if ( movecwd( vol, curdir->d_parent ) < 0 ) {
        err = afp_errno;
        goto delete_done;
    }

    err = netatalk_rmdir_all_errors(fdir->d_u_name);
    if ( err ==  AFP_OK || err == AFPERR_NOOBJ) {
        dirchildremove(curdir, fdir);
        cnid_delete(vol->v_cdb, fdir->d_did);
        dir_remove( vol, fdir );
    }
delete_done:
    if (dp) {
        /* inode is used as key for cnid.
         * Close the descriptor only after cnid_delete
         * has been called.
         */
        closedir(dp);
    }
    return err;
}

int afp_mapid(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct passwd   *pw;
    struct group    *gr;
    char        *name;
    u_int32_t           id;
    int         len, sfunc;
    int         utf8 = 0;

    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    *rbuflen = 0;


    if (sfunc >= 3 && sfunc <= 6) {
        if (afp_version < 30) {
            return( AFPERR_PARAM );
        }
        utf8 = 1;
    }

    switch ( sfunc ) {
    case 1 :
    case 3 :/* unicode */
        memcpy( &id, ibuf, sizeof( id ));
        id = ntohl(id);
        if ( id != 0 ) {
            if (( pw = getpwuid( id )) == NULL ) {
                return( AFPERR_NOITEM );
            }
            len = convert_string_allocate( obj->options.unixcharset, ((!utf8)?obj->options.maccharset:CH_UTF8_MAC),
                                           pw->pw_name, -1, &name);
        } else {
            len = 0;
            name = NULL;
        }
        break;
    case 2 :
    case 4 : /* unicode */
        memcpy( &id, ibuf, sizeof( id ));
        id = ntohl(id);
        if ( id != 0 ) {
            if (NULL == ( gr = (struct group *)getgrgid( id ))) {
                return( AFPERR_NOITEM );
            }
            len = convert_string_allocate( obj->options.unixcharset, (!utf8)?obj->options.maccharset:CH_UTF8_MAC,
                                           gr->gr_name, -1, &name);
        } else {
            len = 0;
            name = NULL;
        }
        break;
#ifdef HAVE_NFSv4_ACLS
    case 5 : /* UUID -> username */
    case 6 : /* UUID -> groupname */
        if ((afp_version < 32) || !(obj->options.flags & OPTION_UUID ))
            return AFPERR_PARAM;
        LOG(log_debug, logtype_afpd, "afp_mapid: valid UUID request");
        uuidtype_t type;
        len = getnamefromuuid( ibuf, &name, &type);
        if (len != 0)       /* its a error code, not len */
            return AFPERR_NOITEM;
        if (type == UUID_USER) {
            if (( pw = getpwnam( name )) == NULL )
                return( AFPERR_NOITEM );
            LOG(log_debug, logtype_afpd, "afp_mapid: name:%s -> uid:%d", name, pw->pw_uid);
            id = htonl(UUID_USER);
            memcpy( rbuf, &id, sizeof( id ));
            id = htonl( pw->pw_uid);
            rbuf += sizeof( id );
            memcpy( rbuf, &id, sizeof( id ));
            rbuf += sizeof( id );
            *rbuflen = 2 * sizeof( id );
        } else {        /* type == UUID_GROUP */
            if (( gr = getgrnam( name )) == NULL )
                return( AFPERR_NOITEM );
            LOG(log_debug, logtype_afpd, "afp_mapid: group:%s -> gid:%d", name, gr->gr_gid);
            id = htonl(UUID_GROUP);
            memcpy( rbuf, &id, sizeof( id ));
            rbuf += sizeof( id );
            id = htonl( gr->gr_gid);
            memcpy( rbuf, &id, sizeof( id ));
            rbuf += sizeof( id );
            *rbuflen = 2 * sizeof( id );
        }
        break;
#endif
    default :
        return( AFPERR_PARAM );
    }

    if (name)
        len = strlen( name );

    if (utf8) {
        u_int16_t tp = htons(len);
        memcpy(rbuf, &tp, sizeof(tp));
        rbuf += sizeof(tp);
        *rbuflen += 2;
    }
    else {
        *rbuf++ = len;
        *rbuflen += 1;
    }
    if ( len > 0 ) {
        memcpy( rbuf, name, len );
    }
    *rbuflen += len;
    if (name)
        free(name);
    return( AFP_OK );
}

int afp_mapname(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct passwd   *pw;
    struct group    *gr;
    int             len, sfunc;
    u_int32_t       id;
    u_int16_t       ulen;

    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    *rbuflen = 0;
    LOG(log_debug, logtype_afpd, "afp_mapname: sfunc: %d, afp_version: %d", sfunc, afp_version);
    switch ( sfunc ) {
    case 1 :
    case 2 : /* unicode */
        if (afp_version < 30) {
            return( AFPERR_PARAM );
        }
        memcpy(&ulen, ibuf, sizeof(ulen));
        len = ntohs(ulen);
        ibuf += 2;
        LOG(log_debug, logtype_afpd, "afp_mapname: alive");
        break;
    case 3 :
    case 4 :
        len = (unsigned char) *ibuf++;
        break;
#ifdef HAVE_NFSv4_ACLS
    case 5 : /* username -> UUID  */
    case 6 : /* groupname -> UUID */
        if ((afp_version < 32) || !(obj->options.flags & OPTION_UUID ))
            return AFPERR_PARAM;
        memcpy(&ulen, ibuf, sizeof(ulen));
        len = ntohs(ulen);
        ibuf += 2;
        break;
#endif
    default :
        return( AFPERR_PARAM );
    }

    ibuf[ len ] = '\0';

    if ( len == 0 )
        return AFPERR_PARAM;
    else {
        switch ( sfunc ) {
        case 1 : /* unicode */
        case 3 :
            if (NULL == ( pw = (struct passwd *)getpwnam( ibuf )) ) {
                return( AFPERR_NOITEM );
            }
            id = pw->pw_uid;
            id = htonl(id);
            memcpy( rbuf, &id, sizeof( id ));
            *rbuflen = sizeof( id );
            break;

        case 2 : /* unicode */
        case 4 :
            LOG(log_debug, logtype_afpd, "afp_mapname: gettgrnam for name: %s",ibuf);
            if (NULL == ( gr = (struct group *)getgrnam( ibuf ))) {
                return( AFPERR_NOITEM );
            }
            id = gr->gr_gid;
            LOG(log_debug, logtype_afpd, "afp_mapname: gettgrnam for name: %s -> id: %d",ibuf, id);
            id = htonl(id);
            memcpy( rbuf, &id, sizeof( id ));
            *rbuflen = sizeof( id );
            break;
#ifdef HAVE_NFSv4_ACLS
        case 5 :        /* username -> UUID */
            LOG(log_debug, logtype_afpd, "afp_mapname: name: %s",ibuf);
            if (0 != getuuidfromname(ibuf, UUID_USER, rbuf))
                return AFPERR_NOITEM;
            *rbuflen = UUID_BINSIZE;
            break;
        case 6 :        /* groupname -> UUID */
            LOG(log_debug, logtype_afpd, "afp_mapname: name: %s",ibuf);
            if (0 != getuuidfromname(ibuf, UUID_GROUP, rbuf))
                return AFPERR_NOITEM;
            *rbuflen = UUID_BINSIZE;
            break;
#endif
        }
    }
    return( AFP_OK );
}

/* ------------------------------------
   variable DID support
*/
int afp_closedir(AFPObj *obj _U_, char *ibuf _U_, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
#if 0
    struct vol   *vol;
    struct dir   *dir;
    u_int16_t    vid;
    u_int32_t    did;
#endif /* 0 */

    *rbuflen = 0;

    /* do nothing as dids are static for the life of the process. */
#if 0
    ibuf += 2;

    memcpy(&vid,  ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (( dir = dirlookup( vol, did )) == NULL ) {
        return( AFPERR_PARAM );
    }

    /* dir_remove -- deletedid */
#endif /* 0 */

    return AFP_OK;
}

/* did creation gets done automatically
 * there's a pb again with case but move it to cname
 */
int afp_opendir(AFPObj *obj _U_, char *ibuf, size_t ibuflen  _U_, char *rbuf, size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *parentdir;
    struct path     *path;
    u_int32_t       did;
    u_int16_t       vid;

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof( vid );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == ( parentdir = dirlookup( vol, did )) ) {
        return afp_errno;
    }

    if (NULL == ( path = cname( vol, parentdir, &ibuf )) ) {
        return get_afp_errno(AFPERR_PARAM);
    }

    if ( *path->m_name != '\0' ) {
        return path_error(path, AFPERR_NOOBJ);
    }

    if ( !path->st_valid && of_stat(path ) < 0 ) {
        return( AFPERR_NOOBJ );
    }
    if ( path->st_errno ) {
        return( AFPERR_NOOBJ );
    }

    memcpy(rbuf, &curdir->d_did, sizeof(curdir->d_did));
    *rbuflen = sizeof(curdir->d_did);
    return AFP_OK;
}
