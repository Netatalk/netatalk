/* 
 * Copyright (C) 1990, 1993 Regents of The University of Michigan
 * All Rights Reserved. See COPYRIGHT
 */


/*
 * This file contains FPCatSearch implementation. FPCatSearch performs
 * file/directory search based on specified criteria. It is used by client
 * to perform fast searches on (propably) big volumes. So, it has to be
 * pretty fast.
 *
 * This implementation bypasses most of adouble/libatalk stuff as long as
 * possible and does a standard filesystem search. It calls higher-level
 * libatalk/afpd functions only when it is really needed, mainly while
 * returning some non-UNIX information or filtering by non-UNIX criteria.
 *
 * Initial version written by Rafal Lewczuk <rlewczuk@pronet.pl>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#if STDC_HEADERS
#include <string.h>
#else
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <netinet/in.h>

#include <netatalk/endian.h>
#include <atalk/afp.h>
#include <atalk/adouble.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* DID_MTAB */
#include "desktop.h"
#include "directory.h"
#include "file.h"
#include "volume.h"
#include "globals.h"
#include "filedir.h"
#include "fork.h"

#ifdef DID_MTAB
#include "parse-mtab.h"
#endif /* DID_MTAB */

#ifdef WITH_CATSEARCH

struct finderinfo {
	u_int32_t f_type;
	u_int32_t creator;
	u_int8_t attrs;    /* File attributes (8 bits)*/
	u_int8_t label;    /* Label (8 bits)*/
	char reserved[22]; /* Unknown (at least for now...) */
};

/* Known attributes:
 * 0x04 - has a custom icon
 * 0x20 - name/icon is locked
 * 0x40 - is invisible
 * 0x80 - is alias
 *
 * Known labels:
 * 0x02 - project 2
 * 0x04 - project 1
 * 0x06 - personal
 * 0x08 - cool
 * 0x0a - in progress
 * 0x0c - hot
 * 0x0e - essential
 */

/* This is our search-criteria structure. */
struct scrit {
	u_int32_t rbitmap;          /* Request bitmap - which values should we check ? */
	u_int16_t fbitmap, dbitmap; /* file & directory bitmap - which values should we return ? */
	u_int16_t attr;             /* File attributes */
	time_t cdate;               /* Creation date */
	time_t mdate;               /* Last modification date */
	time_t bdate;               /* Last backup date */
	u_int32_t pdid;             /* Parent DID */
        u_int16_t offcnt;           /* Offspring count */
	struct finderinfo finfo;    /* Finder info */
	char lname[32];             /* Long name */
};

/*
 * Directory tree search is recursive by its nature. But AFP specification
 * requires FPCatSearch to pause after returning n results and be able to
 * resume the search later. So we have to do recursive search using flat
 * (iterative) algorithm and remember all directories to look into in an
 * stack-like structure. The structure below is one item of directory stack.
 *
 */
struct dsitem {
	char *lname;     /* Long name */
	struct dir *dir; /* Structure describing this directory */
	int pidx;        /* Parent's dsitem structure index. */
	int checked;     /* Have we checked this directory ? */
	char *path;      /* UNIX path to this directory */
};
 

#define DS_BSIZE 128
static int cur_pos = 0;    /* Saved position index (ID) - used to remember "position" across FPCatSearch calls */
static DIR *dirpos = NULL; /* UNIX structure describing currently opened directory. */
static int save_cidx = -1; /* Saved index of currently scanned directory. */

static struct dsitem *dstack = NULL; /* Directory stack data... */
static int dssize = 0;  	     /* Directory stack (allocated) size... */
static int dsidx = 0;   	     /* First free item index... */

static struct scrit c1, c2;          /* search criteria */

/* Puts new item onto directory stack. */
static int addstack(char *lname, struct dir *dir, int pidx)
{
	struct dsitem *ds;

	/* check if we have some space on stack... */
	if (dsidx >= dssize) {
		dssize += DS_BSIZE;
		dstack = realloc(dstack, dssize * sizeof(struct dsitem));	
		if (dstack == NULL)
			return -1;
	}

	/* Put new element. Allocate and copy lname and path. */
	ds = dstack + dsidx++;
	ds->lname = strdup(lname);
	ds->dir = dir;
	ds->pidx = pidx;
	if (pidx >= 0) {
		ds->path = malloc(strlen(dstack[pidx].path) + strlen(ds->lname) + 2);
		strcpy(ds->path, dstack[pidx].path);
		strcat(ds->path, "/");
		strcat(ds->path, ds->lname);
	}

	ds->checked = 0;

	return 0;
}

/* Removes checked items from top of directory stack. Returns index of the first unchecked elements or -1. */
static int reducestack()
{
	int r;
	if (save_cidx != -1) {
		r = save_cidx;
		save_cidx = -1;
		return r;
	}

	while (dsidx > 0) {
		if (dstack[dsidx-1].checked) {
			dsidx--;
			free(dstack[dsidx].lname);
			free(dstack[dsidx].path);
			/* Check if we need to free (or release) dir structures */
		} else
			return dsidx - 1;
	} // while
	return -1;
} /* reducestack() */

/* Clears directory stack. */
static void clearstack() 
{
	save_cidx = -1;
	while (dsidx > 0) {
		dsidx--;
		free(dstack[dsidx].lname);
		free(dstack[dsidx].path);
		/* Check if we need to free (or release) dir structures */
	}
} /* clearstack() */

/* Fills in dir field of dstack[cidx]. Must fill parent dirs' fields if needed... */
static int resolve_dir(struct vol *vol, int cidx)
{
	struct dir *dir, *curdir;
	struct stat statbuf;

	if (dstack[cidx].dir != NULL)
		return 1;

	if (dstack[cidx].pidx < 0)
		return 0;

	if (dstack[dstack[cidx].pidx].dir == NULL && resolve_dir(vol, dstack[cidx].pidx) == 0)
	       return 0;

	curdir = dstack[dstack[cidx].pidx].dir;
	dir = curdir->d_child;
	while (dir) {
		if (strcmp(dir->d_name, dstack[cidx].lname) == 0)
			break;
		dir = (dir == curdir->d_child->d_prev) ? NULL : dir->d_next;
	} /* while */

	if (!dir)
		if (stat(dstack[cidx].path, &statbuf)==-1) {
			syslog(LOG_DEBUG, "resolve_dir: stat %s: %s", dstack[cidx].path, strerror(errno));
			return 0;
		}

	if (!dir && ((dir = adddir(vol, curdir, dstack[cidx].lname, strlen(dstack[cidx].lname),
						dstack[cidx].path, strlen(dstack[cidx].path), &statbuf)) == NULL))
			return 0;
	dstack[cidx].dir = dir;

	return 1;
} /* resolve_dir */

/* Looks up for an opened adouble structure, opens resource fork of selected file. */
static struct adouble *adl_lkup(struct vol *vol, char *upath, int cidx)
{
	static struct adouble ad;
	struct adouble *adp;
	char *mpath = utompath(vol, upath);
	struct ofork *of;

/*
	//if (dstack[cidx].dir == NULL && !resolve_dir(vol, cidx))
	//	return NULL;

	//if ((of = of_findname(vol, dstack[cidx].dir, mpath))) {
	//	adp = of->of_ad;
	//} else {
	*/
		memset(&ad, 0, sizeof(ad));
		adp = &ad;
	/* } */

    	if ( ad_open( upath, ADFLAGS_HF, O_RDONLY, 0, adp) < 0 ) {
        	return NULL;
    	} 
	return adp;	
}

#define CATPBIT_PARTIAL 31
/* Criteria checker. This function returns a 2-bit value. */
/* bit 0 means if checked file meets given criteria. */
/* bit 1 means if it is a directory and we should descent into it. */
/* uname - UNIX name 
 * fname - our fname (translated to UNIX)
 * cidx - index in directory stack
 */
static int crit_check(struct vol *vol, char *uname, char *fname, int cidx) {
	int r = 0;
	struct stat sbuf;
	u_int16_t attr;
	struct finderinfo *finfo = NULL;
	struct adouble *adp = NULL;
	time_t c_date, b_date;

	if (stat(uname, &sbuf) < 0)
		return 0;
	
	if (S_ISDIR(sbuf.st_mode))
		r = 2;

	/* Kind of optimization: 
	 * -- first check things we've already have - filename
	 * -- last check things we get from ad_open()
	 */

	/* Check for filename */
	if (c1.rbitmap & (1<<DIRPBIT_LNAME)) { 
		if (c1.rbitmap & (1<<CATPBIT_PARTIAL)) {
			if (strstr(fname, c1.lname) == NULL)
				goto crit_check_ret;
		} else
			if (strcmp(fname, c1.lname) != 0)
				goto crit_check_ret;
	} /* if (c1.rbitmap & ... */


	/* FIXME */
	if ((unsigned)c2.mdate > 0x7fffffff)
		c2.mdate = 0x7fffffff;
	if ((unsigned)c2.cdate > 0x7fffffff)
		c2.cdate = 0x7fffffff;
	if ((unsigned)c2.bdate > 0x7fffffff)
		c2.bdate = 0x7fffffff;

	/* Check for modification date FIXME: should we look at adouble structure ? */
	if ((c1.rbitmap & (1<<DIRPBIT_MDATE))) 
		if (sbuf.st_mtime < c1.mdate || sbuf.st_mtime > c2.mdate)
			goto crit_check_ret;

	/* Check for creation date... */
	if (c1.rbitmap & (1<<DIRPBIT_CDATE)) {
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			if (ad_getdate(adp, AD_DATE_CREATE, (u_int32_t*)&c_date) >= 0)
				c_date = AD_DATE_TO_UNIX(c_date);
			else c_date = sbuf.st_mtime;
		} else c_date = sbuf.st_mtime;
		if (c_date < c1.cdate || c_date > c2.cdate)
			goto crit_check_ret;
	}

	/* Check for backup date... */
	if (c1.rbitmap & (1<<DIRPBIT_BDATE)) {
		if (adp || (adp == adl_lkup(vol, uname, cidx))) {
			if (ad_getdate(adp, AD_DATE_BACKUP, (u_int32_t*)&b_date) >= 0)
				b_date = AD_DATE_TO_UNIX(b_date);
			else b_date = sbuf.st_mtime;
		} else b_date = sbuf.st_mtime;
		if (b_date < c1.bdate || b_date > c2.bdate)
			goto crit_check_ret;
	}
				
	/* Check attributes */
	if ((c1.rbitmap & (1<<DIRPBIT_ATTR)) && c2.attr != 0)
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			ad_getattr(adp, &attr);
			if ((attr & c2.attr) != c1.attr)
				goto crit_check_ret;
		} else goto crit_check_ret;
		

        /* Check file type ID */
	if ((c1.rbitmap & (1<<DIRPBIT_FINFO)) && c2.finfo.f_type != 0)
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			finfo = (struct finderinfo*)ad_entry(adp, ADEID_FINDERI);
			if (finfo->f_type != c1.finfo.f_type)
				goto crit_check_ret;
		} else goto crit_check_ret;

	/* Check creator ID */
	if ((c1.rbitmap & (1<<DIRPBIT_FINFO)) && c2.finfo.creator != 0)
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			finfo = (struct finderinfo*)ad_entry(adp, ADEID_FINDERI);
			if (finfo->creator != c1.finfo.creator)
				goto crit_check_ret;
		} else goto crit_check_ret;
	
	/* Check finder info attributes */
	if ((c1.rbitmap & (1<<DIRPBIT_FINFO)) && c2.finfo.attrs != 0)
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			finfo = (struct finderinfo*)ad_entry(adp, ADEID_FINDERI);
			if ((finfo->attrs & c2.finfo.attrs) != c1.finfo.attrs)
				goto crit_check_ret;
		} else goto crit_check_ret;

	/* Check label */
	if ((c1.rbitmap & (1<<DIRPBIT_FINFO)) && c2.finfo.label != 0)
		if (adp || (adp = adl_lkup(vol, uname, cidx))) {
			finfo = (struct finderinfo*)ad_entry(adp, ADEID_FINDERI);
			if ((finfo->label & c2.finfo.label) != c1.finfo.label)
				goto crit_check_ret;
		} else goto crit_check_ret;
	
	/* FIXME: Attributes check ! */
	
	/* All criteria are met. */
	r |= 1;
crit_check_ret:
	if (adp != NULL)
		ad_close(adp, ADFLAGS_HF);
	return r;
}  


/* Adds an item to resultset. */
static int rslt_add(struct vol *vol, struct stat *statbuf, char *fname, short cidx, int isdir, char **rbuf)
{
	char *p = *rbuf;
	int l = fname != NULL ? strlen(fname) : 0;
	u_int32_t did;
	char p0;

	p0 = p[0] = cidx != -1 ? l + 7 : l + 5;
	if (p0 & 1) p[0]++;
	p[1] = isdir ? 128 : 0;
	p += 2;
	if (cidx != -1) {
		if (dstack[cidx].dir == NULL && resolve_dir(vol, cidx) == 0)
			return 0;
		did = dstack[cidx].dir->d_did;
		memcpy(p, &did, sizeof(did));
		p += sizeof(did);
	}

	/* Fill offset of returned file name */
	if (fname != NULL) {
		*p++ = 0;
		*p++ = (int)(p - *rbuf) + 1;
		p[0] = l;
		strcpy(p+1, fname);
		p += l + 1;
	}

	if (p0 & 1)
		*p++ = 0;

	*rbuf = p;
	/* *rbuf[0] = (int)(p-*rbuf); */
	return 1;
} /* rslt_add */

#define VETO_STR \
        "./../.AppleDouble/.AppleDB/Network Trash Folder/TheVolumeSettingsFolder/TheFindByContentFolder/.AppleDesktop/.Parent/"

/* This function performs search. It is called directly from afp_catsearch 
 * vol - volume we are searching on ...
 * dir - directory we are starting from ...
 * c1, c2 - search criteria
 * rmatches - maximum number of matches we can return
 * pos - position we've stopped recently
 * rbuf - output buffer
 * rbuflen - output buffer length
*/
static int catsearch(struct vol *vol, struct dir *dir,  
		     int rmatches, int *pos, char *rbuf, u_int32_t *nrecs, int *rsize)
{
	int cidx, r, i;
	char *fname = NULL;
	struct dirent *entry;
	struct stat statbuf;
	int result = AFP_OK;
	int ccr;
	char *orig_dir = NULL;
	int orig_dir_len = 128;
	char *path = vol->v_path;
	char *rrbuf = rbuf;

	if (*pos != 0 && *pos != cur_pos) 
		return AFPERR_CATCHNG;

	/* FIXME: Category "offspring count ! */

	/* So we are beginning... */
	/* We need to initialize all mandatory structures/variables and change working directory appropriate... */
	if (*pos == 0) {
		clearstack();
		if (dirpos != NULL) {
			closedir(dirpos);
			dirpos = NULL;
		} /* if (dirpos != NULL) */
		
		if (addstack("", dir, -1) == -1) {
			result = AFPERR_MISC;
			goto catsearch_end;
		}
		dstack[0].path = strdup(path);
		/* FIXME: Sometimes DID is given by klient ! (correct this one above !) */
	}

	/* Save current path */
	orig_dir = (char*)malloc(orig_dir_len);
	while (getcwd(orig_dir, orig_dir_len-1)==NULL) {
		if (errno != ERANGE) {
			result = AFPERR_MISC;
			goto catsearch_end;
		}
		orig_dir_len += 128; 
		orig_dir = realloc(orig_dir, orig_dir_len);
	} /* while() */
	
	while ((cidx = reducestack()) != -1) {
		if (dirpos == NULL)
			dirpos = opendir(dstack[cidx].path);	
		if (dirpos == NULL) {
			switch (errno) {
			case EACCES:
				dstack[cidx].checked = 1;
				continue;
			case EMFILE:
			case ENFILE:
			case ENOENT:
				result = AFPERR_NFILE;
				break;
			case ENOMEM:
			case ENOTDIR:
			default:
				result = AFPERR_MISC;
			} /* switch (errno) */
			goto catsearch_end;
		}
		chdir(dstack[cidx].path);
		while ((entry=readdir(dirpos)) != NULL) {
			(*pos)++;
			if (veto_file(VETO_STR, entry->d_name))
				continue;
			if (stat(entry->d_name, &statbuf) != 0) {
				switch (errno) {
				case EACCES:
				case ELOOP:
				case ENOENT:
					continue;
				case ENOTDIR:
				case EFAULT:
				case ENOMEM:
				case ENAMETOOLONG:
				default:
					result = AFPERR_MISC;
					goto catsearch_end;
				} /* switch (errno) */
			} /* if (stat(entry->d_name, &statbuf) != 0) */
			fname = utompath(vol, entry->d_name);
			for (i = 0; fname[i] != 0; i++)
				fname[i] = tolower(fname[i]);
			if (strlen(fname) > MACFILELEN) 
				continue;
			ccr = crit_check(vol, entry->d_name, fname, cidx);
			/* bit 0 means that criteria has ben met */
			if (ccr & 1) {
				r = rslt_add(vol, &statbuf, 
					     (c1.fbitmap&(1<<FILPBIT_LNAME))|(c1.dbitmap&(1<<DIRPBIT_LNAME)) ? 
					         utompath(vol, entry->d_name) : NULL,	
					     (c1.fbitmap&(1<<FILPBIT_PDID))|(c1.dbitmap&(1<<DIRPBIT_PDID)) ? 
					         cidx : -1, 
					     S_ISDIR(statbuf.st_mode), &rrbuf); 
				if (r == 0) {
					result = AFPERR_MISC;
					goto catsearch_end;
				} 
				*nrecs += r;
				/* Number of matches limit */
				if (--rmatches == 0) 
					goto catsearch_pause; /* FIXME: timelimit checks ! */
				/* Block size limit */
				if (rrbuf - rbuf >= 448)
					goto catsearch_pause;

			} 
			/* bit 1 means that we have to descend into this directory. */
			if (ccr & 2) {
				if (S_ISDIR(statbuf.st_mode))
					if (addstack(entry->d_name, NULL, cidx) == -1) {
						result = AFPERR_MISC;
						goto catsearch_end;
					} /* if (addstack... */
			}
		} /* while ((entry=readdir(dirpos)) != NULL) */
		closedir(dirpos);dirpos = NULL;
		dstack[cidx].checked = 1;
	} /* while (current_idx = reducestack()) != -1) */

	/* We have finished traversing our tree. Return EOF here. */
	result = AFPERR_EOF;
	goto catsearch_end;

catsearch_pause:
	cur_pos = *pos; 
	save_cidx = cidx;

catsearch_end: /* Exiting catsearch: error condition */
	*rsize = rrbuf - rbuf;
	if (orig_dir != NULL) {
		chdir(orig_dir);
		free(orig_dir);
	}
	return result;
} /* catsearch() */


int afp_catsearch(AFPObj *obj, char *ibuf, int ibuflen,
                  char *rbuf, int *rbuflen)
{
    struct vol *vol;
    u_int16_t   vid;
    u_int32_t   rmatches, reserved;
    u_int32_t	catpos[4];
    u_int32_t   pdid = 0;
    char        *lname = NULL;
    struct dir *dir;
    int ret, rsize, i = 0;
    u_int32_t nrecs = 0;
    static int nrr = 1;
    char *spec1, *spec2, *bspec1, *bspec2;

    memset(&c1, 0, sizeof(c1));
    memset(&c2, 0, sizeof(c2));

    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    *rbuflen = 0;
    if ((vol = getvolbyvid(vid)) == NULL)
        return AFPERR_PARAM;

    memcpy(&rmatches, ibuf, sizeof(rmatches));
    rmatches = ntohl(rmatches);
    ibuf += sizeof(rmatches); 

    /* FIXME: (rl) should we check if reserved == 0 ? */
    ibuf += sizeof(reserved);

    memcpy(catpos, ibuf, sizeof(catpos));
    ibuf += sizeof(catpos);

    memcpy(&c1.fbitmap, ibuf, sizeof(c1.fbitmap));
    c1.fbitmap = c2.fbitmap = ntohs(c1.fbitmap);
    ibuf += sizeof(c1.fbitmap);

    memcpy(&c1.dbitmap, ibuf, sizeof(c1.dbitmap));
    c1.dbitmap = c2.dbitmap = ntohl(c1.dbitmap);
    ibuf += sizeof(c1.dbitmap);

    memcpy(&c1.rbitmap, ibuf, sizeof(c1.rbitmap));
    c1.rbitmap = c2.rbitmap = ntohl(c1.rbitmap);
    ibuf += sizeof(c1.rbitmap);

    if (! (c1.fbitmap || c1.dbitmap)) {
	    *rbuflen = 0;
	    return AFPERR_BITMAP;
    }

    /* Parse file specifications */
    spec1 = ibuf;
    spec2 = ibuf + ibuf[0] + 2;

    spec1 += 2; bspec1 = spec1;
    spec2 += 2; bspec2 = spec2;

    /* File attribute bits... */
    if (c1.rbitmap & (1 << FILPBIT_ATTR)) {
	    memcpy(&c1.attr, ibuf, sizeof(c1.attr));
	    spec1 += sizeof(c1.attr);
	    c1.attr = ntohs(c1.attr);
	    memcpy(&c2.attr, ibuf, sizeof(c2.attr));
	    spec2 += sizeof(c1.attr);
	    c2.attr = ntohs(c2.attr);
    }

    /* Parent DID */
    if (c1.rbitmap & (1 << FILPBIT_PDID)) {
            memcpy(&c1.pdid, spec1, sizeof(pdid));
	    spec1 += sizeof(c1.pdid);
	    memcpy(&c2.pdid, spec2, sizeof(pdid));
	    spec2 += sizeof(c2.pdid);
    } /* FIXME: PDID - do we demarshall this argument ? */

    /* Creation date */
    if (c1.rbitmap & (1 << FILPBIT_CDATE)) {
	    memcpy(&c1.cdate, spec1, sizeof(c1.cdate));
	    spec1 += sizeof(c1.cdate);
	    c1.cdate = AD_DATE_TO_UNIX(c1.cdate);
	    memcpy(&c2.cdate, spec2, sizeof(c2.cdate));
	    spec2 += sizeof(c1.cdate);
	    ibuf += sizeof(c1.cdate);;
	    c2.cdate = AD_DATE_TO_UNIX(c2.cdate);
    }

    /* Modification date */
    if (c1.rbitmap & (1 << FILPBIT_MDATE)) {
	    memcpy(&c1.mdate, spec1, sizeof(c1.mdate));
	    c1.mdate = AD_DATE_TO_UNIX(c1.mdate);
	    spec1 += sizeof(c1.mdate);
	    memcpy(&c2.mdate, spec2, sizeof(c2.mdate));
	    c2.mdate = AD_DATE_TO_UNIX(c2.mdate);
	    spec2 += sizeof(c1.mdate);
    }
    
    /* Backup date */
    if (c1.rbitmap & (1 << FILPBIT_BDATE)) {
	    memcpy(&c1.bdate, spec1, sizeof(c1.bdate));
	    spec1 += sizeof(c1.bdate);
	    c1.bdate = AD_DATE_TO_UNIX(c1.bdate);
	    memcpy(&c2.bdate, spec2, sizeof(c2.bdate));
	    spec2 += sizeof(c2.bdate);
	    c1.bdate = AD_DATE_TO_UNIX(c2.bdate);
    }

    /* Finder info */
    if (c1.rbitmap * (1 << FILPBIT_FINFO)) {
	    memcpy(&c1.finfo, spec1, sizeof(c1.finfo));
	    spec1 += sizeof(c1.finfo);
	    memcpy(&c2.finfo, spec2, sizeof(c2.finfo));
	    spec2 += sizeof(c2.finfo);
    } /* Finder info */

    /* Offspring count - only directories */
    if (c1.dbitmap != 0 && (c1.rbitmap & (1 << DIRPBIT_OFFCNT)) != 0) {
	    memcpy(&c1.offcnt, spec1, sizeof(c1.offcnt));
	    spec1 += sizeof(c1.offcnt);
	    c1.offcnt = ntohs(c1.offcnt);
	    memcpy(&c2.offcnt, spec2, sizeof(c2.offcnt));
	    spec2 += sizeof(c2.offcnt);
	    c2.offcnt = ntohs(c2.offcnt);
    } /* Offspring count */

    /* Long name */
    if (c1.rbitmap & (1 << FILPBIT_LNAME)) {
        /* Get the long filename */	
	memcpy(c1.lname, bspec1 + spec1[1] + 1, (bspec1 + spec1[1])[0]);
	c1.lname[(bspec1 + spec1[1])[0]]= 0;
	for (i = 0; c1.lname[i] != 0; i++)
		c1.lname[i] = tolower(c1.lname[i]);
	/* FIXME: do we need it ? It's always null ! */
	memcpy(c2.lname, bspec2 + spec2[1] + 1, (bspec2 + spec2[1])[0]);
	c2.lname[(bspec2 + spec2[1])[0]]= 0;
	for (i = 0; c2.lname[i] != 0; i++)
		c2.lname[i] = tolower(c2.lname[i]);
    }


    /* Call search */
    *rbuflen = 24;
    ret = catsearch(vol, vol->v_dir, rmatches, &catpos[0], rbuf+24, &nrecs, &rsize);
    memcpy(rbuf, catpos, sizeof(catpos));
    rbuf += sizeof(catpos);
    c1.fbitmap = htons(c1.fbitmap);
    memcpy(rbuf, &c1.fbitmap, sizeof(c1.fbitmap));
    rbuf += sizeof(c1.fbitmap);
    c1.dbitmap = htons(c1.dbitmap);
    memcpy(rbuf, &c1.dbitmap, sizeof(c1.dbitmap));
    rbuf += sizeof(c2.dbitmap);
    nrecs = htonl(nrecs);
    memcpy(rbuf, &nrecs, sizeof(nrecs));
    rbuf += sizeof(nrecs);
    *rbuflen += rsize;

    return ret;
} /* afp_catsearch */

/* FIXME: we need a clean separation between afp stubs and 'real' implementation */
/* (so, all buffer packing/unpacking should be done in stub, everything else 
   should be done in other functions) */

#endif
/* WITH_CATSEARCH */
