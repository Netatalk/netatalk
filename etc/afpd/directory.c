/*
 * $Id: directory.c,v 1.35 2002-08-15 06:20:10 didg Exp $
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

#ifdef FORCE_UIDGID
#include "uid.h"
#endif /* FORCE_UIDGID */

struct dir	*curdir;

#define SENTINEL (&sentinel)
static struct dir sentinel = { SENTINEL, SENTINEL, NULL, DIRTREE_COLOR_BLACK,
                                 NULL, NULL, NULL, NULL, NULL, 0, 0, NULL };
static struct dir	rootpar = { SENTINEL, SENTINEL, NULL, 0,
                                NULL, NULL, NULL, NULL, NULL, 0, 0, NULL };

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
    if (!did)
        return NULL;

    if ( did == DIRDID_ROOT_PARENT ) {
        if (!rootpar.d_did)
            rootpar.d_did = DIRDID_ROOT_PARENT;
        rootpar.d_child = vol->v_dir;
        return( &rootpar );
    }

    dir = vol->v_root;
    while ( dir != SENTINEL ) {
        if (dir->d_did == did)
            return dir->d_name ? dir : NULL;
        dir = (dir->d_did > did) ? dir->d_left : dir->d_right;
    }
    return NULL;
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
    struct dir *ret;
    char		*upath;
    u_int32_t	id;
    static char		path[MAXPATHLEN + 1];
    int len;
    int pathlen;
    char *ptr;
    static char buffer[12 + MAXPATHLEN + 1];
    int buflen = 12 + MAXPATHLEN + 1;

    ret = dirsearch(vol, did);
    if (ret != NULL)
        return ret;

    id = did;
    if ((upath = cnid_resolve(vol->v_db, &id, buffer, buflen)) == NULL) {
        return NULL;
    }
    ptr = path + MAXPATHLEN;
    len = strlen(upath);
    pathlen = len;          /* no 0 in the last part */
    len++;
    strcpy(ptr - len, upath);
    ptr -= len;
    while (1) {
        ret = dirsearch(vol,id);
        if (ret != NULL) {
            break;
        }
        if ((upath = cnid_resolve(vol->v_db, &id, buffer, buflen)) == NULL)
            return NULL;
        len = strlen(upath) + 1;
        pathlen += len;
        if (pathlen > 255)
            return NULL;
        strcpy(ptr - len, upath);
        ptr -= len;
    }
    /* fill the cache */
    ptr--;
    *ptr = (unsigned char)pathlen;
    ptr--;
    *ptr = 2;
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
    free(dir->d_name);
    dir->d_name = NULL;
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
        node->d_name = save.d_name;
    }

    if (node->d_color == DIRTREE_COLOR_BLACK)
        dir_rmrecolor(vol, leaf);
    free(node->d_name);
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
		if (movecwd(vol, vol->v_root) < 0) 
			printf("Yuup cant change dir to v_root\n");
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
char	*path;
{
    char	*p;
    struct stat	st;

    p = mtoupath(vol, path );
    if ( stat( p, &st ) != 0 ) {
        return( NULL );
    }
    if (!S_ISDIR(st.st_mode)) {
        return( NULL );
    }

    if (( dir = adddir( vol, dir, path, strlen( path ), p, strlen(p),
                        &st)) == NULL ) {
        return( NULL );
    }

    if ( movecwd( vol, dir ) < 0 ) {
        return( NULL );
    }

    return( dir );
}

static int deletedir(char *dir)
{
    char path[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent	*de;
    struct stat st;
    int len, err;

    if ((len = strlen(dir)) > sizeof(path))
        return AFPERR_PARAM;

    /* already gone */
    if ((dp = opendir(dir)) == NULL)
        return AFP_OK;

    strcpy(path, dir);
    strcat(path, "/");
    len++;
    while ((de = readdir(dp))) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        strncpy(path + len, de->d_name, sizeof(path) - len);
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if ((err = deletedir(path)) < 0) {
                    closedir(dp);
                    return err;
                }
            } else if (unlink(path) < 0) {
                switch (errno) {
                case ENOENT :
                    continue; /* somebody went and deleted it behind our backs. */
                case EROFS:
                    err = AFPERR_VLOCK;
                    break;
                case EPERM:
                case EACCES :
                    err = AFPERR_ACCESS;
                    break;
                default :
                    err = AFPERR_PARAM;
                }
                closedir(dp);
                return err;
            }
        }
    }
    closedir(dp);

    /* okay. the directory is empty. delete it. note: we already got rid
       of .AppleDouble.  */
    if (rmdir(dir) < 0) {
        switch ( errno ) {
        case ENOENT :
            break;
        case ENOTEMPTY : /* should never happen */
            return( AFPERR_DIRNEMPT );
        case EPERM:
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        default :
            return( AFPERR_PARAM );
        }
    }
    return AFP_OK;
}

/* do a recursive copy. */
static int copydir(char *src, char *dst, int noadouble)
{
    char spath[MAXPATHLEN + 1], dpath[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent	*de;
    struct stat st;
    struct utimbuf      ut;
    int slen, dlen, err;

    /* doesn't exist or the path is too long. */
    if (((slen = strlen(src)) > sizeof(spath) - 2) ||
            ((dlen = strlen(dst)) > sizeof(dpath) - 2) ||
            ((dp = opendir(src)) == NULL))
        return AFPERR_PARAM;

    /* try to create the destination directory */
    if (ad_mkdir(dst, DIRBITS | 0777) < 0) {
        closedir(dp);
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

    /* set things up to copy */
    strcpy(spath, src);
    strcat(spath, "/");
    slen++;
    strcpy(dpath, dst);
    strcat(dpath, "/");
    dlen++;
    err = AFP_OK;
    while ((de = readdir(dp))) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        strncpy(spath + slen, de->d_name, sizeof(spath) - slen);
        if (stat(spath, &st) == 0) {
            strncpy(dpath + dlen, de->d_name, sizeof(dpath) - dlen);

            if (S_ISDIR(st.st_mode)) {
                if ((err = copydir(spath, dpath, noadouble)) < 0)
                    goto copydir_done;
            } else if ((err = copyfile(spath, dpath, NULL, noadouble)) < 0) {
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
        free( dir->d_name );
        free( dir );
    }
}


struct dir *dirnew(const int len)
{
    struct dir *dir;

    dir = (struct dir *) calloc(1, sizeof( struct dir ));
    if (!dir)
        return NULL;

    if ((dir->d_name = (char *) malloc(sizeof(char)*len)) == NULL) {
        free(dir);
        return NULL;
    }

    dir->d_left = dir->d_right = SENTINEL;
    dir->d_next = dir->d_prev = dir;
    return dir;
}


/* XXX: this needs to be changed to handle path types */
char *
cname( vol, dir, cpath )
const struct vol	*vol;
struct dir	*dir;
char	**cpath;
{
    struct dir		*cdir;
    static char		path[ MAXPATHLEN + 1];
    char		*data, *p;
    char        *u;
    int			extend = 0;
    int			len;
	int			olen = 0;
	
    data = *cpath;
    if ( *data++ != 2 ) {			/* path type */
        return( NULL );
    }
    len = (unsigned char) *data++;
    *cpath += len + 2;
    *path = '\0';
    u = NULL;

    for ( ;; ) {
        if ( len == 0 ) {
            if ( !extend && movecwd( vol, dir ) < 0 ) {
            	/* it's tricky:
            	   movecwd failed so dir is not there anymore.
            	   FIXME Is it true with other errors?
            	   if path == '\0' ==> the cpath parameter is that dir,
            	   and maybe we are trying to recreate it! So we can't 
            	   fail here.
            	   
            	*/
    		    if ( dir->d_did == DIRDID_ROOT_PARENT) 
			        return NULL;    		
            	cdir = dir->d_parent;
            	dir_invalidate(vol, dir);
            	if (*path != '\0' || u == NULL) {
            		/* FIXME: if path != '\0' then extend != 0 ?
            		 * u == NUL ==> cpath is something like:
            		 * toto\0\0\0
            		*/
            		return NULL;
            	}
            	if (movecwd(vol, cdir) < 0) {
            		printf("can't change to parent\n");
            		return NULL; /* give up the whole tree is out of synch*/
            	}
				/* restore the previous token */
        		strncpy(path, u, olen);
        		path[olen] = '\0';
            }
            return( path );
        }

        if ( *data == '\0' ) {
            data++;
            len--;
        }
    	u = NULL;

        while ( *data == '\0' && len > 0 ) {
            if ( dir->d_parent == NULL ) {
                return( NULL );
            }
            dir = dir->d_parent;
            data++;
            len--;
        }

        /* would this be faster with strlen + strncpy? */
        p = path;
        if (len > 0) {
        	u = data;
        	olen = len;
		}        
        while ( *data != '\0' && len > 0 ) {
            *p++ = *data++;
            len--;
        }

        /* short cut bits by chopping off a trailing \0. this also
                  makes the traversal happy w/ filenames at the end of the
                  cname. */
        if (len == 1)
            len--;

#ifdef notdef
        /*
         * Dung Nguyen <ntd@adb.fr>
         *
         * AFPD cannot handle paths with "::" if the "::" notation is
         * not at the beginning of the path. The following path will not
         * be interpreted correctly:
         *
         * :a:b:::c: (directory c at the same level as directory a) */
        if ( len > 0 ) {
            data++;
            len--;
        }
#endif /* notdef */
        *p = '\0';

        if ( p != path ) { /* we got something */
            if ( !extend ) {
                cdir = dir->d_child;
                while (cdir) {
                    if ( strcasecmp( cdir->d_name, path ) == 0 ) {
                        break;
                    }
                    cdir = (cdir == dir->d_child->d_prev) ? NULL :
                           cdir->d_next;
                }
                if ( cdir == NULL ) {
                    ++extend;
                    /* if dir == curdir it always succeed,
                       even if curdir is deleted. 
                       it's not a pb because it will failed in extenddir
                    */
                    if ( movecwd( vol, dir ) < 0 ) {
                    	/* dir is not valid anymore 
                    	   we delete dir from the cache and abort.
                    	*/
                    	dir_invalidate(vol, dir);
                        return( NULL );
                    }
                    cdir = extenddir( vol, dir, path );
                }

            } else {
                cdir = extenddir( vol, dir, path );
            }

            if ( cdir == NULL ) {
                if ( len > 0 ) {
                    return( NULL );
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
        return( -1 );
    }

    p = path + sizeof(path) - 1;
    *p-- = '\0';
    *p = '.';
    for ( d = dir; d->d_parent != NULL && d != curdir; d = d->d_parent ) {
        *--p = '/';
        u = mtoupath(vol, d->d_name );
        n = strlen( u );
        p -= n;
        strncpy( p, u, n );
    }
    if ( d != curdir ) {
        *--p = '/';
        n = strlen( vol->v_path );
        p -= n;
        strncpy( p, vol->v_path, n );
    }
    if ( chdir( p ) < 0 ) {
        return( -1 );
    }
    curdir = dir;
    return( 0 );
}

int getdirparams(const struct vol *vol,
                 u_int16_t bitmap,
                 char *upath, struct dir *dir, struct stat *st,
                 char *buf, int *buflen )
{
    struct maccess	ma;
    struct adouble	ad;
    char		*data, *nameoff = NULL;
    DIR			*dp;
    struct dirent	*de;
    int			bit = 0, isad = 1;
    u_int32_t           aint;
    u_int16_t		ashort;
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;
    memset(&uidgid, 0, sizeof(uidgid));
#endif /* FORCE_UIDGID */

    memset(&ad, 0, sizeof(ad));

#ifdef FORCE_UIDGID
    save_uidgid ( &uidgid );
    set_uidgid ( vol );
#endif /* FORCE_UIDGID */

    if ( ad_open( upath, ADFLAGS_HF|ADFLAGS_DIR, O_RDONLY,
                  DIRBITS | 0777, &ad) < 0 ) {
        isad = 0;
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
            } else if (*upath == '.' && strcmp(upath, ".") &&
                       strcmp(upath, "..")) {
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
                if (*upath == '.' && strcmp(upath, ".") &&
                        strcmp(upath, "..")) {
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF,
                           &ashort, sizeof(ashort));
                }
            }
            data += 32;
            break;

        case DIRPBIT_LNAME :
            if (dir->d_name) /* root of parent can have a null name */
                nameoff = data;
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
            if ((dp = opendir( upath ))) {
                while (( de = readdir( dp )) != NULL ) {
                    if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, "."))
                        continue;

                    if (!validupath(vol, de->d_name))
                        continue;

                    /* check for vetoed filenames */
                    if (veto_file(vol->v_veto, de->d_name))
                        continue;

                    /* now check against too long a filename */
                    if (strlen(utompath(vol, de->d_name)) > MACFILELEN)
                        continue;

                    ashort++;
                }
                closedir( dp );
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
            utommode( st, &ma );
#ifndef SENDFILE_FLAVOR_LINUX /* ignore this section if it's linux */
#ifdef HAVE_ACCESS
            accessmode( upath, &ma, dir );
#endif /* HAVE_ACCESS */
#endif /* SENDFILE_FLAVOR_LINUX */
#ifdef AFS	/* If only AFS defined, access() works only for AFS filesystems */ 
            afsmode( upath, &ma, dir );
#endif /* AFS */
            *data++ = ma.ma_user;
            *data++ = ma.ma_world;
            *data++ = ma.ma_group;
            *data++ = ma.ma_owner;
            break;

            /* Client has requested the ProDOS information block.
               Just pass back the same basic block for all
               directories. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :			  /* ProDOS Info Block */
            *data++ = 0x0f;
            *data++ = 0;
            ashort = htons( 0x0200 );
            memcpy( data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            memset( data, 0, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        default :
            if ( isad ) {
                ad_close( &ad, ADFLAGS_HF );
            }
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( nameoff ) {
        ashort = htons( data - buf );
        memcpy( nameoff, &ashort, sizeof( ashort ));

        if ((aint = strlen( dir->d_name )) > MACFILELEN)
            aint = MACFILELEN;

        *data++ = aint;
        memcpy( data, dir->d_name, aint );
        data += aint;
    }
    if ( isad ) {
        ad_close( &ad, ADFLAGS_HF );
    }
    *buflen = data - buf;
    return( AFP_OK );
}

int afp_setdirparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    char	*path;
    u_int16_t	vid, bitmap;
    u_int32_t   did;
    int		rc;

    *rbuflen = 0;
    ibuf += 2;
    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    memcpy( &bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    /*
     * If ibuf is odd, make it even.
     */
    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    if (( rc = setdirparams(vol, path, bitmap, ibuf )) == AFP_OK ) {
        setvoltime(obj, vol );
    }
    return( rc );
}

int setdirparams(const struct vol *vol,
                 char *path, u_int16_t bitmap, char *buf )
{
    struct maccess	ma;
    struct adouble	ad;
    struct utimbuf      ut;
    char                *upath;
    int			bit = 0, aint, isad = 1;
    u_int16_t		ashort, bshort;
    int                 err = AFP_OK;
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;

    memset(&uidgid, 0, sizeof(uidgid));
#endif /* FORCE_UIDGID */

    upath = mtoupath(vol, path);
    memset(&ad, 0, sizeof(ad));
#ifdef FORCE_UIDGID
    save_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
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
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return AFPERR_ACCESS;
        }

        isad = 0;
    } else {
        /*
         * Check to see if a create was necessary. If it was, we'll want
         * to set our name, etc.
         */
        if ( ad_getoflags( &ad, ADFLAGS_HF ) & O_CREAT ) {
            ad_setentrylen( &ad, ADEID_NAME, strlen( curdir->d_name ));
            memcpy( ad_entry( &ad, ADEID_NAME ), curdir->d_name,
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
            if (isad) {
                memcpy( &ashort, buf, sizeof( ashort ));
                ad_getattr(&ad, &bshort);
                if ( ntohs( ashort ) & ATTRBIT_SETCLR ) {
                    bshort |= htons( ntohs( ashort ) & ~ATTRBIT_SETCLR );
                } else {
                    bshort &= ~ashort;
                }
                ad_setattr(&ad, bshort);
            }
            buf += sizeof( ashort );
            break;

        case DIRPBIT_CDATE :
            if (isad) {
                memcpy(&aint, buf, sizeof(aint));
                ad_setdate(&ad, AD_DATE_CREATE, aint);
            }
            buf += sizeof( aint );
            break;

        case DIRPBIT_MDATE :
            memcpy(&aint, buf, sizeof(aint));
            if (isad)
                ad_setdate(&ad, AD_DATE_MODIFY, aint);
            ut.actime = ut.modtime = AD_DATE_TO_UNIX(aint);
            utime(upath, &ut);
            buf += sizeof( aint );
            break;

        case DIRPBIT_BDATE :
            if (isad) {
                memcpy(&aint, buf, sizeof(aint));
                ad_setdate(&ad, AD_DATE_BACKUP, aint);
            }
            buf += sizeof( aint );
            break;

        case DIRPBIT_FINFO :
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
            buf += 6;
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
    if ( isad ) {
        ad_flush( &ad, ADFLAGS_HF );
        ad_close( &ad, ADFLAGS_HF );
    }

#ifdef FORCE_UIDGID
    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
    return err;
}

int afp_createdir(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad;
    struct stat         st;
    struct vol		*vol;
    struct dir		*dir;
    char		*path, *upath;
    u_int32_t		did;
    u_int16_t		vid;
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;

    memset(&uidgid, 0, sizeof(uidgid));
#endif /* FORCE_UIDGID */

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        switch( errno ) {
        case EACCES:
            return( AFPERR_ACCESS );
        case EEXIST:				/* FIXME this on is impossible? */
            return( AFPERR_EXIST );
        default:
            return( AFPERR_NOOBJ );
        }
    }
    /* FIXME check done elswhere? cname was able to move curdir to it! */
	if (*path == '\0')
		return AFPERR_EXIST;
    upath = mtoupath(vol, path);
    {
    int ret;
        if (0 != (ret = check_name(vol, upath))) {
            return  ret;
        }
    }

#ifdef FORCE_UIDGID
    save_uidgid ( &uidgid );
    set_uidgid  ( vol );
#endif /* FORCE_UIDGID */

    if ( ad_mkdir( upath, DIRBITS | 0777 ) < 0 ) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EROFS :
            return( AFPERR_VLOCK );
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

    if (stat(upath, &st) < 0) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return AFPERR_MISC;
    }

    if ((dir = adddir( vol, curdir, path, strlen( path ), upath,
                       strlen(upath), &st)) == NULL) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return AFPERR_MISC;
    }

    if ( movecwd( vol, dir ) < 0 ) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return( AFPERR_PARAM );
    }

    memset(&ad, 0, sizeof(ad));
    if (ad_open( "", vol_noadouble(vol)|ADFLAGS_HF|ADFLAGS_DIR,
                 O_RDWR|O_CREAT, 0666, &ad ) < 0)  {
        if (vol_noadouble(vol))
            goto createdir_done;
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return( AFPERR_ACCESS );
    }

    ad_setentrylen( &ad, ADEID_NAME, strlen( path ));
    memcpy( ad_entry( &ad, ADEID_NAME ), path,
            ad_getentrylen( &ad, ADEID_NAME ));
    ad_flush( &ad, ADFLAGS_HF );
    ad_close( &ad, ADFLAGS_HF );

createdir_done:
    memcpy( rbuf, &dir->d_did, sizeof( u_int32_t ));
    *rbuflen = sizeof( u_int32_t );
    setvoltime(obj, vol );
#ifdef FORCE_UIDGID
    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
    return( AFP_OK );
}


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
    if ( rename( src, dst ) < 0 ) {
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
    if ( ad_open( dst, ADFLAGS_HF|ADFLAGS_DIR, O_RDWR, 0, &ad) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            if (noadouble) {
                len = strlen(newname);
                goto renamedir_done;
            }
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }
    len = strlen( newname );
    ad_setentrylen( &ad, ADEID_NAME, len );
    memcpy( ad_entry( &ad, ADEID_NAME ), newname, len );
    ad_flush( &ad, ADFLAGS_HF );
    ad_close( &ad, ADFLAGS_HF );

renamedir_done:
    if ((buf = (char *) realloc( dir->d_name, len + 1 )) == NULL ) {
        LOG(log_error, logtype_afpd, "renamedir: realloc: %s", strerror(errno) );
        return AFPERR_MISC;
    }
    dir->d_name = buf;
    strcpy( dir->d_name, newname );

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
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;

    memset(&uidgid, 0, sizeof(uidgid));
#endif /* FORCE_UIDGID */

    if ( curdir->d_parent == NULL ) {
        return( AFPERR_ACCESS );
    }

    if ( curdir->d_child != NULL ) {
        return( AFPERR_DIRNEMPT );
    }

    fdir = curdir;

#ifdef FORCE_UIDGID
    save_uidgid ( &uidgid );
    set_uidgid  ( vol );
#endif /* FORCE_UIDGID */

    memset(&ad, 0, sizeof(ad));
    if ( ad_open( ".", ADFLAGS_HF|ADFLAGS_DIR, O_RDONLY,
                  DIRBITS | 0777, &ad) == 0 ) {

        ad_getattr(&ad, &ashort);
        ad_close( &ad, ADFLAGS_HF );
        if ((ashort & htons(ATTRBIT_NODELETE))) {
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
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
#ifdef FORCE_UIDGID
                restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                return AFPERR_DIRNEMPT;
            }

            strcpy(path + DOT_APPLEDOUBLE_LEN, de->d_name);
            if (unlink(path) < 0) {
                closedir(dp);
                switch (errno) {
                case EPERM:
                case EACCES :
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return( AFPERR_ACCESS );
                case EROFS:
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return AFPERR_VLOCK;
                case ENOENT :
                    continue;
                default :
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return( AFPERR_PARAM );
                }
            }
        }
        closedir(dp);
    }

    if ( rmdir( ".AppleDouble" ) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            break;
        case ENOTEMPTY :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_DIRNEMPT );
        case EROFS:
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return AFPERR_VLOCK;
        case EPERM:
        case EACCES :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_ACCESS );
        default :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_PARAM );
        }
    }

    /* now get rid of dangling symlinks */
    if ((dp = opendir("."))) {
        while ((de = readdir(dp))) {
            /* skip this and previous directory */
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            /* bail if it's not a symlink */
            if ((lstat(de->d_name, &st) == 0) && !S_ISLNK(st.st_mode)) {
#ifdef FORCE_UIDGID
                restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                return AFPERR_DIRNEMPT;
            }

            if (unlink(de->d_name) < 0) {
                switch (errno) {
                case EPERM:
                case EACCES :
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return( AFPERR_ACCESS );
                case EROFS:
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return AFPERR_VLOCK;
                case ENOENT :
                    continue;
                default :
#ifdef FORCE_UIDGID
                    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
                    return( AFPERR_PARAM );
                }
            }
        }
        closedir(dp);
    }

    if ( movecwd( vol, curdir->d_parent ) < 0 ) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return( AFPERR_NOOBJ );
    }

    if ( rmdir(mtoupath(vol, fdir->d_name)) < 0 ) {
        switch ( errno ) {
        case ENOENT :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_NOOBJ );
        case ENOTEMPTY :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_DIRNEMPT );
        case EPERM:
        case EACCES :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_ACCESS );
        case EROFS:
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return AFPERR_VLOCK;
        default :
#ifdef FORCE_UIDGID
            restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_PARAM );
        }
    }

    dirchildremove(curdir, fdir);
#ifdef CNID_DB
    cnid_delete(vol->v_db, fdir->d_did);
#endif /* CNID_DB */
    dir_remove( vol, fdir );

#ifdef FORCE_UIDGID
    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
    return( AFP_OK );
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

    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    memcpy( &id, ibuf, sizeof( id ));

    id = ntohl(id);

    if ( id != 0 ) {
        switch ( sfunc ) {
        case 1 :
            if (( pw = getpwuid( id )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            name = pw->pw_name;
            break;

        case 2 :
            if (( gr = (struct group *)getgrgid( id )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            name = gr->gr_name;
            break;

        default :
            *rbuflen = 0;
            return( AFPERR_PARAM );
        }

        len = strlen( name );

    } else {
        len = 0;
        name = NULL;
    }

    *rbuf++ = len;
    if ( len > 0 ) {
        memcpy( rbuf, name, len );
    }
    *rbuflen = len + 1;
    return( AFP_OK );
}

int afp_mapname(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct passwd	*pw;
    struct group	*gr;
    int			len, sfunc;
    u_int32_t           id;

    ibuf++;
    sfunc = (unsigned char) *ibuf++;
    len = (unsigned char) *ibuf++;
    ibuf[ len ] = '\0';

    if ( len != 0 ) {
        switch ( sfunc ) {
        case 3 :
            if (( pw = (struct passwd *)getpwnam( ibuf )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            id = pw->pw_uid;
            break;

        case 4 :
            if (( gr = (struct group *)getgrnam( ibuf )) == NULL ) {
                *rbuflen = 0;
                return( AFPERR_NOITEM );
            }
            id = gr->gr_gid;
            break;
        default :
            *rbuflen = 0;
            return( AFPERR_PARAM );
        }
    } else {
        id = 0;
    }
    id = htonl(id);
    memcpy( rbuf, &id, sizeof( id ));
    *rbuflen = sizeof( id );
    return( AFP_OK );
}

/* variable DID support */
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
    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_PARAM );
    }

    /* dir_remove -- deletedid */
#endif /* 0 */

    return AFP_OK;
}

/* did creation gets done automatically */
int afp_opendir(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct dir		*dir, *parentdir;
    struct stat         st;
    char		*path, *upath;
    u_int32_t		did;
    u_int16_t		vid;
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;

    memset(&uidgid, 0, sizeof(uidgid));
#endif /* FORCE_UIDGID */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof( vid );

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (( parentdir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, parentdir, &ibuf )) == NULL ) {
        switch( errno ) {
        case EACCES:
            return( AFPERR_ACCESS );
        default:
            return( AFPERR_NOOBJ );
        }
    }

    /* see if we already have the directory. */
    upath = mtoupath(vol, path);
    if ( stat( upath, &st ) < 0 ) {
        return( AFPERR_NOOBJ );
    }

    dir = parentdir->d_child;
    while (dir) {
        if (strdiacasecmp(dir->d_name, path) == 0) {
            memcpy(rbuf, &dir->d_did, sizeof(dir->d_did));
            *rbuflen = sizeof(dir->d_did);
            return AFP_OK;
        }
        dir = (dir == parentdir->d_child->d_prev) ? NULL : dir->d_next;
    }

#ifdef FORCE_UIDGID
    save_uidgid ( &uidgid );
    set_uidgid  ( vol );
#endif /* FORCE_UIDGID */

    /* we don't already have a did. add one in. */
    if ((dir = adddir(vol, parentdir, path, strlen(path),
                      upath, strlen(upath), &st)) == NULL) {
#ifdef FORCE_UIDGID
        restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
        return AFPERR_MISC;
    }

    memcpy(rbuf, &dir->d_did, sizeof(dir->d_did));
    *rbuflen = sizeof(dir->d_did);
#ifdef FORCE_UIDGID
    restore_uidgid ( &uidgid );
#endif /* FORCE_UIDGID */
    return AFP_OK;
}
