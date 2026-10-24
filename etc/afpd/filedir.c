/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief AFP commands for both files and directories: FPGetFileDirParms,
 * FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <bstrlib.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/errchk.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_ipc.h>
#ifdef WITH_SPOTLIGHT
#include <atalk/spotlight.h>
#endif
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "ad_cache.h"
#include "fork.h"
#include "unix.h"
#include "virtual_icon.h"
#include "volume.h"

/*!
 * @brief Reply to FPGetFileDirParms for the virtual Icon file of the volume
 * root
 *
 * The reply carries both bitmaps, the file flag, and the synthesized file
 * parameters of virtual_icon_getfilparams.
 *
 * @param[in]  obj      session object
 * @param[in]  vol      volume with the virtual icon
 * @param[in]  fbitmap  AFP file bitmap
 * @param[in]  dbitmap  AFP directory bitmap, echoed only
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  receives the reply length
 * @returns AFP_OK, or the error of virtual_icon_getfilparams
 */
static int virtual_icon_reply(const AFPObj *obj, struct vol *vol,
                              uint16_t fbitmap, uint16_t dbitmap,
                              char *rbuf, size_t *rbuflen)
{
    size_t buflen = 0;
    int ret;

    if (fbitmap && AFP_OK != (ret = virtual_icon_getfilparams(
                                        obj, vol, fbitmap, 0,
                                        rbuf + 3 * sizeof(uint16_t), &buflen))) {
        return ret;
    }

    *(rbuf + 2 * sizeof(uint16_t)) = FILDIRBIT_ISFILE;
    *rbuflen = buflen + 3 * sizeof(uint16_t);
    fbitmap = htons(fbitmap);
    memcpy(rbuf, &fbitmap, sizeof(fbitmap));
    rbuf += sizeof(fbitmap);
    dbitmap = htons(dbitmap);
    memcpy(rbuf, &dbitmap, sizeof(dbitmap));
    rbuf += sizeof(dbitmap) + sizeof(uint8_t);
    *rbuf = 0;
    return AFP_OK;
}

/*!
 * @brief FPGetFileDirParms: return the parameters of a file or directory
 *
 * The reply carries the file and directory bitmaps, a flag that tells files
 * from directories, and the parameters of getfilparams or getdirparams. The
 * virtual Icon file of the volume root answers through virtual_icon_reply,
 * by its CNID and by its name. A directory that the user cannot enter still
 * reports its parameters.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_NOOBJ, AFPERR_PARAM for an unknown volume, or another
 *          AFPERR_* code
 */
int afp_getfildirparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                        char *rbuf, size_t *rbuflen)
{
    struct stat     *st;
    struct vol      *vol;
    struct dir      *dir;
    uint32_t           did;
    int         ret;
    size_t      buflen;
    uint16_t       fbitmap, dbitmap, vid;
    struct path         *s_path;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        /* AFP 3.4 FPGetFileDirParms Table 32: unknown Volume ID is kFPParamErr. */
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    memcpy(&fbitmap, ibuf, sizeof(fbitmap));
    fbitmap = ntohs(fbitmap);
    ibuf += sizeof(fbitmap);
    memcpy(&dbitmap, ibuf, sizeof(dbitmap));
    dbitmap = ntohs(dbitmap);
    ibuf += sizeof(dbitmap);

    /* Intercept FPGetFilDirParms for virtual Icon\r by CNID */
    if (did == htonl(VIRTUAL_ICON_CNID)
            && virtual_icon_enabled(vol)
            && !real_icon_exists(vol)) {
        return virtual_icon_reply(obj, vol, fbitmap, dbitmap,
                                  rbuf, rbuflen);
    }

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    LOG(log_debug, logtype_afpd,
        "getfildirparams(vid:%u, did:%u, f/d:%04x/%04x) {cwdid:%u, cwd: %s, name:'%s'}",
        ntohs(vid), ntohl(dir->d_did), fbitmap, dbitmap,
        ntohl(curdir->d_did), cfrombstr(curdir->d_fullpath), s_path->u_name);
    st   = &s_path->st;

    if (!s_path->st_valid) {
        /* it's a dir and it should be there
         * because we chdir in it in cname or
         * it's curdir (maybe deleted, but then we can't know).
         * So we need to try harder.
         */
        of_statdir(vol, s_path);
    }

    if (s_path->st_errno != 0) {
        /* Intercept virtual Icon\r at volume root */
        if (s_path->st_errno == ENOENT
                && curdir->d_did == DIRDID_ROOT
                && is_virtual_icon_name(s_path->u_name)
                && virtual_icon_enabled(vol)) {
            return virtual_icon_reply(obj, vol, fbitmap, dbitmap,
                                      rbuf, rbuflen);
        }

        if (afp_errno != AFPERR_ACCESS) {
            return AFPERR_NOOBJ;
        }
    }

    buflen = 0;

    if (S_ISDIR(st->st_mode)) {
        if (dbitmap) {
            dir = s_path->d_dir;

            if (!dir) {
                LOG(log_error, logtype_afpd,
                    "afp_getfildirparams: s_path->d_dir NULL (name:\"%s\")",
                    s_path->u_name ? s_path->u_name : "(null)");
                return AFPERR_NOOBJ;
            }

            LOG(log_debug, logtype_afpd,
                "afp_getfildirparams: SUCCESS dir did:%u \"%s\"",
                ntohl(dir->d_did), s_path->u_name ? s_path->u_name : ".");
            ret = getdirparams(obj, vol, dbitmap, s_path, dir,
                               rbuf + 3 * sizeof(uint16_t), &buflen);

            if (ret != AFP_OK) {
                /* kFPObjectNotFound: the DID names no folder at its path */
                if (ret != AFPERR_NOOBJ) {
                    LOG(log_error, logtype_afpd,
                        "afp_getfildirparams: getdirparams failed ret=%d",
                        ret);
                }

                return ret;
            }
        }

        /* this is a directory */
        *(rbuf + 2 * sizeof(uint16_t)) = (char) FILDIRBIT_ISDIR;
    } else {
        if (fbitmap && AFP_OK != (ret = getfilparams(obj, vol, fbitmap, s_path, curdir,
                                                     rbuf + 3 * sizeof(uint16_t), &buflen, 0))) {
            return ret;
        }

        /* this is a file */
        *(rbuf + 2 * sizeof(uint16_t)) = FILDIRBIT_ISFILE;
    }

    *rbuflen = buflen + 3 * sizeof(uint16_t);
    fbitmap = htons(fbitmap);
    memcpy(rbuf, &fbitmap, sizeof(fbitmap));
    rbuf += sizeof(fbitmap);
    dbitmap = htons(dbitmap);
    memcpy(rbuf, &dbitmap, sizeof(dbitmap));
    rbuf += sizeof(dbitmap) + sizeof(uint8_t);
    *rbuf = 0;
    return AFP_OK;
}

/*!
 * @brief FPSetFileDirParms: set the parameters of a file or directory
 *
 * The call dispatches to setdirparams or setfilparams by the type of the
 * object. The virtual Icon file of the volume root refuses changes with
 * AFPERR_OLOCK. A directory that the user cannot enter still accepts changes.
 * The parameters start at the next even offset of the request, and success
 * updates the volume modification time.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_VLOCK for a read-only volume, AFPERR_OLOCK,
 *          AFPERR_NOOBJ, or another AFPERR_* code
 */
int afp_setfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                        char *rbuf _U_, size_t *rbuflen)
{
    struct stat *st;
    struct vol  *vol;
    struct dir  *dir;
    struct path *path;
    uint16_t   vid, bitmap;
    int     did, rc;
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

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if (NULL == (path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    /* Reject modifications to virtual Icon\r file (but allow real ones) */
    if (is_virtual_icon(vol, curdir->d_did, path->u_name)) {
        return AFPERR_OLOCK;
    }

    st   = &path->st;

    if (!path->st_valid) {
        /* it's a dir and it should be there
         * because we chdir in it in cname
         */
        of_statdir(vol, path);
    }

    if (path->st_errno != 0) {
        if (afp_errno != AFPERR_ACCESS) {
            return AFPERR_NOOBJ;
        }
    }

    /*
     * If ibuf is odd, make it even.
     */
    if ((intptr_t)ibuf & 1) {
        ibuf++;
    }

    if (S_ISDIR(st->st_mode)) {
        rc = setdirparams(vol, path, bitmap, ibuf);
    } else {
        rc = setfilparams(obj, vol, path, bitmap, ibuf);
    }

    if (rc == AFP_OK) {
        setvoltime(obj, vol);
    }

    return rc;
}

/*!
 * @brief Reject "." and "..", a name that the AppleDouble layout reserves, or
 * one that the volume vetoes
 *
 * @param[in] vol   volume
 * @param[in] name  Unix name of a file or directory
 * @returns 0 for an acceptable name, AFPERR_EXIST otherwise
 */
int check_name(const struct vol *vol, char *name)
{
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        return AFPERR_EXIST;
    }

    if (!vol->vfs->vfs_validupath(vol, name)) {
        LOG(log_warning, logtype_afpd, "check_name: illegal name: '%s'", name);
        return AFPERR_EXIST;
    }

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name, vol->v_ignorecase)) {
        return AFPERR_EXIST;
    }

    return 0;
}

/*!
 * @brief Move and rename sdir/oldname to newname in the working directory
 *
 * A file takes its CNID from the database, and a directory passes itself as
 * sdir and supplies the DID. The rename-inhibit attribute blocks the call
 * unless the volume ignores it, and check_name screens the new name. The
 * virtual Icon file's name in the volume root fails with AFPERR_EXIST. An
 * unchanged name succeeds at once. Inside one directory a name that another
 * file holds fails with AFPERR_EXIST, and across directories any existing
 * destination fails. So does a destination that this session holds open.
 *
 * After renamefile or renamedir succeeds, the dircache entry and its children
 * follow. FCE and Spotlight get a move event. The AppleDouble metadata and the
 * CNID record get the new parent and name, and sibling afpd processes get
 * hints for the object and both parents.
 *
 * @param[in] obj      session object
 * @param[in] vol      volume of both directories
 * @param[in] sdir     source directory, or the directory itself when isdir is
 *                     nonzero
 * @param[in] sdir_fd  descriptor of the source directory, or -1 for the working
 *                     directory
 * @param[in] oldname  current Mac name
 * @param[in] newname  new Mac name
 * @param[in] isdir    nonzero when the object is a directory
 * @returns AFP_OK, AFPERR_OLOCK, AFPERR_EXIST, AFPERR_PARAM, AFPERR_MISC,
 *          the error of a parent DID that names no folder, or the error of
 *          renamefile or renamedir
 */
static int moveandrename(const AFPObj *obj,
                         struct vol *vol,
                         struct dir *sdir,
                         int sdir_fd,
                         char *oldname,
                         char *newname,
                         int isdir)
{
    char            *oldunixname = NULL;
    char            *upath;
    int             rc;
    struct stat     *st, nst;
    int             adflags;
    struct adouble	ad;
    struct adouble	*adp;
    struct ofork	*opened = NULL;
    struct path     path;
    cnid_t          id;
    int             cwd_fd = -1;
    bstring         saved_sdir_fullpath = NULL;
    ad_init(&ad, vol);
    adp = &ad;
    adflags = 0;

    if (!isdir) {
        if ((oldunixname = strdup(mtoupath(vol, oldname, sdir->d_did,
                                           utf8_encoding(vol->v_obj)))) == NULL) {
            /* can't convert */
            return AFPERR_PARAM;
        }

        size_t oldunixname_len = oldunixname ? strnlen(oldunixname,
                                                       CNID_MAX_PATH_LEN) : 0;
        AFP_CNID_START("cnid_get");
        id = cnid_get(vol->v_cdb, sdir->d_did, oldunixname, oldunixname_len);
        AFP_CNID_DONE();

        /* A backend that could not answer is not "no CNID recorded": the
         * cnid_update() after the rename below repoints this id at the new
         * name */
        if (id == CNID_INVALID && CNID_ERRNO_IS_BACKEND_FAILURE()) {
            LOG(log_warning, logtype_afpd,
                "moveandrename(\"%s\"): CNID backend error, errno=0x%x",
                oldunixname, CNID_ERRNO());
            free(oldunixname);
            return AFPERR_MISC;
        }

        path.st_valid = 0;
        path.u_name = oldunixname;
        opened = of_findnameat(vol, sdir_fd, &path);

        /* reuse struct adouble so it won't break locks; a symlink holds
         * none, and a metadata close would spend its fork's descriptor */
        if (opened && ad_data_fileno(opened->of_ad) >= 0) {
            adp = opened->of_ad;
        }
    } else {
        id = sdir->d_did; /* we already have the CNID */
        /* a parent DID that names no folder fails the request */
        struct dir *sparent = dirlookup_strict(vol, sdir->d_pdid);
        const char *spath;

        if (sparent == NULL) {
            return afp_errno;
        }

        if ((spath = ctoupath(vol, sparent, oldname)) == NULL
                || (oldunixname = strdup(spath)) == NULL) {
            return AFPERR_PARAM;
        }

        adflags = ADFLAGS_DIR;
    }

    /* Save sdir->d_fullpath for LOG and FCE events — sdir may be modified during rename */
    saved_sdir_fullpath = bstrcpy(sdir->d_fullpath);

    if (saved_sdir_fullpath == NULL) {
        LOG(log_error, logtype_afpd, "moveandrename: out of memory");
        free(oldunixname);
        return AFPERR_MISC;
    }

    /*
     * we are in the dest folder so we need to use
     *   a) oldunixname for ad_open
     *   b) fchdir sdir_fd before e.g. ad_open or use *at functions where appropriate
     */

    if (sdir_fd != -1) {
        if ((cwd_fd = open(".", O_RDONLY)) == -1) {
            free(oldunixname);
            bdestroy(saved_sdir_fullpath);
            return AFPERR_MISC;
        }

        if (fchdir(sdir_fd) != 0) {
            rc = AFPERR_MISC;
            goto exit;
        }
    }

    {
        /* Pre-rename: read metadata via cache (strict=true for write/change path) */
        struct dir *src_cached = isdir ? sdir : NULL;

        /* an open fork's adouble is the live copy: the cache would overwrite
         * its header */
        if (!isdir && !opened) {
            src_cached = dircache_search_by_name(vol, sdir, oldunixname,
                                                 strnlen(oldunixname,
                                                         CNID_MAX_PATH_LEN));
        }

        if (!ad_metadata_cached(oldunixname, adflags, adp, vol, src_cached, true,
                                !isdir && path.st_errno == 0 ? &path.st
                                : NULL)) {
            uint16_t bshort;
            ad_getattr(adp, &bshort);
            /* The ad_metadata_cached function handles ad_close() internally */

            if (!(vol->v_ignattr & ATTRBIT_NORENAME)
                    && (bshort & htons(ATTRBIT_NORENAME))) {
                rc = AFPERR_OLOCK;
                goto exit;
            }
        }
    }

    if (sdir_fd != -1) {
        if (fchdir(cwd_fd) != 0) {
            LOG(log_error, logtype_afpd, "moveandrename: %s", strerror(errno));
            rc = AFPERR_MISC;
            goto exit;
        }
    }

    if (NULL == (upath = mtoupath(vol, newname, curdir->d_did,
                                  utf8_encoding(vol->v_obj)))) {
        rc = AFPERR_PARAM;
        goto exit;
    }

    path.u_name = upath;
    st = &path.st;

    if (0 != (rc = check_name(vol, upath))) {
        goto exit;
    }

    /* the virtual Icon\r file holds its name, as a real one would */
    if (is_virtual_icon(vol, curdir->d_did, upath)) {
        rc = AFPERR_EXIST;
        goto exit;
    }

    if (isdir)
        LOG(log_debug, logtype_afpd,
            "moveandrename(\"%s\" -> \"%s/%s\")",
            oldunixname, bdata(curdir->d_fullpath), upath);
    else
        LOG(log_debug, logtype_afpd,
            "moveandrename(\"%s/%s\" -> \"%s/%s\")",
            bdata(saved_sdir_fullpath), oldunixname, bdata(curdir->d_fullpath), upath);

    /* source == destination. we just silently accept this. */
    if ((!isdir && curdir == sdir) || (isdir && curdir->d_did == sdir->d_pdid)) {
        if (strcmp(oldname, newname) == 0) {
            rc = AFP_OK;
            goto exit;
        }

        /* a symlink is a name like any other, as the volume lists it */
        if (ostat(upath, st, vol_syml_opt(vol)) == 0) {
            if (!ostat(oldunixname, &nst, vol_syml_opt(vol))
                    && !(nst.st_dev == st->st_dev
                         && nst.st_ino == st->st_ino)) {
                /* not the same file */
                rc = AFPERR_EXIST;
                goto exit;
            }

            errno = 0;
        }
    } else if (ostat(upath, st, vol_syml_opt(vol)) == 0) {
        rc = AFPERR_EXIST;
        goto exit;
    }

    /* Pre-acquire file cache entry BEFORE rename.
     * Old path is still valid and ctime unchanged for now. */
    struct dir *cachedfile = NULL;

    if (!isdir) {
        cachedfile = dircache_search_by_name(vol, sdir, oldunixname,
                                             strnlen(oldunixname, CNID_MAX_PATH_LEN));
    }

    /* Perform the rename */
    if (!isdir) {
        path.st_valid = 1;
        path.st_errno = errno;

        if (of_findname(vol, &path)) {
            rc = AFPERR_EXIST; /* was AFPERR_BUSY; */
        } else {
            rc = renamefile(vol, curdir, sdir_fd, oldunixname, upath, newname, adp);

            if (rc == AFP_OK) {
                of_rename(vol, opened, sdir, oldunixname, curdir, newname,
                          upath);
            }
        }
    } else {
        rc = renamedir(vol, sdir_fd, oldunixname, upath, curdir, newname);
    }

    if (rc == AFP_OK && id) {
        /* renaming may have moved the file/dir across a filesystem */
        if (ostat(upath, st, vol_syml_opt(vol)) < 0) {
            rc = AFPERR_MISC;
            goto exit;
        }

        /* Save old parent DID for hint (before dir_modify changes sdir->d_pdid).
         * For dirs: sdir->d_pdid is the old parent. For files: sdir->d_did is the parent. */
        cnid_t old_parent_did = isdir ? sdir->d_pdid : sdir->d_did;

        /* Update cache entry in-place after successful rename.
         * Separate file/directory handling because sdir has different semantics:
         *   Directories: sdir IS the directory being renamed (its own cache entry)
         *   Files: sdir is the PARENT directory of the file being renamed */
        if (isdir && sdir) {
            dircache_remove_children_defer(vol, sdir);

            if (dir_modify(vol, sdir, &(struct dir_modify_args) {
            .flags = DCMOD_PATH | DCMOD_STAT,
            .new_pdid = curdir->d_did,
            .new_mname = newname,
            .new_uname = upath,
            .new_pdir_path = curdir->d_fullpath,
            .st = st,
        }) != 0) {
                /* Refresh failed, purged — NULL sdir to prevent use-after-free */
                sdir = NULL;
            }
#if defined(WITH_FCE) || defined(WITH_SPOTLIGHT)
            char event_path[MAXPATHLEN + 1];
            const char *dir_dst = dir_event_path(event_path,
                                                 sizeof(event_path), curdir, upath);
#endif
#ifdef WITH_FCE
            fce_register(obj, FCE_DIR_MOVE, dir_dst, oldunixname);
#endif /* WITH_FCE */
#ifdef WITH_SPOTLIGHT
            sl_index_event(obj, vol, SL_INDEX_DIR_MOVE,
                           dir_dst, bdata(saved_sdir_fullpath));
#endif /* WITH_SPOTLIGHT */
        }

        /* File rename: update cache entry if pre-acquired */
        if (!isdir && cachedfile &&
        dir_modify(vol, cachedfile, &(struct dir_modify_args) {
        .flags = DCMOD_PATH | DCMOD_STAT,
        .new_pdid = curdir->d_did,
        .new_mname = newname,
        .new_uname = upath,
        .new_pdir_path = curdir->d_fullpath,
        .st = st,
    }) != 0) {
            /* Refresh failed, purged — NULL cachedfile */
            cachedfile = NULL;
        }
#if defined(WITH_FCE) || defined(WITH_SPOTLIGHT)

        if (!isdir) {
            char event_path[MAXPATHLEN + 1];
            const char *file_dst = dir_event_path(event_path,
                                                  sizeof(event_path), curdir, upath);
            bstring srcpath = bformat("%s/%s", bdata(saved_sdir_fullpath),
                                      oldunixname);

            if (srcpath != NULL) {
#ifdef WITH_FCE
                fce_register(obj, FCE_FILE_MOVE, file_dst, bdata(srcpath));
#endif
#ifdef WITH_SPOTLIGHT
                sl_index_event(obj, vol, SL_INDEX_FILE_MOVE,
                               file_dst, bdata(srcpath));
#endif
                bdestroy(srcpath);
            }
        }

#endif /* WITH_FCE || WITH_SPOTLIGHT */

        /* Fixup adouble info — separate ad_open for CNID write.
         * adflags includes ADFLAGS_DIR for directories (set at line 301). */
        if (ad_open(adp, upath, adflags | ADFLAGS_HF | ADFLAGS_RDWR) == 0) {
            ad_setid(adp, st->st_dev, st->st_ino, id, curdir->d_did, vol->v_stamp);
            ad_flush(adp);
            /* Update cache with written data before close */
            struct dir *dst_cached = isdir ? sdir
                                         : cachedfile;

            if (dst_cached) {
                ad_store_to_cache(adp, dst_cached);
            }

            ad_close(adp, ADFLAGS_HF);
        }

        /* fix up the catalog entry */
        size_t upath_len = upath ? strnlen(upath, CNID_MAX_PATH_LEN) : 0;
        AFP_CNID_START("cnid_update");
        cnid_update(vol->v_cdb, id, st, curdir->d_did, upath, upath_len);
        AFP_CNID_DONE();

        /* Send hints to afpd siblings — rename/move completed */
        if (isdir && sdir) {
            /* Dir rename: purge children in sibling caches (stale d_fullpath) */
            ipc_send_cache_hint(obj, vol->v_vid, sdir->d_did, CACHE_HINT_DELETE_CHILDREN);
        } else if (!isdir && id != CNID_INVALID) {
            /* File rename: sibling's ostat on old path → ENOENT → removed */
            ipc_send_cache_hint(obj, vol->v_vid, id, CACHE_HINT_REFRESH);
        }

        /* Old parent ctime changed */
        ipc_send_cache_hint(obj, vol->v_vid, old_parent_did, CACHE_HINT_REFRESH);

        /* New parent ctime changed (skip if same dir rename) */
        if (curdir->d_did != old_parent_did) {
            ipc_send_cache_hint(obj, vol->v_vid, curdir->d_did, CACHE_HINT_REFRESH);
        }
    }

exit:

    if (cwd_fd != -1) {
        close(cwd_fd);
    }

    if (oldunixname) {
        free(oldunixname);
    }

    bdestroy(saved_sdir_fullpath);
    return rc;
}

/*!
 * @brief FPRename: rename a file or directory inside its parent directory
 *
 * The volume root fails with AFPERR_NORENAME, and the virtual Icon file of
 * the volume root with AFPERR_OLOCK. An empty new name succeeds without work.
 * The moveandrename call does the rest, and success updates the volume
 * modification time.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_NORENAME, AFPERR_OLOCK, AFPERR_VLOCK, AFPERR_PARAM,
 *          or the error of moveandrename
 */
int afp_rename(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
               size_t *rbuflen)
{
    struct vol  *vol;
    struct dir  *sdir;
    char        *oldname, *newname;
    struct path *path;
    uint32_t   did;
    int         plen;
    uint16_t   vid;
    int         isdir = 0;
    int         rc;
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

    if (NULL == (sdir = dirlookup_strict(vol, did))) {
        return afp_errno;
    }

    /* source pathname */
    if (NULL == (path = cname(vol, sdir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    /* Reject rename of virtual Icon\r file (but allow real ones) */
    if (is_virtual_icon(vol, curdir->d_did, path->u_name)) {
        return AFPERR_OLOCK;
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    isdir = path_isadir(path);

    if (*path->m_name != '\0') {
        /* an extra copy for of_rename */
        strlcpy(oldname, path->m_name, AFPOBJ_TMPSIZ + 1);

        if (isdir) {
            /* curdir parent dir, need to move sdir back */
            sdir = path->d_dir;
        }
    } else {
        if (sdir->d_did == DIRDID_ROOT) {   /* root directory */
            LOG(log_debug, logtype_afpd,
                "afp_rename: volume root cannot be renamed");
            return AFPERR_NORENAME;
        }

        /* move to destination dir */
        if (movecwd(vol, dirlookup(vol, sdir->d_pdid)) < 0) {
            return afp_errno;
        }

        memcpy(oldname, cfrombstr(sdir->d_m_name), blength(sdir->d_m_name) + 1);
    }

    /* another place where we know about the path type */
    if ((plen = copy_path_name(vol, newname, ibuf)) < 0) {
        return AFPERR_PARAM;
    }

    if (!plen) {
        /* newname == oldname same dir */
        return AFP_OK;
    }

    rc = moveandrename(obj, vol, sdir, -1, oldname, newname, isdir);

    if (rc == AFP_OK) {
        setvoltime_entry(obj, vol, curdir->d_did == DIRDID_ROOT);
    }

    return rc;
}

/*!
 * @brief Recursively delete vetoed files and directories
 *
 * @param[in] vol         volume handle
 * @param[in] parent_fd   file descriptor of the parent directory
 * @param[in] name        directory name relative to parent_fd
 * @param[in] in_vetodir  true if we are already in a vetoed directory
 *
 * Recursively scans the directory for vetoed files and tries to delete them,
 * then tries to delete the directory. That may fail if the directory contains
 * normal files that aren't vetoed.
 *
 * @returns 0 if the directory was deleted, otherwise a negative error code.
 */
static int delete_vetoed_files_at(struct vol *vol, int parent_fd,
                                  const char *name, bool in_vetodir)
{
    EC_INIT;
    DIR            *dp = NULL;
    struct dirent  *de;
    struct stat     sb;
    int             directory_fd;
    int             dir_fd;
    bool            vetoed;
    /* Do not let a directory-to-symlink replacement redirect recursion. */
    directory_fd = openat(parent_fd, name,
                          O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

    if (directory_fd == -1) {
        LOG(log_error, logtype_afpd,
            "delete_vetoed_files: cannot open directory \"%s\": %s",
            name, strerror(errno));
        return -1;
    }

    dp = fdopendir(directory_fd);

    if (dp == NULL) {
        LOG(log_error, logtype_afpd,
            "delete_vetoed_files: cannot read directory \"%s\": %s",
            name, strerror(errno));
        close(directory_fd);
        return -1;
    }

    /* fdopendir() owns directory_fd from here on. */
    dir_fd = dirfd(dp);

    if (dir_fd == -1) {
        LOG(log_error, logtype_afpd,
            "delete_vetoed_files: cannot get descriptor for directory \"%s\": %s",
            name, strerror(errno));
        EC_FAIL;
    }

    while ((de = readdir(dp))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        /* Classify symlinks as leaves; all later operations use this dirfd. */
        if (fstatat(dir_fd, de->d_name, &sb, AT_SYMLINK_NOFOLLOW) != 0) {
            LOG(log_error, logtype_afpd, "delete_vetoed_files(\"%s/%s\"): %s",
                name, de->d_name, strerror(errno));
            EC_EXIT_STATUS(AFPERR_DIRNEMPT);
        }

        if (in_vetodir
                || veto_file(vol->v_veto, de->d_name, vol->v_ignorecase)) {
            vetoed = true;
        } else {
            vetoed = false;
        }

        if (vetoed) {
            LOG(log_debug, logtype_afpd,
                "delete_vetoed_files(\"%s/%s\"): deleting vetoed file",
                name, de->d_name);

            switch (sb.st_mode & S_IFMT) {
            case S_IFDIR:
                EC_ZERO(delete_vetoed_files_at(vol, dir_fd, de->d_name,
                                               true));
                break;

            case S_IFREG:
            case S_IFLNK:
                EC_ZERO(netatalk_unlinkat(dir_fd, de->d_name));
                break;

            default:
                break;
            }
        }
    }

    if (closedir(dp) != 0) {
        dp = NULL;
        EC_FAIL;
    }

    dp = NULL;
    EC_ZERO_LOG(unlinkat(parent_fd, name, AT_REMOVEDIR));
EC_CLEANUP:

    if (dp) {
        closedir(dp);
    }

    EC_EXIT;
}

/*!
 * @brief Delete the vetoed files in directory upath and then the directory
 * itself
 *
 * Only a volume with AFPVOL_DELVETO allows it. The work happens in
 * delete_vetoed_files_at, relative to the working directory. A directory that
 * still holds files outside the veto list survives.
 *
 * @param[in] vol         volume of the directory
 * @param[in] upath       directory name, relative to the working directory
 * @param[in] in_vetodir  true when upath itself lies inside a vetoed directory
 * @returns 0 when the directory went away, a negative value otherwise
 */
int delete_vetoed_files(struct vol *vol, const char *upath, bool in_vetodir)
{
    int parent_fd;
    int ret;

    if (!(vol->v_flags & AFPVOL_DELVETO)) {
        return -1;
    }

    parent_fd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (parent_fd == -1) {
        return -1;
    }

    ret = delete_vetoed_files_at(vol, parent_fd, upath, in_vetodir);
    close(parent_fd);
    return ret;
}

/*!
 * @brief Take one item from a directory's cached offspring count
 *
 * A count of no items never held the item that goes, one made outside afpd,
 * so the call drops the count rather than wrap it, and the next request
 * walks the directory.
 *
 * @param[in,out] dir  directory that lost an item
 */
void dir_offcnt_take(struct dir *dir)
{
    if (dir->d_offcnt > 0) {
        dir->d_offcnt--;
    } else {
        dir->d_ctime = 0;
    }
}

/*!
 * @brief FPDelete: delete a file or an empty directory
 *
 * Deleting the virtual Icon file of the volume root fails with AFPERR_OLOCK,
 * and the volume root itself fails with AFPERR_ACCESS. A directory that cname
 * entered goes through deletecurdir. A directory that the user cannot enter
 * goes through a direct rmdir, except on version 2 AppleDouble volumes. A
 * file that this session holds open fails with AFPERR_BUSY. A file whose
 * dircache inode no longer matches fails with AFPERR_NOOBJ.
 *
 * On a volume with AFPVOL_DELVETO a directory that holds only vetoed files
 * still goes away. Success sends FCE and Spotlight events and tells sibling
 * afpd processes.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_BUSY, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_OLOCK,
 *          AFPERR_NOOBJ, AFPERR_VLOCK, or another AFPERR_* code
 */
int afp_delete(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
               size_t *rbuflen)
{
    struct vol  *vol;
    struct dir  *dir;
    struct path *s_path;
    char        *upath;
    int         did;
    int         rc = AFP_OK;
    uint16_t    vid;
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
    /* Validate PARENT directory via strict lookup (stat and inode comparison) */
    LOG(log_debug, logtype_afpd,
        "afp_delete: validating parent did:%u with strict lookup",
        ntohl(did));

    if (NULL == (dir = dirlookup_strict(vol, did))) {
        return afp_errno;
    }

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    /* Reject deletion of virtual Icon\r file (but allow real ones) */
    if (is_virtual_icon(vol, curdir->d_did, s_path->u_name)) {
        return AFPERR_OLOCK;
    }

    upath = s_path->u_name;

    /* Always perform fresh stat on final s_path for delete */
    if (of_statdir(vol, s_path) < 0) {
        switch (errno) {
        case ENOENT:
            return AFPERR_NOOBJ;

        case EACCES:
        case EPERM:
            return AFPERR_ACCESS;

        default:
            return AFPERR_MISC;
        }
    }

    if (path_isadir(s_path)) {
        if (*s_path->m_name != '\0' || curdir->d_did == DIRDID_ROOT) {
            if (vol->v_adouble == AD_VERSION2) {
                return AFPERR_ACCESS;
            }

            if (*s_path->m_name == '\0' && curdir->d_did == DIRDID_ROOT) {
                return AFPERR_ACCESS;
            }

            /* Delete the directory */
            if (rmdir(upath) != 0) {
                switch (errno) {
                case ENOTEMPTY:
                    if (delete_vetoed_files(vol, upath, false) != 0) {
                        return AFPERR_DIRNEMPT;
                    }

                    break;

                case EACCES:
                    return AFPERR_ACCESS;

                default:
                    return AFPERR_MISC;
                }
            }

            struct dir *deldir;

            cnid_t delcnid = CNID_INVALID;
            /* Post-delete cleanup: directory already validated and deleted,
             * now clean up cache and CNID unconditionally */
            deldir = NULL;

            if (upath != NULL) {
                size_t upath_len = strnlen(upath, CNID_MAX_PATH_LEN);
                deldir = dircache_search_by_name(vol, curdir, upath, upath_len);
            }

            if (deldir) {
                LOG(log_debug, logtype_afpd,
                    "afp_delete: cleaning cache for deleted directory \"%s\"", upath);
                /* Delete CNID from database */
                delcnid = deldir->d_did;

                if (delcnid != CNID_INVALID) {
                    AFP_CNID_START("cnid_delete");
                    cnid_delete(vol->v_cdb, delcnid);
                    AFP_CNID_DONE();
                }

                dir_remove_tree(vol, deldir, 0);  /* User-initiated delete cleanup */
            } else {
                /* Directory not in cache, delete CNID manually */
                size_t upath_len = upath ? strnlen(upath, CNID_MAX_PATH_LEN) : 0;
                AFP_CNID_START("cnid_get");
                delcnid = cnid_get(vol->v_cdb, curdir->d_did, upath, upath_len);
                AFP_CNID_DONE();

                if (delcnid != CNID_INVALID) {
                    AFP_CNID_START("cnid_delete");
                    cnid_delete(vol->v_cdb, delcnid);
                    AFP_CNID_DONE();
                }
            }

            /* Send hints to afpd siblings — dir deleted */
            if (delcnid != CNID_INVALID) {
                ipc_send_cache_hint(obj, vol->v_vid, delcnid, CACHE_HINT_DELETE_CHILDREN);
            }

            /* Parent dir ctime changed */
            ipc_send_cache_hint(obj, vol->v_vid, curdir->d_did, CACHE_HINT_REFRESH);
#if defined(WITH_FCE) || defined(WITH_SPOTLIGHT)
            char event_path[MAXPATHLEN + 1];
            const char *deleted_dir = dir_event_path(event_path,
                                                     sizeof(event_path), curdir, upath);
#endif
#ifdef WITH_FCE
            fce_register(obj, FCE_DIR_DELETE, deleted_dir, NULL);
#endif /* WITH_FCE */
#ifdef WITH_SPOTLIGHT
            sl_index_event(obj, vol, SL_INDEX_DIR_DELETE, deleted_dir, NULL);
#endif /* WITH_SPOTLIGHT */
        } else {
            /* we have to cache this, the structs are lost in deletcurdir*/
            /* but we need the positive returncode to send our event */
            bstring dname;

            if ((dname = bstrcpy(curdir->d_u_name)) == NULL) {
                return AFPERR_MISC;
            }

            /* deletecurdir() also handles CNID and dircache cleanup */
            if ((rc = deletecurdir(vol)) == AFP_OK) {
#if defined(WITH_FCE) || defined(WITH_SPOTLIGHT)
                /* deletecurdir() moved cwd to the parent: curdir is now
                 * the deleted directory's parent. */
                char event_path[MAXPATHLEN + 1];
                const char *deleted_dir = dir_event_path(event_path,
                                                         sizeof(event_path), curdir,
                                                         cfrombstr(dname));
#endif
#ifdef WITH_FCE
                fce_register(obj, FCE_DIR_DELETE, deleted_dir, NULL);
#endif /* WITH_FCE */
#ifdef WITH_SPOTLIGHT
                sl_index_event(obj, vol, SL_INDEX_DIR_DELETE,
                               deleted_dir, NULL);
#endif /* WITH_SPOTLIGHT */
            }

            bdestroy(dname);
        }
    } else {
        /* it's a file st_valid should always be true
         * only test for ENOENT because EACCES needs
         * to read meta data in deletefile
         */
        const struct ofork *of;
        uint16_t held_band = 0;
        int held_content = 0;

        if (s_path->st_valid && s_path->st_errno == ENOENT) {
            rc = AFPERR_NOOBJ;
        } else if ((of = of_findname(vol, s_path)) != NULL
                   && of_delete_blocked(of, &held_band, &held_content)) {
            /* Same-session claims block by the same policy as a peer's
             * (deny modes and content locks; an open alone never blocks);
             * one fork suffices to check, since siblings share the adouble
             * that holds all of the session's claims.  Claim-free forks
             * fall through and are swept after a successful delete. */
            LOG(log_note, logtype_afpd,
                "afp_delete(\"%s\"): refused (AFPERR_BUSY): same-session claim:"
                "%s band 0x%03x (refnum %" PRIu16 ")", upath,
                held_content ? " [content lock]" : "",
                held_band, of->of_refnum);
            rc = AFPERR_BUSY;
        } else {
            /* Validate target FILE inode to detect external replacement */
            LOG(log_debug, logtype_afpd,
                "afp_delete: validating target file \"%s\" inode", upath);
            struct dir *cachedfile = NULL;

            if (upath != NULL) {
                size_t upath_len = strnlen(upath, CNID_MAX_PATH_LEN);
                cachedfile = dircache_search_by_name(vol, curdir, upath, upath_len);
            }

            if (cachedfile != NULL && cachedfile->dcache_ino != s_path->st.st_ino) {
                /* File was replaced externally - inode mismatch */
                LOG(log_warning, logtype_afpd,
                    "afp_delete: inode mismatch for \"%s\" (cached:%llu vs current:%llu), file replaced externally",
                    upath, (unsigned long long)cachedfile->dcache_ino,
                    (unsigned long long)s_path->st.st_ino);
                /* Invalidate stale cache entry */
                dir_remove(vol, cachedfile, 1);  /* Invalid: inode mismatch, file replaced */
                /* Return error - the file the client wanted to delete no longer exists */
                return AFPERR_NOOBJ;
            }

            /* The dircache id was inode-validated just above; without a cache
             * hit, deletefile() resolves the CNID itself after the unlink
             * (authoritative against concurrent renames) and reports it back
             * here for the sibling cache hint. */
            cnid_t file_cnid = cachedfile ? cachedfile->d_did : CNID_INVALID;

            /* deletefile() also deletes the CNID */
            if ((rc = deletefile(vol, -1, upath, 1, &file_cnid)) == AFP_OK) {
                if (cachedfile) {
                    (void)dir_remove(vol, cachedfile, 0);
                }

#if defined(WITH_FCE) || defined(WITH_SPOTLIGHT)
                char event_path[MAXPATHLEN + 1];
                const char *deleted_file = dir_event_path(event_path,
                                                          sizeof(event_path), curdir, upath);
#endif
#ifdef WITH_FCE
                fce_register(obj, FCE_FILE_DELETE, deleted_file, NULL);
#endif /* WITH_FCE */
#ifdef WITH_SPOTLIGHT
                sl_index_event(obj, vol, SL_INDEX_FILE_DELETE, deleted_file, NULL);
#endif /* WITH_SPOTLIGHT */

                /* Send hints to afpd siblings — file deleted */
                if (file_cnid != CNID_INVALID) {
                    ipc_send_cache_hint(obj, vol->v_vid, file_cnid, CACHE_HINT_DELETE);
                }

                /* Parent dir ctime changed */
                ipc_send_cache_hint(obj, vol->v_vid, curdir->d_did, CACHE_HINT_REFRESH);

                if (vol->v_tm_used < s_path->st.st_size) {
                    vol->v_tm_used = 0;
                } else {
                    vol->v_tm_used -= s_path->st.st_size;
                }

                /* Sweep this session's remaining (no-claim) forks: they
                 * reference the unlinked inode now.  Refusals above leave all
                 * forks intact.  Last in the branch: of_closefork() overwrites
                 * the static mtoupath() buffer that upath aliases, so no upath
                 * consumer may run after this. */
                of_close_inode_forks(obj, vol, s_path);
            }
        }
    }

    if (rc == AFP_OK) {
        dir_offcnt_take(curdir);
        setvoltime_entry(obj, vol, curdir->d_did == DIRDID_ROOT);
    }

    return rc;
}

/*!
 * @brief Full path of the Unix name u inside dir
 *
 * The result lives in a static buffer that the next call overwrites.
 *
 * @param[in] vol  volume
 * @param[in] dir  directory that holds u
 * @param[in] u    Unix name
 * @returns the full path, or NULL for a NULL argument or a path beyond
 *          MAXPATHLEN
 */
char *absupath(const struct vol *vol, struct dir *dir, char *u)
{
    static char pathbuf[MAXPATHLEN + 1];
    bstring path;

    if (u == NULL || dir == NULL || vol == NULL) {
        return NULL;
    }

    if ((path = fullpath_join(dir->d_fullpath, u)) == NULL) {
        return NULL;
    }

    if (path->slen > MAXPATHLEN) {
        bdestroy(path);
        return NULL;
    }

    LOG(log_debug, logtype_afpd, "absupath: %s", cfrombstr(path));
    strncpy(pathbuf, cfrombstr(path), blength(path) + 1);
    bdestroy(path);
    return pathbuf;
}

/*!
 * @brief Full Unix path of a Mac name inside dir
 *
 * @param[in] vol   volume
 * @param[in] dir   directory that holds the name
 * @param[in] name  Mac name
 * @returns the full path in the static buffer of absupath, or NULL on failure
 */
char *ctoupath(const struct vol *vol, struct dir *dir, char *name)
{
    if (vol == NULL || dir == NULL || name == NULL) {
        return NULL;
    }

    return absupath(vol, dir, mtoupath(vol, name, dir->d_did,
                                       utf8_encoding(vol->v_obj)));
}

/*!
 * @brief FPMoveAndRename: move a file or directory into another directory, with
 * a new name if given
 *
 * The request names the source by DID and path and the destination directory
 * by DID and path, followed by the new name. An empty new name keeps the old
 * one. The virtual Icon file of the volume root cannot move. Without UNIX
 * privileges a moved file takes the mode that its new directory implies, plus
 * the volume file permissions. Success updates the volume modification time.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_OLOCK, AFPERR_VLOCK, AFPERR_NOOBJ, AFPERR_PARAM, or
 *          the error of moveandrename
 */
int afp_moveandrename(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                      char *rbuf _U_, size_t *rbuflen)
{
    struct vol  *vol;
    struct dir  *sdir, *ddir;
    int         isdir;
    char    *oldname, *newname;
    struct path *path;
    int     did;
    int     pdid;
    int         plen;
    uint16_t   vid;
    int         rc;
    int     sdir_fd = -1;
    bool        from_root;
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

    /* source did followed by dest did */
    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(int);

    if (NULL == (sdir = dirlookup_strict(vol, did))) {
        return afp_errno; /* was AFPERR_PARAM */
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(int);

    /* source pathname */
    if (NULL == (path = cname(vol, sdir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    /* Reject move/rename of virtual Icon\r file (but allow real ones) */
    if (is_virtual_icon(vol, curdir->d_did, path->u_name)) {
        return AFPERR_OLOCK;
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    isdir = path_isadir(path);

    if (*path->m_name != '\0') {
        if (isdir) {
            sdir = path->d_dir;
        }

        /* an extra copy for of_rename */
        strlcpy(oldname, path->m_name, AFPOBJ_TMPSIZ + 1);
    } else {
        memcpy(oldname, cfrombstr(sdir->d_m_name), blength(sdir->d_m_name) + 1);
    }

    if ((sdir_fd = open(".", O_RDONLY)) == -1) {
        return AFPERR_MISC;
    }

    /* get the destination directory */
    if (NULL == (ddir = dirlookup_strict(vol, did))) {
        rc = afp_errno;
        goto exit;
    }

    if (NULL == (path = cname(vol, ddir, &ibuf))) {
        rc = AFPERR_NOOBJ;
        goto exit;
    }

    pdid = curdir->d_did;

    if (*path->m_name != '\0') {
        rc = path_error(path, AFPERR_NOOBJ);
        goto exit;
    }

    /* one more place where we know about path type */
    if ((plen = copy_path_name(vol, newname, ibuf)) < 0) {
        rc = AFPERR_PARAM;
        goto exit;
    }

    if (!plen) {
        strlcpy(newname, oldname, AFPOBJ_TMPSIZ + 1);
    }

    /* This does the work */
    LOG(log_debug, logtype_afpd, "afp_move(oldname:'%s', newname:'%s', isdir:%u)",
        oldname, newname, isdir);
    from_root = (isdir ? sdir->d_pdid : sdir->d_did) == DIRDID_ROOT;
    rc = moveandrename(obj, vol, sdir, sdir_fd, oldname, newname, isdir);

    if (rc == AFP_OK) {
        char *upath = mtoupath(vol, newname, pdid, utf8_encoding(obj));

        if (NULL == upath) {
            rc = AFPERR_PARAM;
            goto exit;
        }

        /* if unix priv don't try to match perm with dest folder */
        if (!isdir && !vol_unix_priv(vol)) {
            int  admode = ad_mode("", 0777) | vol_fperm(vol);
            setfilmode(vol, upath, admode, path->st_valid ? &path->st : NULL);
            vol->vfs->vfs_setfilmode(vol, upath, admode, path->st_valid ? &path->st : NULL);
        }

        setvoltime_entry(obj, vol, from_root || curdir->d_did == DIRDID_ROOT);
    }

exit:

    if (sdir_fd != -1) {
        close(sdir_fd);
    }

    return rc;
}

/*!
 * @brief Whether a veto entry of path's length equals path
 *
 * With nocase, letters match in either ASCII case, as ascii_tolower() folds
 * them; every other byte matches only as written.
 *
 * @param[in] entry   first byte of the entry
 * @param[in] path    name to test
 * @param[in] len     length of both
 * @param[in] nocase  whether ASCII letters match in either case
 * @returns 1 for a match, 0 otherwise
 */
static int veto_entry_matches(const char *entry, const char *path, size_t len,
                              int nocase)
{
    if (!nocase) {
        return memcmp(entry, path, len) == 0;
    }

    for (size_t i = 0; i < len; i++) {
        if (entry[i] != path[i]
                && ascii_tolower((unsigned char)entry[i])
                != ascii_tolower((unsigned char)path[i])) {
            return 0;
        }
    }

    return 1;
}

/*!
 * @brief Whether path matches one entry of a slash-separated veto list
 *
 * The list has the form "abc/zxc/", and an entry must equal path byte for
 * byte, or with nocase letter for letter in either ASCII case. The list's
 * end closes its last entry as a slash does. Only an entry of path's length
 * is compared.
 *
 * @param[in] veto_str  slash-separated veto list, or NULL
 * @param[in] path      name to test, or NULL
 * @param[in] nocase    whether ASCII letters match in either case
 * @returns 1 for a match, 0 otherwise
 */
int veto_file(const char *veto_str, const char *path, int nocase)
{
    size_t len;

    if (veto_str == NULL || path == NULL || *path == '\0') {
        return 0;
    }

    len = strlen(path);

    while (*veto_str != '\0') {
        size_t elen = strcspn(veto_str, "/");

        if (elen == len && veto_entry_matches(veto_str, path, len, nocase)) {
            LOG(log_debug, logtype_afpd, "vetoed file:'%s'", path);
            return 1;
        }

        veto_str += elen + (veto_str[elen] == '/');
    }

    return 0;
}
