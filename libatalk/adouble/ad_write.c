/*
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * @brief Write count bytes that start at offset in an AppleDouble fork
 *
 * The write goes through pwrite and leaves the file position alone. The
 * function checks no byte-range locks.
 *
 * @param[in] ad_fd   fork descriptor
 * @param[in] buf     bytes to write
 * @param[in] count   bytes to write
 * @param[in] offset  file offset
 * @returns bytes written, or -1 with errno set
 */
ssize_t adf_pwrite(struct ad_fd *ad_fd, const void *buf, size_t count,
                   off_t offset)
{
    ssize_t		cc;
#ifndef  HAVE_PWRITE

    if (ad_fd->adf_off != offset) {
        if (lseek(ad_fd->adf_fd, offset, SEEK_SET) < 0) {
            return -1;
        }

        ad_fd->adf_off = offset;
    }

    cc = write(ad_fd->adf_fd, buf, count);

    if (cc < 0) {
        return -1;
    }

    ad_fd->adf_off += cc;
#else
    cc = pwrite(ad_fd->adf_fd, buf, count, offset);
#endif
    return cc;
}

/*!
 * @brief Write to the data or resource fork of an AppleDouble
 *
 * A symlink data fork fails with EACCES. With a nonzero end, off counts back
 * from the end of the fork. The ea layout stores the resource fork at offset 0
 * of its EA, or behind the sidecar header without HAVE_EAFD. Version 2 adds
 * the entry offset from the header, and a resource fork write that reaches
 * past ad_rlen extends it. The function checks no byte-range locks.
 *
 * @param[in,out] ad      AppleDouble with the fork open
 * @param[in]     eid     ADEID_DFORK or ADEID_RFORK
 * @param[in]     off     offset within the fork
 * @param[in]     end     0, or nonzero to count off back from the end of the
 *                        fork
 * @param[in]     buf     bytes to write
 * @param[in]     buflen  bytes to write
 * @returns bytes written, or -1 on failure or for any other eid
 */
ssize_t ad_write(struct adouble *ad, uint32_t eid, off_t off, int end,
                 const char *buf, size_t buflen)
{
    EC_INIT;
    struct stat		st;
    ssize_t		cc;
    off_t    r_off;

    if (ad_data_fileno(ad) == AD_SYMLINK) {
        errno = EACCES;
        return -1;
    }

    LOG(log_maxdebug, logtype_ad, "ad_write: off: %ju, size: %zu, eabuflen: %zu",
        (uintmax_t)off, buflen, ad->ad_rlen);

    if (eid == ADEID_DFORK) {
        if (end) {
            if (fstat(ad_data_fileno(ad), &st) < 0) {
                return -1;
            }

            off = st.st_size - off;
        }

        cc = adf_pwrite(&ad->ad_data_fork, buf, buflen, off);
    } else if (eid == ADEID_RFORK) {
        if (end) {
            if (fstat(ad_reso_fileno(ad), &st) < 0) {
                return -1;
            }

            off = st.st_size - off - ad_getentryoff(ad, eid);
        }

        if (ad->ad_vers == AD_VERSION_EA) {
#ifdef HAVE_EAFD
            r_off = off;
#else
            r_off = ADEDOFF_RFORK_OSX + off;
#endif
        } else {
            r_off = ad_getentryoff(ad, eid) + off;
        }

        cc = adf_pwrite(&ad->ad_resource_fork, buf, buflen, r_off);

        if (cc < 0) {
            return -1;    /* nothing written; do NOT touch ad_rlen */
        }

        if (ad->ad_rlen < off + cc) {
            ad->ad_rlen = off + cc;
        }
    } else {
        return -1; /* we don't know how to write if it's not a resource or data fork */
    }

    if (ret != 0) {
        return ret;
    }

    return cc;
}

/*!
 * @brief Truncate or extend an open file to length
 *
 * ftruncate does the work. When it fails on a file shorter than length, the
 * function writes one zero byte at length minus one instead. That covers
 * platforms whose ftruncate cannot extend a file. Without HAVE_PWRITE it also
 * restores the file position afterwards. The caller holds any locks the
 * region needs.
 *
 * @param[in] fd      open file
 * @param[in] length  new length in bytes
 * @returns 0, or -1 with the ftruncate errno, or the write errno when the
 *          extension failed
 */
int sys_ftruncate(int fd, off_t length)
{
#ifndef  HAVE_PWRITE
    off_t           curpos;
#endif
    int             err;
    struct stat	st;
    char            c = 0;

    if (!ftruncate(fd, length)) {
        return 0;
    }

    /* maybe ftruncate doesn't work if we try to extend the size */
    err = errno;
#ifndef  HAVE_PWRITE

    /* we only care about file pointer if we don't use pwrite */
    if ((off_t) -1 == (curpos = lseek(fd, 0, SEEK_CUR))) {
        errno = err;
        return -1;
    }

#endif

    if (fstat(fd, &st) < 0) {
        errno = err;
        return -1;
    }

    if (st.st_size > length) {
        errno = err;
        return -1;
    }

    if (lseek(fd, length - 1, SEEK_SET) != length - 1) {
        errno = err;
        return -1;
    }

    if (1 != write(fd, &c, 1)) {
        /* return the write errno */
        return -1;
    }

#ifndef  HAVE_PWRITE

    if (curpos != lseek(fd, curpos,  SEEK_SET)) {
        errno = err;
        return -1;
    }

#endif
    return 0;
}

/*!
 * @brief Truncate the resource fork of an AppleDouble to size
 *
 * The truncation point is size plus the resource fork entry offset, so the
 * header ahead of the fork survives. ad_rlen follows. A zero size keeps the
 * fork file, because an open fork may still reference it and ad_flush would
 * recreate it. of_closefork deletes empty resource forks.
 *
 * @param[in,out] ad     AppleDouble with the resource fork open
 * @param[in]     uname  file name, for logging only
 * @param[in]     size   new fork length in bytes
 * @returns 0, or -1 with errno set
 */
int ad_rtruncate(struct adouble *ad, const char *uname, const off_t size)
{
    EC_INIT;
    /*
     * We can't delete 0 byte size resource forks either, because a
     * fork may reference the adouble handle with an open fd for the
     * file, which means we would only delete the directory entry, not
     * the file. Subsequently all code that works with fork handles
     * finds the fork open, so e.g. flushing a fork (ad_flush()) will
     * recreate ._ files.  The correct place to delete 0 byte sized
     * resource forks is in of_closefork().
     */
    EC_NEG1(sys_ftruncate(ad_reso_fileno(ad),
                          size + ad->ad_eid[ADEID_RFORK].ade_off));
    ad->ad_rlen = size;
EC_CLEANUP:

    if (ret != 0)
        LOG(log_error, logtype_ad, "ad_rtruncate(\"%s\"): %s",
            fullpathname(uname), strerror(errno));

    EC_EXIT;
}

/*!
 * @brief Truncate the data fork of an AppleDouble to size
 *
 * @param[in,out] ad    AppleDouble with the data fork open
 * @param[in]     size  new fork length in bytes
 * @returns 0, or -1 with errno set
 */
int ad_dtruncate(struct adouble *ad, const off_t size)
{
    if (sys_ftruncate(ad_data_fileno(ad), size) < 0) {
        LOG(log_error, logtype_ad, "sys_ftruncate(fd: %d): %s",
            ad_data_fileno(ad), strerror(errno));
        return -1;
    }

    return 0;
}

/*!
 * @brief Write buflen bytes of buf to dfd, retrying after EINTR
 *
 * @param[in] dfd     destination descriptor
 * @param[in] buf     bytes to write
 * @param[in] buflen  bytes to write
 * @returns 0, or -1 with errno set
 */
static int copy_all(const int dfd, const void *buf,
                    size_t buflen)
{
    ssize_t cc;

    while (buflen > 0) {
        if ((cc = write(dfd, buf, buflen)) < 0) {
            switch (errno) {
            case EINTR:
                continue;

            default:
                return -1;
            }
        }

        buflen -= cc;
    }

    return 0;
}

/*!
 * @brief Transfer one fork from the AppleDouble ads into the AppleDouble add
 *
 * Both forks must be open. The read loop starts at the entry offset of each
 * side and runs until the source returns end of file. A caller buffer serves
 * only when it is larger than the 8 KiB stack buffer.
 *
 * @param[in]     eid     ADEID_DFORK, or any other entry for the resource fork
 * @param[in,out] add     destination AppleDouble
 * @param[in]     ads     source AppleDouble
 * @param[in]     buf     optional transfer buffer, or NULL
 * @param[in]     buflen  size of buf in bytes
 * @returns 0, or -1 with errno set
 */
int copy_fork(int eid, struct adouble *add, struct adouble *ads, uint8_t *buf,
              size_t buflen)
{
    ssize_t cc;
    int     err = 0;
    uint8_t fixedbuf[8192];
    uint8_t *filebuf;
    int     sfd, dfd;

    if (buf != NULL && buflen > sizeof(fixedbuf)) {
        filebuf = buf;
    } else {
        filebuf = fixedbuf;
        buflen = sizeof(fixedbuf);
    }

    if (eid == ADEID_DFORK) {
        sfd = ad_data_fileno(ads);
        dfd = ad_data_fileno(add);
    } else {
        sfd = ad_reso_fileno(ads);
        dfd = ad_reso_fileno(add);
    }

    if ((off_t) -1 == lseek(sfd, ad_getentryoff(ads, eid), SEEK_SET)) {
        return -1;
    }

    if ((off_t) -1 == lseek(dfd, ad_getentryoff(add, eid), SEEK_SET)) {
        return -1;
    }

#if 0 /* ifdef SENDFILE_FLAVOR_LINUX */
    /* doesn't work With 2.6 FIXME, only check for EBADFD ? */
    off_t   offset = 0;
    size_t  size;
    struct stat         st;
#define BUF 128 * 1024 * 1024

    if (fstat(sfd, &st) == 0) {
        while (1) {
            if (offset >= st.st_size) {
                return 0;
            }

            size = (st.st_size - offset > BUF) ? BUF : st.st_size - offset;

            if ((cc = sys_sendfile(dfd, sfd, &offset, size)) < 0) {
                switch (errno) {
                case ENOSYS:
                case EINVAL:  /* there's no guarantee that all fs support sendfile */
                    goto no_sendfile;

                default:
                    return -1;
                }
            }
        }
    }

no_sendfile:
    lseek(sfd, offset, SEEK_SET);
#endif

    while (1) {
        if ((cc = read(sfd, filebuf, buflen)) < 0) {
            if (errno == EINTR) {
                continue;
            }

            err = -1;
            break;
        }

        if (!cc || ((err = copy_all(dfd, filebuf, cc)) < 0)) {
            break;
        }
    }

    return err;
}
