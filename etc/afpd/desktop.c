/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
* @file
* @brief Manage the AppleDesktop folder and its contents
* @bug
* afp_XXXcomment are (the only) functions able to open
* a resource fork when there's no data fork, e.g. after
* it was removed with samba.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <netatalk/at.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_ipc.h>
#include <atalk/unix.h>
#include <atalk/util.h>

#include "ad_cache.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "fork.h"
#include "mangle.h"
#include "volume.h"

#define EXEC_MODE (S_IXGRP | S_IXUSR | S_IXOTH)

/*!
 * @brief Apply a volume root mode change to the desktop database folder
 *
 * The call changes .AppleDesktop, its one- and two-character subdirectories,
 * and their entries. Directories get DIRBITS and mode, files mode without the
 * execute bits. The call refuses a mode that takes read or search access from
 * the owner. It ignores EPERM, and other chmod failures only log.
 *
 * @param[in] vol   volume of the desktop database
 * @param[in] mode  new mode of the volume root
 * @returns 0, or -1 for a refused mode or when getcwd, chdir, or opendir fails
 */
int setdeskmode(const struct vol *vol, const mode_t mode)
{
    EC_INIT;
    char		wd[MAXPATHLEN + 1];
    struct stat         st;
    char		modbuf[MAXPATHLEN + 1];
    struct dirent	*deskp, *subp;
    DIR			*desk, *sub;

    if (!dir_rx_set(mode)) {
        /* want to remove read and search access to owner it will screw the volume */
        return -1;
    }

    if (getcwd(wd, MAXPATHLEN) == NULL) {
        return -1;
    }

    bstring dtpath = bfromcstr(vol->v_dbpath);
    bcatcstr(dtpath, "/" APPLEDESKTOP);
    EC_NEG1(chdir(cfrombstr(dtpath)));

    if ((desk = opendir(".")) == NULL) {
        if (chdir(wd) < 0) {
            LOG(log_error, logtype_afpd, "setdeskmode: chdir %s: %s", wd, strerror(errno));
        }

        EC_FAIL;
    }

    for (deskp = readdir(desk); deskp != NULL; deskp = readdir(desk)) {
        if (strcmp(deskp->d_name, ".") == 0 ||
                strcmp(deskp->d_name, "..") == 0 || strlen(deskp->d_name) > 2) {
            continue;
        }

        if ((sub = opendir(deskp->d_name)) == NULL) {
            continue;
        }

        for (subp = readdir(sub); subp != NULL; subp = readdir(sub)) {
            if (strcmp(subp->d_name, ".") == 0 ||
                    strcmp(subp->d_name, "..") == 0) {
                continue;
            }

            snprintf(modbuf, sizeof(modbuf), "%s/%s", deskp->d_name, subp->d_name);

            /* XXX: need to preserve special modes */
            if (lstat(modbuf, &st) < 0) {
                LOG(log_error, logtype_afpd, "setdeskmode: stat %s: %s", fullpathname(modbuf),
                    strerror(errno));
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                if (ochmod(modbuf,
                           DIRBITS | mode,
                           &st,
                           vol_syml_opt(vol) | vol_chmod_opt(vol)
                          ) < 0 && errno != EPERM) {
                    LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s", fullpathname(modbuf),
                        strerror(errno));
                }
            } else if (ochmod(modbuf,
                              mode & ~EXEC_MODE,
                              &st,
                              vol_syml_opt(vol) | vol_chmod_opt(vol)
                             ) < 0 && errno != EPERM) {
                LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s", fullpathname(modbuf),
                    strerror(errno));
            }
        }

        closedir(sub);

        /* XXX: need to preserve special modes */
        if (ochmod(deskp->d_name,
                   DIRBITS | mode,
                   NULL,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0 && errno != EPERM) {
            LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s",
                fullpathname(deskp->d_name), strerror(errno));
        }
    }

    closedir(desk);

    if (chdir(wd) < 0) {
        LOG(log_error, logtype_afpd, "setdeskmode: chdir %s: %s", wd, strerror(errno));
        EC_FAIL;
    }

    /* XXX: need to preserve special modes */
    if (ochmod(bdata(dtpath),
               DIRBITS | mode,
               NULL,
               vol_syml_opt(vol) | vol_chmod_opt(vol)
              ) < 0 && errno != EPERM) {
        LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s", bdata(dtpath),
            strerror(errno));
    }

EC_CLEANUP:
    bdestroy(dtpath);
    EC_EXIT;
}

/*!
 * @brief Apply a volume root owner change to the desktop database folder
 *
 * The call walks .AppleDesktop like setdeskmode and changes the owner and
 * group of every entry. It ignores EPERM, and other chown failures only log.
 *
 * @param[in] vol  volume of the desktop database
 * @param[in] uid  new owner, or -1 to keep it
 * @param[in] gid  new group, or -1 to keep it
 * @returns 0, or -1 when getcwd, chdir, or opendir fails
 */
int setdeskowner(const struct vol *vol, uid_t uid, gid_t gid)
{
    EC_INIT;
    char		wd[MAXPATHLEN + 1];
    char		modbuf[MAXPATHLEN + 1];
    struct dirent	*deskp, *subp;
    DIR			*desk, *sub;

    if (getcwd(wd, MAXPATHLEN) == NULL) {
        return -1;
    }

    bstring dtpath = bfromcstr(vol->v_dbpath);
    bcatcstr(dtpath, "/" APPLEDESKTOP);
    EC_NEG1(chdir(cfrombstr(dtpath)));

    if ((desk = opendir(".")) == NULL) {
        if (chdir(wd) < 0) {
            LOG(log_error, logtype_afpd, "setdeskowner: chdir %s: %s", wd, strerror(errno));
        }

        EC_FAIL;
    }

    for (deskp = readdir(desk); deskp != NULL; deskp = readdir(desk)) {
        if (strcmp(deskp->d_name, ".") == 0 ||
                strcmp(deskp->d_name, "..") == 0 ||
                strlen(deskp->d_name) > 2) {
            continue;
        }

        if ((sub = opendir(deskp->d_name)) == NULL) {
            continue;
        }

        for (subp = readdir(sub); subp != NULL; subp = readdir(sub)) {
            if (strcmp(subp->d_name, ".") == 0 ||
                    strcmp(subp->d_name, "..") == 0) {
                continue;
            }

            snprintf(modbuf, sizeof(modbuf), "%s/%s", deskp->d_name, subp->d_name);

            /* XXX: add special any uid, ignore group bits */
            if (chown(modbuf, uid, gid) < 0 && errno != EPERM) {
                LOG(log_error, logtype_afpd, "setdeskown: chown %s: %s", fullpathname(modbuf),
                    strerror(errno));
            }
        }

        closedir(sub);

        /* XXX: add special any uid, ignore group bits */
        if (chown(deskp->d_name, uid, gid) < 0 && errno != EPERM) {
            LOG(log_error, logtype_afpd, "setdeskowner: chown %s: %s",
                deskp->d_name, strerror(errno));
        }
    }

    closedir(desk);

    if (chdir(wd) < 0) {
        LOG(log_error, logtype_afpd, "setdeskowner: chdir %s: %s", wd, strerror(errno));
        EC_FAIL;
    }

    if (chown(cfrombstr(dtpath), uid, gid) < 0 && errno != EPERM) {
        LOG(log_error, logtype_afpd, "setdeskowner: chown %s: %s",
            fullpathname(".AppleDouble"), strerror(errno));
    }

EC_CLEANUP:
    bdestroy(dtpath);
    EC_EXIT;
}

/*!
 * @brief Make sure the desktop database folder of a volume exists
 *
 * When the database path lacks .AppleDesktop, the call moves the folder from
 * the volume root. Without either folder, or when the move fails, it creates
 * a new one with mode 0777 under the umask. On a shared volume it works as
 * root, and on a private volume as the owner, as cnid_open() does.
 *
 * @param[in] vol  volume of the desktop database
 */
static void create_appledesktop_folder(const struct vol *vol)
{
    bstring olddtpath = NULL, dtpath = NULL;
    struct stat st;
    const bool priv = (vol->v_flags & AFPVOL_PRIVATE) != 0;
    const bool as_owner = priv && geteuid() == 0;
    gid_t egid = 0;

    if (as_owner && become_owner(vol->v_private_uid, &egid) != 0) {
        LOG(log_error, logtype_afpd,
            "can't create the desktop database of volume \"%s\" as its "
            "owner, uid %ju: %s", vol->v_path, (uintmax_t)vol->v_private_uid,
            strerror(errno));
        return;
    }

    if (!priv) {
        become_root();
    }

    olddtpath = bfromcstr(vol->v_path);
    bcatcstr(olddtpath, "/" APPLEDESKTOP);
    dtpath = bfromcstr(vol->v_dbpath);
    bcatcstr(dtpath, "/" APPLEDESKTOP);

    if (lstat(cfrombstr(dtpath), &st) != 0) {
        if ((lstat(cfrombstr(olddtpath), &st) == 0) && (S_ISDIR(st.st_mode) != 0)) {
            if (rename(bdata(olddtpath), bdata(dtpath)) != 0) {
                LOG(log_error, logtype_afpd,
                    "moving .AppleDesktop from \"%s\" failed: %s; creating new "
                    "dir \"%s\"", bdata(olddtpath), strerror(errno),
                    bdata(dtpath));
                /* shared with the volume's users; umask applies */
                mkdir(cfrombstr(dtpath), 0777); //NOSONAR
            }
        } else {
            LOG(log_debug, logtype_afpd,
                "no valid .AppleDesktop dir found; creating new dir \"%s\"",
                bdata(dtpath));
            /* shared with the volume's users; umask applies */
            mkdir(cfrombstr(dtpath), 0777); //NOSONAR
        }
    }

    if (as_owner) {
        unbecome_owner(egid);
    } else if (!priv) {
        unbecome_root();
    }

    bdestroy(dtpath);
    bdestroy(olddtpath);
}

/*!
 * @brief FPOpenDT: open the desktop database of a volume
 *
 * The call creates the desktop folder when needed and answers with the volume
 * ID as the desktop reference.
 *
 * @param[in]  obj      session object, unused
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, or AFPERR_PARAM for an unknown volume
 */
int afp_opendt(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
               size_t *rbuflen)
{
    struct vol	*vol;
    uint16_t	vid;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));

    if (NULL == (vol = getvolbyvid(vid))) {
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    create_appledesktop_folder(vol);
    memcpy(rbuf, &vid, sizeof(vid));
    *rbuflen = sizeof(vid);
    return AFP_OK;
}

/*!
 * @brief FPCloseDT: close the desktop database of a volume
 *
 * The desktop reference is the volume ID, and the call only checks it.
 *
 * @param[in]  obj      session object, unused
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns AFP_OK, or AFPERR_PARAM for an unknown reference
 */
int afp_closedt(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                char *rbuf _U_, size_t *rbuflen)
{
    uint16_t dtref;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&dtref, ibuf, sizeof(dtref));

    if (NULL == getvolbyvid(dtref)) {
        return AFPERR_PARAM;
    }

    return AFP_OK;
}

static struct savedt	si = { { 0, 0, 0, 0 }, -1, 0, 0 };

/*!
 * @brief Return the path of the icon file of a creator in the desktop database
 *
 * @param[in] vol      volume of the desktop database
 * @param[in] creator  creator code
 * @returns the path in the static buffer of dtfile, or NULL
 */
static char *icon_dtfile(struct vol *vol, uint8_t creator[4])
{
    return dtfile(vol, creator, ".icon");
}

/* icon file opens and locks that failed, for log_backoff() */
static unsigned int icon_failures;

/* FPAddIcon writes made without a lock, for log_backoff() */
static unsigned int icon_unlocked;

/*! @brief Close the icon file descriptor the icon calls share */
static void icon_close(void)
{
    if (si.sdt_fd != -1) {
        close(si.sdt_fd);
        si.sdt_fd = -1;
    }
}

/*!
 * @brief The answer of an icon read call whose icon file did not open
 *
 * @returns AFPERR_NOITEM for a creator without an icon file, AFPERR_MISC
 *          for any other failure, logged with fn under log_backoff()
 */
static int icon_open_failed(const char *fn, struct vol *vol,
                            uint8_t creator[4])
{
    if (errno == ENOENT) {
        return AFPERR_NOITEM;
    }

    if (log_backoff(&icon_failures)) {
        LOG(log_error, logtype_afpd, "%s(%s): open: %s, failure %u", fn,
            icon_dtfile(vol, creator), strerror(errno), icon_failures);
    }

    return AFPERR_MISC;
}

/*!
 * @brief Open the icon file of a creator in the desktop database
 *
 * One descriptor serves the icon file of the last creator and volume, and an
 * open of the same file reuses it with its position. Any other open closes it
 * first. With O_CREAT, a missing file also gets its two parent directories.
 *
 * @param[in] vol      volume of the desktop database
 * @param[in] creator  creator code
 * @param[in] flags    open flags
 * @param[in] mode     creation mode, masked with the parent directory
 *                     permissions
 * @returns 0, or -1 when open fails
 */
static int iconopen(struct vol *vol, uint8_t creator[4], int flags, int mode)
{
    const char	*dtf;
    char	*adt, *adts;

    if (si.sdt_fd != -1
            && memcmp(si.sdt_creator, creator, sizeof(CreatorType)) == 0
            && si.sdt_vid == vol->v_vid) {
        return 0;
    }

    icon_close();
    dtf = icon_dtfile(vol, creator);

    if ((si.sdt_fd = open(dtf, flags, ad_mode(dtf, mode))) < 0) {
        if (errno == ENOENT && (flags & O_CREAT)) {
            if ((adts = strrchr(dtf, '/')) == NULL) {
                return -1;
            }

            *adts = '\0';

            if ((adt = strrchr(dtf, '/')) == NULL) {
                return -1;
            }

            *adt = '\0';
            (void) ad_mkdir(dtf, DIRBITS | 0777);
            *adt = '/';
            (void) ad_mkdir(dtf, DIRBITS | 0777);
            *adts = '/';

            if ((si.sdt_fd = open(dtf, flags, ad_mode(dtf, mode))) < 0) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    memcpy(si.sdt_creator, creator, sizeof(CreatorType));
    si.sdt_vid = vol->v_vid;
    return 0;
}

/*!
 * @brief Lock the icon file for one FPAddIcon
 *
 * Serializes the scan, write and undo of concurrent FPAddIcon requests. The
 * wait for another holder, which need not be an afpd, is bounded to a
 * second. FPGetIcon and FPGetIconInfo take no lock; closing the file drops
 * it.
 *
 * @returns 0, 1 when the file system takes no locks, or -1 with errno set
 */
static int icon_lock(void)
{
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };

    for (int tries = 1;; tries++) {
        if (write_lock(si.sdt_fd, 0, SEEK_SET, 0) == 0) {
            return 0;
        }

        /* an NFS mount without a lock manager, among others: POSIX answers
         * EINVAL, macOS ENOTSUP */
        if (errno == ENOLCK || errno == EOPNOTSUPP || errno == EINVAL) {
            return 1;
        }

#if ENOTSUP != EOPNOTSUPP

        if (errno == ENOTSUP) {
            return 1;
        }

#endif

        if ((errno != EACCES && errno != EAGAIN) || tries == 100) {
            return -1;
        }

        nanosleep(&tick, NULL);
    }
}

/*!
 * @brief Does entry header h match file type ftype and icon type itype, the
 *        key FPGetIcon and FPAddIcon look an icon up by?
 */
static bool icon_entry_is(const uint8_t *h, const uint8_t *ftype,
                          uint8_t itype)
{
    /* tag, file type, icon type, pad, size */
    return memcmp(h + 4, ftype, 4) == 0 && h[8] == itype;
}

/*!
 * @brief Did a read of a 12-byte record header at off, which returned rc, fail
 *        or stop short before the end of an icon file of size bytes?
 *
 * A header the end of the file cuts short is a torn tail, no failure.
 */
static bool icon_read_failed(ssize_t rc, off_t off, off_t size)
{
    return rc < 0 || (rc < 12 && off + rc < size);
}

/*!
 * @brief FPAddIcon: store an icon bitmap in the icon file of a creator
 *
 * Each record of the icon file holds a 12-byte header (tag, file type, icon
 * type, bitmap size) and the bitmap. The bitmap is received whole before the
 * file is opened; data of another size fails with AFPERR_PARAM. Under a write
 * lock, the record of the same file type and icon type, whatever its tag, is
 * overwritten in place; otherwise the icon replaces whatever follows the last
 * whole record. A failed write cuts a new record off again. On a file system
 * that takes no locks the write goes ahead without one, with a warning.
 *
 * Over DSI a request refused before its data arrives drains that data, so the
 * session stays in step.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     receive buffer for the bitmap
 * @param[out] rbuflen  0 on return
 * @returns AFP_OK, AFPERR_ITYPE for a size change, AFPERR_PARAM for an unknown
 *          volume or data of the wrong size, or AFPERR_MISC when the icon file
 *          cannot be opened, locked, read, sought or written
 */
int afp_addicon(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                size_t *rbuflen)
{
    struct vol *vol;
    uint8_t		fcreator[4];
    uint8_t		imh[12];
    uint8_t		irh[12];
    uint8_t		*p;
    int			itype;
    int         locked;
    ssize_t     cc;
    bool        match = false;
    off_t       pos = 0;
    struct stat st;
    uint32_t    ftype;
    uint32_t    itag;
    uint16_t		bsize = 0;
    uint16_t		rsize;
    uint16_t		vid;
    *rbuflen = 0;
    icon_close();
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        if (obj->proto == AFPPROTO_DSI) {
            dsi_writedrain(obj->dsi);
        }

        return AFPERR_PARAM;
    }

    memcpy(fcreator, ibuf, sizeof(fcreator));
    ibuf += sizeof(fcreator);
    memcpy(&ftype, ibuf, sizeof(ftype));
    ibuf += sizeof(ftype);
    itype = (unsigned char) * ibuf;
    ibuf += 2;
    memcpy(&itag, ibuf, sizeof(itag));
    ibuf += sizeof(itag);
    memcpy(&bsize, ibuf, sizeof(bsize));
    bsize = ntohs(bsize);

    /* receive the whole icon first: the file's lock is never held while
     * waiting on a client */
    switch (obj->proto) {
#ifndef NO_DDP

    case AFPPROTO_ASP: {
        size_t buflen = bsize;

        if ((asp_wrtcont(obj->handle, rbuf, &buflen) < 0) || buflen != bsize) {
            return AFPERR_PARAM;
        }
    }
    break;
#endif /* no afp/asp */

    case AFPPROTO_DSI: {
        DSI *dsi = obj->dsi;
        char *wbuf = NULL;
        size_t got = dsi_writeinit(dsi, &wbuf);
        size_t wlen;

        if (got + dsi->datasize != bsize) {
            LOG(log_error, logtype_afpd,
                "afp_addicon(%s): %zu bytes for an icon of %u",
                icon_dtfile(vol, fcreator), got + dsi->datasize, bsize);
            dsi_writeflush(dsi);
            return AFPERR_PARAM;
        }

        if (got > 0) {
            memcpy(rbuf, wbuf, got);
        }

        /* dsi_write() reads 0 at EOF as well as at the end of the payload */
        while (got < bsize
                && (wlen = dsi_write(dsi, rbuf + got, bsize - got)) > 0) {
            got += wlen;
        }

        if (got < bsize) {
            LOG(log_error, logtype_afpd,
                "afp_addicon(%s): the client ended the request with %u bytes "
                "of the icon outstanding", icon_dtfile(vol, fcreator),
                dsi->datasize);
            return AFPERR_PARAM;
        }
    }
    break;
    }

    if (iconopen(vol, fcreator, O_RDWR | O_CREAT, 0666) < 0) {
        if (log_backoff(&icon_failures)) {
            LOG(log_error, logtype_afpd,
                "afp_addicon(%s): open: %s, failure %u",
                icon_dtfile(vol, fcreator), strerror(errno), icon_failures);
        }

        return AFPERR_MISC;
    }

    locked = icon_lock();

    if (locked < 0) {
        if (log_backoff(&icon_failures)) {
            LOG(log_error, logtype_afpd,
                "afp_addicon(%s): lock: %s, failure %u",
                icon_dtfile(vol, fcreator), strerror(errno), icon_failures);
        }

        icon_close();
        return AFPERR_MISC;
    }

    if (locked > 0 && log_backoff(&icon_unlocked)) {
        LOG(log_warning, logtype_afpd,
            "afp_addicon(%s): lock: %s, writing without one, write %u",
            icon_dtfile(vol, fcreator), strerror(errno), icon_unlocked);
    }

    if (fstat(si.sdt_fd, &st) < 0) {
        LOG(log_error, logtype_afpd, "afp_addicon(%s): fstat: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    /*
     * Read icon elements until we find a match to replace, or
     * we get to the end to insert.
     */
    p = imh;
    memcpy(p, &itag, sizeof(itag));
    p += sizeof(itag);
    memcpy(p, &ftype, sizeof(ftype));
    p += sizeof(ftype);
    *p++ = itype;
    *p++ = 0;
    bsize = htons(bsize);
    memcpy(p, &bsize, sizeof(bsize));
    bsize = ntohs(bsize);

    /* pos is the offset of the next header; a header or bitmap cut short by
     * the end of the file ends the records */
    while ((cc = read(si.sdt_fd, irh, sizeof(irh))) == (ssize_t)sizeof(irh)) {
        memcpy(&rsize, irh + 10, sizeof(rsize));
        rsize = ntohs(rsize);

        if (pos + (off_t)sizeof(irh) + rsize > st.st_size) {
            break;
        }

        /* whatever its tag */
        if (icon_entry_is(irh, (const uint8_t *)&ftype, (uint8_t)itype)) {
            if (bsize != rsize) {
                icon_close();
                return AFPERR_ITYPE;
            }

            match = true;
            break;
        }

        if (lseek(si.sdt_fd, (off_t) rsize, SEEK_CUR) < 0) {
            LOG(log_error, logtype_afpd, "afp_addicon(%s): lseek: %s",
                icon_dtfile(vol, fcreator), strerror(errno));
            icon_close();
            return AFPERR_MISC;
        }

        pos += (off_t)sizeof(irh) + rsize;
    }

    /* the cut below would drop the records after a short read */
    if (icon_read_failed(cc, pos, st.st_size)) {
        LOG(log_error, logtype_afpd, "afp_addicon(%s): read: %s",
            icon_dtfile(vol, fcreator),
            cc < 0 ? strerror(errno) : "short read");
        icon_close();
        return AFPERR_MISC;
    }

    /* a new record replaces whatever follows the last whole one */
    if (!match && pos < st.st_size && ftruncate(si.sdt_fd, pos) < 0) {
        LOG(log_error, logtype_afpd, "afp_addicon(%s): ftruncate: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    /* over the match, tag included, or after the last whole record */
    if (lseek(si.sdt_fd, pos, SEEK_SET) < 0) {
        LOG(log_error, logtype_afpd, "afp_addicon(%s): lseek: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    if (write_full(si.sdt_fd, imh, sizeof(imh)) == 0
            && write_full(si.sdt_fd, rbuf, bsize) == 0) {
        icon_close();
        return AFP_OK;
    }

    LOG(log_error, logtype_afpd, "afp_addicon(%s): write: %s",
        icon_dtfile(vol, fcreator), strerror(errno));

    /* undo a new record; one replaced in place keeps its length */
    if (!match && ftruncate(si.sdt_fd, pos) < 0) {
        LOG(log_error, logtype_afpd, "afp_addicon(%s): ftruncate: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
    }

    icon_close();
    return AFPERR_MISC;
}

/*!
 * @brief FPGetIconInfo: look up an icon of a creator by index and return its
 * record header
 *
 * The index counts from 1, and each request walks the icon file from its
 * first entry; a record the file cuts short ends the walk. The reply holds the
 * 12-byte record header.
 *
 * @param[in]  obj      session object, unused
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, AFPERR_NOITEM past the last icon or without an icon file,
 *          AFPERR_PARAM for an unknown volume, or AFPERR_MISC when the icon
 *          file fails to open or to read
 */
int afp_geticoninfo(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                    size_t *rbuflen)
{
    struct vol	*vol;
    unsigned char	fcreator[4], ih[12];
    uint16_t	vid, iindex, bsize;
    ssize_t	rc;
    off_t	off = 0;
    struct stat st;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(fcreator, ibuf, sizeof(fcreator));
    ibuf += sizeof(fcreator);
    memcpy(&iindex, ibuf, sizeof(iindex));
    iindex = ntohs(iindex);

    if (iconopen(vol, fcreator, O_RDONLY, 0) < 0) {
        return icon_open_failed("afp_iconinfo", vol, fcreator);
    }

    if (fstat(si.sdt_fd, &st) < 0) {
        LOG(log_error, logtype_afpd, "afp_iconinfo(%s): fstat: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    for (unsigned int i = 1;; i++) {
        rc = pread(si.sdt_fd, ih, sizeof(ih), off);

        if (icon_read_failed(rc, off, st.st_size)) {
            LOG(log_error, logtype_afpd, "afp_iconinfo(%s): read: %s",
                icon_dtfile(vol, fcreator),
                rc < 0 ? strerror(errno) : "short read");
            icon_close();
            return AFPERR_MISC;
        }

        if (rc != (ssize_t)sizeof(ih)) {
            icon_close();
            return AFPERR_NOITEM;
        }

        memcpy(&bsize, ih + 10, sizeof(bsize));
        off += (off_t)sizeof(ih) + ntohs(bsize);

        /* a record whose bitmap the file cuts short ends the records */
        if (off > st.st_size) {
            icon_close();
            return AFPERR_NOITEM;
        }

        if (i == iindex) {
            memcpy(rbuf, ih, sizeof(ih));
            *rbuflen = sizeof(ih);
            return AFP_OK;
        }
    }
}

/*!
 * @brief FPGetIcon: return the bitmap of an icon from the icon file of a
 * creator
 *
 * The call looks for the first record with the requested file type and icon
 * type and returns up to the requested size of its bitmap; a record the file
 * cuts short ends the records. A read larger than the reply buffer fails with
 * AFPERR_PARAM.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_NOITEM without a matching icon, AFPERR_PARAM for an
 *          unknown volume or a read the reply buffer cannot hold, or
 *          AFPERR_MISC when an open, read or seek of the icon file fails
 */
int afp_geticon(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                size_t *rbuflen)
{
    struct vol	*vol;
    ssize_t	rc, buflen;
    ssize_t	len;
    uint8_t	fcreator[4], ftype[4], itype, ih[12];
    uint16_t	vid, bsize, rsize = 0;
    off_t	pos = 0;
    struct stat st;
    buflen = *rbuflen;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(fcreator, ibuf, sizeof(fcreator));
    ibuf += sizeof(fcreator);
    memcpy(ftype, ibuf, sizeof(ftype));
    ibuf += sizeof(ftype);
    itype = (unsigned char) * ibuf++;
    ibuf++;
    memcpy(&bsize, ibuf, sizeof(bsize));
    bsize = ntohs(bsize);

    if (iconopen(vol, fcreator, O_RDONLY, 0) < 0) {
        return icon_open_failed("afp_geticon", vol, fcreator);
    }

    if (fstat(si.sdt_fd, &st) < 0) {
        LOG(log_error, logtype_afpd, "afp_geticon(%s): fstat: %s",
            icon_dtfile(vol, fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    if (lseek(si.sdt_fd, (off_t) 0L, SEEK_SET) < 0) {
        LOG(log_error, logtype_afpd, "afp_geticon(%s): lseek: %s", icon_dtfile(vol,
                fcreator), strerror(errno));
        icon_close();
        return AFPERR_MISC;
    }

    while ((rc = read(si.sdt_fd, ih, sizeof(ih))) == (ssize_t)sizeof(ih)) {
        memcpy(&rsize, ih + 10, sizeof(rsize));
        rsize = ntohs(rsize);

        /* a record whose bitmap the file cuts short ends the records */
        if (pos + (off_t)sizeof(ih) + rsize > st.st_size) {
            return AFPERR_NOITEM;
        }

        if (icon_entry_is(ih, ftype, itype)) {
            break;
        }

        if (lseek(si.sdt_fd, (off_t) rsize, SEEK_CUR) < 0) {
            LOG(log_error, logtype_afpd, "afp_geticon(%s): lseek: %s", icon_dtfile(vol,
                    fcreator), strerror(errno));
            return AFPERR_MISC;
        }

        pos += (off_t)sizeof(ih) + rsize;
    }

    if (icon_read_failed(rc, pos, st.st_size)) {
        LOG(log_error, logtype_afpd, "afp_geticon(%s): read: %s",
            icon_dtfile(vol, fcreator),
            rc < 0 ? strerror(errno) : "short read");
        return AFPERR_MISC;
    }

    if (rc != (ssize_t)sizeof(ih)) {
        return AFPERR_NOITEM;
    }

    rc = MIN(bsize, rsize);

    if (rc > buflen) {
        return AFPERR_PARAM;
    }

    if ((len = read(si.sdt_fd, rbuf, rc)) < rc) {
        LOG(log_error, logtype_afpd, "afp_geticon(%s): read: %s",
            icon_dtfile(vol, fcreator),
            len < 0 ? strerror(errno) : "short read");
        return AFPERR_MISC;
    }

    *rbuflen = rc;
    return AFP_OK;
}

static const char		hexdig[] = "0123456789abcdef";

/*!
 * @brief Build the path of a desktop database file for a creator
 *
 * The path is v_dbpath/.AppleDesktop, then a directory named by the first
 * creator byte, then the 4 creator bytes and ext. A byte that is not
 * printable, or a slash, becomes 2 hex digits.
 *
 * @param[in] vol      volume of the desktop database
 * @param[in] creator  4-byte creator code
 * @param[in] ext      file name suffix, such as ".appl" or ".icon"
 * @returns the path in a static buffer, or NULL without a database path or for
 *          a path that is too long
 */
char *dtfile(const struct vol *vol, uint8_t creator[], char *ext)
{
    static char	path[MAXPATHLEN + 1];
    char	*p;
    unsigned int i;

    if (vol == NULL || vol->v_dbpath == NULL) {
        LOG(log_error, logtype_afpd, "dtfile: bad volume parameter");
        return NULL;
    }

    if (snprintf(path, MAXPATHLEN + 1, "%s/" APPLEDESKTOP "/",
                 vol->v_dbpath) >= MAXPATHLEN + 1) {
        LOG(log_error, logtype_afpd, "dtfile: path too long");
        return NULL;
    }

    for (p = path; *p != '\0'; p++)
        ;

    if (!isprint(creator[0]) || creator[0] == '/') {
        *p++ = hexdig[(creator[0] & 0xf0) >> 4];
        *p++ = hexdig[creator[0] & 0x0f];
    } else {
        *p++ = creator[0];
    }

    *p++ = '/';

    for (i = 0; i < sizeof(CreatorType); i++) {
        if (!isprint(creator[i]) || creator[i] == '/') {
            *p++ = hexdig[(creator[i] & 0xf0) >> 4];
            *p++ = hexdig[creator[i] & 0x0f];
        } else {
            *p++ = creator[i];
        }
    }

    *p = '\0';
    strlcat(path, ext, sizeof(path));
    return path;
}

/*!
 * @brief Convert an AFP name into the name on disk
 *
 * An empty name becomes ".". A mangled name resolves through demangle, from
 * the object's dircache entry or the CNID database. Any other name converts
 * from UTF8-MAC or the Mac charset to the volume charset.
 *
 * @param[in] vol    volume of the name
 * @param[in] mpath  AFP name of one path element
 * @param[in] did    CNID of the parent directory, or CNID_INVALID to
 *                   convert without demangling
 * @param[in] utf8   nonzero for a UTF8-MAC name of AFP 3
 * @returns the name in a static buffer, or NULL when the conversion fails
 */
char *mtoupath(const struct vol *vol, char *mpath, cnid_t did, int utf8)
{
    /* for convert_charset dest_len parameter +2 */
    static char  upath[MAXPATHLEN + 2];
    char	*m, *u;
    size_t       inplen;
    uint16_t	 flags;

    if (*mpath == '\0') {
        strlcpy(upath, ".", sizeof(upath));
        return upath;
    }

    /* set conversion flags */
    flags = vol->v_mtou_flags;

    if (did != CNID_INVALID && (m = demangle(vol, mpath, did)) != mpath) {
        return m;
    }

    m = mpath;
    u = upath;
    inplen = strlen(m);

    if ((size_t) -1 == convert_charset(utf8 ? CH_UTF8_MAC :
                                       vol->v_maccharset, vol->v_volcharset, vol->v_maccharset, m, inplen, u,
                                       sizeof(upath),
                                       &flags)) {
        LOG(log_error, logtype_afpd, "conversion from %s to %s for %s failed.",
            (utf8) ? "UTF8-MAC" : vol->v_maccodepage, vol->v_volcodepage, mpath);
        return NULL;
    }

    LOG(log_debug9, logtype_afpd, "mtoupath: '%s':'%s'", mpath, upath);
    return upath;
}

/*!
 * @brief Convert a name on disk into the AFP name a client sees
 *
 * The name converts from the volume charset to UTF8-MAC or the Mac charset. A
 * name too long for the client, or with characters it cannot map, goes
 * through mangle, which needs the CNID. A failed conversion mangles "???"
 * instead. A reply packs both names of one object, so each encoding has a
 * static buffer of its own.
 *
 * @param[in] vol    volume of the name
 * @param[in] upath  name on disk of one path element
 * @param[in] id     CNID of the file, or 0
 * @param[in] utf8   nonzero for a UTF8-MAC name of AFP 3
 * @returns the name in the encoding's static buffer, or NULL when the name
 *          needs mangling and id is 0
 */
char *utompath(const struct vol *vol, char *upath, cnid_t id, int utf8)
{
    /* for convert_charset dest_len parameter +2 */
    static char  mpath[2][MAXPATHLEN + 2];
    char        *m, *u;
    uint16_t    flags;
    size_t       outlen;
    m = mpath[utf8 != 0];
    outlen = strlen(upath);
    flags = vol->v_utom_flags;
    u = upath;

    /* convert charsets */
    if ((size_t) -1 == (outlen = convert_charset(vol->v_volcharset,
                                                 utf8 ? CH_UTF8_MAC
                                                 : vol->v_maccharset,
                                                 vol->v_maccharset, u, outlen,
                                                 m, sizeof(mpath[0]),
                                                 &flags))) {
        LOG(log_error, logtype_afpd, "Conversion from %s to %s for %s (%u) failed.",
            vol->v_volcodepage, vol->v_maccodepage, u, ntohl(id));
        goto utompath_error;
    }

    flags = !!(flags & CONV_REQMANGLE);

    if (utf8) {
        flags |= 2;
    }

    m = mangle(vol, m, outlen, upath, id, flags);
    LOG(log_debug9, logtype_afpd, "utompath: '%s':'%s':'%2.2X'", upath, m,
        ntohl(id));
    return m;
utompath_error:
    u = "???";
    m = mangle(vol, u, strlen(u), upath, id, utf8 ? 3 : 1);
    return m;
}

/*!
 * @brief Store a Finder comment in the AppleDouble metadata of a file or
 * directory
 *
 * The comment keeps at most 199 bytes and needs write access. An open fork
 * lends its adouble. Any other object opens its metadata and creates it when
 * missing. Metadata without a comment entry stores nothing. After the write
 * the dircache entry takes the new stat and metadata, and the other sessions
 * get a refresh hint.
 *
 * @param[in] obj   session object
 * @param[in] vol   volume of the object
 * @param[in] path  resolved object
 * @param[in] ibuf  comment as a Pascal string
 * @returns AFP_OK, or AFPERR_ACCESS without write access or metadata
 */
static int ad_addcomment(const AFPObj *obj, struct vol *vol, struct path *path,
                         char *ibuf)
{
    struct ofork        *of;
    char                *name, *upath;
    int                 isadir;
    int			clen;
    struct adouble	ad, *adp;
    clen = (uint8_t) * ibuf++;
    clen = MIN(clen, 199);
    upath = path->u_name;

    if (check_access(obj, vol, upath, OPENACC_WR) < 0) {
        return AFPERR_ACCESS;
    }

    isadir = path_isadir(path);

    if (isadir || !(of = of_findname(vol, path))) {
        ad_init(&ad, vol);
        adp = &ad;
    } else {
        adp = of->of_ad;
    }

    if (ad_open(adp, upath,
                ADFLAGS_HF | (isadir ? ADFLAGS_DIR : 0) | ADFLAGS_CREATE | ADFLAGS_RDWR,
                0666) < 0) {
        return AFPERR_ACCESS;
    }

    if (ad_getentryoff(adp, ADEID_COMMENT) && ad_entry(adp, ADEID_COMMENT)) {
        if (ad_get_MD_flags(adp) & O_CREAT) {
            if (*path->m_name == '\0') {
                name = (char *)curdir->d_m_name->data;
            } else {
                name = path->m_name;
            }

            ad_setname(adp, name);
        }

        ad_setentrylen(adp, ADEID_COMMENT, clen);
        memcpy(ad_entry(adp, ADEID_COMMENT), ibuf, clen);
        /* Acquire cache pointer before flush changes ctime */
        struct dir *cached = isadir ? path->d_dir
                             : dircache_search_by_name(vol, curdir, upath, strnlen(upath,
                                 CNID_MAX_PATH_LEN));
        ad_flush(adp);

        /* Refresh cache after comment write */
        if (cached) {
            struct stat post_st;

            if (ostat(upath, &post_st, 0) == 0) {
                dir_modify(vol, cached, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT | DCMOD_AD,
                    .st = &post_st,
                    .adp = adp,
                });
            }
        }

        /* Send hint to afpd siblings — AD comment write changed ctime */
        cnid_t hint_cnid = cached ? cached->d_did : CNID_INVALID;

        if (hint_cnid == CNID_INVALID && upath) {
            size_t ulen = strnlen(upath, CNID_MAX_PATH_LEN);
            hint_cnid = cnid_get(vol->v_cdb, curdir->d_did, upath, ulen);
        }

        if (hint_cnid != CNID_INVALID) {
            ipc_send_cache_hint(obj, vol->v_vid, hint_cnid, CACHE_HINT_REFRESH);
        }
    }

    ad_close(adp, ADFLAGS_HF);
    return AFP_OK;
}

/*!
 * @brief FPAddComment: store a Finder comment for a file or directory
 *
 * The request carries the volume, the directory, the path, and the comment as
 * a Pascal string at an even offset. The comment goes to ad_addcomment.
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of ad_addcomment, AFPERR_PARAM for an unknown volume, or
 *          the error of the directory or path lookup
 */
int afp_addcomment(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
                   size_t *rbuflen)
{
    struct vol		*vol;
    struct dir		*dir;
    struct path         *path;
    uint32_t           did;
    uint16_t		vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    if (NULL == (path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    if ((intptr_t)ibuf & 1) {
        ibuf++;
    }

    return ad_addcomment(obj, vol, path, ibuf);
}

/*!
 * @brief Read the Finder comment of a file or directory from its AppleDouble
 * metadata
 *
 * With no open fork, a dircache entry that recorded no metadata, with a
 * matching ctime and inode, answers AFPERR_NOITEM without a disk read. A read
 * also fills a dircache entry that held no metadata. A stored comment of 0 or
 * over 199 bytes counts as missing. The reply holds at most 128 bytes,
 * because macOS uses 128 and longer comments break Adobe CS2.
 *
 * @param[in]  vol      volume of the object
 * @param[in]  path     resolved object
 * @param[out] rbuf     receives the comment as a Pascal string
 * @param[out] rbuflen  reply length on return
 * @returns AFP_OK, or AFPERR_NOITEM without metadata or comment
 */
static int ad_getcomment(struct vol *vol, struct path *path, char *rbuf,
                         size_t *rbuflen)
{
    struct adouble	ad, *adp;
    struct ofork        *of;
    char		*upath;
    int                 isadir;
    int			clen;
    upath = path->u_name;
    isadir = path_isadir(path);

    if (isadir || !(of = of_findname(vol, path))) {
        ad_init(&ad, vol);
        adp = &ad;
    } else {
        adp = of->of_ad;
    }

    struct dir *cached = isadir ? path->d_dir
                         : dircache_search_by_name(vol, curdir, upath, strnlen(upath,
                             CNID_MAX_PATH_LEN));

    /* ADEID_COMMENT is not in the Tier 1 cache, so a cache hit cannot
     * serve the comment itself — but the negative AD cache can answer
     * "no metadata, hence no comment" without a disk probe. Only with a
     * ctime/inode-validated entry and no open fork (an open fork may
     * have just created metadata). */
    if (adp == &ad && cached && ad_rlen_meta_absent(cached->dcache_rlen)
            && path->st_valid && path->st_errno == 0
            && cached->dcache_ctime == path->st.st_ctime
            && cached->dcache_ino == path->st.st_ino) {
        return AFPERR_NOITEM;
    }

    if (ad_metadata(upath, (isadir ? ADFLAGS_DIR : 0), adp) < 0) {
        return AFPERR_NOITEM;
    }

    /* Opportunistic AD cache population while the AD is open */
    if (cached && cached->dcache_rlen < 0) {
        ad_store_to_cache(adp, cached);
    }

    if (!ad_getentryoff(adp, ADEID_COMMENT) || !ad_entry(adp, ADEID_COMMENT)) {
        ad_close(adp, ADFLAGS_HF);
        return AFPERR_NOITEM;
    }

    /*
     * Make sure the AD file is not bogus.
     */
    if (ad_getentrylen(adp, ADEID_COMMENT) <= 0 ||
            ad_getentrylen(adp, ADEID_COMMENT) > 199) {
        ad_close(adp, ADFLAGS_HF);
        return AFPERR_NOITEM;
    }

    /* OSX only use 128, greater kill Adobe CS2 */
    clen = (int)MIN(ad_getentrylen(adp, ADEID_COMMENT), 128);
    *rbuf++ = clen;
    memcpy(rbuf, ad_entry(adp, ADEID_COMMENT), clen);
    *rbuflen = clen + 1;
    ad_close(adp, ADFLAGS_HF);
    return AFP_OK;
}

/*!
 * @brief FPGetComment: return the Finder comment of a file or directory
 *
 * @param[in]  obj      session object, unused
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the result of ad_getcomment, AFPERR_PARAM for an unknown volume, or
 *          the error of the directory or path lookup
 */
int afp_getcomment(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                   size_t *rbuflen)
{
    struct vol		*vol;
    struct dir		*dir;
    struct path         *s_path;
    uint32_t		did;
    uint16_t		vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    return ad_getcomment(vol, s_path, rbuf, rbuflen);
}

/*!
 * @brief Remove the Finder comment from the AppleDouble metadata of a file or
 * directory
 *
 * The call needs write access and never creates metadata. It sets the comment
 * length to 0, then refreshes the dircache entry and sends the other sessions
 * a refresh hint.
 *
 * @param[in] obj   session object
 * @param[in] vol   volume of the object
 * @param[in] path  resolved object
 * @returns AFP_OK, AFPERR_ACCESS, AFPERR_NOITEM without metadata, or
 *          AFPERR_PARAM
 */
static int ad_rmvcomment(const AFPObj *obj, struct vol *vol, struct path *path)
{
    struct adouble	ad, *adp;
    struct ofork        *of;
    int                 isadir;
    char		*upath;
    upath = path->u_name;

    if (check_access(obj, vol, upath, OPENACC_WR) < 0) {
        return AFPERR_ACCESS;
    }

    isadir = path_isadir(path);

    if (isadir || !(of = of_findname(vol, path))) {
        ad_init(&ad, vol);
        adp = &ad;
    } else {
        adp = of->of_ad;
    }

    if (ad_open(adp, upath, ADFLAGS_HF | ADFLAGS_RDWR | (isadir ? ADFLAGS_DIR : 0))
            < 0) {
        switch (errno) {
        case ENOENT :
            return AFPERR_NOITEM;

        case EACCES :
            return AFPERR_ACCESS;

        default :
            return AFPERR_PARAM;
        }
    }

    if (ad_getentryoff(adp, ADEID_COMMENT)) {
        /* Acquire cache pointer before flush changes ctime */
        struct dir *cached = isadir ? path->d_dir
                             : dircache_search_by_name(vol, curdir, upath, strnlen(upath,
                                 CNID_MAX_PATH_LEN));
        ad_setentrylen(adp, ADEID_COMMENT, 0);
        ad_flush(adp);

        /* Refresh cache after comment removal */
        if (cached) {
            struct stat post_st;

            if (ostat(upath, &post_st, 0) == 0) {
                dir_modify(vol, cached, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT | DCMOD_AD,
                    .st = &post_st,
                    .adp = adp,
                });
            }
        }

        /* Send hint to afpd siblings — AD comment removal changed ctime */
        cnid_t hint_cnid = cached ? cached->d_did : CNID_INVALID;

        if (hint_cnid == CNID_INVALID && upath) {
            size_t ulen = strnlen(upath, CNID_MAX_PATH_LEN);
            hint_cnid = cnid_get(vol->v_cdb, curdir->d_did, upath, ulen);
        }

        if (hint_cnid != CNID_INVALID) {
            ipc_send_cache_hint(obj, vol->v_vid, hint_cnid, CACHE_HINT_REFRESH);
        }
    }

    ad_close(adp, ADFLAGS_HF);
    return AFP_OK;
}

/*!
 * @brief FPRemoveComment: remove the Finder comment of a file or directory
 *
 * @param[in]  obj      session object
 * @param[in]  ibuf     request block
 * @param[in]  ibuflen  request block length, unused
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of ad_rmvcomment, AFPERR_PARAM for an unknown volume, or
 *          the error of the directory or path lookup
 */
int afp_rmvcomment(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                   char *rbuf _U_, size_t *rbuflen)
{
    struct vol		*vol;
    struct dir		*dir;
    struct path         *s_path;
    uint32_t		did;
    uint16_t		vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    return ad_rmvcomment(obj, vol, s_path);
}
