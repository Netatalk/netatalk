/*
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <atalk/adouble.h>

#ifndef MIN
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif

/* XXX: this would benefit from pwrite. 
 *      locking has to be checked before each stream of consecutive
 *      ad_writes to prevent a lock in the middle from causing problems. 
 */
ssize_t ad_write( ad, eid, off, end, buf, buflen )
    struct adouble	*ad;
    const u_int32_t	eid;
    off_t               off;
    const int		end;
    const char		*buf;
    const size_t	buflen;
{
    struct stat		st;
    ssize_t		cc;

    if ( eid == ADEID_DFORK ) {
	if ( end ) {
	    if ( fstat( ad->ad_df.adf_fd, &st ) < 0 ) {
		return( -1 );
	    }
	    off = st.st_size - off;
	}

	if ( ad->ad_df.adf_off != off ) {
	    if ( lseek( ad->ad_df.adf_fd, (off_t) off, SEEK_SET ) < 0 ) {
		return( -1 );
	    }
	    ad->ad_df.adf_off = off;
	}
	cc = write( ad->ad_df.adf_fd, buf, buflen );
	if ( cc < 0 ) {
	    return( -1 );
	}
	ad->ad_df.adf_off += cc;
    } else {
	if ( end ) {
	    off = ad->ad_eid[ eid ].ade_len - off;
	}
	cc = ad->ad_eid[eid].ade_off + off;

#ifdef USE_MMAPPED_HEADERS
	if (eid != ADEID_RFORK) {
	  memcpy(ad->ad_data + cc, buf, buflen);
	  cc = buflen;
	  goto ad_write_done;
	}	  
#endif

	if ( ad->ad_hf.adf_off != cc ) {
	  if ( lseek( ad->ad_hf.adf_fd, (off_t) cc, SEEK_SET ) < 0 ) {
	      return( -1 );
	  }
	  ad->ad_hf.adf_off = cc;
	}
	  
	if ((cc = write( ad->ad_hf.adf_fd, buf, buflen )) < 0)
	  return( -1 );
	ad->ad_hf.adf_off += cc;
	
#ifndef USE_MMAPPED_HEADERS
	/* sync up our internal buffer */
	if (ad->ad_hf.adf_off < ad_getentryoff(ad, ADEID_RFORK))
	  memcpy(ad->ad_data + ad->ad_hf.adf_off, buf,
		 MIN(sizeof(ad->ad_data) - ad->ad_hf.adf_off, cc));
#else	  
ad_write_done:
#endif
	  if ( ad->ad_eid[ eid ].ade_len < off + cc ) {
	    ad->ad_eid[ eid ].ade_len = off + cc;
	  }
    }

    return( cc );
}

/* set locks here */
int ad_rtruncate( ad, size )
    struct adouble	*ad;
    const size_t	size;
{
    int err;

    if (ad_tmplock(ad, ADEID_RFORK, ADLOCK_WR, 0, 0) < 0)
      return -2;

    if ( ftruncate( ad->ad_hf.adf_fd,
	    size + ad->ad_eid[ ADEID_RFORK ].ade_off ) < 0 ) {
        err = errno;
        ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);
	errno = err;
	return( -1 );
    }

    ad->ad_eid[ ADEID_RFORK ].ade_len = size;
    if ( lseek( ad->ad_hf.adf_fd, ad->ad_eid[ADEID_RFORK].ade_off, 
		SEEK_SET ) < 0 ) {
        err = errno;
        ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);
	errno = err;
	return( -1 );
    }

    ad->ad_hf.adf_off = ad->ad_eid[ADEID_RFORK].ade_off;
    ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);
    return( 0 );
}

int ad_dtruncate(ad, size)
    struct adouble	*ad;
    const size_t	size;
{
    int err;

    if (ad_tmplock(ad, ADEID_DFORK, ADLOCK_WR, 0, 0) < 0)
      return -2;

    if (ftruncate(ad->ad_df.adf_fd, size) < 0) {
      err = errno;
      ad_tmplock(ad, ADEID_DFORK, ADLOCK_CLR, 0, 0);
      errno = err;
    } else 
      ad_tmplock(ad, ADEID_DFORK, ADLOCK_CLR, 0, 0);

    return 0;
}
