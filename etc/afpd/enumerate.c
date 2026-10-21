/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2
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
#include "virtual_icon.h"
#include "volume.h"

#define min(a,b)	((a)<(b)?(a):(b))

/*!
 * @brief Struct to save directory reading context in.
 * @note Used to prevent O(n^2) searches on a directory.
 */
struct savedir {
    u_short	 sd_vid;
    uint32_t	 sd_did;
    size_t	 sd_buflen;
    char	 *sd_buf;
    char	 *sd_last;
    unsigned int sd_sindex;
    /* the kinds sd_sindex counts, from the request's bitmaps: 1 files,
     * 2 folders */
    int sd_kinds;
    /* the index one past the last name of those kinds, 0 until a batch
     * reaches it */
    unsigned int sd_end;
};

/* The byte after a saved name's terminator: its kind, SD_UNKNOWN while
 * neither readdir() nor a stat has told it, SD_SKIP once its stat failed */
#define SD_UNKNOWN 0
#define SD_FILE 1
#define SD_DIR 2
#define SD_SKIP 3
#define SDBUFBRK	2048
#define SD_SLOTS	8

/* The listings saved beside enumerate()'s own, most recently used first;
 * an empty slot has sd_did 0 */
static struct savedir sd_parked[SD_SLOTS - 1];

static struct enumerate_stats enumerate_stat;

/*!
 * @brief Copy the session's directory listing counts
 *
 * @param[out] out  the counts
 */
void enumerate_stats_get(struct enumerate_stats *out)
{
    *out = enumerate_stat;
}

/*!
 * @brief Log the session's directory listing counts
 */
void enumerate_log_stats(void)
{
    LOG(log_info, logtype_afpd,
        "enumerate statistics: reads: %llu, continuation rereads: %llu, "
        "names read: %llu",
        enumerate_stat.reads, enumerate_stat.rereads, enumerate_stat.names);
}

/*!
 * @brief Empty a slot, freeing its names
 *
 * @param[in,out] sd  the slot
 */
static void sd_drop(struct savedir *sd)
{
    const struct vol *vol;

    if (sd->sd_did != 0 && (vol = getvolbyvid(sd->sd_vid)) != NULL) {
        cnid_dir_forget(vol->v_cdb, sd->sd_did);
    }

    free(sd->sd_buf);
    sd->sd_buf = NULL;
    sd->sd_buflen = 0;
    sd->sd_did = 0;
}

/*!
 * @brief Bring a directory's saved listing into sd, or an empty slot
 *
 * The listing sd held is parked first. A directory without a saved listing
 * keeps sd when sd is empty, else takes a parked slot, an empty one before
 * the least recently used, whose names are freed.
 *
 * @param[in,out] sd   enumerate()'s listing in use
 * @param[in]     vid  the directory's volume id
 * @param[in]     did  the directory's id
 */
static void sd_select(struct savedir *sd, uint16_t vid, uint32_t did)
{
    struct savedir s;
    int victim = SD_SLOTS - 2;
    int i;

    if (sd->sd_did == did && sd->sd_vid == vid) {
        return;
    }

    for (i = 0; i < SD_SLOTS - 1; i++) {
        if (sd_parked[i].sd_did == did && sd_parked[i].sd_vid == vid) {
            break;
        }

        if (sd_parked[i].sd_did == 0) {
            victim = i;
        }
    }

    if (i == SD_SLOTS - 1) {
        if (sd->sd_did == 0) {
            return;
        }

        i = victim;
        sd_drop(&sd_parked[i]);
    }

    s = sd_parked[i];
    memmove(&sd_parked[1], &sd_parked[0], (size_t)i * sizeof(s));
    sd_parked[0] = *sd;
    *sd = s;
}

/*!
 * @brief A saved name's kind as readdir() tells it
 *
 * @param[in] de  the name's directory entry
 * @returns SD_DIR or SD_FILE by d_type, SD_UNKNOWN for a symlink, an
 *          unknown type, or where readdir() gives no d_type
 */
static char sd_kind(const struct dirent *de)
{
#ifdef DT_DIR

    if (de->d_type == DT_DIR) {
        return SD_DIR;
    }

    /* a symlink is a file or what it points to, by the volume's options */
    if (de->d_type != DT_UNKNOWN && de->d_type != DT_LNK) {
        return SD_FILE;
    }

#else
    (void)de;
#endif
    return SD_UNKNOWN;
}

/*!
 * @brief Append a directory entry name to the enumeration name buffer
 *
 * Each record is a length byte, the name, a NUL and the name's kind. The
 * buffer doubles when the record does not fit.
 *
 * @param[in]     de     directory entry
 * @param[in]     mname  unused
 * @param[in,out] data   struct savedir that holds the buffer
 * @returns 0, or -1 with ENOMEM when the buffer cannot grow
 */
static int enumerate_loop(struct dirent *de, char *mname _U_, void *data)
{
    struct savedir *sd = data;
    size_t used;
    int len;
    int bytes_needed;
    len = (int) strnlen(de->d_name, UTF8FILELEN_EARLY);
    /* the name, its terminator, its kind and the next length byte */
    bytes_needed = len + 3;
    *(sd->sd_last)++ = len;
    used = (size_t)(sd->sd_last - sd->sd_buf);

    if (used + bytes_needed > sd->sd_buflen) {
        char *buf;

        if (!(buf = realloc(sd->sd_buf, sd->sd_buflen * 2))) {
            LOG(log_error, logtype_afpd, "afp_enumerate: realloc: %s",
                strerror(errno));
            errno = ENOMEM;
            return -1;
        }

        sd->sd_buf = buf;
        sd->sd_buflen *= 2;
        sd->sd_last = sd->sd_buf + used;
    }

    memcpy(sd->sd_last, de->d_name, len + 1);
    sd->sd_last += len + 1;
    *(sd->sd_last)++ = sd_kind(de);
    return 0;
}

/*!
 * @brief Leave a saved name out of the listing and have the folder's
 *        offspring counted again
 *
 * @param[in,out] name  the saved name, its kind byte at name + len + 1
 * @param[in]     len   the record's length byte
 */
static void sd_leave(char *name, int len)
{
    name[len + 1] = SD_SKIP;
    /* FPDelete, or a recount since the read, may have taken the name off
     * the count already */
    curdir->d_ctime = 0;
}

/*!
 * @brief Stat a saved name, from its dircache entry when it has one
 *
 * Records the name's kind. A name whose stat fails is marked SD_SKIP, so
 * that no later batch stats it again, and has the folder's offspring
 * counted again.
 *
 * @param[in]     vol     the listing's volume
 * @param[in,out] name    the saved name, its kind byte at name + len + 1
 * @param[in]     len     the record's length byte, longer than the name
 *                        once a conversion has shortened it
 * @param[out]    s_path  the name, its stat and its dircache entry
 * @returns 0 with s_path filled and its d_cached the name's entry or NULL,
 *          -1 for a name whose stat failed
 */
static int sd_stat(const struct vol *vol, char *name, int len,
                   struct path *s_path)
{
    char *kind = name + len + 1;
    struct dir *cached;

    if (*kind == SD_SKIP) {
        return -1;
    }

    memset(s_path, 0, sizeof(*s_path));
    s_path->u_name = name;

    /* a name afpd created or listed before has a dircache entry, whose
     * fields stand in for a stat */
    if ((cached = dircache_search_by_name(vol, curdir, name,
                                          strlen(name))) != NULL) {
        s_path->st.st_ino = cached->dcache_ino;
        s_path->st.st_ctime = cached->dcache_ctime;
        s_path->st.st_mode = cached->dcache_mode;
        s_path->st.st_mtime = cached->dcache_mtime;
        s_path->st.st_uid = cached->dcache_uid;
        s_path->st.st_gid = cached->dcache_gid;
        s_path->st.st_size = cached->dcache_size;
        s_path->st.st_nlink = 1;
        s_path->st.st_atime = cached->dcache_mtime;
        s_path->st_valid = 1;
        /* getfilparams() and getmetadata() take the CNID, the entry and the
         * Mac name from here */
        s_path->id = cached->d_did;
        s_path->d_cached = cached;
        s_path->m_name = cfrombstr(cached->d_m_name);
        LOG(log_maxdebug, logtype_afpd,
            "enumerate: cache hit for '%s' (skipped stat)", name);
    } else if (of_stat(vol, s_path) == 0) {
        LOG(log_maxdebug, logtype_afpd,
            "enumerate: cache miss for '%s' (performed stat)", name);
    } else {
        sd_leave(name, len);
        return -1;
    }

    *kind = S_ISDIR(s_path->st.st_mode) ? SD_DIR : SD_FILE;
    return 0;
}

/*!
 * @brief Filter a directory entry for enumeration
 *
 * Drops ".", "..", names longer than 255 bytes, the names the volume's VFS
 * modules reserve for their metadata, and vetoed names.
 *
 * @bug A dangling symlink defeats the filter. Move a folder that holds one into
 * the trash and empty the trash. afp_enumerate then returns an empty listing
 * while afp_getdirparams still reports offspring, so the client never calls
 * afp_delete.
 * @sa https://sourceforge.net/p/netatalk/bugs/97/
 *
 * @param[in] vol   volume the entry belongs to
 * @param[in] name  entry name as read from the directory
 * @returns name when the client may see the entry, NULL otherwise
 */
char *check_dirent(const struct vol *vol, char *name)
{
    if (!strcmp(name, "..") || !strcmp(name, ".")) {
        return NULL;
    }

    /* the saved listing and the dircache take at most 255 bytes a name */
    if (strnlen(name, UTF8FILELEN_EARLY + 1) > UTF8FILELEN_EARLY) {
        return NULL;
    }

    if (!vol->vfs->vfs_validupath(vol, name)) {
        return NULL;
    }

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name, vol->v_ignorecase)) {
        return NULL;
    }

    return name;
}

/*!
 * @brief Call fn for every entry of directory name that clients may see
 *
 * The check_dirent call filters the entries and supplies the Mac name. The
 * count covers every visible entry, so a NULL fn only counts.
 *
 * @param[in] vol   volume of the directory
 * @param[in] name  directory to walk
 * @param[in] fn    callback, or NULL to count only
 * @param[in] data  callback data
 * @returns the number of visible entries, or -1 when opendir, readdir or fn
 *          failed
 */
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

    /* readdir() returns NULL at the end and on an error alike */
    for (errno = 0; (de = readdir(dp)) != NULL; errno = 0) {
        if (!(m_name = check_dirent(vol, de->d_name))) {
            continue;
        }

        ret++;

        if (fn && fn(de, m_name, data) < 0) {
            closedir(dp);
            return -1;
        }
    }

    if (errno != 0) {
        ret = -1;
    }

    closedir(dp);
    return ret;
}

/*! This is the maximal length of a single entry for a file/dir in the reply
   block if all bits in the file/dir bitmap are set: header(4) + params(104) +
   macnamelength(1) + macname(31) + utf8(4) + utf8namelen(2) + utf8name(255) +
   oddpadding(1) */

#define REPLY_PARAM_MAXLEN (4 + 104 + 1 + MACFILELEN + 4 + 2 + UTF8FILELEN_EARLY + 1)

/*!
 * @brief Shared body of FPEnumerate, FPEnumerateExt, and FPEnumerateExt2
 *
 * The request names a directory, a file bitmap, a directory bitmap, the
 * requested count, a start index from 1, and the reply size limit.
 * FPEnumerateExt2 widens the start index and the limit to 32 bits. The first
 * request for a directory reads every visible name into a saved listing, and
 * later requests for it continue from that listing; the session keeps the
 * listings of eight directories. Entries in the dircache skip the stat.
 *
 * Each entry carries its length, a file or directory flag, and the parameters
 * of getfilparams or getdirparams, padded to an even length. An entry type
 * with a zero bitmap stays out of the reply. The volume root ends with the
 * virtual Icon file when the volume provides one. An index past the last entry
 * fails with AFPERR_NOOBJ.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @param[in]     ext      0 for FPEnumerate, 1 for FPEnumerateExt, 2 for
 *                         FPEnumerateExt2
 * @returns AFP_OK, AFPERR_NOOBJ at the end of the directory, AFPERR_NODIR,
 *          AFPERR_BITMAP, AFPERR_PARAM, or another AFPERR_* code
 */
static int enumerate(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen, int ext)
{
    static struct savedir	sd = { 0 };
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
    size_t                      header;
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

    /* At least one result bitmap must be nonzero. */
    if (!(fbitmap || dbitmap)) {
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
        return AFPERR_PARAM;
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

    header = (size_t)(ext ? 4 : 2);
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
    sd_select(&sd, vid, curdir->d_did);

    if (sd.sd_buflen == 0) {
        if ((sd.sd_buf = (char *)malloc(SDBUFBRK)) == NULL) {
            LOG(log_error, logtype_afpd, "afp_enumerate: malloc: %s",
                strerror(errno));
            return AFPERR_MISC;
        }

        sd.sd_buflen = SDBUFBRK;
        /* an empty listing until a read fills it */
        *sd.sd_buf = 0;
    }

    /*
     * Read the directory into a pre-malloced buffer, stored
     *		len <name> \0 <kind>
     * The end is indicated by a len of 0.
     */
    if (sindex == 1 || curdir->d_did != sd.sd_did || vid != sd.sd_vid) {
        enumerate_stat.reads++;

        /* a continuation batch whose listing is not saved */
        if (sindex != 1) {
            enumerate_stat.rereads++;
        }

        sd.sd_last = sd.sd_buf;

        /* if dir was in the cache we don't have the inode */
        if ((!o_path->st_valid && ostat(".", &o_path->st, vol_syml_opt(vol)) < 0) ||
                (ret = for_each_dirent(vol, ".", enumerate_loop, (void *)&sd)) < 0) {
            int err = errno;
            sd_drop(&sd);
            errno = err;
            LOG(log_error, logtype_afpd, "enumerate: loop error: %s (%d)", strerror(errno),
                errno);

            switch (errno) {
            case EACCES:
                return AFPERR_ACCESS;

            case ENOTDIR:
                return AFPERR_BADTYPE;

            case EMFILE:
            case ENFILE:
            case ENOMEM:
                return AFPERR_MISC;

            default:
                return AFPERR_NODIR;
            }
        }

        setdiroffcnt(vol, curdir, &o_path->st, ret);

        /* a listing's first read arms its table: the read again of a
         * listing whose slot went would load the rows for every batch */
        if (sindex == 1) {
            cnid_dir_arm(vol->v_cdb, curdir->d_did);
        }

        enumerate_stat.names += ret;
        *sd.sd_last = 0;
        sd.sd_last = sd.sd_buf;
        sd.sd_sindex = 1;
        sd.sd_end = 0;
        sd.sd_vid = vid;
        sd.sd_did = curdir->d_did;
    }

    /*
     * Position sd_last as dictated by sindex.
     */
    int kinds = (fbitmap != 0) | (dbitmap != 0) << 1;

    if (sindex < sd.sd_sindex || kinds != sd.sd_kinds) {
        sd.sd_sindex = 1;
        sd.sd_last = sd.sd_buf;
        sd.sd_kinds = kinds;
        sd.sd_end = 0;
    }

    while (sd.sd_sindex < sindex) {
        len = (unsigned char) * (sd.sd_last)++;

        if (len == 0) {
            sd_drop(&sd);
            return AFPERR_NOOBJ;
        }

        char *name = sd.sd_last;
        sd.sd_last += len + 2;

        /* the index counts the names a batch lists: those whose stat
         * succeeds, of a kind the request asks for */
        if (name[len + 1] == SD_UNKNOWN
                && sd_stat(vol, name, len, &s_path) != 0) {
            continue;
        }

        if (name[len + 1] == SD_SKIP
                || (name[len + 1] == SD_DIR ? dbitmap : fbitmap) == 0) {
            continue;
        }

        sd.sd_sindex++;
    }

    struct stat adst;

    int convert = vol->v_adouble == AD_VERSION_EA
                  && !(vol->v_flags & AFPVOL_NOV2TOEACONV);
    /* a file's adouble:v2 header can only be in this folder's .AppleDouble */
    int v2dir = convert && lstat(".AppleDouble", &adst) == 0;
    /* a batch that fails returns with sd_last back at the batch's first
     * name, so the next batch at sindex resumes there */
    char *batch = sd.sd_last;
    cnid_dir_serve(vol->v_cdb, curdir->d_did);

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

        if (sd_stat(vol, sd.sd_last, len, &s_path) != 0) {
            sd.sd_last += len + 2;
            continue;
        }

        /* NULL on a miss: dir_add() or getfilparams() caches the entry below */
        struct dir *cached = s_path.d_cached;
        /* On-the-fly v2→EA conversion. Skip when 'convert appledouble = no'
         * is set (AFPVOL_NOV2TOEACONV) — eliminates per-file disk IO. */
        const char *convname;
        /* a folder's header is inside the folder; ad_convert() also renames
         * a hex-encoded name */
        int attempt = convert
                      && (S_ISDIR(s_path.st.st_mode)
                          ? !(cached && (cached->d_flags & DIRF_CONVERTED))
                          : v2dir || strstr(sd.sd_last, ":2e")
                          || strstr(sd.sd_last, ":2f"));

        if (attempt
                && ad_convert(sd.sd_last, &s_path.st, vol, &convname) == 0) {
            struct stat oldst;

            /* a folder handed a decoded name stays unmarked, so a failed
             * rename is tried again */
            if (cached && !convname && S_ISDIR(s_path.st.st_mode)) {
                cached->d_flags |= DIRF_CONVERTED;
            }

            /* ad_convert() hands back the decoded name whether or not its
             * rename happened; a failed one leaves the old name in place */
            if (convname && lstat(sd.sd_last, &oldst) == 0) {
                convname = NULL;
            }

            if (convname) {
                /* ad_convert() dehexed the name. A stat built from the
                 * dircache has st_dev 0, which breaks the CNID calls: drop
                 * the entry and stat the new name */
                s_path.u_name = (char *)convname;

                if (cached) {
                    dir_remove(vol, cached, 0);  /* Clean removal, not an expunge */
                    cached = NULL;               /* Treat as cache miss from here */

                    if (of_stat(vol, &s_path) != 0) {
                        sd_leave(sd.sd_last, len);
                        sd.sd_last += len + 2;
                        continue;
                    }
                }

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
                } else if (CNID_ERRNO_IS_BACKEND_FAILURE()) {
                    /* ad_convert has already renamed the file, so the CNID row
                     * names a path that is gone and the next lookup mints a
                     * fresh CNID for it */
                    LOG(log_error, logtype_afpd,
                        "enumerate: CNID backend error (errno=0x%x), the CNID of "
                        "converted \"%s\" still names the pre-conversion file",
                        CNID_ERRNO(), fullpathname(convname));
                }

                /* a name the listing hides, once decoded, leaves it */
                if (!check_dirent(vol, s_path.u_name)) {
                    sd_leave(sd.sd_last, len);
                    sd.sd_last += len + 2;
                    continue;
                }

                /* later batches find the entry under its new name, which
                 * dehexing makes shorter than the saved one */
                if (strlen(convname) < (size_t)len) {
                    memcpy(sd.sd_last, convname, strlen(convname) + 1);
                }
            }
        }

        sd.sd_last += len + 2;

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
                    ret = AFPERR_MISC;
                    goto fail;
                }
            }

            if ((ret = getdirparams(obj, vol, dbitmap, &s_path, dir, data + header,
                                    &esz)) != AFP_OK) {
                goto fail;
            }
        } else {
            if (fbitmap == 0) {
                continue;
            }

            /* files are added to the dircache in getfilparams() -> getmetadata() */
            if (AFP_OK != (ret = getfilparams(obj, vol, fbitmap, &s_path, curdir,
                                              data + header, &esz, 1))) {
                goto fail;
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
                ret = AFPERR_PARAM;
                goto fail;
            }

            sd.sd_last = start;
            break;
        }

        if (first) {
            first = 0;
        }

        sz += esz + header;
        {
            size_t entry_len = esz + header;

            if (entry_len > (ext ? UINT16_MAX : UINT8_MAX)) {
                ret = AFPERR_PARAM;
                goto fail;
            }

            if (ext) {
                temp16 = htons((uint16_t)entry_len);
                memcpy(data, &temp16, sizeof(temp16));
                data += sizeof(temp16);
            } else {
                *data++ = (uint8_t)entry_len;
            }
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

    cnid_dir_serve(vol->v_cdb, CNID_INVALID);

    /*
     * Inject virtual Icon\r entry at the volume root if enabled.
     * This is treated as one extra entry appended after real entries.
     * We track it via sindex: the virtual entry occupies position
     * (real_count + 1). Because sd_sindex is updated below, subsequent
     * requests with higher sindex will skip past it.
     *
     * The virtual entry occupies exactly one sindex position: one past
     * the listing's last entry of the kinds it lists (sd_end, 1-based).
     * Only emit it when the current batch reaches that position, so it is
     * served once and subsequent calls fall through to AFPERR_NOOBJ.
     */
    if (len == 0 && sd.sd_end == 0) {
        sd.sd_end = sindex + actcnt;
    }

    if (len == 0 && actcnt < reqcnt && fbitmap != 0
            && curdir->d_did == DIRDID_ROOT
            && sindex + actcnt == sd.sd_end
            && virtual_icon_enabled(vol)
            && !real_icon_exists(vol)) {
        esz = 0;
        ret = virtual_icon_getfilparams(obj, vol, fbitmap, 0,
                                        data + header, &esz);

        if (ret == AFP_OK) {
            if ((esz + header) & 1) {
                *(data + header + esz) = '\0';
                esz++;
            }

            if (maxsz >= sz + esz + header) {
                sz += esz + header;
                {
                    size_t entry_len = esz + header;

                    if (entry_len > (ext ? UINT16_MAX : UINT8_MAX)) {
                        ret = AFPERR_PARAM;
                        goto fail;
                    }

                    if (ext) {
                        temp16 = htons((uint16_t)entry_len);
                        memcpy(data, &temp16, sizeof(temp16));
                        data += sizeof(temp16);
                    } else {
                        *data++ = (uint8_t)entry_len;
                    }
                }
                *data++ = FILDIRBIT_ISFILE;

                if (ext) {
                    *data++ = 0;
                }

                actcnt++;
            }
        }
    }

    if (actcnt == 0) {
        sd_drop(&sd);

        /*
         * in case were converting adouble stuff:
         * after enumerating the whole dir we should have converted everything
         * thus the .AppleDouble dir shouls be empty thus we can no try to
         * delete it
         */
        if (v2dir) {
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
fail:
    cnid_dir_serve(vol->v_cdb, CNID_INVALID);
    sd.sd_last = batch;
    return ret;
}

/*!
 * @brief FPEnumerate: list directory entries with one-byte entry lengths
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of enumerate
 */
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen,
                  char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/*!
 * @brief FPEnumerateExt: list directory entries with two-byte entry lengths
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of enumerate
 */
int afp_enumerate_ext(AFPObj *obj, char *ibuf, size_t ibuflen,
                      char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/*!
 * @brief FPEnumerateExt2: list directory entries with a 32-bit start index and
 * reply size limit
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of enumerate
 */
int afp_enumerate_ext2(AFPObj *obj, char *ibuf, size_t ibuflen,
                       char *rbuf, size_t *rbuflen)
{
    return enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, 2);
}
