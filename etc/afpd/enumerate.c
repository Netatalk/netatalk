/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/param.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "volume.h"

#define min(a,b)	((a)<(b)?(a):(b))

/*!
 * @brief Struct to save directory reading context in.
 * @note Used to prevent O(n^2) searches on a directory.
 */
struct savedir {
    u_short	 sd_vid;
    uint32_t	 sd_did;
    int		 sd_buflen;
    char	 *sd_buf;
    char	 *sd_last;
    unsigned int sd_sindex;
};
#define SDBUFBRK	2048

static int enumerate_loop(struct dirent *de, char *mname _U_, void *data)
{
    struct savedir *sd = data;
    char *start;
    const char *end;
    int len;
    int bytes_needed;
    end = sd->sd_buf + sd->sd_buflen;
    len = (int) strnlen(de->d_name, UTF8FILELEN_EARLY);
    bytes_needed = 1 + len + 1;
    *(sd->sd_last)++ = len;

    if (sd->sd_last + bytes_needed > end) {
        start = sd->sd_buf;
        char *buf;

        if (!(buf = realloc(sd->sd_buf, sd->sd_buflen + SDBUFBRK))) {
            LOG(log_error, logtype_afpd, "afp_enumerate: realloc: %s",
                strerror(errno));
            /* Restore position to avoid invalid data */
            sd->sd_last--;
            errno = ENOMEM;
            return -1;
        }

        sd->sd_buf = buf;
        sd->sd_buflen += SDBUFBRK;
        sd->sd_last = (sd->sd_last - start) + sd->sd_buf;
        end = sd->sd_buf + sd->sd_buflen;
    }

    memcpy(sd->sd_last, de->d_name, len + 1);
    sd->sd_last += len + 1;
    return 0;
}

/*!
 * @bug
 * Doesn't work with dangling symlink
 * i.e.:
 * - Move a folder with a dangling symlink in the trash
 * - empty the trash
 *
 * afp_enumerate return an empty listing but offspring count != 0 in afp_getdirparams
 * and the client doesn't try to call afp_delete!
 *
 * @sa https://sourceforge.net/p/netatalk/bugs/97/
 *
*/
char *check_dirent(const struct vol *vol, char *name)
{
    if (!strcmp(name, "..") || !strcmp(name, ".")) {
        return NULL;
    }

    if (!vol->vfs->vfs_validupath(vol, name)) {
        return NULL;
    }

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name)) {
        return NULL;
    }

#if 0
    char *m_name = NULL;

    if (NULL == (m_name = utompath(vol, name, 0, utf8_encoding()))) {
        return NULL;
    }

    /* now check against too big a file */
    if (strlen(m_name) > vol->max_filename) {
        return NULL;
    }

#endif
    return name;
}

/* ----------------------------- */
int
for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
{
    DIR             *dp;
    struct dirent	*de;
    char            *m_name;
    int             ret;

    if (NULL == (dp = opendir(name))) {
        return -1;
    }

    ret = 0;

    for (de = readdir(dp); de != NULL; de = readdir(dp)) {
        if (!(m_name = check_dirent(vol, de->d_name))) {
            continue;
        }

        ret++;

        if (fn && fn(de, m_name, data) < 0) {
            closedir(dp);
            return -1;
        }
    }

    closedir(dp);
    return ret;
}

/*! This is the maximal length of a single entry for a file/dir in the reply
   block if all bits in the file/dir bitmap are set: header(4) + params(104) +
   macnamelength(1) + macname(31) + utf8(4) + utf8namelen(2) + utf8name(255) +
   oddpadding(1) */

#define REPLY_PARAM_MAXLEN (4 + 104 + 1 + MACFILELEN + 4 + 2 + UTF8FILELEN_EARLY + 1)

/* ----------------------------- */
static int enumerate(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen, int ext)
{
    static struct savedir	sd = { 0, 0, 0, NULL, NULL, 0 };
    struct vol			*vol;
    struct dir			*dir;
    int				did, ret, len, first = 1;
    size_t			esz;
    char                        *data, *start;
    uint16_t			vid, fbitmap, dbitmap, reqcnt, actcnt = 0;
    uint16_t			temp16;
    uint32_t			sindex, maxsz, sz = 0;
    struct path                 *o_path;
    struct path                 s_path;
    int                         header;

    if (sd.sd_buflen == 0) {
        if ((sd.sd_buf = (char *)malloc(SDBUFBRK)) == NULL) {
            LOG(log_error, logtype_afpd, "afp_enumerate: malloc: %s", strerror(errno));
            *rbuflen = 0;
            return AFPERR_MISC;
        }

        sd.sd_buflen = SDBUFBRK;
    }

    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        *rbuflen = 0;
        return (afp_errno == AFPERR_NOOBJ) ? AFPERR_NODIR : afp_errno;
    }

    memcpy(&fbitmap, ibuf, sizeof(fbitmap));
    fbitmap = ntohs(fbitmap);
    ibuf += sizeof(fbitmap);
    memcpy(&dbitmap, ibuf, sizeof(dbitmap));
    dbitmap = ntohs(dbitmap);
    ibuf += sizeof(dbitmap);

    /* check for proper bitmaps -- the stuff in comments is for
     * variable directory ids. */
    if (!(fbitmap || dbitmap)) {
#if 0
        || (fbitmap & (1 << FILPBIT_PDID)) ||
        (dbitmap & (1 << DIRPBIT_PDID))
#endif
        *rbuflen = 0;
        return AFPERR_BITMAP;
    }

    memcpy(&reqcnt, ibuf, sizeof(reqcnt));
    reqcnt = ntohs(reqcnt);
    ibuf += sizeof(reqcnt);

    if (ext == 2) {
        memcpy(&sindex, ibuf, sizeof(sindex));
        sindex = ntohl(sindex);
        ibuf += sizeof(sindex);
    } else {
        memcpy(&temp16, ibuf, sizeof(temp16));
        sindex = ntohs(temp16);
        ibuf += sizeof(temp16);
    }

    if (!sindex) {
        *rbuflen = 0;
        return AFPERR_PARAM ;
    }

    if (ext == 2) {
        memcpy(&maxsz, ibuf, sizeof(maxsz));
        maxsz = ntohl(maxsz);
        ibuf += sizeof(maxsz);
    } else {
        memcpy(&temp16, ibuf, sizeof(temp16));
        maxsz = ntohs(temp16);
        ibuf += sizeof(temp16);
    }

    header = ext ? 4 : 2;
    header *= sizeof(uint8_t);
    maxsz = min(maxsz, *rbuflen - REPLY_PARAM_MAXLEN);
    o_path = cname(vol, dir, &ibuf);

    if (afp_errno == AFPERR_NOOBJ) {
        afp_errno = AFPERR_NODIR;
    }

    *rbuflen = 0;

    if (NULL == o_path) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    if (*o_path->m_name != '\0') {
        /* it's a file or it's a dir and extendir() was unable to chdir in it */
        return path_error(o_path, AFPERR_NODIR);
    }

    LOG(log_debug, logtype_afpd,
        "enumerate(\"%s/%s\", f/d:%04x/%04x, rc:%u, i:%u, max:%u)",
        getcwdpath(), o_path->u_name, fbitmap, dbitmap, reqcnt, sindex, maxsz);
    data = rbuf + 3 * sizeof(uint16_t);
    sz = 3 * sizeof(uint16_t);	/* fbitmap, dbitmap, reqcount */

    /*
     * Read the directory into a pre-malloced buffer, stored
     *		len <name> \0
     * The end is indicated by a len of 0.
     */
    if (sindex == 1 || curdir->d_did != sd.sd_did || vid != sd.sd_vid) {
        sd.sd_last = sd.sd_buf;

        /* if dir was in the cache we don't have the inode */
        if ((!o_path->st_valid && ostat(".", &o_path->st, vol_syml_opt(vol)) < 0) ||
                (ret = for_each_dirent(vol, ".", enumerate_loop, (void *)&sd)) < 0) {
            LOG(log_error, logtype_afpd, "enumerate: loop error: %s (%d)", strerror(errno),
                errno);

            switch (errno) {
            case EACCES:
                return AFPERR_ACCESS;

            case ENOTDIR:
                return AFPERR_BADTYPE;

            case ENOMEM:
                return AFPERR_MISC;

            default:
                return AFPERR_NODIR;
            }
        }

        setdiroffcnt(curdir, &o_path->st,  ret);
        *sd.sd_last = 0;
        sd.sd_last = sd.sd_buf;
        sd.sd_sindex = 1;
        sd.sd_vid = vid;
        sd.sd_did = curdir->d_did;
    }

    /*
     * Position sd_last as dictated by sindex.
     */
    if (sindex < sd.sd_sindex) {
        sd.sd_sindex = 1;
        sd.sd_last = sd.sd_buf;
    }

    while (sd.sd_sindex < sindex) {
        if (sd.sd_last == NULL) {
            sd.sd_did = 0;
            return AFPERR_MISC;
        }

        len = (unsigned char) * (sd.sd_last)++;

        if (len == 0) {
            sd.sd_did = 0;	/* invalidate sd struct to force re-read */
            return AFPERR_NOOBJ;
        }

        sd.sd_last += len + 1;
        sd.sd_sindex++;
    }

    while ((len = (unsigned char) * (sd.sd_last)) != 0) {
        /*
         * If we've got all we need, send it.
         */
        if (actcnt == reqcnt) {
            break;
        }

        /*
         * Save the start position, in case we exceed the buffer
         * limitation, and have to back up one.
         */
        start = sd.sd_last;
        sd.sd_last++;

        if (*sd.sd_last == 0) {
            /* stat() already failed on this one */
            sd.sd_last += len + 1;
            continue;
        }

        memset(&s_path, 0, sizeof(s_path));
        s_path.u_name = sd.sd_last;
        /* Check cache before stat() to avoid syscalls. Files are cached during
         * creation (FPGetFileDirParams) and directories during creation
         * (FPCreateDir -> dir_add). This allows us to skip stat() for cached entries */
        struct dir *cached = dircache_search_by_name(vol, curdir, sd.sd_last, len);

        if (cached) {
            /* Reconstruct stat from cached fields */
            s_path.st.st_ino = cached->dcache_ino;
            s_path.st.st_ctime = cached->dcache_ctime;
            s_path.st.st_mode = cached->dcache_mode;
            s_path.st.st_mtime = cached->dcache_mtime;
            s_path.st.st_uid = cached->dcache_uid;
            s_path.st.st_gid = cached->dcache_gid;
            s_path.st.st_size = cached->dcache_size;
            /* Populate remaining system struct stat fields with safe defaults.
             * These are NOT used by AFP enumeration (getfilparams/getdirparams)
             * but must be initialized since s_path.st is a full struct stat */
            s_path.st.st_dev = 0;      /* not cached; only needed by ad_convert */
            s_path.st.st_nlink = 1;    /* dirs have >=2, but nlink is unused */
            s_path.st.st_rdev = 0;     /* is not a device file */
            s_path.st.st_blocks = 0;   /* AFP uses st_size, not st_blocks */
            s_path.st.st_blksize = 0;  /* not used by AFP */
            s_path.st.st_atime = cached->dcache_mtime;  /* approximate; not used by AFP */
            s_path.st_valid = 1;
            s_path.st_errno = 0;
            /* Pass CNID so getmetadata() skips redundant cache lookup */
            s_path.id = cached->d_did;
            /* Use cached mac name to skip utompath() charset conversion */
            s_path.m_name = cfrombstr(cached->d_m_name);
            LOG(log_maxdebug, logtype_afpd,
                "enumerate: cache hit for '%s' (skipped stat)", sd.sd_last);
        } else {
            /* Cache miss - fall back to original stat() behavior */
            if (of_stat(vol, &s_path) < 0) {
                /* The next time it won't try to stat it again
                 * another solution would be to invalidate the cache with
                 * sd.sd_did = 0 but if it's not ENOENT error it will start again
                 */
                *sd.sd_last = 0;
                sd.sd_last += len + 1;
                curdir->d_offcnt--;		/* a little lie */
                continue;
            }

            LOG(log_maxdebug, logtype_afpd,
                "enumerate: cache miss for '%s' (performed stat)", sd.sd_last);
            /* Entry will be cached below by dir_add() (dirs) or getfilparams() (files) */
        }

        /* On-the-fly v2→EA conversion. Skip when 'convert appledouble = no'
         * is set (AFPVOL_NOV2TOEACONV) — eliminates per-file disk IO. */
        const char *convname;

        if (!(vol->v_flags & AFPVOL_NOV2TOEACONV)
                && ad_convert(sd.sd_last, &s_path.st, vol, &convname) == 0) {
            if (convname) {
                /* ad_convert renamed the file (v2→EA migration). If we used
                 * cached stat data, it has st_dev=0 which breaks CNID ops.
                 * Proactively remove the stale cache entry and refresh stat. */
                if (cached) {
                    dir_remove(vol, cached, 0);  /* Clean removal, not an expunge */
                    cached = NULL;               /* Treat as cache miss from here */
                    of_stat(vol, &s_path);        /* Refresh stat from disk */
                }

                s_path.u_name = (char *)convname;
                AFP_CNID_START("cnid_lookup");
                s_path.id = cnid_lookup(vol->v_cdb, &s_path.st, curdir->d_did, sd.sd_last,
                                        strlen(sd.sd_last));
                AFP_CNID_DONE();

                if (s_path.id != CNID_INVALID) {
                    AFP_CNID_START("cnid_update");
                    int cnid_up_ret = cnid_update(vol->v_cdb, s_path.id, &s_path.st, curdir->d_did,
                                                  (char *)convname, strlen(convname));
                    AFP_CNID_DONE();

                    if (cnid_up_ret != 0) {
                        LOG(log_error, logtype_afpd, "enumerate: error updating CNID of \"%s\"",
                            fullpathname(convname));
                    }
                }
            }
        }

        sd.sd_last += len + 1;

        if (!cached) {
            s_path.m_name = NULL;
        }

        /*
         * If a fil/dir is not a dir, it's a file. This is slightly
         * inaccurate, since that means /dev/null is a file, /dev/printer
         * is a file, etc.
         */
        if (S_ISDIR(s_path.st.st_mode)) {
            if (dbitmap == 0) {
                continue;
            }

            if (cached) {
                /* Reuse cache hit from initial dircache_search_by_name() */
                dir = cached;
            } else {
                int len = strlen(s_path.u_name);

                if ((dir = dir_add(vol, curdir, &s_path, len)) == NULL) {
                    LOG(log_error, logtype_afpd,
                        "enumerate(vid:%u, did:%u, name:'%s'): error adding dir: '%s'",
                        ntohs(vid), ntohl(did), o_path->u_name, s_path.u_name);
                    return AFPERR_MISC;
                }
            }

            if ((ret = getdirparams(obj, vol, dbitmap, &s_path, dir, data + header,
                                    &esz)) != AFP_OK) {
                return ret;
            }
        } else {
            if (fbitmap == 0) {
                continue;
            }

            /* files are added to the dircache in getfilparams() -> getmetadata() */
            if (AFP_OK != (ret = getfilparams(obj, vol, fbitmap, &s_path, curdir,
                                              data + header, &esz, 1))) {
                return ret;
            }
        }

        /*
         * Make sure entry is an even length, possibly with a null
         * byte on the end.
         */
        if ((esz + header) & 1) {
            *(data + header + esz) = '\0';
            esz++;
        }

        /*
         * Check if we've exceeded the size limit.
         */
        if (maxsz < sz + esz + header) {
            if (first) { /* maxsz can't hold a single reply */
                return AFPERR_PARAM;
            }

            sd.sd_last = start;
            break;
        }

        if (first) {
            first = 0;
        }

        sz += esz + header;

        if (ext) {
            temp16 = htons(esz + header);
            memcpy(data, &temp16, sizeof(temp16));
            data += sizeof(temp16);
        } else {
            *data++ = esz + header;
        }

        *data++ = S_ISDIR(s_path.st.st_mode) ? (char) FILDIRBIT_ISDIR :
                  FILDIRBIT_ISFILE;

        if (ext) {
            *data++ = 0;
        }

        data += esz;
        actcnt++;
        /* FIXME if we rollover 16 bits and it's not FPEnumerateExt2 */
    }

    if (actcnt == 0) {
        sd.sd_did = 0;		/* invalidate sd struct to force re-read */

        /*
         * in case were converting adouble stuff:
         * after enumerating the whole dir we should have converted everything
         * thus the .AppleDouble dir shouls be empty thus we can no try to
         * delete it
         */
        if (vol->v_adouble == AD_VERSION_EA && !(vol->v_flags & AFPVOL_NOV2TOEACONV)) {
            (void)rmdir(".AppleDouble");
        }

        return AFPERR_NOOBJ;
    }

    sd.sd_sindex = sindex + actcnt;
    /*
     * All done, fill in misc junk in rbuf
     */
    fbitmap = htons(fbitmap);
    memcpy(rbuf, &fbitmap, sizeof(fbitmap));
    rbuf += sizeof(fbitmap);
    dbitmap = htons(dbitmap);
    memcpy(rbuf, &dbitmap, sizeof(dbitmap));
    rbuf += sizeof(dbitmap);
    actcnt = htons(actcnt);
    memcpy(rbuf, &actcnt, sizeof(actcnt));
    rbuf += sizeof(actcnt);
    *rbuflen = sz;
    return AFP_OK;
}

/* ----------------------------- */
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen,
                  char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* ----------------------------- */
int afp_enumerate_ext(AFPObj *obj, char *ibuf, size_t ibuflen,
                      char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/* ----------------------------- */
int afp_enumerate_ext2(AFPObj *obj, char *ibuf, size_t ibuflen,
                       char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 2);
}
