/*
 * $Id: directory.c,v 1.66 2003-04-14 18:03:48 didg Exp $
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

#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB */
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <grp.h>
#include <pwd.h>

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

#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"
#include "file.h"
#include "filedir.h"
#include "globals.h"
#include "unix.h"

struct dir	*curdir;
int             afp_errno;

#define SENTINEL (&sentinel)
static struct dir sentinel = { SENTINEL, SENTINEL, NULL, DIRTREE_COLOR_BLACK,
                                 NULL, NULL, NULL, NULL, NULL, 0, 0, 
                                 0, 0, NULL, NULL};
static struct dir rootpar  = { SENTINEL, SENTINEL, NULL, 0,
                                 NULL, NULL, NULL, NULL, NULL, 0, 0, 
                                 0, 0, NULL, NULL};

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

/*
 * redid did assignment for directories. now we use red-black trees.
 * how exciting.
 */
struct dir *
            dirsearch( vol, did )
            const struct vol	*vol;
u_int32_t	did;
{
    struct dir	*dir;


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

    dir = vol->v_root;
    afp_errno = AFPERR_NOOBJ;
    while ( dir != SENTINEL ) {
        if (dir->d_did == did)
            return dir->d_m_name ? dir : NULL;
        dir = (dir->d_did > did) ? dir->d_left : dir->d_right;
    }
    return NULL;
}

/* ------------------- */
#ifdef ATACC
int path_isadir(struct path *o_path)
{
    return o_path->dir != NULL;
#if 0
    return o_path->m_name == '\0' || /* we are in a it */
           !o_path->st_valid ||      /* in cache but we can't chdir in it */ 
           (!o_path->st_errno && S_ISDIR(o_path->st.st_mode)); /* not in cache an can't chdir */
#endif
}
#endif

int get_afp_errno(const int param)
{
    if (afp_errno != AFPERR_DID1)
        return afp_errno;
    return param;
}

/* ------------------- */
struct dir *
            dirsearch_byname( cdir, name )
            struct dir *cdir;
            const char	*name;
{
struct dir *dir;

    if (!strcmp(name, "."))
        return cdir;
    dir = cdir->d_child;
    while (dir) {
        if ( strcmp( dir->d_u_name, name ) == 0 ) {
            break;
        }
        dir = (dir == curdir->d_child->d_prev) ? NULL : dir->d_next;
    }
    return dir;
}            

/* -----------------------------------------
 * if did is not in the cache resolve it with cnid 
 * 
 */
struct dir *
            dirlookup( vol, did )
            const struct vol	*vol;
u_int32_t	did;
{
#ifdef CNID_DB
    struct dir   *ret;
    char	 *upath;
    u_int32_t	 id;
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
    if (NULL == (upath = cnid_resolve(vol->v_db, &id, buffer, buflen)) ) {
        afp_errno = AFPERR_NOOBJ;
        return NULL;
    }
    ptr = path + MAXPATHLEN;
    if (NULL == ( mpath = utompath(vol, upath, utf8) ) ) {
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
        if ( NULL == (upath = cnid_resolve(vol->v_db, &id, buffer, buflen))
             ||
             NULL == (mpath = utompath(vol, upath, utf8))
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
#endif
    return dirsearch(vol, did);
}

/* --------------------------- */
/* rotate the tree to the left */
static void dir_leftrotate(vol, dir)
struct vol *vol;
struct dir *dir;
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
static void dir_rightrotate(vol, dir)
struct vol *vol;
struct dir *dir;
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
static struct dir *dir_rmrecolor(vol, dir)
            struct vol *vol;
struct dir *dir;
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


/* remove the node from the tree. this is just like insertion, but
 * different. actually, it has to worry about a bunch of things that
 * insertion doesn't care about. */
static void dir_remove( vol, dir )
struct vol	*vol;
struct dir	*dir;
{
#ifdef REMOVE_NODES
    struct ofork *of, *last;
    struct dir *node, *leaf;
#endif /* REMOVE_NODES */

    if (!dir || (dir == SENTINEL))
        return;

    /* i'm not sure if it really helps to delete stuff. */
#ifndef REMOVE_NODES 
    if (dir->d_u_name != dir->d_m_name) {
        free(dir->d_u_name);
    }
    free(dir->d_m_name);
    dir->d_m_name = NULL;
    dir->d_u_name = NULL;
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
    }

    if (node->d_color == DIRTREE_COLOR_BLACK)
        dir_rmrecolor(vol, leaf);

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
static void dir_invalidate( vol, dir )
const struct vol *vol;
struct dir *dir;
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
static struct dir *dir_insert(vol, dir)
            const struct vol *vol;
struct dir *dir;
{
    struct dir	*pdir;

    pdir = vol->v_root;
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


/*
 * attempt to extend the current dir. tree to include path
 * as a side-effect, movecwd to that point and return the new dir
 */
static struct dir *
            extenddir( vol, dir, path )
struct vol	*vol;
struct dir	*dir;
struct path *path;
{
    path->u_name = mtoupath(vol, path->m_name, utf8_encoding() );
    path->dir = NULL;

    if ( path->u_name == NULL) {
        afp_errno = AFPERR_PARAM;
        return NULL;
    }
    if (of_stat( path ) != 0 ) {
        return( NULL );
    }

    if (!S_ISDIR(path->st.st_mode)) {
        return( NULL );
    }

    if (( dir = adddir( vol, dir, path)) == NULL ) {
        return( NULL );
    }

    path->dir = dir;
    if ( movecwd( vol, dir ) < 0 ) {
        return( NULL );
    }

    return( dir );
}

/* -------------------
   system rmdir with afp error code.
   ENOENT is not an error.
 */
static int netatalk_rmdir(const char *name)
{
    if (rmdir(name) < 0) {
        switch ( errno ) {
        case ENOENT :
            break;
        case ENOTEMPTY : 
            return AFPERR_DIRNEMPT;
        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;
        case EROFS:
            return AFPERR_VLOCK;
        default :
            return AFPERR_PARAM;
        }
    }
    return AFP_OK;
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

/* -------------------
   system unlink with afp error code.
   ENOENT is not an error.
 */
int netatalk_unlink(const char *name)
{
    if (unlink(name) < 0) {
        switch (errno) {
        case ENOENT :
            break;
        case EROFS:
            return AFPERR_VLOCK;
        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;
        default :
            return AFPERR_PARAM;
        }
    }
    return AFP_OK;
}

/* ------------------- */
static int deletedir(char *dir)
{
    char path[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent	*de;
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
        if (stat(path, &st)) {
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
static int copydir(char *src, char *dst, int noadouble)
{
    char spath[MAXPATHLEN + 1], dpath[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent	*de;
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

        if (stat(spath, &st) == 0) {
            if (strlen(de->d_name) > drem) {
                err = AFPERR_PARAM;
                break;
            }
            strcpy(dpath + dlen, de->d_name);

            if (S_ISDIR(st.st_mode)) {
                if (AFP_OK != (err = copydir(spath, dpath, noadouble)))
                    goto copydir_done;
            } else if (AFP_OK != (err = copyfile(spath, dpath, NULL, noadouble))) {
                goto copydir_done;

            } else {
                /* keep the same time stamp. */
                ut.actime = ut.modtime = st.st_mtime;
                utime(dpath, &ut);
            }
        }
    }

    /* keep the same time stamp. */
    if (stat(src, &st) == 0) {
        ut.actime = ut.modtime = st.st_mtime;
        utime(dst, &ut);
    }

copydir_done:
    closedir(dp);
    return err;
}


/* --- public functions follow --- */

/* NOTE: we start off with at least one node (the root directory). */
struct dir *dirinsert( vol, dir )
            struct vol	*vol;
struct dir	*dir;
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

/* free everything down. we don't bother to recolor as this is only
 * called to free the entire tree */
void dirfreename(struct dir *dir)
{
    if (dir->d_u_name != dir->d_m_name) {
        free(dir->d_u_name);
    }
    free(dir->d_m_name);
}

void dirfree( dir )
struct dir	*dir;
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

    if (m_name == u_name) {
        dir->d_u_name = dir->d_m_name;
    }
    else if ((dir->d_u_name = strdup(u_name)) == NULL) {
        free(dir->d_m_name);
        free(dir);
        return NULL;
    }

    dir->d_left = dir->d_right = SENTINEL;
    dir->d_next = dir->d_prev = dir;
    return dir;
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
cname( vol, dir, cpath )
const struct vol	*vol;
struct dir	*dir;
char	**cpath;
{
    struct dir		   *cdir;
    static char		   path[ MAXPATHLEN + 1];
    static struct path ret;

    char		*data, *p;
    int			extend = 0;
    int			len;
    u_int32_t   hint;
    u_int16_t   len16;
    int         size = 0;
    char        sep;
       	
    data = *cpath;
    afp_errno = AFPERR_NOOBJ;
    switch (ret.m_type = *data) { /* path type */
    case 2:
       data++;
       len = (unsigned char) *data++;
       size = 2;
       sep = 0;
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
    ret.st_errno = 0;
    ret.st_valid = 0;
    ret.dir = NULL;
    for ( ;; ) {
        if ( len == 0 ) {
            if (movecwd( vol, dir ) < 0 ) {
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
            	    ret.m_name = dir->d_m_name;
            	    ret.u_name = dir->d_u_name;
            	    ret.dir = dir;
            	    return &ret;
            	} else if (afp_errno == AFPERR_NOOBJ) {
                    if ( movecwd( vol, dir->d_parent ) < 0 ) {
            	         return NULL;    		
            	    }
            	    strcpy(path, dir->d_m_name);
            	    if (dir->d_m_name == dir->d_u_name) {
            	        ret.u_name = path;
            	    }
            	    else {
            	        size_t tp = strlen(path)+1;

            	        strcpy(path +tp, dir->d_u_name);
            	        ret.u_name =  path +tp;
            	        
            	    }
            	    dir_invalidate(vol, dir);
            	    return &ret;
            	}
            	dir_invalidate(vol, dir);
            	return NULL;
            }
            if (*path == '\0') {
               ret.u_name = ".";
               ret.dir = dir;
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
            len--;
        }

        /* short cut bits by chopping off a trailing \0. this also
                  makes the traversal happy w/ filenames at the end of the
                  cname. */
        if (len == 1)
            len--;

        *p = '\0';

        if ( p != path ) { /* we got something */
            if ( !extend ) {
                cdir = dir->d_child;
                while (cdir) {
                    if ( strcmp( cdir->d_m_name, path ) == 0 ) {
                        break;
                    }
                    cdir = (cdir == dir->d_child->d_prev) ? NULL :
                           cdir->d_next;
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
            }

            if ( cdir == NULL ) {

                if ( len > 0 || !ret.u_name) {
                    return NULL;
                }

            } else {
                dir = cdir;	
                *path = '\0';
            }
        }
    }
}

/*
 * Move curdir to dir, with a possible chdir()
 */
int movecwd( vol, dir)
const struct vol	*vol;
struct dir	*dir;
{
    char path[MAXPATHLEN + 1];
    struct dir	*d;
    char	*p, *u;
    int		n;

    if ( dir == curdir ) {
        return( 0 );
    }
    if ( dir->d_did == DIRDID_ROOT_PARENT) {
        afp_errno = AFPERR_DID1; /* AFPERR_PARAM;*/
        return( -1 );
    }

    p = path + sizeof(path) - 1;
    *p-- = '\0';
    *p = '.';
    for ( d = dir; d->d_parent != NULL && d != curdir; d = d->d_parent ) {
        u = d->d_u_name;
        n = strlen( u );
        if (p -n -1 < path) {
            afp_errno = AFPERR_PARAM;
            return -1;
        }
        *--p = '/';
        p -= n;
        strncpy( p, u, n );
    }
    if ( d != curdir ) {
        n = strlen( vol->v_path );
        if (p -n -1 < path) {
            afp_errno = AFPERR_PARAM;
            return -1;
        }
        *--p = '/';
        p -= n;
        strncpy( p, vol->v_path, n );
    }
    if ( chdir( p ) < 0 ) {
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
    curdir = dir;
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

/* ------------------------------ 
   (".", curdir)
   (name, dir) with curdir:name == dir, from afp_enumerate
*/

int getdirparams(const struct vol *vol,
                 u_int16_t bitmap, struct path *s_path,
                 struct dir *dir, 
                 char *buf, int *buflen )
{
    struct maccess	ma;
    struct adouble	ad;
    char		*data, *l_nameoff = NULL, *utf_nameoff = NULL;
    int			bit = 0, isad = 0;
    u_int32_t           aint;
    u_int16_t		ashort;
    int                 ret;
    u_int32_t           utf8 = 0;
    struct stat *st = &s_path->st;
    char *upath = s_path->u_name;
    
    if ((bitmap & ((1 << DIRPBIT_ATTR)  |
		   (1 << DIRPBIT_CDATE) |
		   (1 << DIRPBIT_MDATE) |
		   (1 << DIRPBIT_BDATE) |
		   (1 << DIRPBIT_FINFO)))) {
        memset(&ad, 0, sizeof(ad));
    	if ( !ad_open( upath, ADFLAGS_HF|ADFLAGS_DIR, O_RDONLY,
                  DIRBITS | 0777, &ad)) {
            isad = 1;
        }
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
            } else if (*dir->d_u_name == '.' && strcmp(dir->d_u_name, ".") 
                        && strcmp(dir->d_u_name, "..")) {
                ashort = htons(ATTRBIT_INVISIBLE);
            } else
                ashort = 0;
            ashort |= htons(ATTRBIT_SHARED);
            memcpy( data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case DIRPBIT_PDID :
            if ( dir->d_did == DIRDID_ROOT) {
                aint = DIRDID_ROOT_PARENT;
            } else if (dir->d_did == DIRDID_ROOT_PARENT) {
                aint = 0;
            } else {
                aint = dir->d_parent->d_did;
            }
            memcpy( data, &aint, sizeof( aint ));
            data += sizeof( aint );
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

                /* dot files are by default invisible */
                if (*dir->d_u_name  == '.' && strcmp(dir->d_u_name , ".") &&
                        strcmp(dir->d_u_name , "..")) {
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF,
                           &ashort, sizeof(ashort));
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
            if (st->st_ctime == dir->ctime) {
               ashort = dir->offcnt;
            }
            else if ((ret = for_each_dirent(vol, upath, NULL,NULL)) >= 0) {
                ashort = ret;
                dir->offcnt = ashort;
                dir->ctime = st->st_ctime;
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

        default :
            if ( isad ) {
                ad_close( &ad, ADFLAGS_HF );
            }
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( l_nameoff ) {
        ashort = htons( data - buf );
        memcpy( l_nameoff, &ashort, sizeof( ashort ));
        data = set_name(vol, data, dir->d_m_name, 0);
    }
    if ( utf_nameoff ) {
        ashort = htons( data - buf );
        memcpy( utf_nameoff, &ashort, sizeof( ashort ));
        data = set_name(vol, data, dir->d_m_name, utf8);
    }
    if ( isad ) {
        ad_close( &ad, ADFLAGS_HF );
    }
    *buflen = data - buf;
    return( AFP_OK );
}

/* ----------------------------- */
int afp_setdirparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    struct path *path;
    u_int16_t	vid, bitmap;
    u_int32_t   did;
    int		rc;

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

    /* FIXME access error or not a file */
    if ( *path->m_name != '\0' ) {
        return (path_isadir( path))? afp_errno:AFPERR_BADTYPE ;
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
    0,  /* stat is not set */
    0,  /* */
};

int setdirparams(const struct vol *vol, 
                 struct path *path, u_int16_t bitmap, char *buf )
{
    struct maccess	ma;
    struct adouble	ad;
    struct utimbuf      ut;
    struct timeval      tv;

    char                *upath;
    int			bit = 0, aint, isad = 1;
    u_int16_t		ashort, bshort;
    int                 err = AFP_OK;
    int                 change_mdate = 0;
    int                 change_parent_mdate = 0;
    int                 newdate = 0;

    upath = path->u_name;
    memset(&ad, 0, sizeof(ad));

    if (ad_open( upath, vol_noadouble(vol)|ADFLAGS_HF|ADFLAGS_DIR,
                 O_RDWR|O_CREAT, 0666, &ad) < 0) {
        /*
         * Check to see what we're trying to set.  If it's anything
         * but ACCESS, UID, or GID, give an error.  If it's any of those
         * three, we don't need the ad to be open, so just continue.
         *
         * note: we also don't need to worry about mdate. also, be quiet
         *       if we're using the noadouble option.
         */
        if (!vol_noadouble(vol) && (bitmap &
                                    ~((1<<DIRPBIT_ACCESS)|(1<<DIRPBIT_UID)|(1<<DIRPBIT_GID)|
                                      (1<<DIRPBIT_MDATE)|(1<<DIRPBIT_PDINFO)))) {
            return AFPERR_ACCESS;
        }

        isad = 0;
    } else {
        /*
         * Check to see if a create was necessary. If it was, we'll want
         * to set our name, etc.
         */
        if ( ad_get_HF_flags( &ad ) & O_CREAT ) {
            ad_setentrylen( &ad, ADEID_NAME, strlen( curdir->d_m_name ));
            memcpy( ad_entry( &ad, ADEID_NAME ), curdir->d_m_name,
                    ad_getentrylen( &ad, ADEID_NAME ));
        }
    }

    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch( bit ) {
        case DIRPBIT_ATTR :
            change_mdate = 1;
            if (isad) {
                memcpy( &ashort, buf, sizeof( ashort ));
                ad_getattr(&ad, &bshort);
                if ( ntohs( ashort ) & ATTRBIT_SETCLR ) {
                    bshort |= htons( ntohs( ashort ) & ~ATTRBIT_SETCLR );
                } else {
                    bshort &= ~ashort;
                }
                ad_setattr(&ad, bshort);
                if ((ashort & htons(ATTRBIT_INVISIBLE)))
		   change_parent_mdate = 1;
            }
            buf += sizeof( ashort );
            break;

        case DIRPBIT_CDATE :
            change_mdate = 1;
            if (isad) {
                memcpy(&aint, buf, sizeof(aint));
                ad_setdate(&ad, AD_DATE_CREATE, aint);
            }
            buf += sizeof( aint );
            break;

        case DIRPBIT_MDATE :
            memcpy(&newdate, buf, sizeof(newdate));
            buf += sizeof( newdate );
            break;

        case DIRPBIT_BDATE :
            change_mdate = 1;
            if (isad) {
                memcpy(&aint, buf, sizeof(aint));
                ad_setdate(&ad, AD_DATE_BACKUP, aint);
            }
            buf += sizeof( aint );
            break;

        case DIRPBIT_FINFO :
            change_mdate = 1;
            /*
             * Alright, we admit it, this is *really* sick!
             * The 4 bytes that we don't copy, when we're dealing
             * with the root of a volume, are the directory's
             * location information. This eliminates that annoying
             * behavior one sees when mounting above another mount
             * point.
             */
            if (isad) {
                if (  curdir->d_did == DIRDID_ROOT ) {
                    memcpy( ad_entry( &ad, ADEID_FINDERI ), buf, 10 );
                    memcpy( ad_entry( &ad, ADEID_FINDERI ) + 14, buf + 14, 18 );
                } else {
                    memcpy( ad_entry( &ad, ADEID_FINDERI ), buf, 32 );
                }
            }
            buf += 32;
            break;

        case DIRPBIT_UID :	/* What kind of loser mounts as root? */
            change_parent_mdate = 1;
            memcpy( &aint, buf, sizeof(aint));
            buf += sizeof( aint );
            if ( (curdir->d_did == DIRDID_ROOT) &&
                    (setdeskowner( ntohl(aint), -1 ) < 0)) {
                switch ( errno ) {
                case EPERM :
                case EACCES :
                    err = AFPERR_ACCESS;
                    goto setdirparam_done;
                    break;
                case EROFS :
                    err = AFPERR_VLOCK;
                    goto setdirparam_done;
                    break;
                default :
                    LOG(log_error, logtype_afpd, "setdirparam: setdeskowner: %s",
                        strerror(errno) );
                    if (!isad) {
                        err = AFPERR_PARAM;
                        goto setdirparam_done;
                    }
                    break;
                }
            }
            if ( setdirowner( ntohl(aint), -1, vol_noadouble(vol) ) < 0 ) {
                switch ( errno ) {
                case EPERM :
                case EACCES :
                    err = AFPERR_ACCESS;
                    goto setdirparam_done;
                    break;
                case EROFS :
                    err = AFPERR_VLOCK;
                    goto setdirparam_done;
                    break;
                default :
                    LOG(log_error, logtype_afpd, "setdirparam: setdirowner: %s",
                        strerror(errno) );
                    break;
                }
            }
            break;
        case DIRPBIT_GID :
            change_parent_mdate = 1;
            memcpy( &aint, buf, sizeof( aint ));
            buf += sizeof( aint );
            if (curdir->d_did == DIRDID_ROOT)
                setdeskowner( -1, ntohl(aint) );

#if 0       /* don't error if we can't set the desktop owner. */
            switch ( errno ) {
            case EPERM :
            case EACCES :
                err = AFPERR_ACCESS;
                goto setdirparam_done;
                break;
            case EROFS :
                err = AFPERR_VLOCK;
                goto setdirparam_done;
                break;
            default :
                LOG(log_error, logtype_afpd, "setdirparam: setdeskowner: %m" );
                if (!isad) {
                    err = AFPERR_PARAM;
                    goto setdirparam_done;
                }
                break;
            }
#endif /* 0 */

            if ( setdirowner( -1, ntohl(aint), vol_noadouble(vol) ) < 0 ) {
                switch ( errno ) {
                case EPERM :
                case EACCES :
                    err = AFPERR_ACCESS;
                    goto setdirparam_done;
                    break;
                case EROFS :
                    err = AFPERR_VLOCK;
                    goto setdirparam_done;
                    break;
                default :
                    LOG(log_error, logtype_afpd, "setdirparam: setdirowner: %s",
                        strerror(errno) );
                    break;
                }
            }
            break;

        case DIRPBIT_ACCESS :
            change_mdate = 1;
            change_parent_mdate = 1;
            ma.ma_user = *buf++;
            ma.ma_world = *buf++;
            ma.ma_group = *buf++;
            ma.ma_owner = *buf++;

            if (curdir->d_did == DIRDID_ROOT)
                setdeskmode(mtoumode( &ma ));
#if 0 /* don't error if we can't set the desktop mode */
            switch ( errno ) {
            case EPERM :
            case EACCES :
                err = AFPERR_ACCESS;
                goto setdirparam_done;
            case EROFS :
                err = AFPERR_VLOCK;
                goto setdirparam_done;
            default :
                LOG(log_error, logtype_afpd, "setdirparam: setdeskmode: %s",
                    strerror(errno) );
                break;
                err = AFPERR_PARAM;
                goto setdirparam_done;
            }
#endif /* 0 */

            if ( setdirmode( mtoumode( &ma ), vol_noadouble(vol),
                         (vol->v_flags & AFPVOL_DROPBOX)) < 0 ) {
                switch ( errno ) {
                case EPERM :
                case EACCES :
                    err = AFPERR_ACCESS;
                    goto setdirparam_done;
                case EROFS :
                    err = AFPERR_VLOCK;
                    goto setdirparam_done;
                default :
                    LOG(log_error, logtype_afpd, "setdirparam: setdirmode: %s",
                        strerror(errno) );
                    err = AFPERR_PARAM;
                    goto setdirparam_done;
                }
            }
            break;

        /* Ignore what the client thinks we should do to the
           ProDOS information block.  Skip over the data and
           report nothing amiss. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :
            if (afp_version < 30) {
                buf += 6;
                break;
            }
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
        ad_flush( &ad, ADFLAGS_HF );
        ad_close( &ad, ADFLAGS_HF );
    }

    if (change_parent_mdate && curdir->d_did != DIRDID_ROOT
            && gettimeofday(&tv, NULL) == 0) {
       if (!movecwd(vol, curdir->d_parent)) {
           newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
           bitmap = 1<<DIRPBIT_MDATE;
           setdirparams(vol, &Cur_Path, bitmap, (char *)&newdate);
           /* should we reset curdir ?*/
       }
    }

    return err;
}

int afp_createdir(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad;
    struct vol		*vol;
    struct dir		*dir;
    char		*upath;
    struct path         *s_path;
    u_int32_t		did;
    u_int16_t		vid;
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
    if (0 != (err = check_name(vol, upath))) {
       return err;
    }

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

    memset(&ad, 0, sizeof(ad));
    if (ad_open( ".", vol_noadouble(vol)|ADFLAGS_HF|ADFLAGS_DIR,
                 O_RDWR|O_CREAT, 0666, &ad ) < 0)  {
        if (vol_noadouble(vol))
            goto createdir_done;
        return( AFPERR_ACCESS );
    }

    ad_setentrylen( &ad, ADEID_NAME, strlen( s_path->m_name ));
    memcpy( ad_entry( &ad, ADEID_NAME ), s_path->m_name,
            ad_getentrylen( &ad, ADEID_NAME ));
    ad_flush( &ad, ADFLAGS_HF );
    ad_close( &ad, ADFLAGS_HF );

createdir_done:
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
int renamedir(src, dst, dir, newparent, newname, noadouble)
char	*src, *dst, *newname;
struct dir	*dir, *newparent;
const int noadouble;
{
    struct adouble	ad;
    struct dir		*parent;
    char                *buf;
    int			len, err;
        
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
            if ((err = copydir(src, dst, noadouble)) < 0) {
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

    memset(&ad, 0, sizeof(ad));
    len = strlen( newname );
    /* rename() succeeded so we need to update our tree even if we can't open
     * .Parent
    */
    if ( !ad_open( dst, ADFLAGS_HF|ADFLAGS_DIR, O_RDWR, 0, &ad)) {
        ad_setentrylen( &ad, ADEID_NAME, len );
        memcpy( ad_entry( &ad, ADEID_NAME ), newname, len );
        ad_flush( &ad, ADFLAGS_HF );
        ad_close( &ad, ADFLAGS_HF );
    }
    
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

    if (( parent = dir->d_parent ) == NULL ) {
        return( AFP_OK );
    }
    if ( parent == newparent ) {
        return( AFP_OK );
    }

    /* detach from old parent and add to new one. */
    dirchildremove(parent, dir);
    dir->d_parent = newparent;
    dirchildadd(newparent, dir);
    return( AFP_OK );
}

#define DOT_APPLEDOUBLE_LEN 13
/* delete an empty directory */
int deletecurdir( vol, path, pathlen )
const struct vol	*vol;
char *path;
int pathlen;
{
    struct dirent *de;
    struct stat st;
    struct dir	*fdir;
    DIR *dp;
    struct adouble	ad;
    u_int16_t		ashort;
    int err;

    if ( curdir->d_parent == NULL ) {
        return( AFPERR_ACCESS );
    }

    fdir = curdir;

    memset(&ad, 0, sizeof(ad));
    if ( ad_open( ".", ADFLAGS_HF|ADFLAGS_DIR, O_RDONLY,
                  DIRBITS | 0777, &ad) == 0 ) {

        ad_getattr(&ad, &ashort);
        ad_close( &ad, ADFLAGS_HF );
        if ((ashort & htons(ATTRBIT_NODELETE))) {
            return  AFPERR_OLOCK;
        }
    }

    /* delete stray .AppleDouble files. this happens to get .Parent files
       as well. */
    if ((dp = opendir(".AppleDouble"))) {
        strcpy(path, ".AppleDouble/");
        while ((de = readdir(dp))) {
            /* skip this and previous directory */
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            /* bail if the file exists in the current directory.
             * note: this will not fail with dangling symlinks */
            if (stat(de->d_name, &st) == 0) {
                closedir(dp);
                return AFPERR_DIRNEMPT;
            }

            strcpy(path + DOT_APPLEDOUBLE_LEN, de->d_name);
            if ((err = netatalk_unlink(path))) {
                closedir(dp);
                return err;
            }
        }
        closedir(dp);
    }

    if ( (err = netatalk_rmdir( ".AppleDouble" ))  ) {
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

    if ( !(err = netatalk_rmdir(fdir->d_u_name))) {
        dirchildremove(curdir, fdir);
#ifdef CNID_DB
        cnid_delete(vol->v_db, fdir->d_did);
#endif /* CNID_DB */
        dir_remove( vol, fdir );
        err = AFP_OK;
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

int afp_mapid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct passwd	*pw;
    struct group	*gr;
    char		*name;
    u_int32_t           id;
    int			len, sfunc;
    int         utf8 = 0;
    
    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    memcpy( &id, ibuf, sizeof( id ));

    id = ntohl(id);
    *rbuflen = 0;

    if ( id != 0 ) {
        switch ( sfunc ) {
        case 1 :
        case 3 :/* unicode */
            if (( pw = getpwuid( id )) == NULL ) {
                return( AFPERR_NOITEM );
            }
            name = pw->pw_name;
            break;

        case 2 :
        case 4 : /* unicode */
            if (( gr = (struct group *)getgrgid( id )) == NULL ) {
                return( AFPERR_NOITEM );
            }
            name = gr->gr_name;
            break;

        default :
            return( AFPERR_PARAM );
        }
        switch ( sfunc ) {
        case 3:
        case 4:
            if (afp_version < 30) {
                return( AFPERR_PARAM );
            }
            utf8 = 1;
            /* map to unicode */
            break;            
        }
        len = strlen( name );

    } else {
        len = 0;
        name = NULL;
    }
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
    return( AFP_OK );
}

int afp_mapname(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct passwd	*pw;
    struct group	*gr;
    int             len, sfunc;
    u_int32_t       id;
    u_int16_t       ulen;

    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    switch ( sfunc ) {
    case 1 : 
    case 2 : /* unicode */
        memcpy(&ulen, ibuf, sizeof(ulen));
        len = ntohs(ulen);
        ibuf += 2;
        break;
    case 3 :
    case 4 :
        len = (unsigned char) *ibuf++;
        break;
    default :
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }

    ibuf[ len ] = '\0';

    if ( len != 0 ) {
        switch ( sfunc ) {
        case 1 : /* unicode */
        case 3 :
            if (( pw = (struct passwd *)getpwnam( ibuf )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            id = pw->pw_uid;
            break;

        case 2 : /* unicode */
        case 4 :
            if (( gr = (struct group *)getgrnam( ibuf )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            id = gr->gr_gid;
            break;
        }
    } else {
        id = 0;
    }
    id = htonl(id);
    memcpy( rbuf, &id, sizeof( id ));
    *rbuflen = sizeof( id );
    return( AFP_OK );
}

/* ------------------------------------
  variable DID support 
*/
int afp_closedir(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
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
int afp_opendir(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct dir		*parentdir;
    struct path		*path;
    u_int32_t		did;
    u_int16_t		vid;

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
        return (path_isadir(path))? afp_errno:AFPERR_BADTYPE ;
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
