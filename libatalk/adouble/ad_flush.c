/*
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
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include <netatalk/endian.h>
#include <atalk/adouble.h>

#include "ad_private.h"

/* rebuild the header */
void ad_rebuild_header(struct adouble *ad)
{
    u_int32_t		eid;
    u_int16_t		nent;
    char		*buf, *nentp;

    /*
     * Rebuild any header information that might have changed.
     */
    buf = ad->ad_data;
    ad->ad_magic = htonl( ad->ad_magic );
    memcpy(buf, &ad->ad_magic, sizeof( ad->ad_magic ));
    ad->ad_magic = ntohl( ad->ad_magic );
    buf += sizeof( ad->ad_magic );
    
    ad->ad_version = htonl( ad->ad_version );
    memcpy(buf, &ad->ad_version, sizeof( ad->ad_version ));
    ad->ad_version = ntohl( ad->ad_version );
    buf += sizeof( ad->ad_version );
    memcpy(buf, ad->ad_filler, sizeof( ad->ad_filler ));
    buf += sizeof( ad->ad_filler );
    
    nentp = buf;
    buf += sizeof( nent );
    for ( eid = 0, nent = 0; eid < ADEID_MAX; eid++ ) {
      if ( ad->ad_eid[ eid ].ade_off == 0 ) {
	continue;
      }
      eid = htonl( eid );
      memcpy(buf, &eid, sizeof( eid ));
      eid = ntohl( eid );
      buf += sizeof( eid );
      ad->ad_eid[ eid ].ade_off = htonl( ad->ad_eid[ eid ].ade_off );
      memcpy(buf, &ad->ad_eid[ eid ].ade_off,
	     sizeof( ad->ad_eid[ eid ].ade_off ));
      ad->ad_eid[ eid ].ade_off = ntohl( ad->ad_eid[ eid ].ade_off );
      buf += sizeof( ad->ad_eid[ eid ].ade_off );
      ad->ad_eid[ eid ].ade_len = htonl( ad->ad_eid[ eid ].ade_len );
      memcpy(buf, &ad->ad_eid[ eid ].ade_len, 
	     sizeof( ad->ad_eid[ eid ].ade_len ));
      ad->ad_eid[ eid ].ade_len = ntohl( ad->ad_eid[ eid ].ade_len );
      buf += sizeof( ad->ad_eid[ eid ].ade_len );
      nent++;
    }
    nent = htons( nent );
    memcpy(nentp, &nent, sizeof( nent ));
}


int ad_flush( ad, adflags )
    struct adouble	*ad;
    int			adflags;
{
#ifndef USE_MMAPPED_HEADERS
    int len;
#endif

    if (( adflags & ADFLAGS_HF ) && ( ad->ad_hf.adf_flags & O_RDWR )) {
	/* sync our header */
        ad_rebuild_header(ad);

#ifdef USE_MMAPPED_HEADERS
	/* now sync it */
#ifdef MS_SYNC
	msync(ad->ad_data, ad_getentryoff(ad, ADEID_RFORK),
	      MS_SYNC | MS_INVALIDATE);
#else
	msync(ad->ad_data, ad_getentryoff(ad, ADEID_RFORK));
#endif

#else
	if ( ad->ad_hf.adf_off != 0 ) {
	    if ( lseek( ad->ad_hf.adf_fd, 0L, SEEK_SET ) < 0L ) {
		return( -1 );
	    }
	    ad->ad_hf.adf_off = 0;
	}

	/* now flush it out */
	len = ad_getentryoff(ad, ADEID_RFORK);
	if (write( ad->ad_hf.adf_fd, ad->ad_data, len) != len) {
	    if ( errno == 0 ) {
		errno = EIO;
	    }
	    return( -1 );
	}
	ad->ad_hf.adf_off = len;
#endif
    }

    return( 0 );
}

/* use refcounts so that we don't have to re-establish fcntl locks. */
int ad_close( ad, adflags )
    struct adouble	*ad;
    int			adflags;
{
    int			err = 0;

    if (( adflags & ADFLAGS_DF ) && ad->ad_df.adf_fd != -1 &&
	!(--ad->ad_df.adf_refcount)) {
	if ( close( ad->ad_df.adf_fd ) < 0 ) {
	    err = -1;
	}
	ad->ad_df.adf_fd = -1;
	adf_lock_free(&ad->ad_df);
    }

    if (( adflags & ADFLAGS_HF ) && ad->ad_hf.adf_fd != -1 &&
	!(--ad->ad_hf.adf_refcount)) {
#ifdef USE_MMAPPED_HEADERS
        if (ad->ad_data != MAP_FAILED)
	  munmap(ad->ad_data, ad_getentryoff(ad, ADEID_RFORK));
#endif
	if ( close( ad->ad_hf.adf_fd ) < 0 ) {
	    err = -1;
	}
	ad->ad_hf.adf_fd = -1;
	adf_lock_free(&ad->ad_hf);
    }

    return( err );
}
