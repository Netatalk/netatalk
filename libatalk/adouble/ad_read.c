/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *  Research Systems Unix Group
 *  The University of Michigan
 *  c/o Mike Clark
 *  535 W. William Street
 *  Ann Arbor, Michigan
 *  +1-313-763-0525
 *  netatalk@itd.umich.edu
 */

/*!
 * @file
 * @brief Reading the forks of an AppleDouble
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
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * @brief Read count bytes that start at offset in an AppleDouble fork
 *
 * The read goes through pread and leaves the file position alone.
 *
 * @param[in]  adf     fork descriptor
 * @param[out] buf     receives the bytes
 * @param[in]  count   bytes to read
 * @param[in]  offset  file offset
 * @returns bytes read, or -1 with errno set
 */
ssize_t adf_pread(struct ad_fd *adf, void *buf, size_t count, off_t offset)
{
    ssize_t     cc;
#ifndef  HAVE_PREAD

    if (adf->adf_off != offset) {
        if (lseek(adf->adf_fd, offset, SEEK_SET) < 0) {
            return -1;
        }

        adf->adf_off = offset;
    }

    if ((cc = read(adf->adf_fd, buf, count)) < 0) {
        return -1;
    }

    adf->adf_off += cc;
#else
    cc = pread(adf->adf_fd, buf, count, offset);
#endif
    return cc;
}

/*!
 * @brief Read from the data or resource fork of an AppleDouble
 *
 * The data fork of a symlink yields the link target, and fails when buflen
 * cannot hold it. Any other eid reads the resource fork, and a closed resource
 * fork yields 0. The ea layout stores the fork at offset 0 of its EA, or
 * behind the sidecar header without HAVE_EAFD. Version 2 adds the entry offset
 * from the header. The function checks no byte-range locks.
 *
 * @param[in,out] ad      AppleDouble with the fork open
 * @param[in]     eid     ADEID_DFORK, or any other entry for the resource fork
 * @param[in]     off     offset within the fork
 * @param[out]    buf     receives the bytes
 * @param[in]     buflen  bytes to read
 * @returns bytes read, 0 at the end of the fork or for a closed resource fork,
 *          or -1 on failure
 */
ssize_t ad_read(struct adouble *ad, const uint32_t eid, off_t off, char *buf,
                const size_t buflen)
{
    ssize_t     cc;
    off_t r_off = 0;

    /* We're either reading the data fork (and thus the data file)
     * or we're reading anything else (and thus the header file). */
    if (eid == ADEID_DFORK) {
        if (ad->ad_data_fork.adf_syml != 0) {
            /* It's a symlink, we already have the target in adf_syml */
            cc = strlen(ad->ad_data_fork.adf_syml);

            if (buflen < cc)
                /* Request buffersize is too small, force AFPERR_PARAM */
            {
                return -1;
            }

            memcpy(buf, ad->ad_data_fork.adf_syml, cc);
        } else {
            cc = adf_pread(&ad->ad_data_fork, buf, buflen, off);
        }
    } else {
        if (! ad_rsrc_open(ad))
            /* resource fork is not open ( cf etc/afp/fork.c) */
        {
            return 0;
        }

        if (ad->ad_vers == AD_VERSION_EA) {
#ifdef HAVE_EAFD
            r_off = off;
#else
            r_off = off + ADEDOFF_RFORK_OSX;
#endif
        } else {
            r_off = ad_getentryoff(ad, eid) + off;
        }

        if ((cc = adf_pread(&ad->ad_resource_fork, buf, buflen, r_off)) < 0) {
            return -1;
        }
    }

    return cc;
}
