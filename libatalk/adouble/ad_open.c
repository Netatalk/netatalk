/*
 * $Id: ad_open.c,v 1.19 2002-08-29 18:57:37 didg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>
#include <atalk/logger.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>

#include <netatalk/endian.h>
#include <atalk/adouble.h>

#include "ad_private.h"

#ifndef MAX
#define MAX(a, b)  ((a) < (b) ? (b) : (a))
#endif /* ! MAX */

/*
 * AppleDouble entry default offsets.
 * The layout looks like this:
 *
 * this is the v1 layout:
 *	  255	  200		  16	  32		  N
 *	|  NAME	|    COMMENT	| FILEI	|    FINDERI	| RFORK	|
 *
 * we need to change it to look like this:
 *
 * v2 layout:
 * field       length (in bytes)
 * NAME        255
 * COMMENT     200
 * FILEDATESI  16     replaces FILEI
 * FINDERI     32  
 * DID          4     new
 * AFPFILEI     4     new
 * SHORTNAME   12     8.3 new
 * RFORK        N
 * 
 * so, all we need to do is replace FILEI with FILEDATESI, move RFORK,
 * and add in the new fields.
 *
 * NOTE: the HFS module will need similar modifications to interact with
 * afpd correctly.
 */
 
#define ADEDOFF_MAGIC        (0)
#define ADEDOFF_VERSION      (ADEDOFF_MAGIC + ADEDLEN_MAGIC)
#define ADEDOFF_FILLER       (ADEDOFF_VERSION + ADEDLEN_VERSION)
#define ADEDOFF_NENTRIES     (ADEDOFF_FILLER + ADEDLEN_FILLER)

/* initial lengths of some of the fields */
#define ADEDLEN_INIT     0

/* make sure we don't redefine ADEDOFF_FILEI */
#ifdef ADEDOFF_FILEI
#undef ADEDOFF_FILEI
#endif /* ADEDOFF_FILEI */

#define ADEID_NUM_V1         5
#define ADEDOFF_NAME_V1	     (AD_HEADER_LEN + ADEID_NUM_V1*AD_ENTRY_LEN)
#define ADEDOFF_COMMENT_V1   (ADEDOFF_NAME_V1 + ADEDLEN_NAME)
#define ADEDOFF_FILEI        (ADEDOFF_COMMENT_V1 + ADEDLEN_COMMENT)
#define ADEDOFF_FINDERI_V1   (ADEDOFF_FILEI + ADEDLEN_FILEI)
#define ADEDOFF_RFORK_V1     (ADEDOFF_FINDERI_V1 + ADEDLEN_FINDERI)

/* i stick things in a slightly different order than their eid order in 
 * case i ever want to separate RootInfo behaviour from the rest of the 
 * stuff. */
#define ADEID_NUM_V2         9
#define ADEDOFF_NAME_V2      (AD_HEADER_LEN + ADEID_NUM_V2*AD_ENTRY_LEN)
#define ADEDOFF_COMMENT_V2   (ADEDOFF_NAME_V2 + ADEDLEN_NAME)
#define ADEDOFF_FILEDATESI   (ADEDOFF_COMMENT_V2 + ADEDLEN_COMMENT)
#define ADEDOFF_FINDERI_V2   (ADEDOFF_FILEDATESI + ADEDLEN_FILEDATESI)
#define ADEDOFF_DID	     (ADEDOFF_FINDERI_V2 + ADEDLEN_FINDERI)
#define ADEDOFF_AFPFILEI     (ADEDOFF_DID + ADEDLEN_DID)
#define ADEDOFF_SHORTNAME    (ADEDOFF_AFPFILEI + ADEDLEN_AFPFILEI)
#define ADEDOFF_PRODOSFILEI  (ADEDOFF_SHORTNAME + ADEDLEN_SHORTNAME)
#define ADEDOFF_RFORK_V2     (ADEDOFF_PRODOSFILEI + ADEDLEN_PRODOSFILEI)



/* we keep local copies of a bunch of stuff so that we can initialize things 
 * correctly. */

/* Bits in the finderinfo data. 
 * see etc/afpd/{directory.c,file.c} for the finderinfo structure
 * layout. */
#define FINDERINFO_CUSTOMICON 0x4
#define FINDERINFO_CLOSEDVIEW 0x100

/* offsets in finderinfo */
#define FINDERINFO_FRTYPEOFF   0
#define FINDERINFO_FRCREATOFF  4
#define FINDERINFO_FRFLAGOFF   8
#define FINDERINFO_FRVIEWOFF  14

/* invisible bit for dot files */
#define ATTRBIT_INVISIBLE     (1 << 0)
#define FINDERINFO_INVISIBLE  (1 << 14)

/* this is to prevent changing timezones from causing problems with
   localtime volumes. the screw-up is 30 years. we use a delta of 5
   years.  */
#define TIMEWARP_DELTA 157680000


struct entry {
  u_int32_t id, offset, len;
};

#if AD_VERSION == AD_VERSION1 
static const struct entry entry_order[] = {
  {ADEID_NAME, ADEDOFF_NAME_V1, ADEDLEN_INIT},
  {ADEID_COMMENT, ADEDOFF_COMMENT_V1, ADEDLEN_INIT},
  {ADEID_FILEI, ADEDOFF_FILEI, ADEDLEN_FILEI},
  {ADEID_FINDERI, ADEDOFF_FINDERI_V1, ADEDLEN_FINDERI},
  {ADEID_RFORK, ADEDOFF_RFORK_V1, ADEDLEN_INIT},
  {0, 0, 0}
};
#else /* AD_VERSION == AD_VERSION2 */
static const struct entry entry_order[] = {
  {ADEID_NAME, ADEDOFF_NAME_V2, ADEDLEN_INIT},
  {ADEID_COMMENT, ADEDOFF_COMMENT_V2, ADEDLEN_INIT},
  {ADEID_FILEDATESI, ADEDOFF_FILEDATESI, ADEDLEN_FILEDATESI},
  {ADEID_FINDERI, ADEDOFF_FINDERI_V2, ADEDLEN_FINDERI},
  {ADEID_DID, ADEDOFF_DID, ADEDLEN_DID},
  {ADEID_AFPFILEI, ADEDOFF_AFPFILEI, ADEDLEN_AFPFILEI},
  {ADEID_SHORTNAME, ADEDOFF_SHORTNAME, ADEDLEN_INIT},
  {ADEID_PRODOSFILEI, ADEDOFF_PRODOSFILEI, ADEDLEN_PRODOSFILEI},
  {ADEID_RFORK, ADEDOFF_RFORK_V2, ADEDLEN_INIT},
  {0, 0, 0}
};
#endif /* AD_VERSION == AD_VERSION2 */

#if AD_VERSION == AD_VERSION2


static __inline__ int ad_v1tov2(struct adouble *ad, const char *path)
{
  struct stat st;
  struct timeval tv;
  u_int16_t attr;
  char *buf;
  int fd, off;
  
  /* check to see if we should convert this header. */
  if (!path || (ad->ad_version != AD_VERSION1))
    return 0;

  /* convert from v1 to v2. what does this mean?
   *  1) change FILEI into FILEDATESI
   *  2) create space for SHORTNAME, AFPFILEI, DID, and PRODOSI
   *  3) move FILEI attributes into AFPFILEI
   *  4) initialize ACCESS field of FILEDATESI.
   *
   *  so, we need 4*12 (entry ids) + 12 (shortname) + 4 (afpfilei) +
   *  4 (did) + 8 (prodosi) = 76 more bytes.  */
  
#define SHIFTDATA (AD_DATASZ2 - AD_DATASZ1)

  /* bail if we can't get a lock */
  if (ad_tmplock(ad, ADEID_RFORK, ADLOCK_WR, 0, 0) < 0) 
    goto bail_err;
  
  if ((fd = open(path, O_RDWR)) < 0) 
    goto bail_lock;
  
  if (gettimeofday(&tv, NULL) < 0) 
    goto bail_lock;
  
  if (fstat(fd, &st) ||
      ftruncate(fd, st.st_size + SHIFTDATA) < 0) {
    goto bail_open;
  }
  
  /* last place for failure. */
  if ((void *) (buf = (char *) 
		mmap(NULL, st.st_size + SHIFTDATA,
		     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == 
	  MAP_FAILED) {
    goto bail_truncate;
  }
  
  off = ad->ad_eid[ADEID_RFORK].ade_off;

#ifdef USE_MMAPPED_HEADERS
  /* okay, unmap our old ad header and point it to our local copy */
  munmap(ad->ad_data, off);
  ad->ad_data = buf;
#endif /* USER_MMAPPED_HEADERS */

  /* move the RFORK. this assumes that the RFORK is at the end */
  memmove(buf + off + SHIFTDATA, buf + off, 
	  ad->ad_eid[ADEID_RFORK].ade_len);
  
  /* now, fix up our copy of the header */
  memset(ad->ad_filler, 0, sizeof(ad->ad_filler));
  
  /* replace FILEI with FILEDATESI */
  ad_getattr(ad, &attr);
  ad->ad_eid[ADEID_FILEDATESI].ade_off = ADEDOFF_FILEDATESI;
  ad->ad_eid[ADEID_FILEDATESI].ade_len = ADEDLEN_FILEDATESI;
  ad->ad_eid[ADEID_FILEI].ade_off = 0;
  ad->ad_eid[ADEID_FILEI].ade_len = 0;
  
  /* add in the new entries */
  ad->ad_eid[ADEID_DID].ade_off = ADEDOFF_DID;
  ad->ad_eid[ADEID_DID].ade_len = ADEDLEN_DID;
  ad->ad_eid[ADEID_AFPFILEI].ade_off = ADEDOFF_AFPFILEI;
  ad->ad_eid[ADEID_AFPFILEI].ade_len = ADEDLEN_AFPFILEI;
  ad->ad_eid[ADEID_SHORTNAME].ade_off = ADEDOFF_SHORTNAME;
  ad->ad_eid[ADEID_SHORTNAME].ade_len = ADEDLEN_INIT;
  ad->ad_eid[ADEID_PRODOSFILEI].ade_off = ADEDOFF_PRODOSFILEI;
  ad->ad_eid[ADEID_PRODOSFILEI].ade_len = ADEDLEN_PRODOSFILEI;
  
  /* shift the old entries (NAME, COMMENT, FINDERI, RFORK) */
  ad->ad_eid[ADEID_NAME].ade_off = ADEDOFF_NAME_V2;
  ad->ad_eid[ADEID_COMMENT].ade_off = ADEDOFF_COMMENT_V2;
  ad->ad_eid[ADEID_FINDERI].ade_off = ADEDOFF_FINDERI_V2;
  ad->ad_eid[ADEID_RFORK].ade_off = ADEDOFF_RFORK_V2;
  
  /* switch to v2 */
  ad->ad_version = AD_VERSION2;
  
  /* move our data buffer to make space for the new entries. */
  memmove(buf + ADEDOFF_NAME_V2, buf + ADEDOFF_NAME_V1,
	  ADEDOFF_RFORK_V1 - ADEDOFF_NAME_V1);
  
  /* now, fill in the space with appropriate stuff. we're
     operating as a v2 file now. */
  ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, tv.tv_sec);
  memset(ad_entry(ad, ADEID_DID), 0, ADEDLEN_DID);
  memset(ad_entry(ad, ADEID_AFPFILEI), 0, ADEDLEN_AFPFILEI);
  ad_setattr(ad, attr);
  memset(ad_entry(ad, ADEID_SHORTNAME), 0, ADEDLEN_SHORTNAME);
  memset(ad_entry(ad, ADEID_PRODOSFILEI), 0, ADEDLEN_PRODOSFILEI);
  
  /* rebuild the header and cleanup */
  ad_rebuild_header(ad);
  munmap(buf, st.st_size + SHIFTDATA);
  close(fd);
  ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);

#ifdef USE_MMAPPED_HEADERS
  /* now remap our header */
  ad->ad_data = mmap(NULL, ADEDOFF_RFORK_V2, PROT_READ | PROT_WRITE,
		     (ad_getoflags(ad, ADFLAGS_HF) & O_RDWR) ? MAP_SHARED :
		     MAP_PRIVATE, ad->ad_hf.adf_fd, 0);
  if (ad->ad_data == MAP_FAILED)
    goto bail_err;
#endif /* USE_MMAPPED_HEADERS */

  return 0;
  
bail_truncate:
  ftruncate(fd, st.st_size);
bail_open:
  close(fd);
bail_lock:
  ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);
bail_err:
  return -1;
}
#endif /* AD_VERSION == AD_VERSION2 */


/* read in the entries */
static __inline__ void parse_entries(struct adouble *ad, char *buf,
				    u_int16_t nentries)
{
    u_int32_t		eid, len, off;

    /* now, read in the entry bits */
    for (; nentries > 0; nentries-- ) {
	memcpy(&eid, buf, sizeof( eid ));
	eid = ntohl( eid );
	buf += sizeof( eid );
	memcpy(&off, buf, sizeof( off ));
	off = ntohl( off );
	buf += sizeof( off );
	memcpy(&len, buf, sizeof( len ));
	len = ntohl( len );
	buf += sizeof( len );

	if ( 0 < eid && eid < ADEID_MAX ) {
	    ad->ad_eid[ eid ].ade_off = off;
	    ad->ad_eid[ eid ].ade_len = len;
	} else {
	    LOG(log_debug, logtype_default, "ad_refresh: nentries %hd  eid %d\n",
		    nentries, eid );
	}
    }
}


/* this reads enough of the header so that we can figure out all of
 * the entry lengths and offsets. once that's done, we just read/mmap
 * the rest of the header in.
 *
 * NOTE: we're assuming that the resource fork is kept at the end of
 *       the file. also, mmapping won't work for the hfs fs until it
 *       understands how to mmap header files. */
static __inline__ int ad_header_read(struct adouble *ad)
{
#ifdef USE_MMAPPED_HEADERS
    char                buf[AD_ENTRY_LEN*ADEID_MAX];
#else /* USE_MMAPPED_HEADERS */
    char                *buf = ad->ad_data;
#endif /* USE_MMAPPED_HEADERS */
    u_int16_t           nentries;
    int                 len;
    static int          warning = 0;

    /* read the header */
    if ( ad->ad_hf.adf_off != 0 ) {
      if ( lseek( ad->ad_hf.adf_fd, 0L, SEEK_SET ) < 0L ) {
		return( -1 );
      }
      ad->ad_hf.adf_off = 0;
    }

    if (read( ad->ad_hf.adf_fd, buf, AD_HEADER_LEN) != AD_HEADER_LEN) {
	if ( errno == 0 ) {
	    errno = EIO;
	}
	return( -1 );
    }
    ad->ad_hf.adf_off = AD_HEADER_LEN;

    memcpy(&ad->ad_magic, buf, sizeof( ad->ad_magic ));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, 
	   sizeof( ad->ad_version ));

    /* tag broken v1 headers. just assume they're all right. 
     * we detect two cases: null magic/version
     *                      byte swapped magic/version
     * XXX: in the future, you'll need the v1compat flag. 
     * (ad->ad_flags & ADFLAGS_V1COMPAT) */
    if (!ad->ad_magic && !ad->ad_version) {
      if (!warning) {
	LOG(log_debug, logtype_default, "notice: fixing up null v1 magic/version.");
	warning++;
      }
      ad->ad_magic = AD_MAGIC;
      ad->ad_version = AD_VERSION1;

    } else if ((ad->ad_magic == AD_MAGIC) && 
	       (ad->ad_version == AD_VERSION1)) {
      if (!warning) {
	LOG(log_debug, logtype_default, "notice: fixing up byte-swapped v1 magic/version.");
	warning++;
      }

    } else {
      ad->ad_magic = ntohl( ad->ad_magic );
      ad->ad_version = ntohl( ad->ad_version );
    }

    if ((ad->ad_magic != AD_MAGIC) || ((ad->ad_version != AD_VERSION1)
#if AD_VERSION == AD_VERSION2
				       && (ad->ad_version != AD_VERSION2)
#endif /* AD_VERSION == AD_VERSION2 */
				       )) {
      errno = EIO;
      LOG(log_debug, logtype_default, "ad_open: can't parse AppleDouble header.");
      return -1;
    }

    memcpy(ad->ad_filler, buf + ADEDOFF_FILLER, sizeof( ad->ad_filler ));
    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof( nentries ));
    nentries = ntohs( nentries );

    /* read in all the entry headers. if we have more than the 
     * maximum, just hope that the rfork is specified early on. */
    len = nentries*AD_ENTRY_LEN;
#ifdef USE_MMAPPED_HEADERS
    if (len > sizeof(buf))
      len = sizeof(buf);
#else /* USE_MMAPPED_HEADERS */
    if (len + AD_HEADER_LEN > sizeof(ad->ad_data))
      len = sizeof(ad->ad_data) - AD_HEADER_LEN;
    buf += AD_HEADER_LEN;
#endif /* USE_MMAPPED_HEADERS */
    if (read(ad->ad_hf.adf_fd, buf, len) != len) {
        if (errno == 0)
	    errno = EIO;
	LOG(log_debug, logtype_default, "ad_header_read: can't read entry info.");
	return -1;
    }
    ad->ad_hf.adf_off += len;

    /* figure out all of the entry offsets and lengths. if we aren't
     * able to read a resource fork entry, bail. */
    parse_entries(ad, buf, nentries);
    if (!ad_getentryoff(ad, ADEID_RFORK)
#ifndef USE_MMAPPED_HEADERS
	|| (ad_getentryoff(ad, ADEID_RFORK) > sizeof(ad->ad_data))
#endif /* ! USE_MMAPPED_HEADERS */
	) {
      LOG(log_debug, logtype_default, "ad_header_read: problem with rfork entry offset."); 
      return -1;
    }

    /* read/mmap up to the beginning of the resource fork. */
#ifdef USE_MMAPPED_HEADERS
    ad->ad_data = mmap(NULL, ad_getentryoff(ad, ADEID_RFORK),
		       PROT_READ | PROT_WRITE,
		       (ad_getoflags(ad, ADFLAGS_HF) & O_RDWR) ? MAP_SHARED :
		       MAP_PRIVATE, ad->ad_hf.adf_fd, 0);
    if (ad->ad_data == MAP_FAILED) 
      return -1;
#else /* USE_MMAPPED_HEADERS */
    buf += len;
    len = ad_getentryoff(ad, ADEID_RFORK) - ad->ad_hf.adf_off;
    if (read(ad->ad_hf.adf_fd, buf, len) != len) {
        if (errno == 0)
	    errno = EIO;
	LOG(log_debug, logtype_default, "ad_header_read: can't read in entries.");
	return -1;
    }
#endif /* USE_MMAPPED_HEADERS */
    
    /* fix up broken dates */
    if (ad->ad_version == AD_VERSION1) {
      struct stat st;
      u_int32_t aint;
      
      if (fstat(ad->ad_hf.adf_fd, &st) < 0) {
	return 1; /* fail silently */
      }
      
      /* check to see if the ad date is wrong. just see if we have
      * a modification date in the future. */
      if (((ad_getdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, &aint)) == 0) &&
	  (aint > TIMEWARP_DELTA + st.st_mtime)) {
	ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, aint - AD_DATE_DELTA);
	ad_getdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, &aint);
	ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, aint - AD_DATE_DELTA);
	ad_getdate(ad, AD_DATE_BACKUP | AD_DATE_UNIX, &aint);
	ad_setdate(ad, AD_DATE_BACKUP | AD_DATE_UNIX, aint - AD_DATE_DELTA);
      }
    }

    return 0;
}


/*
 * Put the .AppleDouble where it needs to be:
 *
 *	    /	a/.AppleDouble/b
 *	a/b
 *	    \	b/.AppleDouble/.Parent
 */
char *
ad_path( path, adflags )
    char	*path;
    int		adflags;
{
    static char	pathbuf[ MAXPATHLEN + 1];
    char	c, *slash, buf[MAXPATHLEN + 1];

    strncpy(buf, path, MAXPATHLEN);
    if ( adflags & ADFLAGS_DIR ) {
	strncpy( pathbuf, buf, MAXPATHLEN );
	if ( *buf != '\0' ) {
	    strcat( pathbuf, "/" );
	}
	slash = ".Parent";
    } else {
	if (( slash = strrchr( buf, '/' )) != NULL ) {
	    c = *++slash;
	    *slash = '\0';
	    strncpy( pathbuf, buf, MAXPATHLEN);
	    *slash = c;
	} else {
	    pathbuf[ 0 ] = '\0';
	    slash = buf;
	}
    }
    strncat( pathbuf, ".AppleDouble/", MAXPATHLEN - strlen(pathbuf));
    strncat( pathbuf, slash, MAXPATHLEN - strlen(pathbuf));

    return( pathbuf );
}

/*
 * Support inherited protection modes for AppleDouble files.  The supplied
 * mode is ANDed with the parent directory's mask value in lieu of "umask",
 * and that value is returned.
 */

#define DEFMASK 07700	/* be conservative */

char 
*ad_dir(path)
    char		*path;
{
    static char		modebuf[ MAXPATHLEN + 1];
    char 		*slash;

    if ( strlen( path ) >= MAXPATHLEN ) {
	return NULL;  /* can't do it */
    }

    /*
     * For a path with directories in it, remove the final component
     * (path or subdirectory name) to get the name we want to stat.
     * For a path which is just a filename, use "." instead.
     */
    strcpy( modebuf, path );
    if (( slash = strrchr( modebuf, '/' )) != NULL ) {
	*slash = '\0';		/* remove pathname component */
    } else {
	modebuf[0] = '.';	/* use current directory */
	modebuf[1] = '\0';
    }
    return modebuf;
}

int
ad_mode( path, mode )
    char		*path;
    int			mode;
{
    struct stat		stbuf;
    char                *p;
    
    if ( mode == 0 ) {
	return( mode );		/* save on syscalls */
    }
    p = ad_dir(path);
    if (!p) {
	return( mode & DEFMASK );  /* can't do it */
    }

    if ( stat( p, &stbuf ) != 0 ) {
	return( mode & DEFMASK );	/* bail out... can't stat dir? */
    }

    return( mode & stbuf.st_mode );
}

/*
 * Use mkdir() with mode bits taken from ad_mode().
 */
int
ad_mkdir( path, mode )
    char		*path;
    int			mode;
{
#ifdef DEBUG
    LOG(log_info, logtype_default, "ad_mkdir: Creating directory with mode %d", mode);
#endif /* DEBUG */
    return mkdir( path, ad_mode( path, mode ) );
}


/*
 * It's not possible to open the header file O_RDONLY -- the read
 * will fail and return an error. this refcounts things now. 
 */
int ad_open( path, adflags, oflags, mode, ad )
    char		*path;
    int			adflags, oflags, mode;
    struct adouble	*ad;
{
    const struct entry  *eid;
    struct stat         st;
    char		*slash, *ad_p;
    int			hoflags, admode;
    u_int16_t           ashort;

    if (ad->ad_inited != AD_INITED) {
        ad_dfileno(ad) = -1;
        ad_hfileno(ad) = -1;
	adf_lock_init(&ad->ad_df);
	adf_lock_init(&ad->ad_hf);
#ifdef USE_MMAPPED_HEADERS
	ad->ad_data = MAP_FAILED;
#endif /* USE_MMAPPED_HEADERS */
        ad->ad_inited = AD_INITED;
        ad->ad_refcount = 1;
    }

    if (adflags & ADFLAGS_DF) { 
        if (ad_dfileno(ad) == -1) {
	  hoflags = (oflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
	  admode = ad_mode( path, mode ); 
	  if (( ad->ad_df.adf_fd = open( path, hoflags, admode )) < 0 ) {
             if (errno == EACCES && !(oflags & O_RDWR)) {
                hoflags = oflags;
                ad->ad_df.adf_fd =open( path, hoflags, admode );
             }
	  }
	  if ( ad->ad_df.adf_fd < 0)
	  	return -1;	
	  ad->ad_df.adf_off = 0;
	  ad->ad_df.adf_flags = hoflags;
	} 
        else {
            /* the file is already open... but */
            if ((oflags & ( O_RDWR | O_WRONLY)) &&             /* we want write access */
            	!(ad->ad_df.adf_flags & ( O_RDWR | O_WRONLY))) /* and it was denied the first time */
            {
                 errno == EACCES;
                 return -1;
            }
	}
	ad->ad_df.adf_refcount++;
    }

    if (adflags & ADFLAGS_HF) {
        if (ad_hfileno(ad) == -1) {
	  ad_p = ad_path( path, adflags );

	  hoflags = oflags & ~O_CREAT;
	  hoflags = (hoflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
	  if (( ad->ad_hf.adf_fd = open( ad_p, hoflags, 0 )) < 0 ) {
            if (errno == EACCES && !(oflags & O_RDWR)) {
                hoflags = oflags & ~O_CREAT;
                ad->ad_hf.adf_fd = open( ad_p, hoflags, 0 );
            }    
          }
	  if ( ad->ad_hf.adf_fd < 0 ) {
	    if (errno == ENOENT && (oflags & O_CREAT) ) {
	      /*
	       * We're expecting to create a new adouble header file,
	       * here.
	       * if ((oflags & O_CREAT) ==> (oflags & O_RDWR)
	       */
	      admode = ad_hf_mode(ad_mode( ad_p, mode )); 
	      errno = 0;
	      if (( ad->ad_hf.adf_fd = open( ad_p, oflags,
					     admode )) < 0 ) {
		/*
		 * Probably .AppleDouble doesn't exist, try to
		 * mkdir it.
		 */
		if ((errno == ENOENT) && 
		    ((adflags & ADFLAGS_NOADOUBLE) == 0)) {
		  if (( slash = strrchr( ad_p, '/' )) == NULL ) {
		    ad_close( ad, adflags );
		    return( -1 );
		  }
		  *slash = '\0';
		  errno = 0;
		  if ( ad_mkdir( ad_p, 0777 ) < 0 ) {
		    ad_close( ad, adflags );
		    return( -1 );
		  }
		  *slash = '/';
		  if (( ad->ad_hf.adf_fd = 
			open( ad_p, oflags, ad_mode( ad_p, mode) )) < 0 ) {
		    ad_close( ad, adflags );
		    return( -1 );
		  }
		} else {
		  ad_close( ad, adflags );
		  return( -1 );
		}
	      }
	      ad->ad_hf.adf_flags = oflags;
	    } else {
	      ad_close( ad, adflags );
	      return( -1 );
	    }
	  } else if ((fstat(ad->ad_hf.adf_fd, &st) == 0) && 
		     (st.st_size == 0)) {
	    /* for 0 length files, treat them as new. */
	    ad->ad_hf.adf_flags = (oflags & ~O_RDONLY) | O_RDWR;
	  } else {
	    ad->ad_hf.adf_flags = hoflags;
	  }
	  ad->ad_hf.adf_off = 0;
	  
	  /*
	   * This is a new adouble header file. Initialize the structure,
	   * instead of reading it.
	   */
	  memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
	  if ( ad->ad_hf.adf_flags & ( O_TRUNC | O_CREAT )) {
	    struct timeval tv;

	    ad->ad_magic = AD_MAGIC;
	    ad->ad_version = AD_VERSION;
	    memset(ad->ad_filler, 0, sizeof( ad->ad_filler ));

#ifdef USE_MMAPPED_HEADERS
	    /* truncate the header file and mmap it. */
	    ftruncate(ad->ad_hf.adf_fd, AD_DATASZ);
	    ad->ad_data = mmap(NULL, AD_DATASZ, PROT_READ | PROT_WRITE,
			       MAP_SHARED, ad->ad_hf.adf_fd, 0);
	    if (ad->ad_data == MAP_FAILED) {
	      ad_close(ad, adflags);
	      return -1;
	    }	    
#else /* USE_MMAPPED_HEADERS */
	    memset(ad->ad_data, 0, sizeof(ad->ad_data));
#endif /* USE_MMAPPED_HEADERS */

	    eid = entry_order;
	    while (eid->id) {
	      ad->ad_eid[eid->id].ade_off = eid->offset;
	      ad->ad_eid[eid->id].ade_len = eid->len;
	      eid++;
	    }
	    
	    /* put something sane in the directory finderinfo */
	    if (adflags & ADFLAGS_DIR) {
	      /* set default view */
	      ashort = htons(FINDERINFO_CLOSEDVIEW);
	      memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRVIEWOFF, 
		     &ashort, sizeof(ashort));
	    } else {
	      /* set default creator/type fields */
	      memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRTYPEOFF,
		     "TEXT", 4);
	      memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRCREATOFF,
		     "UNIX", 4);
	    }

	    /* make things invisible */
	    if ((*path == '.') && strcmp(path, ".") && strcmp(path, "..")) {
	      ashort = htons(ATTRBIT_INVISIBLE);
	      ad_setattr(ad, ashort);
	      ashort = htons(FINDERINFO_INVISIBLE);
	      memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF,
		     &ashort, sizeof(ashort));
	    }

	    if (gettimeofday(&tv, NULL) < 0) {
	      ad_close(ad, adflags);
	      return -1;
	    }
	    
	    /* put something sane in the date fields */
	    ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, tv.tv_sec);
	    ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, tv.tv_sec);
	    ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, tv.tv_sec);
	    ad_setdate(ad, AD_DATE_BACKUP, AD_DATE_START);

	  } else {
	    /*
	     * Read the adouble header in and parse it.
	     */
	    if ((ad_header_read( ad ) < 0)
#if AD_VERSION == AD_VERSION2
		|| (ad_v1tov2(ad, ad_p) < 0)
#endif /* AD_VERSION == AD_VERSION2 */
		) {
	      ad_close( ad, adflags );
	      return( -1 );
	    }
	  }
	}
	else { /* already open */
            if ((oflags & ( O_RDWR | O_WRONLY)) &&             
            	!(ad->ad_hf.adf_flags & ( O_RDWR | O_WRONLY))) {
		if (adflags & ADFLAGS_DF) {
              	    /* don't call with ADFLAGS_HF because we didn't open ressource fork */
	            ad_close( ad, ADFLAGS_DF );
	         }
                 errno == EACCES;
	         return -1;
	    }
	}
	ad->ad_hf.adf_refcount++;
    }
	
    return( 0 );
}

/* to do this with mmap, we need the hfs fs to understand how to mmap
   header files. */
int ad_refresh(struct adouble *ad)
{
#ifdef USE_MMAPPED_HEADERS
  off_t off;
#endif /* USE_MMAPPED_HEADERS */

  if (ad->ad_hf.adf_fd < -1)
    return -1;

#ifdef USE_MMAPPED_HEADERS
  if (ad->ad_data == MAP_FAILED)
    return -1;
  
  /* re-read the header */
  off = ad_getentryoff(ad, ADEID_RFORK);
  memcpy(&nentries, ad->ad_data + ADEDOFF_NENTRIES, sizeof(nentries));
  nentries = ntohs(nentries);
  parse_entries(ad, ad->ad_data + AD_HEADER_LEN, nentries);

  /* check to see if something screwy happened */
  if (!ad_getentryoff(ad, ADEID_RFORK))
    return -1;

  /* if there's a length discrepancy, remap the header. this shouldn't
   * really ever happen. */
  if (off != ad_getentryoff(ad, ADEID_RFORK)) {
    char *buf = ad->ad_data;
    buf = ad->ad_data;
    ad->ad_data = mmap(NULL, ad_getentryoff(ad, ADEID_RFORK), 
		       PROT_READ | PROT_WRITE,
		       (ad_getoflags(ad, ADFLAGS_HF) & O_RDWR) ? 
		       MAP_SHARED : MAP_PRIVATE, ad->ad_hf.adf_fd, 0);
    if (ad->ad_data == MAP_FAILED) {
      ad->ad_data = buf;
      return -1;
    }
    munmap(buf, off);
  }
  return 0;

#else /* USE_MMAPPED_HEADERS */
  return ad_header_read(ad);
#endif /* USE_MMAPPED_HEADERS */
}
