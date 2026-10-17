/*
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
  Copyright (c) 2026 Andy Lemin (andylemin)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * @brief The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr,
 * FPSetExtAttr, and FPRemoveExtAttr
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/afp_util.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

#include "ad_cache.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "extattrs.h"
#include "fork.h"
#include "volume.h"

static const char *ea_finderinfo = "com.apple.FinderInfo";
static const char *ea_resourcefork = "com.apple.ResourceFork";


/***************************************
 * AFP funcs
 ****************************************/

/*!
 * @brief FPListExtAttrs: list the extended attribute names of a file or
 * directory
 *
 * The client calls twice, first for the size of all names and then for the
 * names themselves. A static buffer keeps the names between the two calls, so
 * one scan of the extended attributes serves both.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or an AFPERR_* code
 */
int afp_listextattr(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                    size_t *rbuflen)
{
    int                 ret, oflag = 0, adflags = 0, fd = -1;
    uint16_t            vid;
    uint16_t            bitmap;
    uint32_t            did, maxreply, tmpattr;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct stat         *st;
    struct adouble      ad, *adp = NULL;
    struct ofork	*opened = NULL;
    char                *uname, *FinderInfo;
    char                emptyFinderInfo[32] = { 0 };
    size_t              attrbuflen = 0;
    int                 ad_available = 0;
    char                attrnamebuf[ATTRNAMEBUFSIZ];
    *rbuflen = 0;
    ibuf += 2;
    /* Get Bitmap and MaxReplySize first */
    memcpy(&bitmap, ibuf + 6, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    memcpy(&maxreply, ibuf + 14, sizeof(maxreply));
    maxreply = ntohl(maxreply);
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    vol = getvolbyvid(vid);

    if (vol == NULL) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: getvolbyvid error: %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);
    dir = dirlookup(vol, did);

    if (dir == NULL) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: dirlookup error: %s",
            strerror(errno));
        return afp_errno;
    }

    if (bitmap & kXAttrNoFollow) {
        oflag = O_NOFOLLOW;
    }

    /* Skip Bitmap, ReqCount, StartIndex and maxreply*/
    ibuf += 12;

    /* get name */
    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        LOG(log_debug, logtype_afpd, "afp_listextattr: cname error: %s",
            AfpErr2name(afp_errno));
        return afp_errno;
    }

    st = &s_path->st;

    if (!s_path->st_valid) {
        /* it's a dir in our cache, we didn't stat it, do it now */
        of_statdir(vol, s_path);
    }

    if (s_path->st_errno != 0) {
        return AFPERR_NOOBJ;
    }

    uname = s_path->u_name;
    /*
     * We have to check the FinderInfo for the file, because if they
     * aren't all 0 we must return the synthetic attribute
     * "com.apple.FinderInfo".  Note: the client will never (never
     * seen in traces) request that attribute via FPGetExtAttr !
    */
    adp = &ad;
    ad_init(adp, vol);

    if (path_isadir(s_path)) {
        LOG(log_debug, logtype_afpd, "afp_listextattr(%s): is a dir", uname);
        adflags = ADFLAGS_DIR;
    } else {
        LOG(log_debug, logtype_afpd, "afp_listextattr(%s): is a file", uname);
        opened = of_findname(vol, s_path);

        if (opened) {
            adp = opened->of_ad;
            fd = ad_meta_fileno(adp);
        }
    }

    if (opened) {
        /* Fork-owned adouble — use directly, skip cache.
         * Opportunistically populate cache if not yet loaded.
         * ad_meta_loaded(): a fork open without metadata leaves the
         * adouble's header zeroed — storing it would flip a negative
         * sentinel to a bogus positive entry. */
        ad_available = 1;
        size_t uname_len = strnlen(uname, CNID_MAX_PATH_LEN);
        struct dir *cached = dircache_search_by_name(vol, curdir,
                                                     uname, uname_len);

        if (cached && cached->dcache_rlen < 0 && ad_meta_loaded(adp)) {
            ad_store_to_cache(adp, cached);
        }
    } else {
        /* No open fork — use ad_metadata_cached() (non-strict) */
        struct dir *cached = path_isadir(s_path) ? s_path->d_dir
                             : dircache_search_by_name(vol, curdir, uname, strnlen(uname,
                                 CNID_MAX_PATH_LEN));

        if (ad_metadata_cached(uname, adflags, adp, vol, cached, false,
                               (s_path->st_errno == 0) ? &s_path->st : NULL) != 0) {
            switch (errno) {
            case ENOENT:
                break;

            case EACCES:
                LOG(log_error, logtype_afpd,
                    "afp_listextattr(%s): %s: check resource fork permission?",
                    uname, strerror(errno));
                return AFPERR_ACCESS;

            default:
                LOG(log_error, logtype_afpd, "afp_listextattr(%s): error getting metadata: %s",
                    uname, strerror(errno));
                return AFPERR_MISC;
            }
        } else {
            ad_available = 1;
            /* The ad_metadata_cached function handles ad_close() internally */
        }
    }

    if (ad_available) {
        FinderInfo = ad_entry(adp, ADEID_FINDERI);

        /* Check if FinderInfo equals default and empty FinderInfo*/
        if (FinderInfo && memcmp(FinderInfo, emptyFinderInfo, 32) != 0) {
            /* FinderInfo contains some non 0 bytes -> include "com.apple.FinderInfo" */
            strcpy(attrnamebuf, ea_finderinfo);
            attrbuflen += strlen(ea_finderinfo) + 1;
            LOG(log_debug7, logtype_afpd,
                "afp_listextattr(%s): sending com.apple.FinderInfo", uname);
        }

        /* Now check for Resource fork and add virtual EA "com.apple.ResourceFork" if size > 0 */
        LOG(log_debug7, logtype_afpd, "afp_listextattr(%s): Resource fork size: %llu",
            uname, adp->ad_rlen);

        if (adp->ad_rlen > 0) {
            LOG(log_debug7, logtype_afpd,
                "afp_listextattr(%s): sending com.apple.ResourceFork.", uname);
            strcpy(attrnamebuf + attrbuflen, ea_resourcefork);
            attrbuflen += strlen(ea_resourcefork) + 1;
        }
    }

    ret = vol->vfs->vfs_ea_list(vol, attrnamebuf, &attrbuflen, uname, oflag, fd);

    if (ret != AFP_OK) {
        attrbuflen = 0;

        switch (ret) {
        case AFPERR_BADTYPE:
            /* its a symlink and client requested O_NOFOLLOW */
            LOG(log_debug, logtype_afpd,
                "afp_listextattr(%s): encountered symlink with kXAttrNoFollow", uname);
            ret = AFP_OK;
            break;

        default:
            break;
        }
    }

exit:
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    rbuf += sizeof(bitmap);
    *rbuflen += sizeof(bitmap);
    tmpattr = htonl(attrbuflen);
    memcpy(rbuf, &tmpattr, sizeof(tmpattr));
    rbuf += sizeof(tmpattr);
    *rbuflen += sizeof(tmpattr);

    if (maxreply > 0) {
        memcpy(rbuf, attrnamebuf, attrbuflen);
        *rbuflen += attrbuflen;
    }

    return ret;
}

/*!
 * @brief Terminate a counted extended attribute name in a static buffer for log
 * lines
 *
 * @param[in] ibuf  name from the request
 * @param[in] len   length of the name, capped at 255
 * @returns the static buffer
 */
static char *to_stringz(char *ibuf, uint16_t len)
{
    static char attrmname[256];

    if (len > 255)
        /* don't fool with us */
    {
        len = 255;
    }

    /* we must copy the name as its not 0-terminated and I DONT WANT TO WRITE to ibuf */
    strlcpy(attrmname, ibuf, len + 1);
    return attrmname;
}

/*!
 * @brief Convert a request's EA name from UTF-8 to the unix charset
 *
 * @param[in]  obj    session
 * @param[in]  name   the name as sent, not terminated
 * @param[in]  len    its length
 * @param[out] uname  the converted name
 * @param[in]  size   bytes available at uname
 *
 * @returns AFP_OK, or AFPERR_PARAM for a name that is empty or does not
 *          convert
 */
static int ea_uname(const AFPObj *obj, const char *name, uint16_t len,
                    char *uname, size_t size)
{
    size_t ulen = convert_string(CH_UTF8_MAC, obj->options.unixcharset, name,
                                 len, uname, size);
    return ulen == 0 || ulen == (size_t) -1 ? AFPERR_PARAM : AFP_OK;
}

/*!
 * @brief FPGetExtAttr: return the size or the content of an extended attribute
 *
 * A MaxReplySize of 0 asks for the size only. The name arrives in UTF-8 and
 * converts to the unix charset. An open fork of the file lends its metadata
 * descriptor to the VFS call. kXAttrNoFollow keeps the call off symlink
 * targets.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the VFS result, AFPERR_PARAM for an unknown volume or a name that
 *          is empty or does not convert, or the error of the directory or
 *          path lookup
 */
int afp_getextattr(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                   size_t *rbuflen)
{
    int                 ret, oflag = 0, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did, maxreply;
    char                attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad _U_, *adp = NULL;
    struct ofork	*opened = NULL;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: getvolbyvid error: %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: dirlookup error: %s",
            strerror(errno));
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow) {
        oflag = O_NOFOLLOW;
    }

    /* Skip Offset and ReqCount */
    ibuf += 16;
    /* Get MaxReply */
    memcpy(&maxreply, ibuf, sizeof(maxreply));
    maxreply = ntohl(maxreply);
    ibuf += sizeof(maxreply);

    /* get name */
    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        LOG(log_debug, logtype_afpd, "afp_getextattr: cname error: %s",
            AfpErr2name(afp_errno));
        return afp_errno;
    }

    if ((unsigned long)ibuf & 1) {
        ibuf++;
    }

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);
    LOG(log_debug, logtype_afpd, "afp_getextattr(%s): EA: %s", s_path->u_name,
        to_stringz(ibuf, attrnamelen));

    if ((ret = ea_uname(obj, ibuf, attrnamelen, attruname,
                        sizeof(attruname))) != AFP_OK) {
        return ret;
    }

    /* write bitmap now */
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    rbuf += sizeof(bitmap);
    *rbuflen += sizeof(bitmap);

    if (path_isadir(s_path)) {
        LOG(log_debug, logtype_afpd, "afp_getextattr(%s): is a dir", s_path->u_name);
    } else {
        LOG(log_debug, logtype_afpd, "afp_getextattr(%s): is a file", s_path->u_name);
        opened = of_findname(vol, s_path);

        if (opened) {
            adp = opened->of_ad;
            fd = ad_meta_fileno(adp);
        }
    }

    /*
      Switch on maxreply:
      if its 0 we must return the size of the requested attribute,
      if its non 0 we must return the attribute.
    */
    if (maxreply == 0) {
        ret = vol->vfs->vfs_ea_getsize(vol, rbuf, rbuflen, s_path->u_name, oflag,
                                       attruname, fd);
    } else {
        ret = vol->vfs->vfs_ea_getcontent(vol, rbuf, rbuflen, s_path->u_name, oflag,
                                          attruname, maxreply, fd);
    }

    return ret;
}

/*!
 * @brief FPSetExtAttr: create or replace an extended attribute
 *
 * kXAttrCreate and kXAttrReplace map to O_CREAT and O_TRUNC for the VFS call.
 * A name over 255 bytes or a value over MAX_EA_SIZE gets AFPERR_PARAM. After a
 * write the dircache entry takes the new stat, and the other sessions get a
 * refresh hint.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the VFS result, AFPERR_PARAM, also for a name that is empty or
 *          does not convert, AFPERR_VLOCK on a read-only volume, or the
 *          error of the directory or path lookup
 */
int afp_setextattr(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                   char *rbuf _U_, size_t *rbuflen)
{
    int                 oflag = 0, ret, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did, attrsize;
    char                attruname[256];
    char		*attrmname;
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad _U_, *adp = NULL;
    struct ofork	*opened = NULL;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: getvolbyvid error: %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: dirlookup error: %s",
            strerror(errno));
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow) {
        oflag |= O_NOFOLLOW;
    }

    if (bitmap & kXAttrCreate) {
        oflag |= O_CREAT;
    } else if (bitmap & kXAttrReplace) {
        oflag |= O_TRUNC;
    }

    /* Skip Offset */
    ibuf += 8;

    /* get name */
    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        LOG(log_debug, logtype_afpd, "afp_setextattr: cname error: %s",
            AfpErr2name(afp_errno));
        return afp_errno;
    }

    if (path_isadir(s_path)) {
        LOG(log_debug, logtype_afpd, "afp_setextattr(%s): is a dir", s_path->u_name);
    } else {
        LOG(log_debug, logtype_afpd, "afp_setextattr(%s): is a file", s_path->u_name);
        opened = of_findname(vol, s_path);

        if (opened) {
            adp = opened->of_ad;
            fd = ad_meta_fileno(adp);
        }
    }

    if ((unsigned long)ibuf & 1) {
        ibuf++;
    }

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);

    if (attrnamelen > 255) {
        return AFPERR_PARAM;
    }

    attrmname = ibuf;

    if ((ret = ea_uname(obj, attrmname, attrnamelen, attruname,
                        sizeof(attruname))) != AFP_OK) {
        return ret;
    }

    ibuf += attrnamelen;
    /* get EA size */
    memcpy(&attrsize, ibuf, sizeof(attrsize));
    attrsize = ntohl(attrsize);
    ibuf += sizeof(attrsize);

    if (attrsize > MAX_EA_SIZE)
        /* we arbitrarily make this fatal */
    {
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_setextattr(%s): EA: %s, size: %u",
        s_path->u_name, to_stringz(attrmname, attrnamelen), attrsize);
    ret = vol->vfs->vfs_ea_set(vol, s_path->u_name, attruname, ibuf, attrsize,
                               oflag, fd);

    if (ret == AFP_OK) {
        if (vol->v_vfs_ea != AFPVOL_EA_NONE) {
            setvoltime(obj, vol);
        }

        /* EA set changes ctime — refresh stat cache */
        struct stat post_st;
        struct dir *cached = NULL;

        if (path_isadir(s_path)) {
            cached = curdir;
        } else if (s_path->u_name != NULL) {
            size_t uname_len = strnlen(s_path->u_name, CNID_MAX_PATH_LEN);
            cached = dircache_search_by_name(vol, curdir, s_path->u_name, uname_len);
        }

        if (ostat(s_path->u_name, &post_st, vol_syml_opt(vol)) == 0 && cached) {
            dir_modify(vol, cached, &(struct dir_modify_args) {
                .flags = DCMOD_STAT,
                .st = &post_st,
            });
        }

        /* Send hint to afpd siblings — EA write changed ctime */
        cnid_t hint_cnid = cached ? cached->d_did : CNID_INVALID;

        if (hint_cnid == CNID_INVALID && s_path->u_name) {
            size_t ulen = strnlen(s_path->u_name, CNID_MAX_PATH_LEN);
            hint_cnid = cnid_get(vol->v_cdb, curdir->d_did, s_path->u_name, ulen);
        }

        if (hint_cnid != CNID_INVALID) {
            ipc_send_cache_hint(obj, vol->v_vid, hint_cnid, CACHE_HINT_REFRESH);
        }
    }

    return ret;
}

/*!
 * @brief FPRemoveExtAttr: remove an extended attribute
 *
 * A name over 255 bytes gets AFPERR_PARAM. After a removal the dircache entry
 * takes the new stat, and the other sessions get a refresh hint.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the VFS result, AFPERR_PARAM, also for a name that is empty or
 *          does not convert, AFPERR_VLOCK on a read-only volume, or the
 *          error of the directory or path lookup
 */
int afp_remextattr(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                   char *rbuf _U_, size_t *rbuflen)
{
    int                 oflag = 0, ret, fd = -1;
    uint16_t            vid, bitmap, attrnamelen;
    uint32_t            did;
    char                attruname[256];
    struct vol          *vol;
    struct dir          *dir;
    struct path         *s_path;
    struct adouble	ad _U_, *adp = NULL;
    struct ofork	*opened = NULL;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: getvolbyvid error: %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: dirlookup error: %s",
            strerror(errno));
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if (bitmap & kXAttrNoFollow) {
        oflag |= O_NOFOLLOW;
    }

    /* get name */
    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        LOG(log_debug, logtype_afpd, "afp_remextattr: cname error: %s",
            AfpErr2name(afp_errno));
        return afp_errno;
    }

    if (path_isadir(s_path)) {
        LOG(log_debug, logtype_afpd, "afp_remextattr(%s): is a dir", s_path->u_name);
    } else {
        LOG(log_debug, logtype_afpd, "afp_remextattr(%s): is a file", s_path->u_name);
        opened = of_findname(vol, s_path);

        if (opened) {
            adp = opened->of_ad;
            fd = ad_meta_fileno(adp);
        }
    }

    if ((unsigned long)ibuf & 1) {
        ibuf++;
    }

    /* get length of EA name */
    memcpy(&attrnamelen, ibuf, sizeof(attrnamelen));
    attrnamelen = ntohs(attrnamelen);
    ibuf += sizeof(attrnamelen);

    if (attrnamelen > 255) {
        return AFPERR_PARAM;
    }

    if ((ret = ea_uname(obj, ibuf, attrnamelen, attruname,
                        sizeof(attruname))) != AFP_OK) {
        return ret;
    }

    LOG(log_debug, logtype_afpd, "afp_remextattr(%s): EA: %s", s_path->u_name,
        to_stringz(ibuf, attrnamelen));
    ret = vol->vfs->vfs_ea_remove(vol, s_path->u_name, attruname, oflag, fd);

    if (ret == AFP_OK) {
        if (vol->v_vfs_ea != AFPVOL_EA_NONE) {
            setvoltime(obj, vol);
        }

        /* EA remove changes ctime — refresh stat cache */
        struct stat post_st;
        struct dir *cached = NULL;

        if (path_isadir(s_path)) {
            cached = curdir;
        } else if (s_path->u_name != NULL) {
            size_t uname_len = strnlen(s_path->u_name, CNID_MAX_PATH_LEN);
            cached = dircache_search_by_name(vol, curdir, s_path->u_name, uname_len);
        }

        if (ostat(s_path->u_name, &post_st, vol_syml_opt(vol)) == 0 && cached) {
            dir_modify(vol, cached, &(struct dir_modify_args) {
                .flags = DCMOD_STAT,
                .st = &post_st,
            });
        }

        /* Send hint to afpd siblings — EA removal changed ctime */
        cnid_t hint_cnid = cached ? cached->d_did : CNID_INVALID;

        if (hint_cnid == CNID_INVALID && s_path->u_name) {
            size_t ulen = strnlen(s_path->u_name, CNID_MAX_PATH_LEN);
            hint_cnid = cnid_get(vol->v_cdb, curdir->d_did, s_path->u_name, ulen);
        }

        if (hint_cnid != CNID_INVALID) {
            ipc_send_cache_hint(obj, vol->v_vid, hint_cnid, CACHE_HINT_REFRESH);
        }
    }

    return ret;
}
