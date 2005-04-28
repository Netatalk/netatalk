/*
 * $Id: ad_flush.c,v 1.7 2005-04-28 20:49:52 bfernhomberg Exp $
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
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <atalk/adouble.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ad_private.h"
#if AD_VERSION == AD_VERSION1 

#define EID_DISK(a) (a)

#else

static const u_int32_t set_eid[] = {
0,1,2,3,4,5,6,7,8,
9,10,11,12,13,14,15,
AD_DEV, AD_INO, AD_SYN, AD_ID
};

#define EID_DISK(a) (set_eid[a])
#endif

/* rebuild the header */
void ad_rebuild_header(struct adouble *ad)
{
    u_int32_t		eid;
    u_int32_t 		temp;
    
    u_int16_t		nent;
    char		*buf, *nentp;

    /*
     * Rebuild any header information that might have changed.
     */
    buf = ad->ad_data;

    temp = htonl( ad->ad_magic );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );
    
    temp = htonl( ad->ad_version );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );

    memcpy(buf, ad->ad_filler, sizeof( ad->ad_filler ));
    buf += sizeof( ad->ad_filler );
    
    nentp = buf;
    buf += sizeof( nent );
    for ( eid = 0, nent = 0; eid < ADEID_MAX; eid++ ) {
      if ( ad->ad_eid[ eid ].ade_off == 0 ) {
	continue;
      }
      temp = htonl( EID_DISK(eid) );
      memcpy(buf, &temp, sizeof( temp ));
      buf += sizeof( temp );

      temp = htonl( ad->ad_eid[ eid ].ade_off );
      memcpy(buf, &temp, sizeof( temp ));
      buf += sizeof( temp );

      temp = htonl( ad->ad_eid[ eid ].ade_len );
      memcpy(buf, &temp, sizeof( temp ));
      buf += sizeof( temp );
      nent++;
    }
    nent = htons( nent );
    memcpy(nentp, &nent, sizeof( nent ));
}


int ad_flush( ad, adflags )
    struct adouble	*ad;
    int			adflags;
{
    int len;

    if (( adflags & ADFLAGS_HF ) && ( ad->ad_hf.adf_flags & O_RDWR )) {
	/* sync our header */
        if (ad->ad_rlen > 0xffffffff) {
            ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
        }
        else {
            ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
        }
        ad_rebuild_header(ad);
	len = ad_getentryoff(ad, ADEID_RFORK);
	/* now flush it out */
        if (adf_pwrite(&ad->ad_hf, ad->ad_data, len, 0) != len) {
	    if ( errno == 0 ) {
		errno = EIO;
	    }
	    return( -1 );
	}
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
	if ( close( ad->ad_hf.adf_fd ) < 0 ) {
	    err = -1;
	}
	ad->ad_hf.adf_fd = -1;
	adf_lock_free(&ad->ad_hf);
    }

    return( err );
}
