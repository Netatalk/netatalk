/*
 * $Id: ad_read.c,v 1.3 2001-06-29 14:14:46 rufustfirefly Exp $
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
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <atalk/adouble.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif /* ! MIN */

/* XXX: this would probably benefit from pread. 
 *      locks have to be checked before each stream of consecutive 
 *      ad_reads to prevent a denial in the middle from causing
 *      problems. */
ssize_t ad_read( ad, eid, off, buf, buflen)
    struct adouble	*ad;
    const u_int32_t 	eid;
    off_t               off;
    char		*buf;
    const size_t	buflen;
{
    ssize_t		cc;

    /* We're either reading the data fork (and thus the data file)
     * or we're reading anything else (and thus the header file). */
    if ( eid == ADEID_DFORK ) {
	if ( ad->ad_df.adf_off != off ) {
	    if ( lseek( ad->ad_df.adf_fd, (off_t) off, SEEK_SET ) < 0 ) {
		return( -1 );
	    }
	    ad->ad_df.adf_off = off;
	}
	if (( cc = read( ad->ad_df.adf_fd, buf, buflen )) < 0 ) {
	    return( -1 );
	}
	ad->ad_df.adf_off += cc;
    } else {
        cc = ad->ad_eid[eid].ade_off + off;

#ifdef USE_MMAPPED_HEADERS
	if (eid != ADEID_RFORK) {
	  memcpy(buf, ad->ad_data + cc, buflen);
	  cc = buflen;
	  goto ad_read_done;
	}
#endif /* USE_MMAPPED_HEADERS */	
	if ( ad->ad_hf.adf_off != cc ) {
	  if ( lseek( ad->ad_hf.adf_fd, (off_t) cc, SEEK_SET ) < 0 ) {
	      return( -1 );
	  }
	  ad->ad_hf.adf_off = cc;
	}
	  
	if (( cc = read( ad->ad_hf.adf_fd, buf, buflen )) < 0 ) {
	  return( -1 );
	}
	  
#ifndef USE_MMAPPED_HEADERS
	/*
	 * We've just read in bytes from the disk that we read earlier
	 * into ad_data. If we're going to write this buffer out later,
	 * we need to update ad_data.
	 */
	if (ad->ad_hf.adf_off < ad_getentryoff(ad, ADEID_RFORK)) {
	    if ( ad->ad_hf.adf_flags & O_RDWR ) {
	      memcpy(buf, ad->ad_data + ad->ad_hf.adf_off,
		     MIN(sizeof( ad->ad_data ) - ad->ad_hf.adf_off, cc));
	    } else {
	      memcpy(ad->ad_data + ad->ad_hf.adf_off, buf,
		     MIN(sizeof( ad->ad_data ) - ad->ad_hf.adf_off, cc));
	    }
	}
	ad->ad_hf.adf_off += cc;
#else /* ! USE_MMAPPED_HEADERS */
ad_read_done:
#endif /* ! USE_MMAPPED_HEADERS */
    }

    return( cc );
}
