/*
 * $Id: ad_open.c,v 1.31 2003-06-06 20:46:38 srittau Exp $
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
 * 
 * NOTE: I don't use inline because a good compiler should be
 * able to optimize all the static below. Didier
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
  if (ad_tmplock(ad, ADEID_RFORK, ADLOCK_WR, 0, 0, 0) < 0) 
    goto bail_err;
  
  if ((fd = open(path, O_RDWR)) < 0) 
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

  /* move the RFORK. this assumes that the RFORK is at the end */
  memmove(buf + off + SHIFTDATA, buf + off, 
	  ad->ad_eid[ADEID_RFORK].ade_len);
  
  munmap(buf, st.st_size + SHIFTDATA);
  close(fd);

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
  memmove(ad->ad_data + ADEDOFF_NAME_V2, ad->ad_data + ADEDOFF_NAME_V1,
	  ADEDOFF_RFORK_V1 - ADEDOFF_NAME_V1);
  
  /* now, fill in the space with appropriate stuff. we're
     operating as a v2 file now. */
  ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, st.st_mtime);
  memset(ad_entry(ad, ADEID_DID), 0, ADEDLEN_DID);
  memset(ad_entry(ad, ADEID_AFPFILEI), 0, ADEDLEN_AFPFILEI);
  ad_setattr(ad, attr);
  memset(ad_entry(ad, ADEID_SHORTNAME), 0, ADEDLEN_SHORTNAME);
  memset(ad_entry(ad, ADEID_PRODOSFILEI), 0, ADEDLEN_PRODOSFILEI);
  
  /* rebuild the header and cleanup */
  ad_flush(ad, ADFLAGS_HF );
  ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0, 0);

  return 0;
  
bail_truncate:
  ftruncate(fd, st.st_size);
bail_open:
  close(fd);
bail_lock:
  ad_tmplock(ad, ADEID_RFORK, ADLOCK_CLR, 0, 0, 0);
bail_err:
  return -1;
}
#endif /* AD_VERSION == AD_VERSION2 */

mode_t ad_hf_mode (mode_t mode)
{
    /* fnctl lock need write access */
    if ((mode & S_IRUSR))
        mode |= S_IWUSR;
    if ((mode & S_IRGRP))
        mode |= S_IWGRP;
    if ((mode & S_IROTH))
        mode |= S_IWOTH;
    /* if write mode set add read mode */
    if ((mode & S_IWUSR))
        mode |= S_IRUSR;
    if ((mode & S_IWGRP))
        mode |= S_IRGRP;
    if ((mode & S_IWOTH))
        mode |= S_IROTH;

    return mode;
}

/* ------------------------------------- 
  read in the entries 
*/
static void parse_entries(struct adouble *ad, char *buf,
				    u_int16_t nentries)
{
    u_int32_t	eid, len, off;
    int         warning = 0;

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
	} else if (!warning) {
	    warning = 1;
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
static int ad_header_read(struct adouble *ad, struct stat *hst)
{
    char                *buf = ad->ad_data;
    u_int16_t           nentries;
    int                 len;
    ssize_t             header_len;
    static int          warning = 0;
    struct stat         st;

    /* read the header */
    if ((header_len = adf_pread( &ad->ad_hf, buf, sizeof(ad->ad_data), 0)) < 0) {
	return -1;
    }
    if (header_len < AD_HEADER_LEN) {
        errno = EIO;
        return -1;
    }

    memcpy(&ad->ad_magic, buf, sizeof( ad->ad_magic ));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, sizeof( ad->ad_version ));

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

    } else if (ad->ad_magic == AD_MAGIC && ad->ad_version == AD_VERSION1) {
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

    if (len + AD_HEADER_LEN > sizeof(ad->ad_data))
      len = sizeof(ad->ad_data) - AD_HEADER_LEN;

    buf += AD_HEADER_LEN;
    if (len > header_len - AD_HEADER_LEN) {
        errno = EIO;
	LOG(log_debug, logtype_default, "ad_header_read: can't read entry info.");
	return -1;
    }

    /* figure out all of the entry offsets and lengths. if we aren't
     * able to read a resource fork entry, bail. */
    nentries = len / AD_ENTRY_LEN;
    parse_entries(ad, buf, nentries);
    if (!ad_getentryoff(ad, ADEID_RFORK)
	|| (ad_getentryoff(ad, ADEID_RFORK) > sizeof(ad->ad_data))
	) {
      LOG(log_debug, logtype_default, "ad_header_read: problem with rfork entry offset."); 
      return -1;
    }

    if (ad_getentryoff(ad, ADEID_RFORK) > header_len) {
	errno = EIO;
	LOG(log_debug, logtype_default, "ad_header_read: can't read in entries.");
	return -1;
    }
    
    if (hst == NULL) {
        hst = &st;
        if (fstat(ad->ad_hf.adf_fd, &st) < 0) {
	    return 1; /* fail silently */
	 }
    }
    ad->ad_rlen = hst->st_size - ad_getentryoff(ad, ADEID_RFORK);

    /* fix up broken dates */
    if (ad->ad_version == AD_VERSION1) {
      u_int32_t aint;
      
      /* check to see if the ad date is wrong. just see if we have
      * a modification date in the future. */
      if (((ad_getdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, &aint)) == 0) &&
	  (aint > TIMEWARP_DELTA + hst->st_mtime)) {
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
    const char	*path;
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
	if (NULL != ( slash = strrchr( buf, '/' )) ) {
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
    const char		*path;
{
    static char		modebuf[ MAXPATHLEN + 1];
    char 		*slash;

    if ( strlen( path ) >= MAXPATHLEN ) {
        errno = ENAMETOOLONG;
	return NULL;  /* can't do it */
    }

    /*
     * For a path with directories in it, remove the final component
     * (path or subdirectory name) to get the name we want to stat.
     * For a path which is just a filename, use "." instead.
     */
    strcpy( modebuf, path );
    if (NULL != ( slash = strrchr( modebuf, '/' )) ) {
	*slash = '\0';		/* remove pathname component */
    } else {
	modebuf[0] = '.';	/* use current directory */
	modebuf[1] = '\0';
    }
    return modebuf;
}

/* ---------------- */
static uid_t default_uid = -1;

int ad_setfuid(const uid_t id)
{
    default_uid = id;
    return 0;
}

/* ---------------- */
uid_t ad_getfuid(void) 
{
    return default_uid;
}

/* ---------------- 
   return inode of path parent directory
*/
static int ad_stat(const char *path, struct stat *stbuf)
{
    char                *p;

    p = ad_dir(path);
    if (!p) {
        return -1;
    }

    return stat( p, stbuf );
}

/* ---------------- 
   if we are root change path user/ group
   It can be a native function for BSD cf. FAQ.Q10
   path:  pathname to chown 
   stbuf: parent directory inode
   
   use fstat and fchown or lchown with linux?
*/
#define EMULATE_SUIDDIR
 
static int ad_chown(const char *path, struct stat *stbuf)
{
int ret = 0;
#ifdef EMULATE_SUIDDIR
uid_t id;

    if (default_uid != -1) {  
        /* we are root (admin) */
        id = (default_uid)?default_uid:stbuf->st_uid;
	ret = chown( path, id, stbuf->st_gid );
    }
#endif    
    return ret;
}

/* ---------------- 
   return access right and inode of path parent directory
*/
static int ad_mode_st(const char *path, int *mode, struct stat *stbuf)
{
    if (*mode == 0) {
       return -1;
    }
    if (ad_stat(path, stbuf) != 0) {
	*mode &= DEFMASK;
	return -1;
    }
    *mode &= stbuf->st_mode;
    return 0;    
}

/* ---------------- 
   return access right of path parent directory
*/
int
ad_mode( path, mode )
    const char		*path;
    int			mode;
{
    struct stat		stbuf;
    ad_mode_st(path, &mode, &stbuf);
    return mode;
}

/*
 * Use mkdir() with mode bits taken from ad_mode().
 */
int
ad_mkdir( path, mode )
    const char		*path;
    int			mode;
{
int ret;
int st_invalid;
struct stat stbuf;

#ifdef DEBUG
    LOG(log_info, logtype_default, "ad_mkdir: Creating directory with mode %d", mode);
#endif /* DEBUG */

    st_invalid = ad_mode_st(path, &mode, &stbuf);
    ret = mkdir( path, mode );
    if (ret || st_invalid)
    	return ret;
    ad_chown(path, &stbuf);

    return ret;    
}

/* ----------------- */
static int ad_error(struct adouble *ad, int adflags)
{
    if ((adflags & ADFLAGS_NOHF)) {
        /* FIXME double check : set header offset ?*/
        return 0;
    }
    if ((adflags & ADFLAGS_DF)) {
	ad_close( ad, ADFLAGS_DF );
    }
    return -1 ;
}

static int new_rfork(const char *path, struct adouble *ad, int adflags);

#ifdef  HAVE_PREAD
#define AD_SET(a) 
#else 
#define AD_SET(a) a = 0
#endif
/*
 * It's not possible to open the header file O_RDONLY -- the read
 * will fail and return an error. this refcounts things now. 
 */
int ad_open( path, adflags, oflags, mode, ad )
    const char		*path;
    int			adflags, oflags, mode;
    struct adouble	*ad;
{
    struct stat         st;
    char		*slash, *ad_p;
    int			hoflags, admode;
    int                 st_invalid;
    int                 open_df = 0;
    
    if (ad->ad_inited != AD_INITED) {
        ad_dfileno(ad) = -1;
        ad_hfileno(ad) = -1;
	adf_lock_init(&ad->ad_df);
	adf_lock_init(&ad->ad_hf);
        ad->ad_inited = AD_INITED;
        ad->ad_refcount = 1;
    }

    if ((adflags & ADFLAGS_DF)) { 
        if (ad_dfileno(ad) == -1) {
	  hoflags = (oflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
	  admode = mode;
	  st_invalid = ad_mode_st(path, &admode, &st);
          ad->ad_df.adf_fd =open( path, hoflags, admode );
	  if (ad->ad_df.adf_fd < 0 ) {
             if ((errno == EACCES || errno == EROFS) && !(oflags & O_RDWR)) {
                hoflags = oflags;
                ad->ad_df.adf_fd =open( path, hoflags, admode );
             }
	  }
	  if ( ad->ad_df.adf_fd < 0)
	  	return -1;

	  AD_SET(ad->ad_df.adf_off);
	  ad->ad_df.adf_flags = hoflags;
	  if ((oflags & O_CREAT) && !st_invalid) {
	      /* just created, set owner if admin (root) */
	      ad_chown(path, &st);
	  }
	} 
        else {
            /* the file is already open... but */
            if ((oflags & ( O_RDWR | O_WRONLY)) &&             /* we want write access */
            	!(ad->ad_df.adf_flags & ( O_RDWR | O_WRONLY))) /* and it was denied the first time */
            {
                 errno = EACCES;
                 return -1;
            }
	    /* FIXME 
	     * for now ad_open is never called with O_TRUNC or O_EXCL if the file is
	     * already open. Should we check for it? ie
	     * O_EXCL --> error 
	     * O_TRUNC --> truncate the fork.
	     * idem for ressource fork.
	     */
	}
	open_df = ADFLAGS_DF;
	ad->ad_df.adf_refcount++;
    }

    if (!(adflags & ADFLAGS_HF)) 
        return 0;
        
    if (ad_hfileno(ad) != -1) { /* the file is already open */
        if ((oflags & ( O_RDWR | O_WRONLY)) &&             
            	!(ad->ad_hf.adf_flags & ( O_RDWR | O_WRONLY))) {
	    if (open_df) {
                /* don't call with ADFLAGS_HF because we didn't open ressource fork */
	        ad_close( ad, open_df );
	    }
            errno = EACCES;
	    return -1;
	}
	ad_refresh(ad);
	ad->ad_hf.adf_refcount++;
	return 0;
    }

    ad_p = ad_path( path, adflags );

    hoflags = oflags & ~O_CREAT;
    hoflags = (hoflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
    ad->ad_hf.adf_fd = open( ad_p, hoflags, 0 );
    if (ad->ad_hf.adf_fd < 0 ) {
        if ((errno == EACCES || errno == EROFS) && !(oflags & O_RDWR)) {
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
	    admode = mode;
	    st_invalid = ad_mode_st(ad_p, &admode, &st);
	    admode = ad_hf_mode(admode); 
	    errno = 0;
	    ad->ad_hf.adf_fd = open( ad_p, oflags,admode );
	    if ( ad->ad_hf.adf_fd < 0 ) {
		/*
		 * Probably .AppleDouble doesn't exist, try to
		 * mkdir it.
		 */
		if (errno == ENOENT && (adflags & ADFLAGS_NOADOUBLE) == 0) {
		    if (NULL == ( slash = strrchr( ad_p, '/' )) ) {
		        return ad_error(ad, adflags);
		    }
		    *slash = '\0';
		    errno = 0;
		    if ( ad_mkdir( ad_p, 0777 ) < 0 ) {
		        return ad_error(ad, adflags);
		    }
		    *slash = '/';
		    admode = mode;
		    st_invalid = ad_mode_st(ad_p, &admode, &st);
		    admode = ad_hf_mode(admode); 
		    ad->ad_hf.adf_fd = open( ad_p, oflags, admode);
		    if ( ad->ad_hf.adf_fd < 0 ) {
		        return ad_error(ad, adflags);
		    }
		} else {
		     return ad_error(ad, adflags);
		}
	    }
	    ad->ad_hf.adf_flags = oflags;
	    /* just created, set owner if admin owner (root) */
	    if (!st_invalid) {
	        ad_chown(path, &st);
	    }
	}
	else {
	    return ad_error(ad, adflags);
	}
    } else if (fstat(ad->ad_hf.adf_fd, &st) == 0 && st.st_size == 0) {
	/* for 0 length files, treat them as new. */
	ad->ad_hf.adf_flags = (oflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR | O_TRUNC;
    } else {
        ad->ad_hf.adf_flags = hoflags;
    }
    AD_SET(ad->ad_hf.adf_off);
	  
    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_hf.adf_refcount++;
    if ((ad->ad_hf.adf_flags & ( O_TRUNC | O_CREAT ))) {
        /*
         * This is a new adouble header file. Initialize the structure,
         * instead of reading it.
        */
        if (new_rfork(path, ad, adflags) < 0) {
            /* the file is already deleted, perm, whatever, so return an error*/
            ad_close(ad, adflags);
	    return -1;
	}
    } else {
	    /* Read the adouble header in and parse it.*/
	if ((ad_header_read( ad , &st) < 0)
#if AD_VERSION == AD_VERSION2
		|| (ad_v1tov2(ad, ad_p) < 0)
#endif /* AD_VERSION == AD_VERSION2 */
        ) {
            ad_close( ad, adflags );
	    return( -1 );
	}
    }
    return 0 ;
}

/* ----------------------------------- */
static int new_rfork(const char *path, struct adouble *ad, int adflags)
{
#if 0
    struct timeval      tv;
#endif    
    const struct entry  *eid;
    u_int16_t           ashort;
    struct stat         st;

    ad->ad_magic = AD_MAGIC;
    ad->ad_version = AD_VERSION;

    memset(ad->ad_filler, 0, sizeof( ad->ad_filler ));
    memset(ad->ad_data, 0, sizeof(ad->ad_data));

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
	memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRTYPEOFF,"TEXT", 4);
	memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRCREATOFF,"UNIX", 4);
    }

    /* make things invisible */
    if ((*path == '.') && strcmp(path, ".") && strcmp(path, "..")) {
        ashort = htons(ATTRBIT_INVISIBLE);
	ad_setattr(ad, ashort);
	ashort = htons(FINDERINFO_INVISIBLE);
	memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF,
		     &ashort, sizeof(ashort));
    }

#if 0
    if (gettimeofday(&tv, NULL) < 0) {
	return -1;
    } 
#endif
    
    if (stat(path, &st) < 0) {
	return -1;
    }
	    
    /* put something sane in the date fields */
    ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_BACKUP, AD_DATE_START);
    return 0;
}

/* to do this with mmap, we need the hfs fs to understand how to mmap
   header files. */
int ad_refresh(struct adouble *ad)
{

  if (ad->ad_hf.adf_fd < -1)
    return -1;

  return ad_header_read(ad, NULL);
}
