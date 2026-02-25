/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2025-2026 Andy Lemin (andylemin)
 *
 * All Rights Reserved.  See COPYRIGHT.
 * You may also use it under the terms of the General Public License (GPL). See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <utime.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/afp_util.h>
#include <atalk/cnid.h>
#include <atalk/errchk.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_ipc.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/uuid.h>
#include <atalk/vfs.h>

#include "desktop.h"
#include "dircache.h"
#include "ad_cache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "mangle.h"
#include "unix.h"
#include "volume.h"

/*
 * FIXMEs, loose ends after the dircache rewrite:
 * o merge dircache_search_by_name and dir_add ??
 * o case-insensitivity is gone from cname
 */


/*******************************************************************************************
 * Globals
 ******************************************************************************************/

int         afp_errno;
/* As long as directory.c hasn't got its own init call, this get initialized in dircache_init */
struct dir rootParent = {
    /* d_did set at runtime in dircache_init() — DIRDID_ROOT_PARENT uses htonl() which
     * is not a compile-time constant on all platforms */
    .dcache_mode = S_IFDIR | 0755,
    /* rootParent.d_rights_cache init to 0xffffffff ('access rights not set/unknown') */
    .d_rights_cache = 0xffffffff,
    /* dcache_rlen init to -1 ('resource fork length unknown/not set') */
    .dcache_rlen = (off_t) -1,
    /* All other fields zero-initialized by C11 designated init rules */
};
struct dir  *curdir = &rootParent;
struct path Cur_Path = {
    0,
    "",  /* mac name */
    ".", /* unix name */
    0,   /* id */
    NULL,/* pointer to struct dir */
    0,   /* stat is not set */
    0,   /* errno */
    {0} /* struct stat */
};

/*
 * dir_remove queues struct dirs to be freed here. We can't just delete them immeidately
 * e.g. in dircache_search_by_id, because a caller somewhere up the stack might be
 * referencing it.
 * So instead:
 * - we mark it as invalid by setting d_did to CNID_INVALID (ie 0)
 * - queue it in "invalid_dircache_entries" queue
 * - which is finally freed at the end of every AFP func in afp_dsi.c.
 */
q_t *invalid_dircache_entries;


/*******************************************************************************************
 * Locals
 ******************************************************************************************/


/* -------------------------
   appledouble mkdir afp error code.
*/
static int netatalk_mkdir(const struct vol *vol, const char *name)
{
    int ret;
    struct stat st;

    if (vol->v_flags & AFPVOL_UNIX_PRIV) {
        if (lstat(".", &st) < 0) {
            return AFPERR_MISC;
        }

        int mode = (DIRBITS & (~S_ISGID & st.st_mode)) | (0777 & ~vol->v_umask);
        LOG(log_maxdebug, logtype_afpd,
            "netatalk_mkdir(\"%s\") {parent mode: %04o, vol umask: %04o}",
            name, st.st_mode, vol->v_umask);
        ret = mkdir(name, mode);
    } else {
        ret = ad_mkdir(name, DIRBITS | 0777);
    }

    if (ret < 0) {
        switch (errno) {
        case ENOENT :
            return AFPERR_NOOBJ;

        case EROFS :
            return AFPERR_VLOCK;

        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;

        case EEXIST :
            return AFPERR_EXIST;

        case ENOSPC :
        case EDQUOT :
            return AFPERR_DFULL;

        default :
            return AFPERR_PARAM;
        }
    }

    return AFP_OK;
}

/* ------------------- */
static int deletedir(const struct vol *vol, int dirfd, char *dir)
{
    char path[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent   *de;
    struct stat st;
    size_t len;
    int err = AFP_OK;
    size_t remain;

    if ((len = strlen(dir)) + 2 > sizeof(path)) {
        return AFPERR_PARAM;
    }

    /* already gone */
    if ((dp = opendirat(dirfd, dir)) == NULL) {
        return AFP_OK;
    }

    snprintf(path, MAXPATHLEN, "%s/", dir);
    len++;
    remain = strlen(path) - len - 1;

    while ((de = readdir(dp)) && err == AFP_OK) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        if (strlen(de->d_name) > remain) {
            err = AFPERR_PARAM;
            break;
        }

        strcpy(path + len, de->d_name);

        if (ostatat(dirfd, path, &st, vol_syml_opt(vol))) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            err = deletedir(vol, dirfd, path);
        } else {
            err = netatalk_unlinkat(dirfd, path);
        }
    }

    closedir(dp);

    /* okay. the directory is empty. delete it. note: we already got rid
       of .AppleDouble.  */
    if (err == AFP_OK) {
        err = netatalk_rmdir(dirfd, dir);
    }

    return err;
}

/*! do a recursive copy. */
static int copydir(struct vol *vol, struct dir *ddir, int dirfd, char *src,
                   char *dst)
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
            ((dp = opendirat(dirfd, src)) == NULL)) {
        return AFPERR_PARAM;
    }

    /* try to create the destination directory */
    if (AFP_OK != (err = netatalk_mkdir(vol, dst))) {
        closedir(dp);
        return err;
    }

    /* set things up to copy */
    snprintf(spath, MAXPATHLEN, "%s/", src);
    slen++;
    srem = strlen(spath) - slen - 1;
    snprintf(dpath, MAXPATHLEN, "%s/", dst);
    dlen++;
    drem = strlen(dpath) - dlen - 1;
    err = AFP_OK;

    while ((de = readdir(dp))) {
        /* skip this and previous directory */
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        if (strlen(de->d_name) > srem) {
            err = AFPERR_PARAM;
            break;
        }

        strcpy(spath + slen, de->d_name);

        if (ostatat(dirfd, spath, &st, vol_syml_opt(vol)) == 0) {
            if (strlen(de->d_name) > drem) {
                err = AFPERR_PARAM;
                break;
            }

            strcpy(dpath + dlen, de->d_name);

            if (S_ISDIR(st.st_mode)) {
                if (AFP_OK != (err = copydir(vol, ddir, dirfd, spath, dpath))) {
                    goto copydir_done;
                }
            } else if (AFP_OK != (err = copyfile(vol, vol, ddir, dirfd, spath, dpath, NULL,
                                                 NULL))) {
                goto copydir_done;
            } else {
                /* keep the same time stamp. */
                ut.actime = ut.modtime = st.st_mtime;
                utime(dpath, &ut);
            }
        }
    }

    /* keep the same time stamp. */
    if (ostatat(dirfd, src, &st, vol_syml_opt(vol)) == 0) {
        ut.actime = ut.modtime = st.st_mtime;
        utime(dst, &ut);
    }

copydir_done:
    closedir(dp);
    return err;
}

/*!
 * is our cached offspring count valid?
 */
static int diroffcnt(struct dir *dir, struct stat *st)
{
    return st->st_ctime == dir->d_ctime;
}

/* --------------------- */
static int invisible_dots(const struct vol *vol, const char *name)
{
    return vol_inv_dots(vol) && *name  == '.' && strcmp(name, ".")
           && strcmp(name, "..");
}

/* ------------------ */
static int set_dir_errors(struct path *path, const char *where, int err)
{
    switch (err) {
    case EPERM :
    case EACCES :
        return AFPERR_ACCESS;

    case EROFS :
        return AFPERR_VLOCK;
    }

    LOG(log_error, logtype_afpd, "setdirparam(%s): %s: %s",
        fullpathname(path->u_name), where, strerror(err));
    return AFPERR_PARAM;
}

/*!
 * @brief Convert name in client encoding to server encoding
 *
 * Convert ret->m_name to ret->u_name from client encoding to server encoding.
 * This only gets called from cname().
 *
 * @returns 0 on success, -1 on error
 *
 * @note If the passed ret->m_name is mangled, we'll demangle it
 */
static int cname_mtouname(const struct vol *vol, struct dir *dir,
                          struct path *ret, int toUTF8)
{
    static char temp[MAXPATHLEN + 1];
    char *t;
    cnid_t fileid = 0;

    if (vol->v_obj->afp_version >= 30) {
        if (toUTF8) {
            if (dir->d_did == DIRDID_ROOT_PARENT) {
                /*
                 * With uft8 volume name is utf8-mac, but requested path may be a mangled longname. See #2611981.
                 * So we compare it with the longname from the current volume and if they match
                 * we overwrite the requested path with the utf8 volume name so that the following
                 * strcmp can match.
                 */
                ucs2_to_charset(vol->v_maccharset, vol->v_macname, temp, AFPVOL_MACNAMELEN + 1);

                if (strcasecmp(ret->m_name, temp) == 0) {
                    ucs2_to_charset(CH_UTF8_MAC, vol->v_u8mname, ret->m_name, AFPVOL_U8MNAMELEN);
                }
            } else {
                /* toUTF8 */
                if (mtoUTF8(vol, ret->m_name, strlen(ret->m_name), temp,
                            MAXPATHLEN) == (size_t) -1) {
                    afp_errno = AFPERR_PARAM;
                    return -1;
                }

                strcpy(ret->m_name, temp);
            }
        }

        /* check for OS X mangled filename :( */
        t = demangle_osx(vol, ret->m_name, dir->d_did, &fileid);

        /* demangle_osx() calls dirlookup() which might have clobbered curdir */
        if (curdir == NULL && movecwd(vol, dir) < 0) {
            return -1;
        }

        LOG(log_maxdebug, logtype_afpd,
            "cname_mtouname('%s',did:%u) {demangled:'%s', fileid:%u}",
            ret->m_name, ntohl(dir->d_did), t, ntohl(fileid));

        if (t != ret->m_name) {
            ret->u_name = t;

            /* duplicate work but we can't reuse all convert_char we did in demangle_osx
             * flags weren't the same
             */
            if ((t = utompath(vol, ret->u_name, fileid, utf8_encoding(vol->v_obj)))) {
                /* at last got our view of mac name */
                strcpy(ret->m_name, t);
            }
        }
    } /* afp_version >= 30 */

    /* If we haven't got it by now, get it */
    if (ret->u_name == NULL) {
        if ((ret->u_name = mtoupath(vol, ret->m_name, dir->d_did,
                                    utf8_encoding(vol->v_obj))) == NULL) {
            afp_errno = AFPERR_PARAM;
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Build struct path from struct dir
 *
 * The final movecwd in cname failed, possibly with EPERM or ENOENT. We:
 * 1. move cwd into parent dir (we're often already there, but not always)
 * 2. set struct path to the dirname
 * 3. in case of
 *    AFPERR_ACCESS: the dir is there, we just can't chdir into it
 *    AFPERR_NOOBJ: the dir was there when we stated it in cname, so we have a race
 *                  4. indicate there's no dir for this path
 *                  5. remove the dir
 */
static struct path *path_from_dir(struct vol *vol, struct dir *dir,
                                  struct path *ret)
{
    if (dir->d_did == DIRDID_ROOT_PARENT || dir->d_did == DIRDID_ROOT) {
        return NULL;
    }

    switch (afp_errno) {
    case AFPERR_ACCESS:
        if (movecwd(vol, dirlookup(vol, dir->d_pdid)) < 0) { /* 1 */
            return NULL;
        }

        memcpy(ret->m_name, cfrombstr(dir->d_m_name),
               blength(dir->d_m_name) + 1); /* 3 */

        if (dir->d_m_name == dir->d_u_name) {
            ret->u_name = ret->m_name;
        } else {
            ret->u_name =  ret->m_name + blength(dir->d_m_name) + 1;
            memcpy(ret->u_name, cfrombstr(dir->d_u_name), blength(dir->d_u_name) + 1);
        }

        ret->d_dir = dir;
        LOG(log_debug, logtype_afpd,
            "cname('%s') {path-from-dir: AFPERR_ACCESS. curdir:'%s', path:'%s'}",
            cfrombstr(dir->d_fullpath),
            cfrombstr(curdir->d_fullpath),
            ret->u_name);
        return ret;

    case AFPERR_NOOBJ:
        if (movecwd(vol, dirlookup(vol, dir->d_pdid)) < 0) { /* 1 */
            return NULL;
        }

        memcpy(ret->m_name, cfrombstr(dir->d_m_name), blength(dir->d_m_name) + 1);

        if (dir->d_m_name == dir->d_u_name) {
            ret->u_name = ret->m_name;
        } else {
            ret->u_name =  ret->m_name + blength(dir->d_m_name) + 1;
            memcpy(ret->u_name, cfrombstr(dir->d_u_name), blength(dir->d_u_name) + 1);
        }

        ret->d_dir = NULL;      /* 4 */
        dir_remove(vol, dir, 0);   /* 5 - Proactive cleanup, not invalid on use */
        return ret;

    default:
        return NULL;
    }

    /* DEADC0DE: never get here */
    return NULL;
}


/*********************************************************************************************
 * Interface
 ********************************************************************************************/

int get_afp_errno(const int param)
{
    if (afp_errno != AFPERR_DID1) {
        return afp_errno;
    }

    return param;
}

/* Forward declaration for retry helper */
static struct dir *dirlookup_internal(const struct vol *vol, cnid_t did,
                                      int retry, int strict);

/*!
 * @brief Retry helper for dirlookup_internal on ENOENT
 *
 * When stat() fails with ENOENT, this function cleans stale cache entries
 * and retries the lookup via CNID database. This handles race conditions where
 * directories are renamed/moved by external processes between cache and stat.
 *
 * @param[in] vol         Volume
 * @param[in] did         Target DID that failed stat
 * @param[in] strict      Validation mode (0=initial target, 1=parent recursion)
 * @param[in,out] fullpath_ptr  Pointer to fullpath to cleanup
 * @param[in,out] upath_ptr     Pointer to upath to cleanup
 *
 * @returns Retried struct dir from CNID, or NULL on failure
 */
static struct dir *dirlookup_internal_retry(const struct vol *vol,
        cnid_t did,
        int strict,
        bstring *fullpath_ptr,
        char **upath_ptr)
{
    LOG(log_debug, logtype_afpd,
        "dirlookup_internal(did:%u): stat failed, cleaning stale entries, retrying via CNID",
        ntohl(did));
    /* Clean cached stale target if present */
    struct dir *stale_child = dircache_search_by_did(vol, did);

    if (stale_child) {
        LOG(log_debug, logtype_afpd,
            "dirlookup_internal(did:%u): cleaning stale target entry",
            ntohl(did));

        /* If strict=0 (initial target) & directory, also clean children.
         * If strict=1 (parent recursion), skip children to avoid cascade. */
        if (!strict && !(stale_child->d_flags & DIRF_ISFILE)) {
            (void)dircache_remove_children(vol, stale_child);
        }

        dir_remove(vol, stale_child, 0);  /* Proactive cleanup of stale entry */
    }

    /* Cleanup current failed attempt */
    if (*fullpath_ptr) {
        bdestroy(*fullpath_ptr);
        *fullpath_ptr = NULL;
    }

    if (*upath_ptr) {
        free(*upath_ptr);
        *upath_ptr = NULL;
    }

    /* Retry with cleaned cache (cache miss → CNID lookup) */
    return dirlookup_internal(vol, did, 0, strict);
}

/*!
 * @brief Internal CNID (Directory ID) resolution with retry control
 *
 * Resolve a CNID (Directory ID), allocate a struct dir for it.
 *
 * Algorithm:
 * 1. Check for special DIDs 0 (invalid), 1 (root parent), and 2 (root).
 * 2. Search dircache:
 *    - If found and valid, return cached entry
 *    - If strict mode, validate inode before use
 * 3. On cache miss, query CNID database for path component
 * 4. Recurse to build parent chain (terminates at root or cache hit)
 * 5. Build fullpath and stat to verify existence
 * 6. Create new struct dir and populate
 * 7. Add new entry to cache
 *
 * On ENOENT during stat (step 5), if retry=1:
 * - Clean stale cache entries
 * - Retry once via CNID (prevents TOCTOU issues)
 *
 * @param[in] vol    pointer to struct vol
 * @param[in] did    CNID to resolve
 * @param[in] retry  1 = allow one retry on ENOENT, 0 = no retry
 * @param[in] strict 1 = strict dircache lookup (validate with stat+inode), 0 = optimistic
 *
 * @returns pointer to struct dir, or NULL with afp_errno set
 */
static struct dir *dirlookup_internal(const struct vol *vol, cnid_t did,
                                      int retry, int strict)
{
#define DIRLOOKUP_LOG_FMT "dirlookup_internal(did:%u)"
    static char  buffer[CNID_PATH_OVERHEAD + MAXPATHLEN + 1];
    struct stat  st;
    struct dir   *ret = NULL, *pdir;
    bstring      fullpath = NULL;
    char         *upath = NULL, *mpath;
    cnid_t       cnid, pdid;
    int          buflen = CNID_PATH_OVERHEAD + MAXPATHLEN + 1;
    int          utf8;
    int          err = 0;
    LOG(log_debug, logtype_afpd,
        DIRLOOKUP_LOG_FMT ": (retry: %d, strict: %d): START",
        ntohl(did), retry, strict);

    /* check for did 0, 1 and 2 */
    if (did == 0 || vol == NULL) { /* 1 */
        afp_errno = AFPERR_PARAM;
        ret = NULL;
        goto exit;
    } else if (did == DIRDID_ROOT_PARENT) {
        rootParent.d_vid = vol->v_vid;
        rootParent.d_did = DIRDID_ROOT_PARENT;
        ret = &rootParent;
        goto exit;
    } else if (did == DIRDID_ROOT) {
        ret = vol->v_root;
        goto exit;
    }

    /* Search the cache first */
    if ((ret = dircache_search_by_did(vol, did)) != NULL) { /* 2a */
        /* Cache HIT */
        extern AFPObj *AFPobj;

        if (!AFPobj->options.dircache_files && (ret->d_flags & DIRF_ISFILE)) {
            /* Files not supported (dircache files = no) - reject */
            afp_errno = AFPERR_BADTYPE;
            ret = NULL;
            goto exit;
        }

        /* Strict validation (parent recursion) */
        if (strict) { /* 2b */
            struct stat validate_st;

            if (ostat(cfrombstr(ret->d_fullpath), &validate_st, vol_syml_opt(vol)) != 0 ||
                    validate_st.st_ino != ret->dcache_ino) {
                /* Cache is stale, clean and fall through to CNID lookup */
                LOG(log_debug, logtype_afpd, DIRLOOKUP_LOG_FMT ": cache stale, cleaning",
                    ntohl(did));
                dir_remove(vol, ret, 0);  /* Cleanup during strict validation */
                ret = NULL;
            } else {
                /* Validated, safe to use */
                goto exit;
            }
        } else {
            /* Optimistic lookup: dircache_search_by_did() validation based on the probabilistic
             * model (should_validate_cache_entry). If invalid, caught during use and repaired on the fly */
            goto exit;
        }
    }

    /* Cache MISS - Search the CNID database */
    cnid = did;
    LOG(log_debug, logtype_afpd, "dirlookup(did: %u): querying CNID database",
        ntohl(did));
    AFP_CNID_START("cnid_resolve");
    upath = cnid_resolve(vol->v_cdb, &cnid, buffer, buflen); /* 3 */
    AFP_CNID_DONE();

    /* CNID stores relative parent-child relationships. Input: cnid = did (DID to resolve),
     * Outputs: upath = basename only ("mydir") + cnid modified to parent DID
     * So we need to build the rest of the fullpath */
    if (upath == NULL) {
        afp_errno = AFPERR_NOOBJ;
        err = 1;
        goto exit;
    }

    if ((upath = strdup(upath)) == NULL) {
        afp_errno = AFPERR_NOOBJ;
        err = 1;
        goto exit;
    }

    /* Strict recurse parent (rebuild parent-chain), returns when either:
     * DIRDID_ROOT is hit OR a valid cache entry found. retry once at each level */
    pdid = cnid;

    if ((pdir = dirlookup_internal(vol, pdid, 1, 1)) == NULL) { /* 4 */
        err = 1;
        goto exit;
    }

    /* Build the fullpath from cached parent-chain + CNID's upath */
    if ((fullpath = bstrcpy(pdir->d_fullpath)) == NULL
            || bconchar(fullpath, '/') != BSTR_OK
            || bcatcstr(fullpath, upath) != BSTR_OK) { /* 5 */
        err = 1;
        goto exit;
    }

    /* Stat derived fullpath */
    LOG(log_debug, logtype_afpd, "dirlookup(did: %u): stating \"%s\"",
        ntohl(did), cfrombstr(fullpath));

    if (ostat(cfrombstr(fullpath), &st, vol_syml_opt(vol)) != 0) { /* 5a */
        /* File system object at fullpath not found (possible external system change) */
        switch (errno) {
        case ENOENT:

            /* Target's fullpath doesn't exist - clean target, retry via CNID */
            if (retry) {
                ret = dirlookup_internal_retry(vol, did, strict, &fullpath, &upath);
                goto exit;
            }

            afp_errno = AFPERR_NOOBJ;
            err = 1;
            goto exit;

        case EPERM:
            afp_errno = AFPERR_ACCESS;
            err = 1;
            goto exit;

        default:
            afp_errno = AFPERR_MISC;
            err = 1;
            goto exit;
        }
    } else {
        /* Filesystem object at fullpath found (st now contains valid stat data) */
        extern AFPObj *AFPobj;

        if (!AFPobj->options.dircache_files && !S_ISDIR(st.st_mode)) {
            /* Lookup found file, but files are disabled in dircache - reject */
            LOG(log_debug, logtype_afpd,
                DIRLOOKUP_LOG_FMT
                ": file found but 'dircache files' disabled in config, rejecting",
                ntohl(did));
            afp_errno = AFPERR_BADTYPE;
            err = 1;
            goto exit;
        }
    }

    /* Get macname from unix name */
    utf8 = utf8_encoding(vol->v_obj);

    if ((mpath = utompath(vol, upath, did, utf8)) == NULL) {
        afp_errno = AFPERR_NOOBJ;
        err = 1;
        goto exit;
    }

    /* Create new struct for dircache */
    if ((ret = dir_new(mpath, upath, vol, pdid, did, fullpath,
                       &st)) == NULL) { /* 6 */
        LOG(log_error, logtype_afpd, DIRLOOKUP_LOG_FMT ": {%s, %s}: %s", ntohl(did),
            mpath, upath, strerror(errno));
        err = 1;
        goto exit;
    }

    /* Add new struct to the cache */
    if (dircache_add(vol, ret) != 0) { /* 7 */
        err = 1;
        goto exit;
    }

exit:

    /* Log result and cleanup */
    if (ret) {
        LOG(log_debug, logtype_afpd,
            DIRLOOKUP_LOG_FMT ": SUCCESS - pdid: %u, path: \"%s\"",
            ntohl(ret->d_did), ntohl(ret->d_pdid), cfrombstr(ret->d_fullpath));
    } else if (err) {
        LOG(log_debug, logtype_afpd,
            DIRLOOKUP_LOG_FMT ": FAILED - %s",
            ntohl(did), AfpErr2name(afp_errno));
    }

    if (err) {
        /* If dir_new failed, fullpath ownership not transferred to ret - free resources */
        if (fullpath) {
            bdestroy(fullpath);
        }

        if (ret) {
            dir_free(ret);
            ret = NULL;
        }
    }

    if (upath) {
        free(upath);
    }

#undef DIRLOOKUP_LOG_FMT
    return ret;
}


/*!
 * @brief Optimistic CNID resolution for read/safe code paths
 *
 * Resolves a CNID to its cached entry using probabilistic validation.
 * Safe for read operations (enumerate, getparams, opendir, openfork)
 * because stale entries are detected on use and recovered via movecwd().
 *
 * @param[in] vol   pointer to struct vol
 * @param[in] did   DID to resolve
 *
 * @returns pointer to struct dir (may have DIRF_ISFILE flag for files)
 */
struct dir *dirlookup(const struct vol *vol, cnid_t did)
{
    /*
    retry=1: Allow cache clean and fallback to CNID once at each level
    strict=0: Uses optimistic dircache lookup
    */
    return dirlookup_internal(vol, did, 1, 0);
}

/*!
 * @brief Validated DID resolution for write/change code paths
 *
 * Like dirlookup(), but performs stat()+inode validation to ensure the cached
 * entry matches the current filesystem state. Gurantees operating on valid entry.
 * Use for all write/change operations: delete, setdirparams, setfilparams,
 * setfildirparams, setacl, copyfile (dest), exchangefiles, createdir, createfile
 *
 * @param[in] vol   pointer to struct vol
 * @param[in] did   DID to resolve
 *
 * @returns pointer to validated struct dir, or NULL with afp_errno set
 */
struct dir *dirlookup_strict(const struct vol *vol, cnid_t did)
{
    struct dir *dir;
    struct stat validate_st;
    /* First do normal lookup via public interface */
    dir = dirlookup(vol, did);

    if (dir == NULL) {
        return NULL;
    }

    /* Skip validation for special directories */
    if (dir->d_did == DIRDID_ROOT || dir->d_did == DIRDID_ROOT_PARENT) {
        return dir;
    }

    /* Validate with stat and inode compare to detect rename/move race or
    changes by other users or external systems */
    if (ostat(cfrombstr(dir->d_fullpath), &validate_st, vol_syml_opt(vol)) != 0) {
        LOG(log_info, logtype_afpd,
            "dirlookup_strict(did:%u): path '%s' no longer exists, cleaning cache",
            ntohl(did), cfrombstr(dir->d_fullpath));
        goto stale;
    }

    if (validate_st.st_ino != dir->dcache_ino) {
        LOG(log_info, logtype_afpd,
            "dirlookup_strict(did:%u): inode mismatch (cached:%llu, actual:%llu), path '%s' was renamed/moved",
            ntohl(did), (unsigned long long)dir->dcache_ino,
            (unsigned long long)validate_st.st_ino, cfrombstr(dir->d_fullpath));
        goto stale;
    }

    /* Validation passed */
    return dir;
stale:

    /* Clean stale entries and retry via CNID. dircache_remove_children and
     * dir_remove handle curdir recovery internally, guaranteeing curdir != NULL. */
    if (!(dir->d_flags & DIRF_ISFILE)) {
        (void)dircache_remove_children(vol, dir);
    }

    (void)dir_remove(vol, dir, 0);  /* Proactive cleanup before retry */
    /* Retry - will now miss cache and query CNID */
    dir = dirlookup(vol, did);

    if (dir == NULL) {
        LOG(log_debug, logtype_afpd,
            "dirlookup_strict(did:%u): CNID retry failed", ntohl(did));
    }

    return dir;
}

/*!
 * @brief Construct struct dir
 *
 * Construct struct dir from parameters.
 *
 * @param[in] m_name   directory name in UTF8-dec
 * @param[in] u_name   directory name in server side encoding
 * @param[in] vol      pointer to struct vol
 * @param[in] pdid     Parent CNID
 * @param[in] did      CNID
 * @param[in] path     Full unix path to object
 * @param[in] st       struct stat of object
 *
 * @returns pointer to new struct dir or NULL on error
 *
 * @note Most of the time mac name and unix name are the same.
 */
struct dir *dir_new(const char *m_name,
                    const char *u_name,
                    const struct vol *vol,
                    cnid_t pdid,
                    cnid_t did,
                    bstring path,
                    struct stat *st)
{
    struct dir *dir;
    dir = (struct dir *) calloc(1, sizeof(struct dir));

    if (!dir) {
        return NULL;
    }

    if ((dir->d_m_name = bfromcstr(m_name)) == NULL) {
        free(dir);
        return NULL;
    }

    if (convert_string_allocate((utf8_encoding(vol->v_obj)) ? CH_UTF8_MAC :
                                vol->v_maccharset,
                                CH_UCS2,
                                m_name,
                                -1, (char **)&dir->d_m_name_ucs2) == (size_t) -1) {
        LOG(log_error, logtype_afpd,
            "dir_new(did: %u) {%s, %s}: couldn't set UCS2 name", ntohl(did), m_name,
            u_name);
        dir->d_m_name_ucs2 = NULL;
    }

    if (m_name == u_name || !strcmp(m_name, u_name)) {
        dir->d_u_name = dir->d_m_name;
    } else if ((dir->d_u_name = bfromcstr(u_name)) == NULL) {
        bdestroy(dir->d_m_name);
        free(dir);
        return NULL;
    }

    dir->d_did = did;
    dir->d_pdid = pdid;
    dir->d_vid = vol->v_vid;
    dir->d_fullpath = path;
    dir->dcache_ctime = st->st_ctime;
    dir->dcache_ino = st->st_ino;
    /* Populate additional stat fields for enumerate optimization */
    dir->dcache_mode = st->st_mode;
    dir->dcache_mtime = st->st_mtime;
    dir->dcache_uid = st->st_uid;
    dir->dcache_gid = st->st_gid;
    dir->dcache_size = st->st_size;
    /* Not yet loaded — triggers ad_metadata() on first access */
    dir->dcache_rlen = (off_t) - 1;

    if (!S_ISDIR(st->st_mode)) {
        dir->d_flags = DIRF_ISFILE;
    }

    dir->d_rights_cache = 0xffffffff;
    dir->arc_list =
        0;  /* ARC_NONE (= 0, defined in dircache.c), safe with calloc zero-init */
    return dir;
}

/*!
 * @brief Free a struct dir and all its members
 *
 * @param dir (rw) pointer to struct dir
 */
void dir_free(struct dir *dir)
{
    if (dir->d_u_name != dir->d_m_name) {
        bdestroy(dir->d_u_name);
    }

    if (dir->d_m_name_ucs2) {
        free(dir->d_m_name_ucs2);
    }

    bdestroy(dir->d_m_name);
    bdestroy(dir->d_fullpath);
    free(dir);
}

/*!
 * @brief Update a cached entry in-place with selective field updates
 *
 * Modifies specified field groups on an existing cache entry while
 * maintaining hash table consistency. The DCMOD_* bitmask in
 * args->flags controls which fields are updated — unset groups
 * are not touched.
 *
 * When DCMOD_PATH is set and new_uname or new_pdid differs from current
 * values, the DID/name index is automatically reindexed. The CNID index
 * (d_vid, d_did) is NEVER changed.
 *
 * Always promotes the entry in ARC (signals recency to eviction algorithm).
 *
 * Common call patterns:
 *   Rename:             DCMOD_PATH | DCMOD_STAT
 *   After setfilparams: DCMOD_STAT | DCMOD_AD  (Phase 2)
 *   Stat-only refresh:  DCMOD_STAT
 *
 * @param[in]     vol   Volume (required)
 * @param[in,out] dir   Cache entry to update (required)
 * @param[in]     args  Parameter struct with DCMOD_* flags and field values
 *
 * @returns 0 on success, -1 on error (hash re-insert failure)
 */
int dir_modify(const struct vol *vol, struct dir *dir,
               const struct dir_modify_args *args)
{
    if (!vol) {
        LOG(log_error, logtype_afpd,
            "dir_modify: called with NULL vol, skipping");
        return -1;
    }

    if (!dir) {
        LOG(log_error, logtype_afpd,
            "dir_modify: called with NULL dir, skipping");
        return -1;
    }

    AFP_ASSERT(args);

    /* If entry was evicted (d_did set to CNID_INVALID by dir_remove),
     * return gracefully — the caller's cached pointer is stale. */
    if (dir->d_did == CNID_INVALID) {
        LOG(log_debug, logtype_afpd,
            "dir_modify: entry already evicted (d_did=CNID_INVALID), skipping");
        return 0;
    }

    /* DCMOD_AD and DCMOD_AD_INV are mutually exclusive (R6 D-009) */
    AFP_ASSERT(!(args->flags & DCMOD_AD) || !(args->flags & DCMOD_AD_INV));
    int needs_reindex = 0;

    /* Self-healing: if entry is not in the cache, add it (R5 M-004 / R6 E-018).
     * dir_modify() should always result in an up-to-date cached entry.
     * Skip reserved CNIDs (root_parent, root) */
    if (dir->qidx_node == NULL && ntohl(dir->d_did) >= CNID_START) {
        AFP_ASSERT(dir->d_u_name != NULL);    /* Must be initialized */
        AFP_ASSERT(dir->d_fullpath != NULL);  /* Must be initialized */
        LOG(log_warning, logtype_afpd,
            "dir_modify(did:%u): entry not in cache, adding", ntohl(dir->d_did));

        if (dircache_add(vol, dir) != 0) {
            LOG(log_error, logtype_afpd,
                "dir_modify(did:%u): failed to add to cache", ntohl(dir->d_did));
            return -1;
        }
    }

    LOG(log_debug, logtype_afpd,
        "dir_modify: did:%u flags:0x%x(%s%s%s%s) '%s'",
        ntohl(dir->d_did), args->flags,
        (args->flags & DCMOD_PATH) ? "PATH" : "",
        (args->flags & DCMOD_STAT) ? "|STAT" : "",
        (args->flags & DCMOD_AD) ? "|AD" : "",
        (args->flags & DCMOD_AD_INV) ? "|AD_INV" : "",
        dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");

    /* --- DCMOD_PATH: Update path/name/parent --- */
    if (args->flags & DCMOD_PATH) {
        if (args->new_pdid != 0 && args->new_pdid != dir->d_pdid) {
            needs_reindex = 1;
        }

        if (args->new_uname != NULL
                && strcmp(args->new_uname, cfrombstr(dir->d_u_name)) != 0) {
            needs_reindex = 1;
        }

        LOG(log_debug, logtype_afpd,
            "dir_modify: PATH old pdid:%u uname:'%s' path:'%s' -> "
            "new pdid:%u uname:'%s' reindex:%d",
            ntohl(dir->d_pdid),
            dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)",
            dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)",
            args->new_pdid ? ntohl(args->new_pdid) : ntohl(dir->d_pdid),
            args->new_uname ? args->new_uname
            : (dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)"),
            needs_reindex);

        /* Remove from DID/name index if key is changing */
        if (needs_reindex) {
            dircache_remove(vol, dir, DIDNAME_INDEX);
        }

        if (args->new_pdid != 0) {
            dir->d_pdid = args->new_pdid;
        }

        if (args->new_uname != NULL) {
            const char *mname = args->new_mname ? args->new_mname : args->new_uname;

            if (dir->d_u_name != dir->d_m_name) {
                bdestroy(dir->d_u_name);
            }

            bdestroy(dir->d_m_name);
            dir->d_m_name = bfromcstr(mname);
            dir->d_u_name = (strcmp(mname, args->new_uname) == 0)
                            ? dir->d_m_name : bfromcstr(args->new_uname);

            if (dir->d_m_name_ucs2) {
                free(dir->d_m_name_ucs2);
                dir->d_m_name_ucs2 = NULL;
            }

            if (convert_string_allocate(
                        (utf8_encoding(vol->v_obj)) ? CH_UTF8_MAC : vol->v_maccharset,
                        CH_UCS2, mname, -1,
                        (char **)&dir->d_m_name_ucs2) == (size_t) -1) {
                dir->d_m_name_ucs2 = NULL;
            }
        }

        if (args->new_pdir_path != NULL) {
            const char *uname = args->new_uname ? args->new_uname : cfrombstr(
                                    dir->d_u_name);
            bdestroy(dir->d_fullpath);
            dir->d_fullpath = bstrcpy(args->new_pdir_path);
            bconchar(dir->d_fullpath, '/');
            bcatcstr(dir->d_fullpath, uname);
        }

        /* Re-insert into DID/name index if key changed.
         * On failure, remove the entry entirely to avoid orphans
         * (entry would be in CNID index + queue but not in DIDNAME index). */
        if (needs_reindex) {
            if (dircache_reindex_didname(vol, dir) != 0) {
                LOG(log_error, logtype_afpd,
                    "dir_modify: reindex failed, removing entry did:%u",
                    ntohl(dir->d_did));
                dir_remove(vol, dir, 0);  /* Clean removal from ALL indexes */
                return -1;
            }

            LOG(log_debug, logtype_afpd,
                "dir_modify: PATH reindexed did:%u -> '%s'",
                ntohl(dir->d_did),
                dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
        }
    }

    /* --- DCMOD_STAT: Refresh stat fields --- */
    if ((args->flags & DCMOD_STAT) && args->st) {
        /* Detect external changes before updating cached stat fields */
        bool ino_changed = (dir->dcache_ino != 0 &&
                            dir->dcache_ino != args->st->st_ino);
        bool ctime_changed = (dir->dcache_ctime != 0 &&
                              dir->dcache_ctime != args->st->st_ctime);

        if (ino_changed) {
            /* Different inode = different file. AD is always stale. */
            dir->dcache_rlen = (off_t) -1;
            /* Refresh CNID from database */
            cnid_t new_cnid = CNID_INVALID;
            AFP_CNID_START("cnid_lookup");
            new_cnid = cnid_lookup(vol->v_cdb, args->st, dir->d_pdid,
                                   cfrombstr(dir->d_u_name),
                                   blength(dir->d_u_name));
            AFP_CNID_DONE();

            if (new_cnid == CNID_INVALID) {
                if (dir == curdir) {
                    /* Do not remove curdir — callers rely on it being valid.
                     * AD cache already invalidated (dcache_rlen = -1 above) */
                    LOG(log_error, logtype_afpd,
                        "dir_modify: inode change on curdir! — no CNID for "
                        "new inode, keeping stale entry (did:%u, ino:%llu->%llu)",
                        ntohl(dir->d_did),
                        (unsigned long long)dir->dcache_ino,
                        (unsigned long long)args->st->st_ino);
                } else {
                    /* No CNID. Remove stale entry. */
                    LOG(log_debug, logtype_afpd,
                        "dir_modify: inode change — no CNID for new inode, "
                        "removing stale entry did:%u (ino:%llu->%llu)",
                        ntohl(dir->d_did),
                        (unsigned long long)dir->dcache_ino,
                        (unsigned long long)args->st->st_ino);
                    dir_remove(vol, dir, 0);
                    return 0;
                }
            }

            if (new_cnid != dir->d_did) {
                LOG(log_debug, logtype_afpd,
                    "dir_modify: inode change — CNID refreshed "
                    "did:%u->%u (ino:%llu->%llu)",
                    ntohl(dir->d_did), ntohl(new_cnid),
                    (unsigned long long)dir->dcache_ino,
                    (unsigned long long)args->st->st_ino);
                /* Remove from all indexes, update DID, re-add */
                dircache_remove(vol, dir,
                                DIRCACHE | DIDNAME_INDEX | QUEUE_INDEX);
                dir->d_did = new_cnid;

                if (dircache_add(vol, dir) != 0) {
                    LOG(log_error, logtype_afpd,
                        "dir_modify: DID re-index failed for did:%u",
                        ntohl(new_cnid));
                }
            }
        } else if (ctime_changed && dir->dcache_rlen != (off_t) -1) {
            /* Ctime changed (same inode): metadata modified externally.
             * Invalidate AD cache — re-read from disk on next access.
             * Covers dcache_rlen >= 0 (cached AD) and dcache_rlen == -2 (no AD)
             * When DCMOD_AD is also set, ad_store_to_cache() runs after
             * this section and re-populates dcache_rlen with caller data. */
            LOG(log_debug, logtype_afpd,
                "dir_modify: ctime change — invalidating AD cache "
                "did:%u (ctime:%ld->%ld)",
                ntohl(dir->d_did),
                (long)dir->dcache_ctime, (long)args->st->st_ctime);
            dir->dcache_rlen = (off_t) -1;
        }

        dir->dcache_ctime = args->st->st_ctime;
        dir->dcache_ino = args->st->st_ino;
        dir->dcache_mode = args->st->st_mode;
        dir->dcache_mtime = args->st->st_mtime;
        dir->dcache_uid = args->st->st_uid;
        dir->dcache_gid = args->st->st_gid;
        dir->dcache_size = args->st->st_size;
        LOG(log_debug, logtype_afpd,
            "dir_modify: STAT did:%u ino:%llu mode:0%o size:%lld",
            ntohl(dir->d_did),
            (unsigned long long)dir->dcache_ino,
            (unsigned int)dir->dcache_mode,
            (long long)dir->dcache_size);
    }

    /* --- DCMOD_AD: Refresh AD fields from adouble --- */
    if ((args->flags & DCMOD_AD) && args->adp) {
        /* ad_store_to_cache() overwrites dcache_filedatesi and recomputes
         * served mdate = max(ad_mdate, dcache_mtime) using the just-updated
         * dcache_mtime from DCMOD_STAT above. */
        ad_store_to_cache(args->adp, dir);
    }

    /* Auto-recompute mdate ONLY if DCMOD_AD was NOT also set
     * (ad_store_to_cache already recomputed mdate from fresh AD + stat).
     * This avoids duplicate mdate computation when both flags are set. */
    if ((args->flags & DCMOD_STAT) && args->st
            && !(args->flags & DCMOD_AD) && dir->dcache_rlen >= 0) {
        /* Maintains invariant: dcache_filedatesi[AD_DATE_MODIFY]
         * always contains max(ad_mdate, dcache_mtime) in network byte order. */
        uint32_t ad_mdate;
        memcpy(&ad_mdate, dir->dcache_filedatesi + AD_DATE_MODIFY,
               sizeof(ad_mdate));
        time_t ad_mtime = AD_DATE_TO_UNIX(ad_mdate);

        if (args->st->st_mtime > ad_mtime) {
            uint32_t st_mdate = AD_DATE_FROM_UNIX(args->st->st_mtime);
            memcpy(dir->dcache_filedatesi + AD_DATE_MODIFY,
                   &st_mdate, sizeof(st_mdate));
        }
    }

    /* --- DCMOD_AD_INV: Invalidate AD cache --- */
    if (args->flags & DCMOD_AD_INV) {
        dir->dcache_rlen = (off_t) -1;
        memset(dir->dcache_finderinfo, 0, 32);
        memset(dir->dcache_filedatesi, 0, 16);
        memset(dir->dcache_afpfilei, 0, 4);
    }

    /* Promote in ARC — entry is actively being used.
     * DCMOD_NO_PROMOTE skips promotion for cross-process hints so that
     * User1's write activity does not promote entries in User2's cache.
     * LRU mode: no-op (LRU is FIFO by design). */
    if (!(args->flags & DCMOD_NO_PROMOTE)) {
        dircache_promote(dir);
    }

    LOG(log_debug, logtype_afpd,
        "dir_modify: OK did:%u -> '%s'",
        ntohl(dir->d_did),
        dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
    return 0;
}

/*!
 * @brief Create struct dir from struct path
 *
 * Create a new struct dir from struct path. Then add it to the cache.
 *
 * 1. Open adouble file, get CNID from it.
 * 2. Search the database, hinting with the CNID from (1).
 * 3. Build fullpath and create struct dir.
 * 4. Add it to the cache.
 *
 * @param[in] vol       pointer to struct vol, possibly modified in callee
 * @param[in] dir       pointer to parent directory
 * @param[in,out] path  pointer to struct path with valid path->u_name
 * @param[in] len       strlen of path->u_name
 *
 * @returns Pointer to new struct dir or NULL on error.
 *
 * @note Function also assigns path->m_name from path->u_name.
 */
struct dir *dir_add(struct vol *vol, const struct dir *dir, struct path *path,
                    int len)
{
    int err = 0;
    struct dir  *cdir = NULL;
    cnid_t      id;
    struct adouble  ad;
    struct adouble *adp = NULL;
    bstring fullpath = NULL;
    AFP_ASSERT(vol);
    AFP_ASSERT(dir);
    AFP_ASSERT(path);
    AFP_ASSERT(len > 0);
    cdir = NULL;

    if (path->u_name != NULL) {
        size_t uname_len = strnlen(path->u_name, CNID_MAX_PATH_LEN);
        cdir = dircache_search_by_name(vol, dir, path->u_name, uname_len);
    }

    if (cdir != NULL) {
        /* there's a stray entry in the dircache */
        LOG(log_debug, logtype_afpd,
            "dir_add(did:%u,'%s/%s'): {stray cache entry: did:%u,'%s', removing}",
            ntohl(dir->d_did), cfrombstr(dir->d_fullpath), path->u_name,
            ntohl(cdir->d_did), cfrombstr(dir->d_fullpath));
        int remove_result = dir_remove(vol, cdir, 0);  /* Proactive cleanup of stray */

        if (remove_result != 0) {
            LOG(log_error, logtype_afpd,
                "dir_add: CRITICAL dir_remove failed (returned -1), curdir recovery failed but curdir fallback successful");
            dircache_dump();
            AFP_PANIC("dir_add");
        }
    }

    /* get_id needs adp for reading CNID from adouble file */
    ad_init(&ad, vol);

    if ((ad_open(&ad, path->u_name,
                 ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDONLY)) == 0) { /* 1 */
        adp = &ad;
    }

    /* Get CNID */
    if ((id = get_id(vol, adp, &path->st, dir->d_did, path->u_name,
                     len)) == CNID_INVALID) { /* 2 */
        err = 1;
        goto exit;
    }

    if (adp) {
        ad_close(adp, ADFLAGS_HF);
    }

    /* Get macname from unixname */
    if (path->m_name == NULL) {
        if ((path->m_name = utompath(vol, path->u_name, id,
                                     utf8_encoding(vol->v_obj))) == NULL) {
            LOG(log_error, logtype_afpd, "dir_add(\"%s\"): can't assign macname",
                path->u_name);
            err = 2;
            goto exit;
        }
    }

    /* Build fullpath */
    if (((fullpath = bstrcpy(dir->d_fullpath)) == NULL)  /* 3 */
            || (bconchar(fullpath, '/') != BSTR_OK)
            || (bcatcstr(fullpath, path->u_name)) != BSTR_OK) {
        LOG(log_error, logtype_afpd, "dir_add: fullpath: %s", strerror(errno));
        err = 3;
        goto exit;
    }

    /* Allocate and initialize struct dir */
    if ((cdir = dir_new(path->m_name,
                        path->u_name,
                        vol,
                        dir->d_did,
                        id,
                        fullpath,
                        &path->st)) == NULL) { /* 3 */
        err = 4;
        goto exit;
    }

    if ((dircache_add(vol, cdir)) != 0) { /* 4 */
        LOG(log_error, logtype_afpd, "dir_add: fatal dircache error: %s",
            cfrombstr(fullpath));
        exit(EXITERR_SYS);
    }

exit:

    if (err != 0) {
        LOG(log_debug, logtype_afpd, "dir_add('%s/%s'): error: %u",
            dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)", path->u_name, err);

        if (adp) {
            ad_close(adp, ADFLAGS_HF);
        }

        if (!cdir && fullpath) {
            bdestroy(fullpath);
        }

        if (cdir) {
            dir_free(cdir);
        }

        cdir = NULL;
    } else {
        /* no error */
        LOG(log_debug, logtype_afpd, "dir_add(did:%u,'%s/%s'): {cached: %u,'%s'}",
            ntohl(dir->d_did), cfrombstr(dir->d_fullpath), path->u_name,
            ntohl(cdir->d_did), cfrombstr(cdir->d_fullpath));
    }

    return cdir;
}

/*!
 * @brief Free the queue with invalid struct dirs
 *
 * @note This gets called at the end of every AFP func.
 */
void dir_free_invalid_q(void)
{
    struct dir *dir;

    while ((dir = (struct dir *)dequeue(invalid_dircache_entries))) {
        dir_free(dir);
    }
}

/*!
 * @brief Remove a file/directory from dircache with automatic curdir recovery
 *
 * This function centralizes global curdir safety for all callers.
 * When removing a cache entry that is curdir, it attempts curdir recovery via
 * CNID database and falls back to volume root if recovery fails.
 *
 * 1. Check the dir
 * 2. Detect if removing curdir and save DID
 * 3. Remove from cache and queue for deallocation
 * 4. Set curdir=NULL if removing curdir (safer than dangling pointer)
 * 5. Attempt recovery via dirlookup(saved_did)
 * 6. If recovery fails, fallback to vol->v_root or rootParent
 * 7. Mark entry invalid
 *
 * @param[in] vol            volume pointer
 * @param[in,out] dir        directory/file entry to remove from cache
 * @param[in] report_invalid 1 = report as invalid_on_use (entry was used and found stale),
 *                           0 = don't report (proactive cleanup, user deletion, etc.)
 * @returns 0 on success, -1 if curdir was removed and recovery failed
 *          (curdir guaranteed non-NULL on return)
 */
int dir_remove(const struct vol *vol, struct dir *dir, int report_invalid)
{
    AFP_ASSERT(vol);
    AFP_ASSERT(dir);

    if (dir->d_did == DIRDID_ROOT_PARENT || dir->d_did == DIRDID_ROOT
            || dir->d_did == CNID_INVALID) { /* 1 */
        return 0;
    }

    LOG(log_debug, logtype_afpd,
        "dir_remove(did:%u,'%s'): {removing from cache, report_invalid=%d}",
        ntohl(dir->d_did),
        dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)",
        report_invalid);
    /* Save curdir DID BEFORE removal - needed for recovery if dir==curdir */
    cnid_t saved_curdir_did = CNID_INVALID;
    bool removing_curdir = false;

    if (curdir == dir && !(dir->d_flags & DIRF_ISFILE)) {
        /* We're about to null curdir - save DID for recovery attempt */
        saved_curdir_did = curdir->d_did;
        removing_curdir = true;
    }

    /* Report invalid_on_use ONLY if this entry was returned to caller and found stale during use */
    if (report_invalid) {
        dircache_report_invalid_entry(dir);
    }

    /* Remove the dircache entry */
    dircache_remove(vol, dir, DIRCACHE | DIDNAME_INDEX | QUEUE_INDEX); /* 2 */
    /* Queue pruned entry for memory deallocation */
    enqueue(invalid_dircache_entries, dir); /* 3 */

    /* Only null out curdir if we're removing a directory */
    if (curdir == dir && !(dir->d_flags & DIRF_ISFILE)) { /* 4 */
        curdir = NULL;
    }

    /* Mark invalid */
    dir->d_did = CNID_INVALID;              /* 5 */

    /* Attempt curdir recovery if we just removed it.
     * Returns error if recovery fails so caller knows curdir is invalid. */
    if (removing_curdir && !curdir) {
        LOG(log_debug, logtype_afpd,
            "dir_remove(did:%u): curdir was removed, attempting recovery",
            ntohl(saved_curdir_did));
        /* Try to recover via CNID - succeeds if directory still exists */
        struct dir *recovered = dirlookup(vol, saved_curdir_did);

        if (recovered) {
            curdir = recovered;
            LOG(log_debug, logtype_afpd,
                "dir_remove: curdir recovery SUCCESS (did:%u), path: \"%s\"",
                ntohl(saved_curdir_did),
                curdir->d_fullpath ? cfrombstr(curdir->d_fullpath) : "(null)");
            /* Success - curdir recovered */
            return 0;
        }

        /* Recovery via CNID failed - check why */
        if (afp_errno == AFPERR_NOOBJ) {
            /* Directory was deleted - this is normal during cleanup, not an error */
            LOG(log_debug, logtype_afpd,
                "dir_remove: curdir (did:%u) was deleted, trying parent fallback",
                ntohl(saved_curdir_did));
        } else {
            /* Other error during recovery */
            LOG(log_debug, logtype_afpd,
                "dir_remove: curdir recovery failed (did:%u), afp_errno=%d (%s)",
                ntohl(saved_curdir_did), afp_errno, AfpErr2name(afp_errno));
        }

        /* Fallback hierarchy: Try parent first, otherwise volume root */
        struct dir *fallback = dirlookup_strict(vol, dir->d_pdid);

        if (fallback && fallback != dir) {
            /* Parent exists - use it */
            curdir = fallback;
            LOG(log_debug, logtype_afpd,
                "dir_remove: fallback to validated parent (did:%u)",
                ntohl(curdir->d_did));
            return 0;
        }

        /* Last resort, use volume root */
        AFP_ASSERT(vol->v_root);
        curdir = vol->v_root;
        LOG(log_debug, logtype_afpd,
            "dir_remove: fallback to volume root (did:%u)",
            ntohl(curdir->d_did));
        return 0;  /* Fallback successful, not an error */
    }

    /* Success - curdir unchanged or not affected */
    return 0;
}

/*!
 * @brief Resolve a catalog node name path
 *
 * 1. Evaluate path type
 * 2. Move to start dir, if we can't, it might be e.g. because of EACCES, build
 *    path from dirname, so e.g. getdirparams has sth it can chew on. curdir
 *    is dir parent then. All this is done in path_from_dir().
 * 3. Parse next cnode name in path, cases:
 * 4.   single "\0" -> do nothing
 * 5.   two or more consecutive "\0" -> chdir("..") one or more times
 * 6.   cnode name -> copy it to path.m_name
 * 7. Get unix name from mac name
 * 8. Special handling of request with did 1
 * 9. stat the cnode name
 * 10. If it's not there, it's probably an afp_createfile|dir,
 *     return with curdir = dir parent, struct path = dirname
 * 11. If it's there and it's a file, it must should be the last element of the requested
 *     path. Return with curdir = cnode name parent dir, struct path = filename
 * 12. Treat symlinks like files, don't follow them
 * 13. If it's a dir:
 * 14. Search the dircache for it
 * 15. If it's not in the cache, create a struct dir for it and add it to the cache
 * 16. chdir into the dir and
 * 17. set m_name to the mac equivalent of "."
 * 18. goto 3
 */
struct path *cname(struct vol *vol, struct dir *dir, char **cpath)
{
    static char        path[MAXPATHLEN + 1];
    static struct path ret;
    struct dir  *cdir;
    char        *data, *p;
    int         len;
    uint32_t   hint;
    uint16_t   len16;
    int         size = 0;
    int         toUTF8 = 0;
    /* Save original len before while loop modifies it */
    int         original_len = 0;
    LOG(log_maxdebug, logtype_afpd, "cname('%s'): {start}",
        dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
    data = *cpath;
    afp_errno = AFPERR_NOOBJ;
    memset(&ret, 0, sizeof(ret));
    ret.m_type = *data;

    /* 1 */
    switch (ret.m_type) {
    case 2:
        data++;
        len = (unsigned char) * data++;
        size = 2;

        if (vol->v_obj->afp_version >= 30) {
            ret.m_type = 3;
            toUTF8 = 1;
        }

        break;

    case 3:
        if (vol->v_obj->afp_version >= 30) {
            data++;
            memcpy(&hint, data, sizeof(hint));
            hint = ntohl(hint);
            data += sizeof(hint);
            memcpy(&len16, data, sizeof(len16));
            len = ntohs(len16);
            data += 2;
            size = 7;
            break;
        }

    /* else it's an error */
    default:
        afp_errno = AFPERR_PARAM;
        return NULL;
    }

    *cpath += len + size;
    path[0] = 0;
    ret.m_name = path;
    /* Save before while loop modifies it */
    original_len = len;

    if (movecwd(vol, dir) < 0) {
        LOG(log_debug, logtype_afpd, "cname(did:%u): failed to chdir to '%s'",
            ntohl(dir->d_did), dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");

        if (len == 0) {
            return path_from_dir(vol, dir, &ret);
        } else {
            return NULL;
        }
    }

    /* After movecwd(), use curdir for dir. movecwd() may invalidate dir during stale
     * entry cleaning (dir_remove sets d_did=0). movecwd always updates curdir. */
    dir = curdir;

    /* 3 */
    while (len) {
        /* 4 or 5 */
        if (*data == 0) {
            data++;
            len--;

            /* 5 */
            while (len > 0 && *data == 0) {
                /* Check if we're at volume root - can't navigate higher */
                if (dir->d_did == DIRDID_ROOT) {
                    LOG(log_info, logtype_afpd,
                        "cname: rejecting '..' navigation above volume root");
                    afp_errno = AFPERR_PARAM;
                    return NULL;
                }

                /* chdir to parrent dir */
                if ((dir = dirlookup(vol, dir->d_pdid)) == NULL) {
                    return NULL;
                }

                if (movecwd(vol, dir) < 0) {
                    dir_remove(vol, dir, 0);  /* Cleanup after movecwd failure */
                    return NULL;
                }

                /* Update dir after movecwd (may invalidate during stale cleanup) */
                dir = curdir;
                data++;
                len--;
            }

            continue;
        }

        /* 6 */
        for (p = path; *data != 0 && len > 0; len--) {
            *p++ = *data++;

            /* FIXME safeguard, limit of early Mac OS X */
            if (p > &path[UTF8FILELEN_EARLY]) {
                afp_errno = AFPERR_PARAM;
                return NULL;
            }
        }

        /* Terminate string */
        *p = 0;
        ret.u_name = NULL;

        /* 7 */
        if (cname_mtouname(vol, dir, &ret, toUTF8) != 0) {
            LOG(log_error, logtype_afpd, "cname('%s'): error from cname_mtouname", path);
            return NULL;
        }

        LOG(log_maxdebug, logtype_afpd, "cname('%s'): {node: '%s}",
            dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)", ret.u_name);

        /* Prevent access to our special folders like .AppleDouble */
        if (check_name(vol, ret.u_name)) {
            /* the name is illegal */
            LOG(log_info, logtype_afpd, "cname: illegal path: '%s'", ret.u_name);
            afp_errno = AFPERR_PARAM;

            if (vol->v_obj->options.flags & OPTION_VETOMSG) {
                bstring message =
                    bformat("Attempt to access vetoed file or directory \"%s\" in directory \"%s\"",
                            ret.u_name, bdata(dir->d_u_name));

                /* Client may make multiple attempts, only send the message the first time */
                if (setmessage(bdata(message)) == 0) {
                    kill(getpid(), SIGUSR2);
                }

                bdestroy(message);
            }

            return NULL;
        }

        /* 8 */
        if (dir->d_did == DIRDID_ROOT_PARENT) {
            /*
             * Special case: CNID 1
             * root parent (did 1) has one child: the volume. Requests for did=1 with
             * some <name> must check against the volume name.
             */
            if ((strcmp(cfrombstr(vol->v_root->d_m_name), ret.m_name)) == 0) {
                cdir = vol->v_root;
            } else {
                return NULL;
            }
        } else {
            /*
             * CNID != 1, e.g. most of the times we take this way.
             * Now check if current path-part is a file or dir:
             * o if it's dir we have to step into it
             * o if it's a file we expect it to be the last part of the requested path
             *   and thus call continue which should terminate the while loop because
             *   len = 0. Ok?
             */
            if (of_stat(vol, &ret) != 0) { /* 9 */
                /*
                 * ret.u_name doesn't exist, might be afp_createfile|dir
                 * that means it should have been the last part
                 */
                if (len > 0) {
                    /* it wasn't the last part, so we have a bogus path request */
                    afp_errno = AFPERR_NOOBJ;
                    return NULL;
                }

                /*
                 * this will terminate clean in while (1) because len == 0,
                 * probably afp_createfile|dir
                 */
                LOG(log_maxdebug, logtype_afpd,
                    "cname('%s'): {leave-cnode ENOENT (probably create request): '%s'}",
                    cfrombstr(dir->d_fullpath), ret.u_name);
                continue; /* 10 */
            }

            switch (ret.st.st_mode & S_IFMT) {
            /* 11 */
            case S_IFREG:
                LOG(log_debug, logtype_afpd, "cname('%s'): {file: '%s'}",
                    dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)", ret.u_name);

                if (len > 0) {
                    /* it wasn't the last part, so we have a bogus path request */
                    afp_errno = AFPERR_PARAM;
                    return NULL;
                }

                /* continues while loop */
                continue;

            /* 12 */
            case S_IFLNK:
                LOG(log_debug, logtype_afpd, "cname('%s'): {link: '%s'}",
                    dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)", ret.u_name);

                if (len > 0) {
                    LOG(log_warning, logtype_afpd, "cname('%s'): {symlinked dir: '%s'}",
                        dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)", ret.u_name);
                    afp_errno = AFPERR_PARAM;
                    return NULL;
                }

                /* continues while loop */
                continue;

            /* 13 */
            case S_IFDIR:
                break;

            default:
                LOG(log_info, logtype_afpd, "cname: special file: '%s'", ret.u_name);
                afp_errno = AFPERR_NODIR;
                return NULL;
            }

            /* Search the cache */
            size_t unamelen = strnlen(ret.u_name, CNID_MAX_PATH_LEN);
            /* 14 */
            cdir = dircache_search_by_name(vol, dir, ret.u_name, unamelen);

            if (cdir == NULL) {
                /* Not in cache, create one */
                /* 15 */
                cdir = dir_add(vol, dir, &ret, (int)unamelen);

                if (cdir == NULL) {
                    LOG(log_error, logtype_afpd,
                        "cname(did:%u, name:'%s', cwd:'%s'): failed to add dir",
                        ntohl(dir->d_did), ret.u_name ? ret.u_name : "(null)", getcwdpath());
                    return NULL;
                }
            }
        } /* if/else cnid==1 */

        /* Now chdir to the evaluated dir */
        /* 16 */
        if (movecwd(vol, cdir) < 0) {
            LOG(log_debug, logtype_afpd,
                "cname(cwd:'%s'): failed to chdir to new subdir '%s': %s",
                (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "(null)",
                cdir->d_fullpath ? cfrombstr(cdir->d_fullpath) : "(null)", strerror(errno));

            if (len == 0) {
                return path_from_dir(vol, cdir, &ret);
            } else {
                return NULL;
            }
        }

        /* Use curdir instead of cdir - movecwd() updates curdir (including after self-healing).
         * cdir pointer may be freed during self-healing, so using it after movecwd() is unsafe. */
        dir = curdir;
        /* 17, so we later know last token was a dir */
        ret.m_name[0] = 0;
    } /* while (len) */

    if (curdir->d_did == DIRDID_ROOT_PARENT) {
        afp_errno = AFPERR_DID1;
        return NULL;
    }

    LOG(log_debug, logtype_afpd,
        "cname: end check: ret.m_name[0]=%d, original_len=%d, len=%d",
        (int)ret.m_name[0], original_len, len);

    if (ret.m_name[0] == 0) {
        /* Last part was a dir */
        /* Force "." into a useable static buffer */
        ret.u_name = mtoupath(vol, ret.m_name, 0, 1);
        /* Use curdir (updated by movecwd, handles self-healing) */
        ret.d_dir = curdir;
    } else if (original_len == 0) {
        /* Special case: original_len was 0 from start (statting directory itself via empty path).
         * ret.m_name[0] is not 0 because the assignment in the while loop never executed.
         * But we're in the right directory after movecwd() (which may have self-healed).
         * Set up return structure to stat current directory. */
        /* Mark as directory */
        ret.m_name[0] = 0;
        /* "." */
        ret.u_name = mtoupath(vol, ret.m_name, 0, 1);
        /* Use curdir (updated by movecwd, includes healing) */
        ret.d_dir = curdir;
        LOG(log_info, logtype_afpd,
            "cname: len==0 fix applied, ret.d_dir set to did:%u path:\"%s\"",
            curdir ? ntohl(curdir->d_did) : 0,
            curdir ? (curdir->d_fullpath ? cfrombstr(curdir->d_fullpath) : "(null)") :
            "NULL");
    }

    LOG(log_debug, logtype_afpd, "cname('%s') {end: curdir:'%s', path:'%s'}",
        dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)",
        (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "(null)",
        ret.u_name ? ret.u_name : "(null)");
    return &ret;
}

/*!
 * @brief chdir() to dir
 *
 * @param[in] vol   pointer to struct vol
 * @param[in] dir   pointer to struct dir
 *
 * @returns 0 on success, -1 on error with afp_errno set appropriately
 */
int movecwd(const struct vol *vol, struct dir *dir)
{
    int ret;
    cnid_t saved_did;

    /* Defensive NULL checks - return error instead of asserting */
    if (vol == NULL || dir == NULL) {
        LOG(log_error, logtype_afpd, "movecwd: NULL parameter (vol:%p, dir:%p)",
            (void*)vol, (void*)dir);
        afp_errno = AFPERR_PARAM;
        return -1;
    }

    LOG(log_maxdebug, logtype_afpd, "movecwd: from: curdir:\"%s\", cwd:\"%s\"",
        (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "INVALID",
        getcwdpath());

    if (dir->d_did == DIRDID_ROOT_PARENT) {
        curdir = &rootParent;
        return 0;
    }

    /* Save DID before dir_remove() frees dir (if dir stale) */
    saved_did = dir->d_did;
    LOG(log_debug, logtype_afpd, "movecwd(to: did: %u, \"%s\")",
        ntohl(dir->d_did), dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");

    if ((ret = ochdir(cfrombstr(dir->d_fullpath), vol_syml_opt(vol))) != 0) {
        /* Failed to change directory */
        LOG(log_debug, logtype_afpd, "movecwd(\"%s\"): %s",
            dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)", strerror(errno));

        /* Self-heal stale dircache entries when chdir fails */
        if (ret != 1 && errno == ENOENT) {
            LOG(log_debug, logtype_afpd,
                "movecwd: stale dircache entry, cleaning and retrying via CNID");

            /* Clean cache - dircache_remove_children and dir_remove handle curdir recovery */
            if (!(dir->d_flags & DIRF_ISFILE)) {
                (void)dircache_remove_children(vol, dir);
            }

            (void)dir_remove(vol, dir, 0);  /* Self-healing cleanup */
            /* Both functions guarantee curdir != NULL (recovered or vol->v_root/rootParent) */

            /* Retry via CNID with strict validation */
            if ((dir = dirlookup_strict(vol, saved_did)) != NULL) {
                LOG(log_debug, logtype_afpd,
                    "movecwd: dirlookup_strict returned validated path \"%s\", attempting chdir",
                    dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
                ret = ochdir(cfrombstr(dir->d_fullpath), vol_syml_opt(vol));

                if (ret == 0) {
                    curdir = dir;
                    LOG(log_debug, logtype_afpd,
                        "movecwd: self-healing SUCCESS to did:%u", ntohl(saved_did));
                    return 0;
                }

                LOG(log_info, logtype_afpd,
                    "movecwd: CNID retry chdir failed: %s", strerror(errno));
            } else {
                LOG(log_debug, logtype_afpd,
                    "movecwd: dirlookup_strict returned NULL for did:%u (does not exist)",
                    ntohl(saved_did));
            }

            /* Special movecwd requirement: sync process CWD with curdir pointer.
             * If curdir is at fallback, attempt to sync process CWD. */
            if (curdir && curdir->d_fullpath && curdir != &rootParent &&
                    ochdir(cfrombstr(curdir->d_fullpath), vol_syml_opt(vol)) == 0) {
                LOG(log_debug, logtype_afpd,
                    "movecwd: synced process CWD to curdir fallback");
            }
        }

        if (ret == 1) {
            /* p is a symlink or getcwd failed */
            afp_errno = AFPERR_BADTYPE;

            if (chdir(vol->v_path) < 0) {
                LOG(log_error, logtype_afpd, "can't chdir back'%s': %s", vol->v_path,
                    strerror(errno));
                /* XXX what do we do here? */
            }

            curdir = vol->v_root;
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

        return -1;
    }

    curdir = dir;
    return 0;
}

/*!
 * We can't use unix file's perm to support Apple's inherited protection modes.
 * If we aren't the file's owner we can't change its perms when moving it and smb
 * nfs,... don't even try.
 */
int check_access(const AFPObj *obj, struct vol *vol, char *path, int mode)
{
    struct maccess ma;
    char *p;
    p = ad_dir(path);

    if (!p) {
        return -1;
    }

    accessmode(obj, vol, p, &ma, curdir, NULL);

    if ((mode & OPENACC_WR) && !(ma.ma_user & AR_UWRITE)) {
        return -1;
    }

    if ((mode & OPENACC_RD) && !(ma.ma_user & AR_UREAD)) {
        return -1;
    }

    return 0;
}

/* --------------------- */
int file_access(const AFPObj *obj, struct vol *vol, struct path *path, int mode)
{
    struct maccess ma;
    accessmode(obj, vol, path->u_name, &ma, curdir, &path->st);
    LOG(log_debug, logtype_afpd, "file_access(\"%s\"): mapped user mode: 0x%02x",
        path->u_name, ma.ma_user);

    if ((mode & OPENACC_WR) && !(ma.ma_user & AR_UWRITE)) {
        LOG(log_debug, logtype_afpd, "file_access(\"%s\"): write access denied",
            path->u_name);
        return -1;
    }

    if ((mode & OPENACC_RD) && !(ma.ma_user & AR_UREAD)) {
        LOG(log_debug, logtype_afpd, "file_access(\"%s\"): read access denied",
            path->u_name);
        return -1;
    }

    return 0;
}

/* --------------------- */
void setdiroffcnt(struct dir *dir, struct stat *st,  uint32_t count)
{
    dir->d_offcnt = count;
    dir->d_ctime = st->st_ctime;
    dir->d_flags &= ~DIRF_CNID;
}


/* ---------------------
 * is our cached also for reenumerate id?
 */
int dirreenumerate(struct dir *dir, struct stat *st)
{
    return st->st_ctime == dir->d_ctime && (dir->d_flags & DIRF_CNID);
}

/* ------------------------------
   (".", curdir)
   (name, dir) with curdir:name == dir, from afp_enumerate
*/

int getdirparams(const AFPObj *obj,
                 const struct vol *vol,
                 uint16_t bitmap, struct path *s_path,
                 struct dir *dir,
                 char *buf, size_t *buflen)
{
    struct maccess  ma;
    struct adouble  ad;
    char        *data, *l_nameoff = NULL, *utf_nameoff = NULL;
    char        *ade = NULL;
    int         bit = 0, ad_available = 0;
    uint32_t           aint;
    uint16_t       ashort;
    int                 ret;
    uint32_t           utf8 = 0;
    cnid_t              pdid;
    struct stat *st = &s_path->st;
    char *upath = s_path->u_name;

    if (bitmap & ((1 << DIRPBIT_ATTR)  |
                  (1 << DIRPBIT_CDATE) |
                  (1 << DIRPBIT_MDATE) |
                  (1 << DIRPBIT_BDATE) |
                  (1 << DIRPBIT_FINFO))) {
        ad_init(&ad, vol);

        if (!ad_metadata_cached(upath, ADFLAGS_DIR, &ad, vol, dir, false,
                                (s_path->st_errno == 0) ? &s_path->st : NULL)) {
            ad_available = 1;
            /* The ad_metadata_cached function handles ad_close() internally. */
        }
    }

    pdid = dir->d_pdid;
    data = buf;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {
        case DIRPBIT_ATTR :
            if (ad_available) {
                ad_getattr(&ad, &ashort);
            } else if (invisible_dots(vol, cfrombstr(dir->d_u_name))) {
                ashort = htons(ATTRBIT_INVISIBLE);
            } else {
                ashort = 0;
            }

            ashort &= ~htons(vol->v_ignattr);
            memcpy(data, &ashort, sizeof(ashort));
            data += sizeof(ashort);
            break;

        case DIRPBIT_PDID :
            memcpy(data, &pdid, sizeof(pdid));
            data += sizeof(pdid);
            LOG(log_debug, logtype_afpd, "metadata('%s'):     Parent DID: %u",
                s_path->u_name, ntohl(pdid));
            break;

        case DIRPBIT_CDATE :
            if (!ad_available || (ad_getdate(&ad, AD_DATE_CREATE, &aint) < 0)) {
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            }

            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case DIRPBIT_MDATE :
            aint = AD_DATE_FROM_UNIX(st->st_mtime);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case DIRPBIT_BDATE :
            if (!ad_available || (ad_getdate(&ad, AD_DATE_BACKUP, &aint) < 0)) {
                aint = AD_DATE_START;
            }

            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case DIRPBIT_FINFO :
            if (ad_available && (ade = ad_entry(&ad, ADEID_FINDERI)) != NULL) {
                memcpy(data, ade, 32);
            } else { /* no appledouble */
                memset(data, 0, 32);

                /* dot files are by default visible */
                if (invisible_dots(vol, cfrombstr(dir->d_u_name))) {
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
                }
            }

            data += 32;
            break;

        case DIRPBIT_LNAME :

            /* root of parent can have a null name */
            if (dir->d_m_name) {
                l_nameoff = data;
            } else {
                memset(data, 0, sizeof(uint16_t));
            }

            data += sizeof(uint16_t);
            break;

        case DIRPBIT_SNAME :
            memset(data, 0, sizeof(uint16_t));
            data += sizeof(uint16_t);
            break;

        case DIRPBIT_DID :
            memcpy(data, &dir->d_did, sizeof(aint));
            data += sizeof(aint);
            LOG(log_debug, logtype_afpd, "metadata('%s'):            DID: %u",
                s_path->u_name, ntohl(dir->d_did));
            break;

        case DIRPBIT_OFFCNT :
            ashort = 0;

            /* this needs to handle current directory access rights */
            if (diroffcnt(dir, st)) {
                ashort = (dir->d_offcnt > 0xffff) ? 0xffff : dir->d_offcnt;
            } else if ((ret = for_each_dirent(vol, upath, NULL, NULL)) >= 0) {
                setdiroffcnt(dir, st,  ret);
                ashort = (dir->d_offcnt > 0xffff) ? 0xffff : dir->d_offcnt;
            }

            ashort = htons(ashort);
            memcpy(data, &ashort, sizeof(ashort));
            data += sizeof(ashort);
            break;

        case DIRPBIT_UID :
            aint = htonl(st->st_uid);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case DIRPBIT_GID :
            aint = htonl(st->st_gid);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case DIRPBIT_ACCESS :
            accessmode(obj, vol, upath, &ma, dir, st);
            *data++ = ma.ma_user;
            *data++ = ma.ma_world;
            *data++ = ma.ma_group;
            *data++ = ma.ma_owner;
            break;

        /* Client has requested the ProDOS information block.
           Just pass back the same basic block for all
           directories. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :
            if (obj->afp_version >= 30) {
                /* UTF8 name */
                utf8 = kTextEncodingUTF8;

                /* root of parent can have a null name */
                if (dir->d_m_name) {
                    utf_nameoff = data;
                } else {
                    memset(data, 0, sizeof(uint16_t));
                }

                data += sizeof(uint16_t);
                aint = 0;
                memcpy(data, &aint, sizeof(aint));
                data += sizeof(aint);
            } else {
                /* ProDOS Info Block */
                *data++ = 0x0f;
                *data++ = 0;
                ashort = htons(0x0200);
                memcpy(data, &ashort, sizeof(ashort));
                data += sizeof(ashort);
                memset(data, 0, sizeof(ashort));
                data += sizeof(ashort);
            }

            break;

        case DIRPBIT_UNIXPR :
            /* accessmode may change st_mode with ACLs */
            accessmode(obj, vol, upath, &ma, dir, st);
            aint = htonl(st->st_uid);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            aint = htonl(st->st_gid);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            aint = st->st_mode;
            /* Remove SGID, OSX doesn't like it ... */
            aint = htonl(aint & ~S_ISGID);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            *data++ = ma.ma_user;
            *data++ = ma.ma_world;
            *data++ = ma.ma_group;
            *data++ = ma.ma_owner;
            break;

        default :
            return AFPERR_BITMAP;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

    if (l_nameoff) {
        ashort = htons(data - buf);
        memcpy(l_nameoff, &ashort, sizeof(ashort));
        data = set_name(vol, data, pdid, cfrombstr(dir->d_m_name), dir->d_did, 0);
    }

    if (utf_nameoff) {
        ashort = htons(data - buf);
        memcpy(utf_nameoff, &ashort, sizeof(ashort));
        data = set_name(vol, data, pdid, cfrombstr(dir->d_m_name), dir->d_did, utf8);
    }

    *buflen = data - buf;
    return AFP_OK;
}

/* ----------------------------- */
int path_error(struct path *path, int error)
{
    /* - a dir with access error
     * - no error it's a file
     * - file not found
     */
    if (path_isadir(path)) {
        return afp_errno;
    }

    if (path->st_valid && path->st_errno) {
        return error;
    }

    return AFPERR_BADTYPE ;
}

/* ----------------------------- */
int afp_setdirparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                     char *rbuf _U_, size_t *rbuflen)
{
    struct vol  *vol;
    struct dir  *dir;
    struct path *path;
    uint16_t   vid, bitmap;
    uint32_t   did;
    int     rc;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    if (vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(int);

    if (NULL == (dir = dirlookup_strict(vol, did))) {
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if (NULL == (path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    if (*path->m_name != '\0') {
        rc = path_error(path, AFPERR_NOOBJ);

        /* maybe we are trying to set perms back */
        if (rc != AFPERR_ACCESS) {
            return rc;
        }
    }

    /*
     * If ibuf is odd, make it even.
     */
    if ((intptr_t)ibuf & 1) {
        ibuf++;
    }

    if (AFP_OK == (rc = setdirparams(vol, path, bitmap, ibuf))) {
        setvoltime(obj, vol);
    }

    return rc;
}

/*!
 * @note assume path == '\0' e.g. it's a directory in canonical form
 */
int setdirparams(struct vol *vol, struct path *path, uint16_t d_bitmap,
                 char *buf)
{
    struct maccess  ma;
    struct adouble  ad;
    struct utimbuf      ut;
    struct timeval      tv;
    char                *upath;
    char                *ade = NULL;
    struct dir          *dir;
    int         bit, isad = 0;
    int                 cdate, bdate;
    int                 owner, group;
    uint16_t       ashort, bshort, oshort;
    int                 err = AFP_OK;
    int                 change_mdate = 0;
    int                 change_parent_mdate = 0;
    int                 newdate = 0;
    uint16_t           bitmap = d_bitmap;
    uint8_t              finder_buf[32];
    uint32_t       upriv;
    mode_t              mpriv = 0;
    bool                set_upriv = false, set_maccess = false;
    LOG(log_debug, logtype_afpd, "setdirparams(\"%s\", bitmap: %02x)", path->u_name,
        bitmap);
    bit = 0;
    upath = path->u_name;
    dir   = path->d_dir;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {
        case DIRPBIT_ATTR :
            change_mdate = 1;
            memcpy(&ashort, buf, sizeof(ashort));
            buf += sizeof(ashort);
            break;

        case DIRPBIT_CDATE :
            change_mdate = 1;
            memcpy(&cdate, buf, sizeof(cdate));
            buf += sizeof(cdate);
            break;

        case DIRPBIT_MDATE :
            memcpy(&newdate, buf, sizeof(newdate));
            buf += sizeof(newdate);
            break;

        case DIRPBIT_BDATE :
            change_mdate = 1;
            memcpy(&bdate, buf, sizeof(bdate));
            buf += sizeof(bdate);
            break;

        case DIRPBIT_FINFO :
            change_mdate = 1;
            memcpy(finder_buf, buf, 32);
            buf += 32;
            break;

        case DIRPBIT_UID :
            /* What kind of loser mounts as root? */
            memcpy(&owner, buf, sizeof(owner));
            buf += sizeof(owner);
            break;

        case DIRPBIT_GID :
            memcpy(&group, buf, sizeof(group));
            buf += sizeof(group);
            break;

        case DIRPBIT_ACCESS :
            set_maccess = true;
            change_mdate = 1;
            ma.ma_user = *buf++;
            ma.ma_world = *buf++;
            ma.ma_group = *buf++;
            ma.ma_owner = *buf++;
            mpriv = mtoumode(&ma) | vol->v_dperm;
            break;

        /* Ignore what the client thinks we should do to the
           ProDOS information block.  Skip over the data and
           report nothing amiss. <shirsch@ibm.net> */
        case DIRPBIT_PDINFO :
            if (vol->v_obj->afp_version < 30) {
                buf += 6;
            } else {
                err = AFPERR_BITMAP;
                bitmap = 0;
            }

            break;

        case DIRPBIT_UNIXPR :
            if (vol_unix_priv(vol)) {
                set_upriv = true;
                /* Read owner/group from buffer - used later by setdeskowner/setdirowner */
                memcpy(&owner, buf, sizeof(owner));
                buf += sizeof(owner);
                memcpy(&group, buf, sizeof(group));
                buf += sizeof(group);
                change_mdate = 1;
                memcpy(&upriv, buf, sizeof(upriv));
                buf += sizeof(upriv);
                upriv = ntohl(upriv) | vol->v_dperm;
                break;
            }

        /* fall through */
        default :
            err = AFPERR_BITMAP;
            bitmap = 0;
            break;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

    if (d_bitmap & ((1 << DIRPBIT_ATTR) | (1 << DIRPBIT_CDATE) |
                    (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_FINFO))) {
        ad_init(&ad, vol);

        if (ad_open(&ad, upath, ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_CREATE |
                    ADFLAGS_RDWR, 0777) != 0) {
            LOG(log_debug, logtype_afpd, "setdirparams(\"%s\", bitmap: %02x): need adouble",
                path->u_name, d_bitmap);
            return AFPERR_ACCESS;
        }

        if (ad_get_MD_flags(&ad) & O_CREAT) {
            ad_setname(&ad, cfrombstr(curdir->d_m_name));
        }

        isad = 1;
    }

    bit = 0;
    bitmap = d_bitmap;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {
        case DIRPBIT_ATTR :
            if (isad) {
                ad_getattr(&ad, &bshort);
                oshort = bshort;

                if (ntohs(ashort) & ATTRBIT_SETCLR) {
                    ashort &= ~htons(vol->v_ignattr);
                    bshort |= htons(ntohs(ashort) & ~ATTRBIT_SETCLR);
                } else {
                    bshort &= ~ashort;
                }

                if ((bshort & htons(ATTRBIT_INVISIBLE)) != (oshort & htons(
                            ATTRBIT_INVISIBLE))) {
                    change_parent_mdate = 1;
                }

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
            if (isad && (ade = ad_entry(&ad, ADEID_FINDERI)) != NULL) {
                /* Fixes #2802236 */
                uint16_t fflags;
                memcpy(&fflags, finder_buf + FINDERINFO_FRFLAGOFF, sizeof(uint16_t));
                fflags &= htons(~FINDERINFO_ISHARED);
                memcpy(finder_buf + FINDERINFO_FRFLAGOFF, &fflags, sizeof(uint16_t));

                /* #2802236 end */

                if (dir->d_did == DIRDID_ROOT) {
                    /*
                     * Alright, we admit it, this is *really* sick!
                     * The 4 bytes that we don't copy, when we're dealing
                     * with the root of a volume, are the directory's
                     * location information. This eliminates that annoying
                     * behavior one sees when mounting above another mount
                     * point.
                     */
                    memcpy(ade, finder_buf, 10);
                    memcpy(ade + 14, finder_buf + 14, 18);
                } else {
                    memcpy(ade, finder_buf, 32);
                }
            }

            break;

        /* What kind of loser mounts as root? */
        case DIRPBIT_UID :
            if ((dir->d_did == DIRDID_ROOT) &&
                    (setdeskowner(vol, ntohl(owner), -1) < 0)) {
                err = set_dir_errors(path, "setdeskowner", errno);

                if (isad && err == AFPERR_PARAM) {
                    err = AFP_OK; /* ??? */
                } else {
                    goto setdirparam_done;
                }
            }

            if (setdirowner(vol, upath, ntohl(owner), -1) < 0) {
                err = set_dir_errors(path, "setdirowner", errno);
                goto setdirparam_done;
            }

            break;

        case DIRPBIT_GID :
            if (dir->d_did == DIRDID_ROOT) {
                setdeskowner(vol, -1, ntohl(group));
            }

            if (setdirowner(vol, upath, -1, ntohl(group)) < 0) {
                err = set_dir_errors(path, "setdirowner", errno);
                goto setdirparam_done;
            }

            break;

        case DIRPBIT_ACCESS :
            break;

        case DIRPBIT_PDINFO :
            if (vol->v_obj->afp_version >= 30) {
                err = AFPERR_BITMAP;
                goto setdirparam_done;
            }

            break;

        case DIRPBIT_UNIXPR :
            if (!vol_unix_priv(vol)) {
                err = AFPERR_BITMAP;
                goto setdirparam_done;
            }

            break;

        default :
            err = AFPERR_BITMAP;
            goto setdirparam_done;
            break;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

setdirparam_done:

    if (change_mdate && newdate == 0 && gettimeofday(&tv, NULL) == 0) {
        newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
    }

    if (newdate) {
        if (isad) {
            ad_setdate(&ad, AD_DATE_MODIFY, newdate);
        }

        if (upath) {
            ut.actime = ut.modtime = AD_DATE_TO_UNIX(newdate);
            utime(upath, &ut);
        }
    }

    if (isad) {
        if (path->st_valid && !path->st_errno) {
            struct stat *st = &path->st;

            if (dir && dir->d_pdid) {
                ad_setid(&ad, st->st_dev, st->st_ino,  dir->d_did, dir->d_pdid, vol->v_stamp);
            }
        }

        if (ad_flush(&ad) != 0) {
            switch (errno) {
            case EACCES:
                err = AFPERR_ACCESS;
                break;

            default:
                err = AFPERR_MISC;
                break;
            }
        }

        /* Refresh AD cache after writing metadata (before close) */
        {
            struct stat post_st;

            if (dir && ostat(upath, &post_st, 0) == 0) {
                dir_modify(vol, dir, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT | DCMOD_AD,
                    .st = &post_st,
                    .adp = &ad,
                });
            }
        }
        ad_close(&ad, ADFLAGS_HF);

        /* Send hint to afpd siblings — dir AD metadata changed (FinderInfo, dates) */
        if (dir && dir->d_did != CNID_INVALID) {
            ipc_send_cache_hint(AFPobj, vol->v_vid, dir->d_did, CACHE_HINT_REFRESH);
        }
    }

    if (err == AFP_OK) {
        if (set_maccess == true) {
            if (dir->d_did == DIRDID_ROOT) {
                setdeskmode(vol, mpriv);

                if (!dir_rx_set(mpriv)) {
                    /* we can't remove read and search for owner on volume root */
                    err = AFPERR_ACCESS;
                    goto setprivdone;
                }
            }

            if (setdirunixmode(vol, upath, mpriv) < 0) {
                LOG(log_info, logtype_afpd, "setdirparams(\"%s\"): setdirunixmode: %s",
                    fullpathname(upath), strerror(errno));
                err = set_dir_errors(path, "setdirmode", errno);
            }
        }

        if ((set_upriv == true) && vol_unix_priv(vol)) {
            if (dir->d_did == DIRDID_ROOT) {
                if (!dir_rx_set(upriv)) {
                    /* we can't remove read and search for owner on volume root */
                    err = AFPERR_ACCESS;
                    goto setprivdone;
                }

                setdeskowner(vol, -1, ntohl(group));
                setdeskmode(vol, upriv);
            }

            if (setdirowner(vol, upath, -1, ntohl(group)) < 0) {
                LOG(log_info, logtype_afpd, "setdirparams(\"%s\"): setdirowner: %s",
                    fullpathname(upath), strerror(errno));
                err = set_dir_errors(path, "setdirowner", errno);
                goto setprivdone;
            }

            if (setdirunixmode(vol, upath, upriv) < 0) {
                LOG(log_info, logtype_afpd, "setdirparams(\"%s\"): setdirunixmode: %s",
                    fullpathname(upath), strerror(errno));
                err = set_dir_errors(path, "setdirunixmode", errno);
            }
        }

        /* Send hint to afpd siblings — dir permissions changed (chmod/chown) */
        if ((set_maccess || set_upriv) && dir && dir->d_did != CNID_INVALID) {
            ipc_send_cache_hint(AFPobj, vol->v_vid, dir->d_did, CACHE_HINT_REFRESH);
        }

        /* Update dircache stat fields after permission changes (mode/uid/gid/ctime) */
        if ((set_maccess || set_upriv) && dir) {
            struct stat st;

            if (ostat(upath, &st, vol_syml_opt(vol)) == 0) {
                dir_modify(vol, dir, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT,
                    .st = &st,
                });

                /* Recalculate rights cache based on new permissions */
                if (dir == curdir) {
                    dir->d_rights_cache = 0xffffffff;
                    extern AFPObj *AFPobj;
                    AFP_ASSERT(AFPobj);
                    struct maccess fresh_ma;
                    accessmode(AFPobj, vol, upath, &fresh_ma, dir, &st);
                }
            }
        }
    }

setprivdone:

    if (change_parent_mdate && dir && dir->d_did != DIRDID_ROOT
            && gettimeofday(&tv, NULL) == 0) {
        if (movecwd(vol, dirlookup(vol, dir->d_pdid)) == 0) {
            newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
            /* be careful with bitmap because now dir is null */
            bitmap = 1 << DIRPBIT_MDATE;
            setdirparams(vol, &Cur_Path, bitmap, (char *)&newdate);
            /* should we reset curdir ?*/
        }
    }

    return err;
}

int afp_syncdir(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
                size_t *rbuflen)
{
#ifdef HAVE_DIRFD
    DIR                  *dp;
#endif
    int                  dfd;
    struct vol           *vol;
    struct dir           *dir;
    uint32_t            did;
    uint16_t            vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    /*
     * Here's the deal:
     * if it's CNID 2 our only choice to meet the specs is call sync.
     * For any other CNID just sync that dir. To my knowledge the
     * intended use of FPSyncDir is to sync the volume so all we're
     * ever going to see here is probably CNID 2. Anyway, we' prepared.
     */

    if (ntohl(did) == 2) {
        sync();
    } else {
        if (NULL == (dir = dirlookup(vol, did))) {
            return afp_errno;
        }

        if (movecwd(vol, dir) < 0) {
            return AFPERR_NOOBJ;
        }

        /*
         * Assuming only OSens that have dirfd also may require fsyncing directories
         * in order to flush metadata e.g. Linux.
         */
#ifdef HAVE_DIRFD

        if (NULL == (dp = opendir("."))) {
            switch (errno) {
            case ENOENT :
                return AFPERR_NOOBJ;

            case EACCES :
                return AFPERR_ACCESS;

            default :
                return AFPERR_PARAM;
            }
        }

        LOG(log_debug, logtype_afpd, "afp_syncdir: dir: '%s'",
            dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
        dfd = dirfd(dp);

        if (fsync(dfd) < 0)
            LOG(log_error, logtype_afpd, "afp_syncdir(%s):  %s",
                dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)", strerror(errno));

        /* closes dfd too */
        closedir(dp);
#endif

        if (-1 == (dfd = open(vol->ad_path(".", ADFLAGS_DIR), O_RDWR))) {
            switch (errno) {
            case ENOENT:
                return AFPERR_NOOBJ;

            case EACCES:
                return AFPERR_ACCESS;

            default:
                return AFPERR_PARAM;
            }
        }

        LOG(log_debug, logtype_afpd, "afp_syncdir: ad-file: '%s'",
            vol->ad_path(".", ADFLAGS_DIR));

        if (fsync(dfd) < 0)
            LOG(log_error, logtype_afpd, "afp_syncdir(%s): %s",
                dir->d_u_name ? vol->ad_path(cfrombstr(dir->d_u_name), ADFLAGS_DIR) : "(null)",
                strerror(errno));

        close(dfd);
    }

    return AFP_OK;
}

int afp_createdir(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                  size_t *rbuflen)
{
    struct adouble  ad;
    struct vol      *vol;
    struct dir      *dir;
    char        *upath;
    struct path         *s_path;
    uint32_t       did;
    uint16_t       vid;
    int                 err;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    if (vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup_strict(vol, did))) {
        return afp_errno;
    }

    /* for concurrent access we need to be sure we are not in the
     * folder we want to create...
     */
    if (movecwd(vol, dir) < 0) {
        /* Return error set by movecwd() */
        return afp_errno;
    }

    /* Use movecwd's result (dir invalidated) */
    dir = curdir;

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_PARAM);
    }

    /* cname was able to move curdir to it! */
    if (*s_path->m_name == '\0') {
        return AFPERR_EXIST;
    }

    upath = s_path->u_name;

    if (AFP_OK != (err = netatalk_mkdir(vol, upath))) {
        return err;
    }

    if (of_stat(vol, s_path) < 0) {
        return AFPERR_MISC;
    }

    curdir->d_offcnt++;

    if (!s_path->u_name) {
        LOG(log_error, logtype_afpd, "afp_createdir: s_path->u_name is NULL");
        return AFPERR_MISC;
    }

    size_t uname_len = strnlen(s_path->u_name, CNID_MAX_PATH_LEN);

    if ((dir = dir_add(vol, curdir, s_path, (int)uname_len)) == NULL) {
        return AFPERR_MISC;
    }

    if (movecwd(vol, dir) < 0) {
        return afp_errno;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, ".", ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_CREATE | ADFLAGS_RDWR,
                0777) < 0)  {
        return AFPERR_ACCESS;
    }

    ad_setname(&ad, s_path->m_name);
    ad_setid(&ad, s_path->st.st_dev, s_path->st.st_ino, dir->d_did, did,
             vol->v_stamp);
    fce_register(obj, FCE_DIR_CREATE, bdata(curdir->d_fullpath), NULL);
    ad_flush(&ad);
    /* Eagerly populate AD cache for newly created directory */
    ad_store_to_cache(&ad, dir);
    ad_close(&ad, ADFLAGS_HF);
    /* Send hint to afpd siblings — parent dir ctime changed due to new dir creation */
    ipc_send_cache_hint(obj, vol->v_vid, curdir->d_did, CACHE_HINT_REFRESH);
    memcpy(rbuf, &dir->d_did, sizeof(uint32_t));
    *rbuflen = sizeof(uint32_t);
    setvoltime(obj, vol);
    return AFP_OK;
}

/*!
 * @brief Rename a directory
 * @param vol       volume
 * @param dirfd     -1 means ignore dirfd (or use AT_FDCWD), otherwise src is relative to dirfd
 * @param src       old unix filename (not a pathname)
 * @param dst       new unix filename (not a pathname)
 * @param newparent curdir
 * @param newname   new mac name
 */
int renamedir(struct vol *vol,
              int dirfd,
              char *src,
              char *dst,
              struct dir *newparent,
              char *newname)
{
    struct adouble  ad;
    int             err;

    /* Source validation and cache cleaning handled by moveandrename */
    if (unix_rename(dirfd, src, -1, dst) < 0) {
        switch (errno) {
        case ENOENT :
            return AFPERR_NOOBJ;

        case EACCES :
            return AFPERR_ACCESS;

        case EROFS:
            return AFPERR_VLOCK;

        case EINVAL:
            /* tried to move directory into a subdirectory of itself */
            return AFPERR_CANTMOVE;

        case EXDEV:

            /* this needs to copy and delete. bleah. that means we have
             * to deal with entire directory hierarchies. */
            if ((err = copydir(vol, newparent, dirfd, src, dst)) < 0) {
                deletedir(vol, -1, dst);
                return err;
            }

            if ((err = deletedir(vol, dirfd, src)) < 0) {
                return err;
            }

            break;

        default:
            LOG(log_error, logtype_afpd,
                "renamedir(\"%s\" -> \"%s\"): unexpected errno %d: %s",
                src ? src : "(null)", dst ? dst : "(null)", errno, strerror(errno));
            return AFPERR_PARAM;
        }
    }

    vol->vfs->vfs_renamedir(vol, dirfd, src, dst);
    ad_init(&ad, vol);

    if (ad_open(&ad, dst, ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDWR) == 0) {
        ad_setname(&ad, newname);
        ad_flush(&ad);
        ad_close(&ad, ADFLAGS_HF);
    }

    return AFP_OK;
}

/*! delete an empty directory */
int deletecurdir(struct vol *vol)
{
    struct dir  *fdir, *pdir;
    struct adouble  ad;
    uint16_t       ashort;
    int err;

    if ((pdir = dirlookup(vol, curdir->d_pdid)) == NULL) {
        return AFPERR_ACCESS;
    }

    fdir = curdir;
    ad_init(&ad, vol);

    /* we never want to create a resource fork here, we are going to delete it */
    if (ad_metadata_cached(".", ADFLAGS_DIR, &ad, vol, fdir, true, NULL) == 0) {
        ad_getattr(&ad, &ashort);
        /* The ad_metadata_cached function handles ad_close() internally */

        if (!(vol->v_ignattr & ATTRBIT_NODELETE)
                && (ashort & htons(ATTRBIT_NODELETE))) {
            return  AFPERR_OLOCK;
        }
    }

    err = vol->vfs->vfs_deletecurdir(vol);

    if (err) {
        LOG(log_error, logtype_afpd,
            "deletecurdir: error deleting AppleDouble files in \"%s\"",
            (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "(null)");
        return err;
    }

    if (movecwd(vol, pdir) < 0) {
        err = afp_errno;
        goto delete_done;
    }

    LOG(log_debug, logtype_afpd, "deletecurdir: moved to \"%s\"",
        (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "(null)");
    err = netatalk_rmdir_all_errors(-1, cfrombstr(fdir->d_u_name));

    switch (err) {
    case AFP_OK:
    case AFPERR_NOOBJ:
        break;

    case AFPERR_DIRNEMPT:
        if (delete_vetoed_files(vol, bdata(fdir->d_u_name), false) != 0) {
            goto delete_done;
        }

        err = AFP_OK;
        break;

    default:
        LOG(log_error, logtype_afpd,
            "deletecurdir(\"%s\"): netatalk_rmdir_all_errors error",
            (curdir && curdir->d_fullpath) ? cfrombstr(curdir->d_fullpath) : "(null)");
        goto delete_done;
    }

    /* For directory deletion: prune all child dircache entries */
    if (!(fdir->d_flags & DIRF_ISFILE)) {
        dircache_remove_children(vol, fdir);
    }

    /* Save DID before dir_remove() frees fdir */
    cnid_t fdir_did = fdir->d_did;
    /* Delete the directory's CNID */
    AFP_CNID_START("cnid_delete");
    cnid_delete(vol->v_cdb, fdir_did);
    AFP_CNID_DONE();
    /* Remove from dircache */
    dir_remove(vol, fdir, 0);  /* User-initiated delete, not invalid on use */
    /* Send hint to afpd siblings — dir deleted, purge children in sibling caches */
    ipc_send_cache_hint(AFPobj, vol->v_vid, fdir_did, CACHE_HINT_DELETE_CHILDREN);
delete_done:
    return err;
}

int afp_mapid(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
              size_t *rbuflen)
{
    struct passwd   *pw;
    struct group    *gr;
    char        *name;
    uint32_t           id;
    int         len, sfunc;
    int         utf8 = 0;
    ibuf++;
    sfunc = (unsigned char) * ibuf++;
    *rbuflen = 0;

    if (sfunc >= 3 && sfunc <= 6) {
        if (obj->afp_version < 30) {
            return AFPERR_PARAM;
        }

        utf8 = 1;
    }

    switch (sfunc) {
    case 1 :
    case 3 :/* unicode */
        memcpy(&id, ibuf, sizeof(id));
        id = ntohl(id);

        if (id != 0) {
            if ((pw = getpwuid(id)) == NULL) {
                return AFPERR_NOITEM;
            }

            len = convert_string_allocate(obj->options.unixcharset,
                                          ((!utf8) ? obj->options.maccharset : CH_UTF8_MAC),
                                          pw->pw_name, -1, &name);
        } else {
            len = 0;
            name = NULL;
        }

        break;

    case 2 :
    case 4 : /* unicode */
        memcpy(&id, ibuf, sizeof(id));
        id = ntohl(id);

        if (id != 0) {
            if (NULL == (gr = (struct group *)getgrgid(id))) {
                return AFPERR_NOITEM;
            }

            len = convert_string_allocate(obj->options.unixcharset,
                                          (!utf8) ? obj->options.maccharset : CH_UTF8_MAC,
                                          gr->gr_name, -1, &name);
        } else {
            len = 0;
            name = NULL;
        }

        break;

    case 5 : /* UUID -> username */
    case 6 : /* UUID -> groupname */
        if ((obj->afp_version < 32) || !(obj->options.flags & OPTION_UUID)) {
            return AFPERR_PARAM;
        }

        LOG(log_debug, logtype_afpd, "afp_mapid: valid UUID request");
        uuidtype_t type;
        len = getnamefromuuid((unsigned char *) ibuf, &name, &type);

        if (len != 0) {     /* its a error code, not len */
            return AFPERR_NOITEM;
        }

        switch (type) {
        case UUID_USER:
            if ((pw = getpwnam(name)) == NULL) {
                return AFPERR_NOITEM;
            }

            LOG(log_debug, logtype_afpd, "afp_mapid: name:%s -> uid:%d", name, pw->pw_uid);
            id = htonl(UUID_USER);
            memcpy(rbuf, &id, sizeof(id));
            id = htonl(pw->pw_uid);
            rbuf += sizeof(id);
            memcpy(rbuf, &id, sizeof(id));
            rbuf += sizeof(id);
            *rbuflen = 2 * sizeof(id);
            break;

        case UUID_GROUP:
            if ((gr = getgrnam(name)) == NULL) {
                return AFPERR_NOITEM;
            }

            LOG(log_debug, logtype_afpd, "afp_mapid: group:%s -> gid:%d", name, gr->gr_gid);
            id = htonl(UUID_GROUP);
            memcpy(rbuf, &id, sizeof(id));
            rbuf += sizeof(id);
            id = htonl(gr->gr_gid);
            memcpy(rbuf, &id, sizeof(id));
            rbuf += sizeof(id);
            *rbuflen = 2 * sizeof(id);
            break;

        default:
            return AFPERR_MISC;
        }

        break;

    default :
        return AFPERR_PARAM;
    }

    if (name) {
        len = strlen(name);
    }

    if (utf8) {
        uint16_t tp = htons(len);
        memcpy(rbuf, &tp, sizeof(tp));
        rbuf += sizeof(tp);
        *rbuflen += 2;
    } else {
        *rbuf++ = len;
        *rbuflen += 1;
    }

    if (name && len > 0) {
        memcpy(rbuf, name, len);
    }

    *rbuflen += len;

    if (name) {
        free(name);
    }

    return AFP_OK;
}

int afp_mapname(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                size_t *rbuflen)
{
    struct passwd   *pw;
    struct group    *gr;
    int             len, sfunc;
    uint32_t       id;
    uint16_t       ulen;
    ibuf++;
    sfunc = (unsigned char) * ibuf++;
    *rbuflen = 0;
    LOG(log_debug, logtype_afpd, "afp_mapname: sfunc: %d", sfunc);

    switch (sfunc) {
    case 1 :
    case 2 : /* unicode */
        if (obj->afp_version < 30) {
            return AFPERR_PARAM;
        }

        memcpy(&ulen, ibuf, sizeof(ulen));
        len = ntohs(ulen);
        ibuf += 2;
        LOG(log_debug, logtype_afpd, "afp_mapname: alive");
        break;

    case 3 :
    case 4 :
        len = (unsigned char) * ibuf++;
        break;

    case 5 : /* username -> UUID  */
    case 6 : /* groupname -> UUID */
        if ((obj->afp_version < 32) || !(obj->options.flags & OPTION_UUID)) {
            return AFPERR_PARAM;
        }

        memcpy(&ulen, ibuf, sizeof(ulen));
        len = ntohs(ulen);
        ibuf += 2;
        break;

    default :
        return AFPERR_PARAM;
    }

    if (len >= ibuflen - 1) {
        return AFPERR_PARAM;
    }

    ibuf[len] = '\0';

    if (len == 0) {
        return AFPERR_PARAM;
    } else {
        switch (sfunc) {
        case 1 : /* unicode */
        case 3 :
            if (NULL == (pw = (struct passwd *)getpwnam(ibuf))) {
                return AFPERR_NOITEM;
            }

            id = pw->pw_uid;
            id = htonl(id);
            memcpy(rbuf, &id, sizeof(id));
            *rbuflen = sizeof(id);
            break;

        case 2 : /* unicode */
        case 4 :
            LOG(log_debug, logtype_afpd, "afp_mapname: getgrnam for name: %s", ibuf);

            if (NULL == (gr = (struct group *)getgrnam(ibuf))) {
                return AFPERR_NOITEM;
            }

            id = gr->gr_gid;
            LOG(log_debug, logtype_afpd, "afp_mapname: getgrnam for name: %s -> id: %d",
                ibuf, id);
            id = htonl(id);
            memcpy(rbuf, &id, sizeof(id));
            *rbuflen = sizeof(id);
            break;

        case 5 :        /* username -> UUID */
            LOG(log_debug, logtype_afpd, "afp_mapname: name: %s", ibuf);

            if (0 != getuuidfromname(ibuf, UUID_USER, (unsigned char *)rbuf)) {
                return AFPERR_NOITEM;
            }

            *rbuflen = UUID_BINSIZE;
            break;

        case 6 :        /* groupname -> UUID */
            LOG(log_debug, logtype_afpd, "afp_mapname: name: %s", ibuf);

            if (0 != getuuidfromname(ibuf, UUID_GROUP, (unsigned char *)rbuf)) {
                return AFPERR_NOITEM;
            }

            *rbuflen = UUID_BINSIZE;
            break;
        }
    }

    return AFP_OK;
}

/*
   variable DID support
*/
int afp_closedir(AFPObj *obj _U_, char *ibuf _U_, size_t ibuflen _U_,
                 char *rbuf _U_, size_t *rbuflen)
{
    *rbuflen = 0;
    /* do nothing as dids are static for the life of the process. */
    return AFP_OK;
}

/* did creation gets done automatically
 * there's a pb again with case but move it to cname
 */
int afp_opendir(AFPObj *obj _U_, char *ibuf, size_t ibuflen  _U_, char *rbuf,
                size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *parentdir;
    struct path     *path;
    uint32_t       did;
    uint16_t       vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (parentdir = dirlookup(vol, did))) {
        return afp_errno;
    }

    if (NULL == (path = cname(vol, parentdir, &ibuf))) {
        return get_afp_errno(AFPERR_PARAM);
    }

    if (*path->m_name != '\0') {
        return path_error(path, AFPERR_NOOBJ);
    }

    if (!path->st_valid && of_stat(vol, path) < 0) {
        return AFPERR_NOOBJ;
    }

    if (path->st_errno) {
        return AFPERR_NOOBJ;
    }

    memcpy(rbuf, &curdir->d_did, sizeof(curdir->d_did));
    *rbuflen = sizeof(curdir->d_did);
    return AFP_OK;
}
