/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2010      Frank Lahm
 *
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/asp.h>
#include <atalk/cnid.h>
#include <atalk/dsi.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>

#include "ad_cache.h"
#include "dircache.h"
#include "desktop.h"
#include "directory.h"
#include "file.h"
#include "fork.h"
#include "virtual_icon.h"
#include "volume.h"

static int getforkparams(const AFPObj *obj, struct ofork *ofork,
                         uint16_t bitmap, char *buf, size_t *buflen)
{
    struct path         path;
    struct stat     *st;
    struct adouble  *adp;
    struct dir      *dir;
    struct vol      *vol;

    /* Virtual forks use synthesized params */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return virtual_icon_getfilparams(obj, ofork->of_vol, bitmap,
                                         buf, buflen);
    }

    /* can only get the length of the opened fork */
    if (((bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN)))
            && (ofork->of_flags & AFPFORK_RSRC))
            ||
            ((bitmap & ((1 << FILPBIT_RFLEN) | (1 << FILPBIT_EXTRFLEN)))
             && (ofork->of_flags & AFPFORK_DATA))) {
        return AFPERR_BITMAP;
    }

    if (! AD_META_OPEN(ofork->of_ad)) {
        adp = NULL;
    } else {
        adp = ofork->of_ad;
    }

    vol = ofork->of_vol;
    dir = dirlookup(vol, ofork->of_did);

    if (NULL == (path.u_name = mtoupath(vol, of_name(ofork), dir->d_did,
                                        utf8_encoding(obj)))) {
        return AFPERR_MISC;
    }

    path.m_name = of_name(ofork);
    path.id = 0;
    st = &path.st;

    if (bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN) |
                  (1 << FILPBIT_FNUM) | (1 << FILPBIT_CDATE) |
                  (1 << FILPBIT_MDATE) | (1 << FILPBIT_BDATE))) {
        if (ad_data_fileno(ofork->of_ad) <= 0) {
            /* 0 is for symlink */
            if (movecwd(vol, dir) < 0) {
                return AFPERR_NOOBJ;
            }

            if (lstat(path.u_name, st) < 0) {
                return AFPERR_NOOBJ;
            }
        } else {
            if (fstat(ad_data_fileno(ofork->of_ad), st) < 0) {
                return AFPERR_BITMAP;
            }
        }
    }

    return getmetadata(obj, vol, bitmap, &path, dir, buf, buflen, adp);
}

static off_t get_off_t(char **ibuf, int is64)
{
    uint32_t             temp;
    off_t                 ret;
    ret = 0;
    memcpy(&temp, *ibuf, sizeof(temp));
    ret = ntohl(temp); /* ntohl is unsigned */
    *ibuf += sizeof(temp);

    if (is64) {
        memcpy(&temp, *ibuf, sizeof(temp));
        *ibuf += sizeof(temp);
        ret = ntohl(temp) | ((unsigned long long) ret << 32);
    } else {
        ret = (int)ret; /* sign extend */
    }

    return ret;
}

static int set_off_t(off_t offset, char *rbuf, int is64)
{
    uint32_t  temp;
    int        ret;
    ret = 0;

    if (is64) {
        temp = htonl((uint64_t) offset >> 32);
        memcpy(rbuf, &temp, sizeof(temp));
        rbuf += sizeof(temp);
        ret = sizeof(temp);
        offset &= 0xffffffff;
    }

    temp = htonl(offset);
    memcpy(rbuf, &temp, sizeof(temp));
    ret += sizeof(temp);
    return ret;
}

static int is_neg(int is64, off_t val)
{
    if (val < 0 || (sizeof(off_t) == 8 && !is64 && (val & 0x80000000U))) {
        return 1;
    }

    return 0;
}

static int sum_neg(int is64, off_t offset, off_t reqcount)
{
    if (is_neg(is64, offset + reqcount)) {
        return 1;
    }

    return 0;
}

static int fork_setmode(const AFPObj *obj _U_, struct adouble *adp, int eid,
                        int access, int ofrefnum)
{
    int ret;
    int readset;
    int writeset;
    int denyreadset;
    int denywriteset;

    if (!(access & (OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD))) {
        return ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_NONE, 1,
                       ofrefnum);
    }

    if (access & (OPENACC_RD | OPENACC_DRD)) {
        if ((readset = ad_testlock(adp, eid, AD_FILELOCK_OPEN_RD)) < 0) {
            return readset;
        }

        if ((denyreadset = ad_testlock(adp, eid, AD_FILELOCK_DENY_RD)) < 0) {
            return denyreadset;
        }

        if ((access & OPENACC_RD) && denyreadset) {
            errno = EACCES;
            return -1;
        }

        if ((access & OPENACC_DRD) && readset) {
            errno = EACCES;
            return -1;
        }

        /* boolean logic is not enough, because getforkmode is not always telling the
         * true
         */
        if (access & OPENACC_RD) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_RD, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }

        if (access & OPENACC_DRD) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_DENY_RD, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }
    }

    /* ------------same for writing -------------- */
    if (access & (OPENACC_WR | OPENACC_DWR)) {
        if ((writeset = ad_testlock(adp, eid, AD_FILELOCK_OPEN_WR)) < 0) {
            return writeset;
        }

        if ((denywriteset = ad_testlock(adp, eid, AD_FILELOCK_DENY_WR)) < 0) {
            return denywriteset;
        }

        if ((access & OPENACC_WR) && denywriteset) {
            errno = EACCES;
            return -1;
        }

        if ((access & OPENACC_DWR) && writeset) {
            errno = EACCES;
            return -1;
        }

        if (access & OPENACC_WR) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_WR, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }

        if (access & OPENACC_DWR) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_DENY_WR, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }
    }

    /*
     * Fix for Bug 560: put the Solaris share reservation after our locking stuff.
     * Note that this still leaves room for a race condition where between placing our own
     * locks above and putting the Solaris share move below another client puts a lock.
     * We then end up with set locks from above and return with an error code, the proper
     * fix requires a sane cleanup function for the error path in this function.
     */
#ifdef HAVE_FSHARE_T
    fshare_t shmd;

    if (obj->options.flags & OPTION_SHARE_RESERV) {
        shmd.f_access = (access & OPENACC_RD ? F_RDACC : 0) | (access & OPENACC_WR ?
                        F_WRACC : 0);

        if (shmd.f_access == 0)
            /* we must give an access mode, otherwise fcntl will complain */
        {
            shmd.f_access = F_RDACC;
        }

        shmd.f_deny = (access & OPENACC_DRD ? F_RDDNY : F_NODNY) |
                      (access & OPENACC_DWR) ? F_WRDNY : 0;
        shmd.f_id = ofrefnum;
        int fd = (eid == ADEID_DFORK) ? ad_data_fileno(adp) : ad_reso_fileno(adp);

        if (fd != -1 && fd != AD_SYMLINK && fcntl(fd, F_SHARE, &shmd) != 0) {
            LOG(log_debug, logtype_afpd, "fork_setmode: fcntl: %s", strerror(errno));
            errno = EACCES;
            return -1;
        }
    }

#endif
    return 0;
}

/* ----------------------- */
int afp_openfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                 size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *dir;
    struct ofork    *ofork, *opened;
    struct adouble  *adsame = NULL;
    size_t          buflen;
    int             ret, adflags, eid;
    uint32_t        did;
    uint16_t        vid, bitmap, access, ofrefnum;
    char            fork, *path, *upath;
    struct stat     *st;
    uint16_t        bshort;
    struct path     *s_path;
    ibuf++;
    fork = *ibuf++;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    *rbuflen = 0;

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(int);

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    memcpy(&access, ibuf, sizeof(access));
    access = ntohs(access);
    ibuf += sizeof(access);

    if ((vol->v_flags & AFPVOL_RO) && (access & OPENACC_WR)) {
        return AFPERR_VLOCK;
    }

    /* Save ibuf position for invalid-on-use retry */
    char *saved_ibuf = ibuf;

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_PARAM);
    }

    if (*s_path->m_name == '\0') {
        /* it's a dir ! */
        return  AFPERR_BADTYPE;
    }

    /* stat() data fork st is set because it's not a dir */
    switch (s_path->st_errno) {
    case 0:
        break;

    case ENOENT:

        /* Virtual Icon\r file at volume root */
        if (curdir->d_did == DIRDID_ROOT
                && is_virtual_icon_name(s_path->u_name)
                && virtual_icon_enabled(vol)) {
            size_t vlen;
            const unsigned char *vdata = virtual_icon_get_rfork(vol, &vlen);

            if (!vdata) {
                return AFPERR_MISC;
            }

            /*
             * If write access is requested, materialize the virtual
             * file: create a real empty Icon\r on disk so the Finder
             * can replace the default icon with a custom one.  Then
             * fall through to the normal open path.
             */
            if (access & OPENACC_WR) {
                int cfd = open(s_path->u_name,
                               O_CREAT | O_WRONLY, 0666);

                if (cfd < 0) {
                    LOG(log_error, logtype_afpd,
                        "afp_openfork(virtual Icon\\r): "
                        "create failed: %s",
                        strerror(errno));
                    return AFPERR_MISC;
                }

                /* Seed the resource fork with the virtual icon data
                 * so the Finder sees a valid resource fork. */
                if (sys_fsetxattr(cfd, AD_EA_RESO,
                                  vdata, vlen, 0) < 0) {
                    LOG(log_warning, logtype_afpd,
                        "afp_openfork(virtual Icon\\r): "
                        "seed rfork xattr failed: %s",
                        strerror(errno));
                }

                close(cfd);
                /* Set FinderInfo invisible flag so the Finder
                 * hides the Icon\r file in directory listings. */
                {
                    struct adouble icon_ad;
                    ad_init(&icon_ad, vol);

                    if (ad_open(&icon_ad, s_path->u_name,
                                ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                                0666) == 0) {
                        char *finfo = ad_entry(&icon_ad, ADEID_FINDERI);

                        if (finfo) {
                            memset(finfo, 0, ADEDLEN_FINDERI);
                            uint16_t flags = htons(FINDERINFO_INVISIBLE);
                            memcpy(finfo + FINDERINFO_FRFLAGOFF,
                                   &flags, sizeof(flags));
                            ad_flush(&icon_ad);
                        }

                        ad_close(&icon_ad, ADFLAGS_HF);
                    } else {
                        LOG(log_warning, logtype_afpd,
                            "afp_openfork(virtual Icon\\r): "
                            "metadata init failed: %s",
                            strerror(errno));
                    }
                }

                if (ostat(s_path->u_name, &s_path->st,
                          vol_syml_opt(vol)) < 0) {
                    return AFPERR_MISC;
                }

                s_path->st_errno = 0;
                s_path->st_valid = 1;
                break;  /* fall through to normal open */
            }

            /* Allocate a minimal ofork for the virtual file.
             * We use a synthetic stat and adouble. */
            struct stat vst;
            struct adouble *vad;
            memset(&vst, 0, sizeof(vst));
            vst.st_mode = S_IFREG | 0444;
            vst.st_ino = VIRTUAL_ICON_CNID;  /* synthetic inode */
            vst.st_dev = 0;
            eid = (fork == OPENFORK_DATA) ? ADEID_DFORK : ADEID_RFORK;
            vad = malloc(sizeof(struct adouble));

            if (!vad) {
                return AFPERR_MISC;
            }

            ad_init(vad, vol);
            vad->ad_name = strdup(VIRTUAL_ICON_NAME);

            if (!vad->ad_name) {
                free(vad);
                return AFPERR_MISC;
            }

            ofork = of_alloc(vol, curdir, (char *)VIRTUAL_ICON_NAME,
                             &ofrefnum, eid, vad, &vst);

            if (!ofork) {
                free(vad->ad_name);
                free(vad);
                return AFPERR_NFILE;
            }

            ofork->of_flags |= AFPFORK_VIRTUAL | AFPFORK_ACCRD;
            ofork->of_virtual_data = vdata;
            ofork->of_virtual_len = (off_t)vlen;
            /* Fill reply with file params */
            buflen = 0;
            ret = virtual_icon_getfilparams(obj, vol, bitmap,
                                            rbuf + 2 * sizeof(uint16_t),
                                            &buflen);

            if (ret != AFP_OK) {
                LOG(log_error, logtype_afpd,
                    "afp_openfork(virtual Icon\\r): getfilparams failed: %d",
                    ret);
                of_dealloc(ofork);
                return ret;
            }

            *rbuflen = buflen + 2 * sizeof(uint16_t);
            bitmap = htons(bitmap);
            memcpy(rbuf, &bitmap, sizeof(uint16_t));
            rbuf += sizeof(uint16_t);
            memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
            return AFP_OK;
        }

        /* Invalid-on-use recovery: stale cached path may have caused
         * cname() to resolve to a non-existent location. Retry with
         * dirlookup_strict() which validates via stat+inode check,
         * evicts stale entries, and rebuilds from CNID. */
        if (dir->d_did != DIRDID_ROOT && dir->d_did != DIRDID_ROOT_PARENT) {
            LOG(log_debug, logtype_afpd,
                "afp_openfork: ENOENT for did:%u, retrying with strict lookup",
                ntohl(did));
            ibuf = saved_ibuf;

            if (!(dir->d_flags & DIRF_ISFILE)) {
                (void)dircache_remove_children(vol, dir);
            }

            (void)dir_remove(vol, dir, 0);
            dir = dirlookup_strict(vol, did);

            if (dir != NULL) {
                s_path = cname(vol, dir, &ibuf);

                if (s_path != NULL && s_path->st_errno == 0) {
                    break;  /* Recovery succeeded — continue normally */
                }
            }
        }

        return AFPERR_NOOBJ;

    case EACCES:
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;

    default:
        LOG(log_error, logtype_afpd, "afp_openfork(%s): %s", s_path->m_name,
            strerror(errno));
        return AFPERR_PARAM;
    }

    upath = s_path->u_name;
    path = s_path->m_name;
    st = &s_path->st;

    if (!vol_unix_priv(vol)) {
        if (check_access(obj, vol, upath, access) < 0) {
            return AFPERR_ACCESS;
        }
    } else {
        if (file_access(obj, vol, s_path, access) < 0) {
            return AFPERR_ACCESS;
        }
    }

    if ((opened = of_findname(vol, s_path))) {
        adsame = opened->of_ad;
    }

    adflags = ADFLAGS_SETSHRMD;

    if (fork == OPENFORK_DATA) {
        eid = ADEID_DFORK;
        adflags |= ADFLAGS_DF;
    } else {
        eid = ADEID_RFORK;
        adflags |= ADFLAGS_RF;
    }

    if (access & OPENACC_WR) {
        adflags |= ADFLAGS_RDWR;

        if (fork != OPENFORK_DATA)
            /*
             * We only try to create the resource
             * fork if the user wants to open it for write acess.
             */
        {
            adflags |= ADFLAGS_CREATE;
        }
    } else {
        adflags |= ADFLAGS_RDONLY;
    }

    if ((ofork = of_alloc(vol, curdir, path, &ofrefnum, eid, adsame, st)) == NULL) {
        return AFPERR_NFILE;
    }

    LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\", %s, %s)",
        fullpathname(s_path->u_name),
        (fork == OPENFORK_DATA) ? "data" : "reso",
        !(access & OPENACC_WR) ? "O_RDONLY" : "O_RDWR");
    ret = AFPERR_NOOBJ;

    /* First ad_open(), opens data or resource fork */
    if (ad_open(ofork->of_ad, upath, adflags, 0666) < 0) {
        switch (errno) {
        case EROFS:
            ret = AFPERR_VLOCK;

        case EACCES:
            goto openfork_err;

        case ENOENT:
            goto openfork_err;

        case EMFILE :
        case ENFILE :
            ret = AFPERR_NFILE;
            goto openfork_err;

        case EISDIR :
            ret = AFPERR_BADTYPE;
            goto openfork_err;

        default:
            LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name,
                strerror(errno));
            ret = AFPERR_PARAM;
            goto openfork_err;
        }
    }

    /*
     * Create metadata if we open rw, otherwise only open existing metadata
     */
    if (access & OPENACC_WR) {
        adflags = ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE;
    } else {
        adflags = ADFLAGS_HF | ADFLAGS_RDONLY;
    }

    if (ad_open(ofork->of_ad, upath, adflags, 0666) == 0) {
        ofork->of_flags |= AFPFORK_META;

        if (ad_get_MD_flags(ofork->of_ad) & O_CREAT) {
            LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\"): setting CNID", upath);
            cnid_t id;

            if ((id = get_id(vol, ofork->of_ad, st, dir->d_did, upath,
                             strlen(upath))) == CNID_INVALID) {
                LOG(log_error, logtype_afpd, "afp_createfile(\"%s\"): CNID error", upath);
                goto openfork_err;
            }

            (void)ad_setid(ofork->of_ad, st->st_dev, st->st_ino, id, dir->d_did,
                           vol->v_stamp);
            ad_flush(ofork->of_ad);
        }
    } else {
        switch (errno) {
        case EACCES:
        case ENOENT:
            /* no metadata? We don't care! */
            break;

        case EROFS:
            ret = AFPERR_VLOCK;

        case EMFILE :
        case ENFILE :
            ret = AFPERR_NFILE;
            goto openfork_err;

        case EISDIR :
            ret = AFPERR_BADTYPE;
            goto openfork_err;

        default:
            LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name,
                strerror(errno));
            ret = AFPERR_PARAM;
            goto openfork_err;
        }
    }

    if ((adflags & ADFLAGS_RF) && (ad_get_RF_flags(ofork->of_ad) & O_CREAT)) {
        if (ad_setname(ofork->of_ad, path)) {
            ad_flush(ofork->of_ad);
        }
    }

    if ((ret = getforkparams(obj, ofork, bitmap, rbuf + 2 * sizeof(int16_t),
                             &buflen)) != AFP_OK) {
        ad_close(ofork->of_ad, adflags | ADFLAGS_SETSHRMD);
        goto openfork_err;
    }

    *rbuflen = buflen + 2 * sizeof(uint16_t);
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(uint16_t));
    rbuf += sizeof(uint16_t);

    /* check WriteInhibit bit if we have a resource fork
     * the test is done here, after some client traffic capture
     */
    if (ad_meta_fileno(ofork->of_ad) != -1) {   /* META */
        ad_getattr(ofork->of_ad, &bshort);

        if ((bshort & htons(ATTRBIT_NOWRITE)) && (access & OPENACC_WR)) {
            ad_close(ofork->of_ad, adflags | ADFLAGS_SETSHRMD);
            of_dealloc(ofork);
            ofrefnum = 0;
            memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
            return AFPERR_OLOCK;
        }
    }

    /*
     * synchronization locks:
     */

    /* don't try to lock non-existent rforks. */
    if ((eid == ADEID_DFORK)
            || (ad_reso_fileno(ofork->of_ad) != -1)
            || (ofork->of_ad->ad_vers == AD_VERSION_EA)) {
        ret = fork_setmode(obj, ofork->of_ad, eid, access, ofrefnum);

        /* can we access the fork? */
        if (ret < 0) {
            ofork->of_flags |= AFPFORK_ERROR;
            ret = errno;
            ad_close(ofork->of_ad, adflags | ADFLAGS_SETSHRMD);
            of_dealloc(ofork);

            switch (ret) {
            case EAGAIN: /* return data anyway */
            case EACCES:
            case EINVAL:
                ofrefnum = 0;
                memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
                return AFPERR_DENYCONF;

            default:
                *rbuflen = 0;
                LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_lock: %s", s_path->m_name,
                    strerror(ret));
                return AFPERR_PARAM;
            }
        }

        if (access & OPENACC_WR) {
            ofork->of_flags |= AFPFORK_ACCWR;
        }
    }

    /* the file may be open read only without resource fork */
    if (access & OPENACC_RD) {
        ofork->of_flags |= AFPFORK_ACCRD;
    }

    LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\"): fork: %" PRIu16,
        fullpathname(s_path->m_name), ofork->of_refnum);
    memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
    return AFP_OK;
openfork_err:
    of_dealloc(ofork);

    if (errno == EACCES) {
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;
    }

    return ret;
}

/*!
 * @brief Invalidate cached rfork data for a file identified by its open fork.
 *
 * Looks up the file's dircache entry via parent DID + name, and invalidates
 * the AD cache (Tier 1 + Tier 2). Used after rfork writes and truncations.
 */
static void rfork_invalidate_for_ofork(const struct vol *vol,
                                       struct ofork *ofork)
{
    const struct dir *parentdir = dirlookup(vol, ofork->of_did);

    if (parentdir) {
        char *name = of_name(ofork);
        struct dir *cached = dircache_search_by_name(vol, parentdir,
                             name, strnlen(name, MAXPATHLEN));

        if (cached) {
            dir_modify(vol, cached, &(struct dir_modify_args) {
                .flags = DCMOD_AD_INV,
            });
        }
    }
}

int afp_setforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf _U_,
                      size_t *rbuflen)
{
    struct ofork    *ofork;
    struct vol      *vol;
    struct dir      *dir;
    off_t       size;
    uint16_t       ofrefnum, bitmap;
    int                 err;
    int                 is64;
    int                 eid;
    off_t       st_size;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    *rbuflen = 0;

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_setforkparams: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return AFPERR_OLOCK;
    }

    vol = ofork->of_vol;

    if ((dir = dirlookup(vol, ofork->of_did)) == NULL) {
        LOG(log_error, logtype_afpd, "%s: bad fork did",  of_name(ofork));
        return AFPERR_MISC;
    }

    if (movecwd(vol, dir) != 0) {
        LOG(log_error, logtype_afpd, "%s: bad fork directory",
            dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
        return afp_errno;
    }

    if (ofork->of_vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
        return AFPERR_ACCESS;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        return AFPERR_PARAM;
    }

    if (((bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN)))
            && eid == ADEID_RFORK
        ) ||
            ((bitmap & ((1 << FILPBIT_RFLEN) | (1 << FILPBIT_EXTRFLEN)))
             && eid == ADEID_DFORK)) {
        return AFPERR_BITMAP;
    }

    is64 = 0;

    if (bitmap & ((1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN))) {
        if (obj->afp_version >= 30) {
            is64 = 4;
        } else {
            return AFPERR_BITMAP;
        }
    }

    if (ibuflen < 2 + sizeof(ofrefnum) + sizeof(bitmap) + is64 + 4) {
        return AFPERR_PARAM ;
    }

    size = get_off_t(&ibuf, is64);

    if (size < 0) {
        /* Some MacOS don't return an error they just don't change the size! */
        return AFPERR_PARAM;
    }

    if (bitmap == (1 << FILPBIT_DFLEN) || bitmap == (1 << FILPBIT_EXTDFLEN)) {
        st_size = ad_size(ofork->of_ad, eid);
        err = -2;

        if (st_size > size &&
                ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size - size,
                           ofork->of_refnum) < 0) {
            goto afp_setfork_err;
        }

        err = ad_dtruncate(ofork->of_ad, size);

        if (st_size > size) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size - size,
                       ofork->of_refnum);
        }

        if (err < 0) {
            goto afp_setfork_err;
        }
    } else if (bitmap == (1 << FILPBIT_RFLEN)
               || bitmap == (1 << FILPBIT_EXTRFLEN)) {
        ad_refresh(NULL, ofork->of_ad);
        st_size = ad_size(ofork->of_ad, eid);
        err = -2;

        if (st_size > size &&
                ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size - size,
                           ofork->of_refnum) < 0) {
            goto afp_setfork_err;
        }

        err = ad_rtruncate(ofork->of_ad, mtoupath(ofork->of_vol, of_name(ofork),
                           ofork->of_did, utf8_encoding(obj)), size);

        if (st_size > size) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size - size,
                       ofork->of_refnum);
        }

        if (err < 0) {
            goto afp_setfork_err;
        }

        if (ad_flush(ofork->of_ad) < 0) {
            LOG(log_error, logtype_afpd, "afp_setforkparams(%s): ad_flush: %s",
                of_name(ofork), strerror(errno));

            /* ad_rtruncate() succeeded but ad_flush() failed — the fork IS
             * truncated on disk. Invalidate cached rfork data so the next
             * access re-reads from disk (self-healing). */
            if (eid == ADEID_RFORK && obj->options.dircache_rfork_budget > 0) {
                rfork_invalidate_for_ofork(vol, ofork);
            }

            return AFPERR_PARAM;
        }
    } else {
        return AFPERR_BITMAP;
    }

    /* Invalidate cached rfork data after truncate/extend.
     * Only resource fork truncation (ADEID_RFORK) affects the rfork cache. */
    if (eid == ADEID_RFORK && obj->options.dircache_rfork_budget > 0) {
        rfork_invalidate_for_ofork(vol, ofork);
    }

    return AFP_OK;
afp_setfork_err:

    if (err == -2) {
        return AFPERR_LOCK;
    } else {
        switch (errno) {
        case EROFS:
            return AFPERR_VLOCK;

        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;

        case EDQUOT:
        case EFBIG:
        case ENOSPC:
            LOG(log_error, logtype_afpd, "afp_setforkparams: DISK FULL");
            return AFPERR_DFULL;

        default:
            return AFPERR_PARAM;
        }
    }
}

/* for this to work correctly, we need to check for locks before each
 * read and write. that's most easily handled by always doing an
 * appropriate check before each ad_read/ad_write. other things
 * that can change files like truncate are handled internally to those
 * functions.
 */
#define ENDBIT(a)  ((a) & 0x80)
#define UNLOCKBIT(a) ((a) & 0x01)


/* ---------------------- */
static int byte_lock(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen, int is64)
{
    struct ofork    *ofork;
    off_t               offset, length;
    int                 eid;
    uint16_t       ofrefnum;
    uint8_t            flags;
    int                 lockop;
    *rbuflen = 0;
    /* figure out parameters */
    ibuf++;
    flags = *ibuf; /* first bit = endflag, lastbit = lockflag */
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "byte_lock: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    /* Virtual forks don't support byte locking */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return AFP_OK;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        return AFPERR_PARAM;
    }

    offset = get_off_t(&ibuf, is64);
    length = get_off_t(&ibuf, is64);

    if (length == -1) {
        length = BYTELOCK_MAX;
    } else if (!length || is_neg(is64, length)) {
        return AFPERR_PARAM;
    } else if ((length >= AD_FILELOCK_BASE)
               && -1 == (ad_reso_fileno(ofork->of_ad))) { /* HF ?*/
        return AFPERR_LOCK;
    }

    if (ENDBIT(flags)) {
        offset += ad_size(ofork->of_ad, eid);
        /* FIXME what do we do if file size > 2 GB and
           it's not byte_lock_ext?
        */
    }

    /* error if we have a negative offset */
    if (offset < 0) {
        return AFPERR_PARAM;
    }

    /* if the file is a read-only file, we use read locks instead of
     * write locks. that way, we can prevent anyone from initiating
     * a write lock. */
    lockop = UNLOCKBIT(flags) ? ADLOCK_CLR : ADLOCK_WR;

    if (ad_lock(ofork->of_ad, eid, lockop, offset, length,
                ofork->of_refnum) < 0) {
        switch (errno) {
        case EACCES:
        case EAGAIN:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_LOCK;

        case ENOLCK:
            return AFPERR_NLOCK;

        case EINVAL:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_RANGEOVR;

        case EBADF:
        default:
            return AFPERR_PARAM;
        }
    }

    *rbuflen = set_off_t (offset, rbuf, is64);
    return AFP_OK;
}

/* --------------------------- */
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen)
{
    return byte_lock(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* --------------------------- */
int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                     size_t *rbuflen)
{
    return byte_lock(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

#undef UNLOCKBIT

/*!
 * @brief Read *rbuflen bytes from fork at offset
 *
 * @param[in] ofork        fork handle
 * @param[in] eid          data fork or resource fork entry id
 * @param[in] offset       offset
 * @param[in] rbuf         data buffer
 * @param[in,out] rbuflen  in: number of bytes to read, out: bytes read
 *
 * @returns         AFP status code
 */
static int read_file(const struct ofork *ofork, int eid, off_t offset,
                     char *rbuf, size_t *rbuflen)
{
    ssize_t cc;
    int eof = 0;
    cc = ad_read(ofork->of_ad, eid, offset, rbuf, *rbuflen);

    if (cc < 0) {
        LOG(log_error, logtype_afpd, "afp_read(%s): ad_read: %s", of_name(ofork),
            strerror(errno));
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    if ((size_t)cc < *rbuflen) {
        eof = 1;
    }

    *rbuflen = cc;

    if (eof) {
        return AFPERR_EOF;
    }

    return AFP_OK;
}

/*!
 * @brief Serve resource fork data from Tier 2 cache via DSI framing
 *
 * Shared helper for cache-hit and just-populated cache paths in read_fork().
 * Handles dsi_readinit/dsi_read/dsi_readdone loop and LRU promotion.
 *
 * @param[in]  dsi           DSI connection
 * @param[in]  cached_entry  dir entry with populated dcache_rfork_buf
 * @param[in]  ibuf          DSI write buffer (repurposed from command buffer)
 * @param[in]  offset        Starting byte offset in the rfork
 * @param[in]  reqcount      Requested byte count
 * @param[in]  err           AFP error code (AFPERR_EOF or AFP_OK)
 * @param[out] rbuflen       Output: bytes remaining (set by dsi_read)
 *
 * @returns 0 on success (goto afp_read_done), -1 on DSI error (goto afp_read_exit)
 */
static int rfork_cache_serve_from_buf(DSI *dsi, struct dir *cached_entry,
                                      char *ibuf, off_t offset,
                                      off_t reqcount, int err,
                                      size_t *rbuflen)
{
    ssize_t cc;

    /* Defense-in-depth: validate bounds before any memcpy from rfork buffer.
     * Callers maintain offset + reqcount <= dcache_rlen, but verify here to
     * prevent heap over-read if invariants are ever violated. */
    if (offset < 0 || reqcount < 0
            || offset + reqcount > cached_entry->dcache_rlen) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_serve_from_buf: bounds violation "
            "(offset=%lld, reqcount=%lld, rlen=%lld, did:%u)",
            (long long)offset, (long long)reqcount,
            (long long)cached_entry->dcache_rlen,
            ntohl(cached_entry->d_did));
        return -1;
    }

    *rbuflen = MIN(reqcount, dsi->server_quantum);
    memcpy(ibuf, (char *)cached_entry->dcache_rfork_buf + offset, *rbuflen);
    offset += *rbuflen;

    if ((cc = dsi_readinit(dsi, ibuf, *rbuflen, reqcount, err)) < 0) {
        return -1;
    }

    *rbuflen = cc;

    while (*rbuflen > 0) {
        if (offset >= cached_entry->dcache_rlen) {
            break;
        }

        size_t avail = (size_t)(cached_entry->dcache_rlen - offset);

        if (avail == 0) {
            break;    /* Safety: should not happen, all data sent */
        }

        size_t tocopy = MIN((size_t) * rbuflen, avail);
        memcpy(ibuf, (char *)cached_entry->dcache_rfork_buf + offset, tocopy);
        offset += tocopy;
        cc = dsi_read(dsi, ibuf, tocopy);

        if (cc < 0) {
            return -1;
        }

        *rbuflen = cc;
    }

    dsi_readdone(dsi);

    /* Promote in rfork LRU */
    if (cached_entry->rfork_lru_node) {
        queue_move_to_tail(rfork_lru, cached_entry->rfork_lru_node);
    }

    return 0;
}

static int read_fork(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                     char *rbuf _U_, size_t *rbuflen, int is64)
{
    DSI *dsi;
    struct ofork *ofork;
    off_t        offset;
    off_t        saveoff = 0;
    off_t        reqcount = 0;
    off_t        savereqcount = 0;
    off_t        size;
    ssize_t      cc;
    ssize_t      dsi_cc;
    ssize_t      err;
    int          eid;
    uint16_t     ofrefnum;
    /* we break the AFP spec here by not supporting nlmask and nlchar anymore */
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(u_short);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_read: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCRD) == 0) {
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else { /* fork wasn't opened. this should never really happen. */
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    /* zero request count */
    err = AFP_OK;

    if (!reqcount) {
        goto afp_read_err;
    }

    /* Virtual fork: serve data from in-memory buffer */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        if (ofork->of_flags & AFPFORK_DATA) {
            /* Data fork is empty for virtual Icon\r */
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        /* Resource fork: serve from virtual data buffer */
        if (offset >= ofork->of_virtual_len) {
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        off_t avail = ofork->of_virtual_len - offset;

        if (reqcount > avail) {
            reqcount = avail;
            err = AFPERR_EOF;
        }

        size_t tocopy = (size_t)reqcount;

        switch (obj->proto) {
#ifndef NO_DDP

        case AFPPROTO_ASP:
            *rbuflen = MIN(tocopy, *rbuflen);
            memcpy(rbuf, ofork->of_virtual_data + offset, *rbuflen);

            if (*rbuflen < tocopy) {
                err = AFPERR_EOF;
            }

            return (int)err;
#endif

        case AFPPROTO_DSI:
            dsi = obj->dsi;

            if (dsi_readinit(dsi, (char *)ofork->of_virtual_data + offset,
                             tocopy, tocopy, (int)err) < 0) {
                *rbuflen = 0;
                return AFPERR_PARAM;
            }

            dsi_readdone(dsi);
            *rbuflen = 0;
            return (int)err;

        default:
            LOG(log_error, logtype_afpd, "afp_read: unknown protocol %d", obj->proto);
            *rbuflen = 0;
            return AFPERR_PARAM;
        }
    }

    AFP_READ_START((long)reqcount);
    /* Tier 2 rfork cache: resolve dircache entry for resource fork reads.
     * Both lookups are O(1) hash operations — no syscalls, no CNID DB queries.
     * Guarded by budget > 0 so disabled (default) incurs zero overhead. */
    struct dir *cached_entry = NULL;

    if (eid == ADEID_RFORK
            && !(ofork->of_flags & AFPFORK_ACCWR)
            && obj->options.dircache_rfork_budget > 0) {
        const struct dir *parentdir = dirlookup(ofork->of_vol, ofork->of_did);

        if (parentdir) {
            char *fname = of_name(ofork);
            cached_entry = dircache_search_by_name(ofork->of_vol, parentdir,
                                                   fname, strnlen(fname, MAXPATHLEN));

            if (cached_entry) {
                rfork_stat_lookups++;
            }
        }
    }

    switch (obj->proto) {
#ifndef NO_DDP

    case AFPPROTO_ASP:
        if (obj->options.flags & OPTION_AFP_READ_LOCK) {
            if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, offset, reqcount,
                           ofork->of_refnum) < 0) {
                err = AFPERR_LOCK;
                goto afp_read_err;
            }
        }

        /* for asp, just send a packet at the requested buffer size */
        *rbuflen = MIN((size_t)reqcount, *rbuflen);
        cc = read_file(ofork, eid, offset, rbuf, rbuflen);

        if (cc < 0) {
            err = cc;
            goto afp_read_done;
        }

        break;
#endif

    case AFPPROTO_DSI:
        dsi = obj->dsi;
        /* reqcount isn't always truthful. we need to deal with that. */
        size = ad_size(ofork->of_ad, eid);
        LOG(log_debug, logtype_afpd,
            "afp_read(fork: %" PRIu16 " [%s], off: %" PRIu64 ", len: %" PRIu64 ", size: %"
            PRIu64 ")",
            ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
            reqcount, size);

        if (offset >= size) {
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        /* subtract off the offset */
        if (reqcount + offset > size) {
            reqcount = size - offset;
            err = AFPERR_EOF;
        }

        savereqcount = reqcount;
        saveoff = offset;

        if (reqcount < 0 || offset < 0) {
            err = AFPERR_PARAM;
            goto afp_read_err;
        }

        if (obj->options.flags & OPTION_AFP_READ_LOCK) {
            if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, offset, reqcount,
                           ofork->of_refnum) < 0) {
                err = AFPERR_LOCK;
                goto afp_read_err;
            }
        }

        /* === Tier 2 rfork cache check — before sendfile === */
        if (cached_entry && cached_entry->dcache_rfork_buf
                && cached_entry->dcache_rlen > 0
                && cached_entry->dcache_rlen == size) {
            /* Serve entire request from Tier 2 cache — zero I/O. */
            if (rfork_cache_serve_from_buf(dsi, cached_entry, ibuf,
                                           offset, reqcount, (int)err,
                                           rbuflen) < 0) {
                goto afp_read_exit;
            }

            rfork_stat_hits++;
            goto afp_read_done;
        }

        /* Size mismatch: cache stale — invalidate rfork buffer + Tier 1 AD */
        if (cached_entry && cached_entry->dcache_rfork_buf
                && cached_entry->dcache_rlen > 0
                && cached_entry->dcache_rlen != size) {
            LOG(log_debug, logtype_afpd,
                "rfork_cache: size mismatch (cached:%lld vs live:%lld), "
                "invalidating (did:%u)",
                (long long)cached_entry->dcache_rlen, (long long)size,
                ntohl(cached_entry->d_did));
            rfork_cache_free(cached_entry);
            rfork_stat_invalidated++;
            rfork_stat_misses++;     /* Size mismatch counts as a miss */
            cached_entry->dcache_rlen = (off_t) -1;
            memset(cached_entry->dcache_finderinfo, 0, 32);
            memset(cached_entry->dcache_filedatesi, 0, 16);
            memset(cached_entry->dcache_afpfilei, 0, 4);
            cached_entry = NULL;
        }

        /* Cache miss: upgrade to full-fork ad_read if cacheable */
        if (cached_entry && cached_entry->dcache_rlen > 0
                && (size_t)cached_entry->dcache_rlen <= rfork_max_entry_size
                && !cached_entry->dcache_rfork_buf
                && rfork_cache_store_from_fd(cached_entry, ofork->of_ad,
                                             ADEID_RFORK) == 0) {
            rfork_stat_added++;

            /* Serve from newly-populated cache */
            if (rfork_cache_serve_from_buf(dsi, cached_entry, ibuf,
                                           offset, reqcount, (int)err,
                                           rbuflen) < 0) {
                goto afp_read_exit;
            }

            rfork_stat_hits++;
            goto afp_read_done;
        }

        /* store_from_fd failed — fall through to normal fd read */

        /* Count miss: any rfork cache lookup that didn't result in a hit */
        if (cached_entry) {
            rfork_stat_misses++;
        }

#ifdef WITH_SENDFILE

        if (!(eid == ADEID_DFORK && ad_data_fileno(ofork->of_ad) == AD_SYMLINK) &&
                !(obj->options.flags & OPTION_NOSENDFILE)) {
            int fd = ad_readfile_init(ofork->of_ad, eid, &offset, 0);

            if (dsi_stream_read_file(dsi, fd, offset, reqcount, (int)err) < 0) {
                LOG(log_error, logtype_afpd, "afp_read(%s): ad_readfile: %s",
                    of_name(ofork), strerror(errno));
                goto afp_read_exit;
            }

            goto afp_read_done;
        }

#endif
        *rbuflen = MIN((size_t)reqcount, (size_t)dsi->server_quantum);
        cc = read_file(ofork, eid, offset, ibuf, rbuflen);

        if (cc < 0) {
            err = cc;
            goto afp_read_done;
        }

        LOG(log_debug, logtype_afpd,
            "afp_read(name: \"%s\", offset: %jd, reqcount: %jd): got %jd bytes from file",
            of_name(ofork), (intmax_t)offset, (intmax_t)reqcount, (intmax_t)*rbuflen);
        offset += *rbuflen;
        /*
         * dsi_readinit() returns size of next read buffer. by this point,
         * we know that we're sending some data. if we fail, something
         * horrible happened.
         */
        dsi_cc = dsi_readinit(dsi, ibuf, *rbuflen, reqcount, (int)err);

        if (dsi_cc < 0) {
            goto afp_read_exit;
        }

        *rbuflen = (size_t)dsi_cc;

        while (*rbuflen > 0) {
            /*
             * This loop isn't really entered anymore, we've already
             * sent the whole requested block in dsi_readinit().
             */
            cc = read_file(ofork, eid, offset, ibuf, rbuflen);

            if (cc < 0) {
                goto afp_read_exit;
            }

            offset += *rbuflen;
            /* dsi_read() also returns buffer size of next allocation */
            dsi_cc = dsi_read(dsi, ibuf, *rbuflen); /* send it off */

            if (dsi_cc < 0) {
                goto afp_read_exit;
            }

            *rbuflen = (size_t)dsi_cc;
        }

        dsi_readdone(dsi);
        goto afp_read_done;
afp_read_exit:
        LOG(log_error, logtype_afpd, "afp_read(%s): %s", of_name(ofork),
            strerror(errno));
        dsi_readdone(dsi);

        if (obj->options.flags & OPTION_AFP_READ_LOCK) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,
                       ofork->of_refnum);
        }

        obj->exit(EXITERR_CLNT);
        break;
    } /* switch */

afp_read_done:

    if (obj->options.flags & OPTION_AFP_READ_LOCK) {
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,
                   ofork->of_refnum);
    }

    AFP_READ_DONE();
    return (int)err;
afp_read_err:
    *rbuflen = 0;
    return (int)err;
}

/* ---------------------- */
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
             size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* ---------------------- */
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/* ---------------------- */
int afp_flush(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
              size_t *rbuflen)
{
    struct vol *vol;
    uint16_t vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    of_flush(vol);
    return AFP_OK;
}

int afp_flushfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                  char *rbuf _U_, size_t *rbuflen)
{
    struct ofork    *ofork;
    uint16_t       ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_flushfork: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_flushfork(fork: %s)",
        (ofork->of_flags & AFPFORK_DATA) ? "d" : "r");

    if (flushfork(ofork) < 0) {
        LOG(log_error, logtype_afpd, "afp_flushfork(%s): %s", of_name(ofork),
            strerror(errno));
    }

    return AFP_OK;
}

/*!
  @bug
  There is a lot to tell about fsync, fdatasync, F_FULLFSYNC.
  fsync(2) on OSX is implemented differently than on other platforms.
  @sa http://mirror.linux.org.au/pub/linux.conf.au/2007/video/talks/278.pdf
*/
int afp_syncfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                 char *rbuf _U_, size_t *rbuflen)
{
    struct ofork        *ofork;
    uint16_t           ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_syncfork: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_syncfork(fork: %s)",
        (ofork->of_flags & AFPFORK_DATA) ? "d" : "r");

    if (flushfork(ofork) < 0) {
        LOG(log_error, logtype_afpd, "flushfork(%s): %s", of_name(ofork),
            strerror(errno));
        return AFPERR_MISC;
    }

    return AFP_OK;
}

/* this is very similar to closefork */
int flushfork(struct ofork *ofork)
{
    struct timeval tv;
    int err = 0, doflush = 0;

    /* Nothing to flush for virtual forks */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return 0;
    }

    if (ad_data_fileno(ofork->of_ad) != -1 &&
            fsync(ad_data_fileno(ofork->of_ad)) < 0) {
        LOG(log_error, logtype_afpd, "flushfork(%s): dfile(%d) %s",
            of_name(ofork), ad_data_fileno(ofork->of_ad), strerror(errno));
        err = -1;
    }

    if (ad_reso_fileno(ofork->of_ad) != -1 &&     /* HF */
            (ofork->of_flags & AFPFORK_RSRC)) {
        /* read in the rfork length */
        ad_refresh(NULL, ofork->of_ad);

        /* set the date if we're dirty */
        if ((ofork->of_flags & AFPFORK_DIRTY) && !gettimeofday(&tv, NULL)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY | AD_DATE_UNIX, tv.tv_sec);
            ofork->of_flags &= ~AFPFORK_DIRTY;
            doflush++;
        }

        /* flush the header */
        if (doflush && ad_flush(ofork->of_ad) < 0) {
            err = -1;
        }

        if (fsync(ad_reso_fileno(ofork->of_ad)) < 0) {
            err = -1;
        }

        if (err < 0)
            LOG(log_error, logtype_afpd, "flushfork(%s): hfile(%d) %s",
                of_name(ofork), ad_reso_fileno(ofork->of_ad), strerror(errno));
    }

    return err;
}

int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
                  size_t *rbuflen)
{
    struct ofork    *ofork;
    uint16_t       ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_debug, logtype_afpd,
            "afp_closefork: of_find(%" PRIu16 ") could not locate fork"
            " (wire bytes: 0x%02x%02x; stale close from client)",
            ofrefnum,
            (unsigned char)ibuf[0], (unsigned char)ibuf[1]);
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_closefork(fork: %" PRIu16 " [%s])",
        ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "rsrc");

    if (of_closefork(obj, ofork) < 0) {
        LOG(log_error, logtype_afpd, "afp_closefork: of_closefork: %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    return AFP_OK;
}


static ssize_t write_file(struct ofork *ofork, int eid,
                          off_t offset, char *rbuf,
                          size_t rbuflen)
{
    ssize_t cc;
    LOG(log_debug, logtype_afpd, "write_file(off: %ju, size: %zu)",
        (uintmax_t)offset, rbuflen);

    if ((cc = ad_write(ofork->of_ad, eid, offset, 0,
                       rbuf, rbuflen)) < 0) {
        switch (errno) {
        case EDQUOT :
        case EFBIG :
        case ENOSPC :
            LOG(log_error, logtype_afpd, "write_file: DISK FULL");
            return AFPERR_DFULL;

        case EACCES:
            return AFPERR_ACCESS;

        default :
            LOG(log_error, logtype_afpd, "afp_write(%s): ad_write: %s", of_name(ofork),
                strerror(errno));
            return AFPERR_PARAM;
        }
    }

    return cc;
}


/*
 * FPWrite. NOTE: on an error, we always use afp_write_err as
 * the client may have sent us a bunch of data that's not reflected
 * in reqcount et al.
 */
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                      size_t *rbuflen, int is64)
{
    struct ofork    *ofork;
    off_t           offset;
    off_t           saveoff;
    off_t           reqcount;
    off_t           oldsize;
    off_t           newsize;
    int             endflag;
    int             eid;
    int             err = AFP_OK;
    uint16_t        ofrefnum;
    DSI *dsi = obj->dsi;
    char *rcvbuf = NULL;
    size_t          rcvbuflen = 0;
    ssize_t         cc;

    if (dsi) {
        rcvbuf = (char *)dsi->commands;
        rcvbuflen = dsi->server_quantum;
    }

    /* figure out parameters */
    ibuf++;
    endflag = ENDBIT(*ibuf);
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_write: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    LOG(log_debug, logtype_afpd,
        "afp_write(fork: %" PRIu16 " [%s], off: %" PRIu64 ", size: %" PRIu64 ")",
        ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
        reqcount);

    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        err = AFPERR_OLOCK;
        goto afp_write_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
        err = AFPERR_ACCESS;
        goto afp_write_err;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        err = AFPERR_ACCESS; /* should never happen */
        goto afp_write_err;
    }

    oldsize = ad_size(ofork->of_ad, eid);

    if (endflag) {
        offset += oldsize;
    }

    /* handle bogus parameters */
    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    newsize = ((offset + reqcount) > oldsize) ? (offset + reqcount) : oldsize;

    /* offset can overflow on 64-bit capable filesystems.
     * report disk full if that's going to happen. */
    if (sum_neg(is64, offset, reqcount)) {
        LOG(log_error, logtype_afpd, "write_fork: DISK FULL");
        err = AFPERR_DFULL;
        goto afp_write_err;
    }

    if (!reqcount) { /* handle request counts of 0 */
        err = AFP_OK;
        *rbuflen = set_off_t (offset, rbuf, is64);
        goto afp_write_err;
    }

    AFP_WRITE_START((long)reqcount);
    saveoff = offset;

    if (obj->options.flags & OPTION_AFP_READ_LOCK) {
        if (ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, saveoff, reqcount,
                       ofork->of_refnum) < 0) {
            err = AFPERR_LOCK;
            goto afp_write_err;
        }
    }

    switch (obj->proto) {
#ifndef NO_DDP

    case AFPPROTO_ASP:
        if (asp_wrtcont(obj->handle, rbuf, rbuflen) < 0) {
            *rbuflen = 0;
            LOG(log_error, logtype_afpd, "afp_write: asp_wrtcont: %s", strerror(errno));
            return AFPERR_PARAM;
        }

        if (obj->options.flags & OPTION_DEBUG) {
            printf("(write) len: %ld\n", (unsigned long)*rbuflen);
            bprint(rbuf, *rbuflen);
        }

        if ((cc = write_file(ofork, eid, offset, rbuf, *rbuflen)) < 0) {
            *rbuflen = 0;

            if (obj->options.flags & OPTION_AFP_READ_LOCK) {
                ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount,
                           ofork->of_refnum);
            }

            return cc;
        }

        offset += cc;
        break;
#endif /* no afp/asp */

    case AFPPROTO_DSI: {
        if (dsi == NULL) {
            *rbuflen = 0;
            return AFPERR_MISC;
        }

        /* find out what we have already */
        if ((cc = dsi_writeinit(dsi, rcvbuf, rcvbuflen)) > 0) {
            ssize_t written;

            if ((written = write_file(ofork, eid, offset, rcvbuf, cc)) != cc) {
                dsi_writeflush(dsi);
                *rbuflen = 0;

                if (obj->options.flags & OPTION_AFP_READ_LOCK) {
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount, ofork->of_refnum);
                }

                if (written > 0)
                    /* It's used for the read size and as error code in write_file(), ugh */
                {
                    written = AFPERR_MISC;
                }

                return written;
            }
        }

        offset += cc;
#ifdef WITH_RECVFILE

        if (obj->options.flags & OPTION_RECVFILE) {
            LOG(log_maxdebug, logtype_afpd,
                "afp_write(fork: %" PRIu16 " [%s], off: %" PRIu64 ", size: %" PRIu32 ")",
                ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
                dsi->datasize);

            if ((cc = ad_recvfile(ofork->of_ad, eid, dsi->socket, offset, dsi->datasize,
                                  obj->options.splice_size)) < dsi->datasize) {
                switch (errno) {
                case EDQUOT:
                case EFBIG:
                case ENOSPC:
                    cc = AFPERR_DFULL;
                    dsi_writeflush(dsi);
                    break;

                case ENOSYS:
                    goto afp_write_loop;

                default:
                    /* Low level error, can't do much to back up */
                    cc = AFPERR_MISC;
                    LOG(log_error, logtype_afpd, "afp_write: ad_writefile: %s", strerror(errno));
                }

                *rbuflen = 0;

                if (obj->options.flags & OPTION_AFP_READ_LOCK) {
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount, ofork->of_refnum);
                }

                return cc;
            }

            offset += cc;
            goto afp_write_done;
        }

#endif
afp_write_loop:

        /* loop until everything gets written. currently
         * dsi_write handles the end case by itself. */
        while ((cc = dsi_write(dsi, rcvbuf, rcvbuflen))) {
            if ((cc = write_file(ofork, eid, offset, rcvbuf, cc)) < 0) {
                dsi_writeflush(dsi);
                *rbuflen = 0;

                if (obj->options.flags & OPTION_AFP_READ_LOCK) {
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount, ofork->of_refnum);
                }

                return cc;
            }

            LOG(log_debug, logtype_afpd, "afp_write: wrote: %jd, offset: %jd",
                (intmax_t)cc, (intmax_t)offset);
            offset += cc;
        }
    }
    break;
    }

afp_write_done:

    if (obj->options.flags & OPTION_AFP_READ_LOCK) {
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount,
                   ofork->of_refnum);
    }

    if (ad_meta_fileno(ofork->of_ad) != -1) {   /* META */
        ofork->of_flags |= AFPFORK_DIRTY;
    }

    /* Invalidate cached rfork data after write — content changed on disk.
     * Only resource fork writes (ADEID_RFORK) affect the rfork cache. */
    if (eid == ADEID_RFORK && obj->options.dircache_rfork_budget > 0) {
        rfork_invalidate_for_ofork(ofork->of_vol, ofork);
    }

    /* we have modified any fork, remember until close_fork */
    ofork->of_flags |= AFPFORK_MODIFIED;
    /* update write count */
    ofork->of_vol->v_appended += (newsize > oldsize) ? (newsize - oldsize) : 0;
    *rbuflen = set_off_t (offset, rbuf, is64);
    AFP_WRITE_DONE();
    return AFP_OK;
afp_write_err:

    if (obj->proto == AFPPROTO_DSI) {
        dsi_writeinit(dsi, rcvbuf, rcvbuflen);
        dsi_writeflush(dsi);
    }

    if (err != AFP_OK) {
        *rbuflen = 0;
    }

    return err;
}

/* ---------------------------- */
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* ----------------------------
 * FIXME need to deal with SIGXFSZ signal
 */
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/* ---------------------------- */
int afp_getforkparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                      size_t *rbuflen)
{
    struct ofork    *ofork;
    int             ret;
    uint16_t       ofrefnum, bitmap;
    size_t          buflen;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    *rbuflen = 0;

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_getforkparams: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    if (AD_META_OPEN(ofork->of_ad)) {
        if (ad_refresh(NULL, ofork->of_ad) < 0) {
            LOG(log_error, logtype_afpd, "getforkparams(%s): ad_refresh: %s",
                of_name(ofork), strerror(errno));
            return AFPERR_PARAM;
        }
    }

    if (AFP_OK != (ret = getforkparams(obj, ofork, bitmap, rbuf + sizeof(u_short),
                                       &buflen))) {
        return ret;
    }

    *rbuflen = buflen + sizeof(u_short);
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    return AFP_OK;
}
