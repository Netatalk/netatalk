/*
 * $Id: ad_read.c,v 1.10 2010-02-10 14:05:37 franklahm Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/adouble.h>
#include <string.h>
#include <sys/param.h>

#ifndef MIN
#define MIN(a,b)    ((a)<(b)?(a):(b))
#endif /* ! MIN */

ssize_t adf_pread(struct ad_fd *ad_fd, void *buf, size_t count, off_t offset)
{
    ssize_t     cc;

#ifndef  HAVE_PREAD
    if ( ad_fd->adf_off != offset ) {
        if ( lseek( ad_fd->adf_fd, offset, SEEK_SET ) < 0 ) {
            return -1;
        }
        ad_fd->adf_off = offset;
    }
    if (( cc = read( ad_fd->adf_fd, buf, count )) < 0 ) {
        return -1;
    }
    ad_fd->adf_off += cc;
#else
    cc = pread(ad_fd->adf_fd, buf, count, offset );
#endif
    return cc;
}

/* XXX: locks have to be checked before each stream of consecutive
 *      ad_reads to prevent a denial in the middle from causing
 *      problems. */
ssize_t ad_read( struct adouble *ad, const u_int32_t eid, off_t off, char *buf, const size_t buflen)
{
    ssize_t     cc;

    /* We're either reading the data fork (and thus the data file)
     * or we're reading anything else (and thus the header file). */
    if ( eid == ADEID_DFORK ) {
        if (ad->ad_data_fork.adf_syml !=0 ) {
            /* It's a symlink, we already have the target in adf_syml */
            cc = strlen(ad->ad_data_fork.adf_syml);
            if (buflen < cc)
                /* Request buffersize is too small, force AFPERR_PARAM */
                return -1;
            memcpy(buf, ad->ad_data_fork.adf_syml, cc);
        } else {
            cc = adf_pread(&ad->ad_data_fork, buf, buflen, off);
        }
    } else {
        off_t r_off;

        if ( ad_reso_fileno( ad ) == -1 ) {
            /* resource fork is not open ( cf etc/afp/fork.c) */
            return 0;
        }
        r_off = ad_getentryoff(ad, eid) + off;

        if (( cc = adf_pread( &ad->ad_resource_fork, buf, buflen, r_off )) < 0 ) {
            return( -1 );
        }
        /*
         * We've just read in bytes from the disk that we read earlier
         * into ad_data. If we're going to write this buffer out later,
         * we need to update ad_data.
         * FIXME : always false?
         */
        if (r_off < ad_getentryoff(ad, ADEID_RFORK)) {
            if ( ad->ad_resource_fork.adf_flags & O_RDWR ) {
                memcpy(buf, ad->ad_data + r_off,
                       MIN(sizeof( ad->ad_data ) - r_off, cc));
            } else {
                memcpy(ad->ad_data + r_off, buf,
                       MIN(sizeof( ad->ad_data ) - r_off, cc));
            }
        }
    }

    return( cc );
}
