/*
 * $Id: file.c,v 1.86 2003-02-19 18:59:52 jmarcus Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#include <utime.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>

#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB */
#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"
#include "file.h"
#include "filedir.h"
#include "globals.h"

/* the format for the finderinfo fields (from IM: Toolbox Essentials):
 * field         bytes        subfield    bytes
 * 
 * files:
 * ioFlFndrInfo  16      ->       type    4  type field
 *                             creator    4  creator field
 *                               flags    2  finder flags:
 *					     alias, bundle, etc.
 *                            location    4  location in window
 *                              folder    2  window that contains file
 * 
 * ioFlXFndrInfo 16      ->     iconID    2  icon id
 *                              unused    6  reserved 
 *                              script    1  script system
 *                              xflags    1  reserved
 *                           commentID    2  comment id
 *                           putawayID    4  home directory id
 */

const u_char ufinderi[] = {
                              'T', 'E', 'X', 'T', 'U', 'N', 'I', 'X',
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0
                          };

/* FIXME mpath : unix or mac name ? (for now it's mac name ) */
void *get_finderinfo(const char *mpath, struct adouble *adp, void *data)
{
    struct extmap	*em;

    if (adp) {
        memcpy(data, ad_entry(adp, ADEID_FINDERI), 32);
    }
    else {
        memcpy(data, ufinderi, 32);
    }

    if ((!adp  || !memcmp(ad_entry(adp, ADEID_FINDERI),ufinderi , 8 )) 
            	&& (em = getextmap( mpath ))
    ) {
        memcpy(data, em->em_type, sizeof( em->em_type ));
        memcpy(data + 4, em->em_creator, sizeof(em->em_creator));
    }
    return data;
}

/* ---------------------
*/
char *set_name(char *data, const char *name, u_int32_t utf8) 
{
    u_int32_t           aint;

    aint = strlen( name );

    if (!utf8) {
        if (afp_version >= 30) {
            /* the name is in utf8 */
        }
        if (aint > MACFILELEN)
            aint = MACFILELEN;
        *data++ = aint;
    }
    else {
        u_int16_t temp;

        if (aint > 255)  /* FIXME safeguard, anyway if no ascii char it's game over*/
           aint = 255;

        utf8 = htonl(utf8);
        memcpy(data, &utf8, sizeof(utf8));
        data += sizeof(utf8);
        
        temp = htons(aint);
        memcpy(data, &temp, sizeof(temp));
        data += sizeof(temp);
    }

    memcpy( data, name, aint );
    data += aint;

    return data;
}

/*
 * FIXME: PDINFO is UTF8 and doesn't need adp
*/
#define PARAM_NEED_ADP(b) ((b) & ((1 << FILPBIT_ATTR)  |\
				  (1 << FILPBIT_CDATE) |\
				  (1 << FILPBIT_MDATE) |\
				  (1 << FILPBIT_BDATE) |\
				  (1 << FILPBIT_FINFO) |\
				  (1 << FILPBIT_RFLEN) |\
				  (1 << FILPBIT_EXTRFLEN) |\
				  (1 << FILPBIT_PDINFO)))


/* -------------------------- */
int getmetadata(struct vol *vol,
                 u_int16_t bitmap,
                 struct path *path, struct dir *dir, 
                 char *buf, int *buflen, struct adouble *adp, int attrbits )
{
#ifndef USE_LASTDID
    struct stat		lst, *lstp;
#endif /* USE_LASTDID */
    char		*data, *l_nameoff = NULL, *upath;
    char                *utf_nameoff = NULL;
    int			bit = 0;
    u_int32_t		aint;
    u_int16_t		ashort;
    u_char              achar, fdType[4];
    u_int32_t           utf8 = 0;
    struct stat         *st;
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin getmetadata:");
#endif /* DEBUG */

    upath = path->u_name;
    st = &path->st;

    data = buf;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch ( bit ) {
        case FILPBIT_ATTR :
            if ( adp ) {
                ad_getattr(adp, &ashort);
            } else if (*upath == '.') {
                ashort = htons(ATTRBIT_INVISIBLE);
            } else
                ashort = 0;
#if 0
            /* FIXME do we want a visual clue if the file is read only
             */
            struct maccess	ma;
            accessmode( ".", &ma, dir , NULL);
            if ((ma.ma_user & AR_UWRITE)) {
            	accessmode( upath, &ma, dir , st);
            	if (!(ma.ma_user & AR_UWRITE)) {
                	attrbits |= ATTRBIT_NOWRITE;
                }
            }
#endif
            if (attrbits)
                ashort = htons(ntohs(ashort) | attrbits);
            memcpy(data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case FILPBIT_PDID :
            memcpy(data, &dir->d_did, sizeof( u_int32_t ));
            data += sizeof( u_int32_t );
            break;

        case FILPBIT_CDATE :
            if (!adp || (ad_getdate(adp, AD_DATE_CREATE, &aint) < 0))
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_MDATE :
            if ( adp && (ad_getdate(adp, AD_DATE_MODIFY, &aint) == 0)) {
                if ((st->st_mtime > AD_DATE_TO_UNIX(aint))) {
                   aint = AD_DATE_FROM_UNIX(st->st_mtime);
                }
            } else {
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            }
            memcpy(data, &aint, sizeof( int ));
            data += sizeof( int );
            break;

        case FILPBIT_BDATE :
            if (!adp || (ad_getdate(adp, AD_DATE_BACKUP, &aint) < 0))
                aint = AD_DATE_START;
            memcpy(data, &aint, sizeof( int ));
            data += sizeof( int );
            break;

        case FILPBIT_FINFO :
	    get_finderinfo(path->m_name, adp, (char *)data);
            if (!adp) {
                if (*upath == '.') { /* make it invisible */
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
                }
            }

            data += 32;
            break;

        case FILPBIT_LNAME :
            l_nameoff = data;
            data += sizeof( u_int16_t );
            break;

        case FILPBIT_SNAME :
            memset(data, 0, sizeof(u_int16_t));
            data += sizeof( u_int16_t );
            break;

        case FILPBIT_FNUM :
            aint = 0;
#if AD_VERSION > AD_VERSION1
            /* look in AD v2 header */
            if (adp)
                memcpy(&aint, ad_entry(adp, ADEID_DID), sizeof(aint));
#endif /* AD_VERSION > AD_VERSION1 */

#ifdef CNID_DB
            aint = cnid_add(vol->v_db, st, dir->d_did, upath,
                            strlen(upath), aint);
            /* Throw errors if cnid_add fails. */
            if (aint == CNID_INVALID) {
                switch (errno) {
                case CNID_ERR_PARAM:
                    LOG(log_error, logtype_afpd, "getfilparams: Incorrect parameters passed to cnid_add");
                    return(AFPERR_PARAM);
                case CNID_ERR_PATH:
                    return(AFPERR_PARAM);
                case CNID_ERR_DB:
                case CNID_ERR_MAX:
                    return(AFPERR_MISC);
                }
            }
#endif /* CNID_DB */

            if (aint == 0) {
                /*
                 * What a fucking mess.  First thing:  DID and FNUMs are
                 * in the same space for purposes of enumerate (and several
                 * other wierd places).  While we consider this Apple's bug,
                 * this is the work-around:  In order to maintain constant and
                 * unique DIDs and FNUMs, we monotonically generate the DIDs
                 * during the session, and derive the FNUMs from the filesystem.
                 * Since the DIDs are small, we insure that the FNUMs are fairly
                 * large by setting thier high bits to the device number.
                 *
                 * AFS already does something very similar to this for the
                 * inode number, so we don't repeat the procedure.
                 *
                 * new algorithm:
                 * due to complaints over did's being non-persistent,
                 * here's the current hack to provide semi-persistent
                 * did's:
                 *      1) we reserve the first bit for file ids.
                 *      2) the next 7 bits are for the device.
                 *      3) the remaining 24 bits are for the inode.
                 *
                 * both the inode and device information are actually hashes
                 * that are then truncated to the requisite bit length.
                 *
                 * it should be okay to use lstat to deal with symlinks.
                 */
#ifdef USE_LASTDID
                aint = htonl(( st->st_dev << 16 ) | (st->st_ino & 0x0000ffff));
#else /* USE_LASTDID */
                lstp = lstat(upath, &lst) < 0 ? st : &lst;
                aint = htonl(CNID(lstp, 1));
#endif /* USE_LASTDID */
            }

            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_DFLEN :
            if  (st->st_size > 0xffffffff)
               aint = 0xffffffff;
            else
               aint = htonl( st->st_size );
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_RFLEN :
            if ( adp ) {
                if (adp->ad_rlen > 0xffffffff)
                    aint = 0xffffffff;
                else
                    aint = htonl( adp->ad_rlen);
            } else {
                aint = 0;
            }
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

            /* Current client needs ProDOS info block for this file.
               Use simple heuristic and let the Mac "type" string tell
               us what the PD file code should be.  Everything gets a
               subtype of 0x0000 unless the original value was hashed
               to "pXYZ" when we created it.  See IA, Ver 2.
               <shirsch@adelphia.net> */
        case FILPBIT_PDINFO :
            if (afp_version >= 30) { /* UTF8 name */
                utf8 = kTextEncodingUTF8;
                utf_nameoff = data;
                data += sizeof( u_int16_t );
                aint = 0;
                memcpy(data, &aint, sizeof( aint ));
                data += sizeof( aint );
            }
            else {
                if ( adp ) {
                    memcpy(fdType, ad_entry( adp, ADEID_FINDERI ), 4 );

                    if ( memcmp( fdType, "TEXT", 4 ) == 0 ) {
                        achar = '\x04';
                        ashort = 0x0000;
                    }
                    else if ( memcmp( fdType, "PSYS", 4 ) == 0 ) {
                        achar = '\xff';
                        ashort = 0x0000;
                    }
                    else if ( memcmp( fdType, "PS16", 4 ) == 0 ) {
                        achar = '\xb3';
                        ashort = 0x0000;
                    }
                    else if ( memcmp( fdType, "BINA", 4 ) == 0 ) {
                        achar = '\x00';
                        ashort = 0x0000;
                    }
                    else if ( fdType[0] == 'p' ) {
                        achar = fdType[1];
                        ashort = (fdType[2] * 256) + fdType[3];
                    }
                    else {
                        achar = '\x00';
                        ashort = 0x0000;
                    }
                }
                else {
                    achar = '\x00';
                    ashort = 0x0000;
                }

                *data++ = achar;
                *data++ = 0;
                memcpy(data, &ashort, sizeof( ashort ));
                data += sizeof( ashort );
                memset(data, 0, sizeof( ashort ));
                data += sizeof( ashort );
            }
            break;
        case FILPBIT_EXTDFLEN:
            aint = htonl(st->st_size >> 32);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            aint = htonl(st->st_size);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;
        case FILPBIT_EXTRFLEN:
            aint = 0;
            if (adp) 
                aint = htonl(adp->ad_rlen >> 32);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            if (adp) 
                aint = htonl(adp->ad_rlen);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;
        default :
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( l_nameoff ) {
        ashort = htons( data - buf );
        memcpy(l_nameoff, &ashort, sizeof( ashort ));
        data = set_name(data, path->m_name, 0);
    }
    if ( utf_nameoff ) {
        ashort = htons( data - buf );
        memcpy(utf_nameoff, &ashort, sizeof( ashort ));
        data = set_name(data, path->m_name, utf8);
    }
    *buflen = data - buf;
    return (AFP_OK);
}
                
/* ----------------------- */
int getfilparams(struct vol *vol,
                 u_int16_t bitmap,
                 struct path *path, struct dir *dir, 
                 char *buf, int *buflen )
{
    struct adouble	ad, *adp;
    struct ofork        *of;
    char		    *upath;
    u_int16_t		attrbits = 0;
    int                 opened = 0;
    int rc;    

#ifdef DEBUG
    LOG(log_info, logtype_default, "begin getfilparams:");
#endif /* DEBUG */

    opened = PARAM_NEED_ADP(bitmap);
    adp = NULL;
    if (opened) {
        upath = path->u_name;
        if ((of = of_findname(path))) {
            adp = of->of_ad;
    	    attrbits = ((of->of_ad->ad_df.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);
    	    attrbits |= ((of->of_ad->ad_hf.adf_refcount > of->of_ad->ad_df.adf_refcount)? ATTRBIT_ROPEN : 0);
        } else {
            memset(&ad, 0, sizeof(ad));
            adp = &ad;
        }

        if ( ad_open( upath, ADFLAGS_HF, O_RDONLY, 0, adp) < 0 ) {
             adp = NULL;
        }
        else {
    	    /* FIXME 
    	       we need to check if the file is open by another process.
    	       it's slow so we only do it if we have to:
    	       - bitmap is requested.
    	       - we don't already have the answer!
    	    */
    	    if ((bitmap & (1 << FILPBIT_ATTR))) {
	         if (!(attrbits & ATTRBIT_ROPEN)) {
	    	 }
    		 if (!(attrbits & ATTRBIT_DOPEN)) {
	    	 }
    	    }
    	}
    }
    rc = getmetadata(vol, bitmap, path, dir, buf, buflen, adp, attrbits);
    if ( adp ) {
        ad_close( adp, ADFLAGS_HF );
    }
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end getfilparams:");
#endif /* DEBUG */

    return( rc );
}

/* ----------------------------- */
int afp_createfile(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad, *adp;
    struct vol		*vol;
    struct dir		*dir;
    struct ofork        *of = NULL;
    char		*path, *upath;
    int			creatf, did, openf, retvalue = AFP_OK;
    u_int16_t		vid;
    int                 ret;
    struct path		*s_path;
    
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_createfile:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf++;
    creatf = (unsigned char) *ibuf++;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did));
    ibuf += sizeof( did );

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return afp_errno;
    }

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return afp_errno;
    }

    if ( *s_path->m_name == '\0' ) {
        return( AFPERR_BADTYPE );
    }

    upath = s_path->u_name;
    if (0 != (ret = check_name(vol, upath))) 
       return  ret;
    
    /* if upath is deleted we already in trouble anyway */
    if ((of = of_findname(s_path))) {
        adp = of->of_ad;
    } else {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    }
    if ( creatf) {
        /* on a hard create, fail if file exists and is open */
        if (of)
            return AFPERR_BUSY;
        openf = O_RDWR|O_CREAT|O_TRUNC;
    } else {
    	/* on a soft create, if the file is open then ad_open won't fail
    	   because open syscall is not called
    	*/
    	if (of) {
    		return AFPERR_EXIST;
    	}
        openf = O_RDWR|O_CREAT|O_EXCL;
    }

    if ( ad_open( upath, vol_noadouble(vol)|ADFLAGS_DF|ADFLAGS_HF|ADFLAGS_NOHF,
                  openf, 0666, adp) < 0 ) {
        switch ( errno ) {
        case EEXIST :
            return( AFPERR_EXIST );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }
    if ( ad_hfileno( adp ) == -1 ) {
         /* on noadouble volumes, just creating the data fork is ok */
         if (vol_noadouble(vol))
             goto createfile_done;
         /* FIXME with hard create on an existing file, we already
          * corrupted the data file.
          */
         netatalk_unlink( upath );
         ad_close( adp, ADFLAGS_DF );
         return AFPERR_ACCESS;
    }

    path = s_path->m_name;
    ad_setentrylen( adp, ADEID_NAME, strlen( path ));
    memcpy(ad_entry( adp, ADEID_NAME ), path,
           ad_getentrylen( adp, ADEID_NAME ));
    ad_flush( adp, ADFLAGS_DF|ADFLAGS_HF );
    ad_close( adp, ADFLAGS_DF|ADFLAGS_HF );

createfile_done:
    curdir->offcnt++;

#ifdef DROPKLUDGE
    if (vol->v_flags & AFPVOL_DROPBOX) {
        retvalue = matchfile2dirperms(upath, vol, did);
    }
#endif /* DROPKLUDGE */

    setvoltime(obj, vol );

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_createfile");
#endif /* DEBUG */

    return (retvalue);
}

int afp_setfilparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    struct path *s_path;
    int		did, rc;
    u_int16_t	vid, bitmap;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_setfilparams:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return afp_errno; /* was AFPERR_NOOBJ */
    }

    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return afp_errno;
    }

    if (path_isadir(s_path)) {
        return( AFPERR_BADTYPE ); /* it's a directory */
    }

    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    if (AFP_OK == ( rc = setfilparams(vol, s_path, bitmap, ibuf )) ) {
        setvoltime(obj, vol );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_setfilparams:");
#endif /* DEBUG */

    return( rc );
}

/*
 * cf AFP3.0.pdf page 252 for change_mdate and change_parent_mdate logic  
 *
*/
extern struct path Cur_Path;

int setfilparams(struct vol *vol,
                 struct path *path, u_int16_t bitmap, char *buf )
{
    struct adouble	ad, *adp;
    struct ofork        *of;
    struct extmap	*em;
    int			bit = 0, isad = 1, err = AFP_OK;
    char                *upath;
    u_char              achar, *fdType, xyy[4];
    u_int16_t		ashort, bshort;
    u_int32_t		aint;
    struct utimbuf	ut;

    int                 change_mdate = 0;
    int                 change_parent_mdate = 0;
    int                 newdate = 0;
    struct timeval      tv;


#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin setfilparams:");
#endif /* DEBUG */

    upath = path->u_name;
    if ((of = of_findname(path))) {
        adp = of->of_ad;
    } else {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    }

    if (check_access(upath, OPENACC_WR ) < 0) {
        return AFPERR_ACCESS;
    }

    if (ad_open( upath, vol_noadouble(vol) | ADFLAGS_HF,
                 O_RDWR|O_CREAT, 0666, adp) < 0) {
        /* for some things, we don't need an adouble header */
        if (bitmap & ~(1<<FILPBIT_MDATE)) {
            return vol_noadouble(vol) ? AFP_OK : AFPERR_ACCESS;
        }
        isad = 0;
    } else if ((ad_get_HF_flags( adp ) & O_CREAT) ) {
        ad_setentrylen( adp, ADEID_NAME, strlen( path->m_name ));
        memcpy(ad_entry( adp, ADEID_NAME ), path->m_name,
               ad_getentrylen( adp, ADEID_NAME ));
    }

    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch(  bit ) {
        case FILPBIT_ATTR :
            change_mdate = 1;
            memcpy(&ashort, buf, sizeof( ashort ));
            ad_getattr(adp, &bshort);
            if ( ntohs( ashort ) & ATTRBIT_SETCLR ) {
                bshort |= htons( ntohs( ashort ) & ~ATTRBIT_SETCLR );
            } else {
                bshort &= ~ashort;
            }
            if ((ashort & htons(ATTRBIT_INVISIBLE)))
                change_parent_mdate = 1;
            ad_setattr(adp, bshort);
            buf += sizeof( ashort );
            break;

        case FILPBIT_CDATE :
            change_mdate = 1;
            memcpy(&aint, buf, sizeof(aint));
            ad_setdate(adp, AD_DATE_CREATE, aint);
            buf += sizeof( aint );
            break;

        case FILPBIT_MDATE :
            memcpy(&newdate, buf, sizeof( newdate ));
            buf += sizeof( newdate );
            break;

        case FILPBIT_BDATE :
            change_mdate = 1;
            memcpy(&aint, buf, sizeof(aint));
            ad_setdate(adp, AD_DATE_BACKUP, aint);
            buf += sizeof( aint );
            break;

        case FILPBIT_FINFO :
            change_mdate = 1;

            if (!memcmp( ad_entry( adp, ADEID_FINDERI ), ufinderi, 8 )
                    && ( 
                     ((em = getextmap( path->m_name )) &&
                      !memcmp(buf, em->em_type, sizeof( em->em_type )) &&
                      !memcmp(buf + 4, em->em_creator,sizeof( em->em_creator)))
                     || ((em = getdefextmap()) &&
                      !memcmp(buf, em->em_type, sizeof( em->em_type )) &&
                      !memcmp(buf + 4, em->em_creator,sizeof( em->em_creator)))
            )) {
                memcpy(buf, ufinderi, 8 );
            }

            memcpy(ad_entry( adp, ADEID_FINDERI ), buf, 32 );
            buf += 32;
            break;

            /* Client needs to set the ProDOS file info for this file.
               Use a defined string for TEXT to support crlf
               translations and convert all else into pXYY per Inside
               Appletalk.  Always set the creator as "pdos".  Changes
               from original by Marsha Jackson. */
        case FILPBIT_PDINFO :
            if (afp_version < 30) { /* else it's UTF8 name */
                achar = *buf;
                buf += 2;
                /* Keep special case to support crlf translations */
                if ((unsigned int) achar == 0x04) {
	       	    fdType = (u_char *)"TEXT";
		    buf += 2;
                } else {
            	    xyy[0] = ( u_char ) 'p';
            	    xyy[1] = achar;
            	    xyy[3] = *buf++;
            	    xyy[2] = *buf++;
            	    fdType = xyy;
	        }
                memcpy(ad_entry( adp, ADEID_FINDERI ), fdType, 4 );
                memcpy(ad_entry( adp, ADEID_FINDERI ) + 4, "pdos", 4 );
                break;
            }
            /* fallthrough */
        default :
            err = AFPERR_BITMAP;
            goto setfilparam_done;
        }

        bitmap = bitmap>>1;
        bit++;
    }

setfilparam_done:
    if (change_mdate && newdate == 0 && gettimeofday(&tv, NULL) == 0) {
       newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
    }
    if (newdate) {
       if (isad)
          ad_setdate(adp, AD_DATE_MODIFY, newdate);
       ut.actime = ut.modtime = AD_DATE_TO_UNIX(newdate);
       utime(upath, &ut);
    }

    if (isad) {
        ad_flush( adp, ADFLAGS_HF );
        ad_close( adp, ADFLAGS_HF );

    }

    if (change_parent_mdate && gettimeofday(&tv, NULL) == 0) {
        newdate = AD_DATE_FROM_UNIX(tv.tv_sec);
        bitmap = 1<<FILPBIT_MDATE;
        setdirparams(vol, &Cur_Path, bitmap, (char *)&newdate);
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end setfilparams:");
#endif /* DEBUG */
    return err;
}

/*
 * renamefile and copyfile take the old and new unix pathnames
 * and the new mac name.
 *
 * src         the source path 
 * dst         the dest filename in current dir
 * newname     the dest mac name
 * adp         adouble struct of src file, if open, or & zeroed one
 *
 */
int renamefile(src, dst, newname, noadouble, adp )
char	*src, *dst, *newname;
const int         noadouble;
struct adouble    *adp;
{
    char	adsrc[ MAXPATHLEN + 1];
    int		len;
    int		rc;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin renamefile:");
#endif /* DEBUG */

    if ( unix_rename( src, dst ) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EPERM:
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        case EXDEV :			/* Cross device move -- try copy */
           /* NOTE: with open file it's an error because after the copy we will 
            * get two files, it's fixable for our process (eg reopen the new file, get the
            * locks, and so on. But it doesn't solve the case with a second process
            */
    	    if (adp->ad_df.adf_refcount || adp->ad_hf.adf_refcount) {
    	        /* FIXME  warning in syslog so admin'd know there's a conflict ?*/
    	        return AFPERR_OLOCK; /* little lie */
    	    }
            if (AFP_OK != ( rc = copyfile(src, dst, newname, noadouble )) ) {
                /* on error copyfile delete dest */
                return( rc );
            }
            return deletefile(NULL, src, 0);
        default :
            return( AFPERR_PARAM );
        }
    }

    strcpy( adsrc, ad_path( src, 0 ));

    if (unix_rename( adsrc, ad_path( dst, 0 )) < 0 ) {
        struct stat st;
        int err;
        
        err = errno;        
	if (errno == ENOENT) {
	    struct adouble    ad;

            if (stat(adsrc, &st)) /* source has no ressource fork, */
                return AFP_OK;
            
            /* We are here  because :
             * -there's no dest folder. 
             * -there's no .AppleDouble in the dest folder.
             * if we use the struct adouble passed in parameter it will not
             * create .AppleDouble if the file is already opened, so we
             * use a diff one, it's not a pb,ie it's not the same file, yet.
             */
            memset(&ad, 0, sizeof(ad));
            if (!ad_open(dst, ADFLAGS_HF, O_RDWR | O_CREAT, 0666, &ad)) {
            	ad_close(&ad, ADFLAGS_HF);
    	        if (!unix_rename( adsrc, ad_path( dst, 0 )) ) 
                   err = 0;
                else 
                   err = errno;
            }
            else { /* it's something else, bail out */
	        err = errno;
	    }
	}
	/* try to undo the data fork rename,
	 * we know we are on the same device 
	*/
	if (err) {
    	    unix_rename( dst, src ); 
    	    /* return the first error */
    	    switch ( err) {
            case ENOENT :
                return AFPERR_NOOBJ;
            case EPERM:
            case EACCES :
                return AFPERR_ACCESS ;
            case EROFS:
                return AFPERR_VLOCK;
            default :
                return AFPERR_PARAM ;
            }
        }
    }

    /* don't care if we can't open the newly renamed ressource fork
     */
    if ( !ad_open( dst, ADFLAGS_HF, O_RDWR, 0666, adp)) {
        len = strlen( newname );
        ad_setentrylen( adp, ADEID_NAME, len );
        memcpy(ad_entry( adp, ADEID_NAME ), newname, len );
        ad_flush( adp, ADFLAGS_HF );
        ad_close( adp, ADFLAGS_HF );
    }
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end renamefile:");
#endif /* DEBUG */

    return( AFP_OK );
}

int copy_path_name(char *newname, char *ibuf)
{
char        type = *ibuf;
size_t      plen = 0;
u_int16_t   len16;
u_int32_t   hint;

    if ( type != 2 && !(afp_version >= 30 && type == 3) ) {
        return -1;
    }
    ibuf++;
    switch (type) {
    case 2:
        if (( plen = (unsigned char)*ibuf++ ) != 0 ) {
            strncpy( newname, ibuf, plen );
            newname[ plen ] = '\0';
            if (strlen(newname) != plen) {
                /* there's \0 in newname, e.g. it's a pathname not
                 * only a filename. 
                */
            	return -1;
            }
        }
        break;
    case 3:
        memcpy(&hint, ibuf, sizeof(hint));
        ibuf += sizeof(hint);
           
        memcpy(&len16, ibuf, sizeof(len16));
        ibuf += sizeof(len16);
        plen = ntohs(len16);
        if (plen) {
            strncpy( newname, ibuf, plen );
            newname[ plen ] = '\0';
            if (strlen(newname) != plen) {
            	return -1;
            }
        }
        break;
    }
    return plen;
}

/* -----------------------------------
*/
int afp_copyfile(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    char	*newname, *p, *upath;
    struct path *s_path;
    u_int32_t	sdid, ddid;
    int         err, retvalue = AFP_OK;
    u_int16_t	svid, dvid;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_copyfile:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&svid, ibuf, sizeof( svid ));
    ibuf += sizeof( svid );
    if (NULL == ( vol = getvolbyvid( svid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy(&sdid, ibuf, sizeof( sdid ));
    ibuf += sizeof( sdid );
    if (NULL == ( dir = dirlookup( vol, sdid )) ) {
        return afp_errno;
    }

    memcpy(&dvid, ibuf, sizeof( dvid ));
    ibuf += sizeof( dvid );
    memcpy(&ddid, ibuf, sizeof( ddid ));
    ibuf += sizeof( ddid );

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return afp_errno;
    }
    if ( path_isadir(s_path) ) {
        return( AFPERR_BADTYPE );
    }

    /* don't allow copies when the file is open.
     * XXX: the spec only calls for read/deny write access.
     *      however, copyfile doesn't have any of that info,
     *      and locks need to stay coherent. as a result,
     *      we just balk if the file is opened already. */

    newname = obj->newtmp;
    strcpy( newname, s_path->m_name );

    if (of_findname(s_path))
        return AFPERR_DENYCONF;

    p = ctoupath( vol, curdir, newname );
    if (!p) {
        return AFPERR_PARAM;
    
    }
#ifdef FORCE_UIDGID
    /* FIXME svid != dvid && dvid's user can't read svid */
#endif
    if (NULL == ( vol = getvolbyvid( dvid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    if (NULL == ( dir = dirlookup( vol, ddid )) ) {
        return afp_errno;
    }

    if (( s_path = cname( vol, dir, &ibuf )) == NULL ) {
        return afp_errno;
    }
    if ( *s_path->m_name != '\0' ) {
        return (path_isadir( s_path))? AFPERR_PARAM:AFPERR_BADTYPE ;
    }

    /* one of the handful of places that knows about the path type */
    if (copy_path_name(newname, ibuf) < 0) {
        return( AFPERR_PARAM );
    }

    upath = mtoupath(vol, newname);
    if ( (err = copyfile(p, upath , newname, vol_noadouble(vol))) < 0 ) {
        return err;
    }
    curdir->offcnt++;

#ifdef DROPKLUDGE
    if (vol->v_flags & AFPVOL_DROPBOX) {
        retvalue=matchfile2dirperms(upath, vol, ddid); /* FIXME sdir or ddid */
    }
#endif /* DROPKLUDGE */

    setvoltime(obj, vol );

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_copyfile:");
#endif /* DEBUG */

    return( retvalue );
}


static __inline__ int copy_all(const int dfd, const void *buf,
                               size_t buflen)
{
    ssize_t cc;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin copy_all:");
#endif /* DEBUG */

    while (buflen > 0) {
        if ((cc = write(dfd, buf, buflen)) < 0) {
            switch (errno) {
            case EINTR:
                continue;
            case EDQUOT:
            case EFBIG:
            case ENOSPC:
                return AFPERR_DFULL;
            case EROFS:
                return AFPERR_VLOCK;
            default:
                return AFPERR_PARAM;
            }
        }
        buflen -= cc;
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end copy_all:");
#endif /* DEBUG */

    return AFP_OK;
}

/* -------------------------- */
static int copy_fd(int dfd, int sfd)
{
    ssize_t cc;
    int     err = AFP_OK;
    char    filebuf[8192];

#ifdef SENDFILE_FLAVOR_LINUX
    struct stat         st;

    if (fstat(sfd, &st) == 0) {
        if ((cc = sendfile(dfd, sfd, NULL, st.st_size)) < 0) {
            switch (errno) {
            case EINVAL:  /* there's no guarantee that all fs support sendfile */
                break;
            case EDQUOT:
            case EFBIG:
            case ENOSPC:
                return AFPERR_DFULL;
            case EROFS:
                return AFPERR_VLOCK;
            default:
                return AFPERR_PARAM;
            }
        }
        else {
           return AFP_OK;
        }
    }
#endif /* SENDFILE_FLAVOR_LINUX */

    while (1) {
        if ((cc = read(sfd, filebuf, sizeof(filebuf))) < 0) {
            if (errno == EINTR)
                continue;
            err = AFPERR_PARAM;
            break;
        }

        if (!cc || ((err = copy_all(dfd, filebuf, cc)) < 0))
            break;
    }
    return err;
}

/* ----------------------------------
 * if newname is NULL (from directory.c) we don't want to copy ressource fork.
 * because we are doing it elsewhere.
 */
int copyfile(src, dst, newname, noadouble )
char	*src, *dst, *newname;
const int   noadouble;
{
    struct adouble	ads, add;
    int			len, err = AFP_OK;
    int                 adflags;
    
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin copyfile:");
#endif /* DEBUG */

    memset(&ads, 0, sizeof(ads));
    memset(&add, 0, sizeof(add));
    adflags = ADFLAGS_DF;
    if (newname) {
        adflags |= ADFLAGS_HF;
    }

    if (ad_open(src , adflags | ADFLAGS_NOHF, O_RDONLY, 0, &ads) < 0) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }
    if (ad_open(dst , adflags | noadouble, O_RDWR|O_CREAT|O_EXCL, 0666, &add) < 0) {
        ad_close( &ads, adflags );
        if (EEXIST != (err = errno)) {
            deletefile(NULL, dst, 0);
        }
        switch ( err ) {
        case EEXIST :
            return AFPERR_EXIST;
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        default :
            return( AFPERR_PARAM );
        }
    }
    if (ad_hfileno(&ads) == -1 || AFP_OK == (err = copy_fd(ad_hfileno(&add), ad_hfileno(&ads)))){
        /* copy the data fork */
	err = copy_fd(ad_dfileno(&add), ad_dfileno(&ads));
    }

    if (newname) {
        len = strlen( newname );
        ad_setentrylen( &add, ADEID_NAME, len );
        memcpy(ad_entry( &add, ADEID_NAME ), newname, len );
    }

    ad_close( &ads, adflags );
    ad_flush( &add, adflags );
    if (ad_close( &add, adflags ) <0) {
       err = errno;
    }
    if (err != AFP_OK) {
        deletefile(NULL, dst, 0);
        switch ( err ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end copyfile:");
#endif /* DEBUG */

    return( AFP_OK );
}


/* -----------------------------------
   vol: not NULL delete cnid entry. then we are in curdir and file is a only filename
   checkAttrib:   1 check kFPDeleteInhibitBit (deletfile called by afp_delete)

   when deletefile is called we don't have lock on it, file is closed (for us)
   untrue if called by renamefile
   
   ad_open always try to open file RDWR first and ad_lock takes care of
   WRITE lock on read only file.
*/
int deletefile( vol, file, checkAttrib )
struct vol      *vol;
char		*file;
int         checkAttrib;
{
    struct adouble	ad;
    int			adflags, err = AFP_OK;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin deletefile:");
#endif /* DEBUG */

    /* try to open both forks at once */
    adflags = ADFLAGS_DF|ADFLAGS_HF;
    while(1) {
        memset(&ad, 0, sizeof(ad));
        if ( ad_open( file, adflags, O_RDONLY, 0, &ad ) < 0 ) {
            switch (errno) {
            case ENOENT:
                if (adflags == ADFLAGS_DF)
                    return AFPERR_NOOBJ;
                   
                /* that failed. now try to open just the data fork */
                adflags = ADFLAGS_DF;
                continue;

            case EACCES:
                return AFPERR_ACCESS;
            case EROFS:
                return AFPERR_VLOCK;
            default:
                return( AFPERR_PARAM );
            }
        }
        break;	/* from the while */
    }
    /*
     * Does kFPDeleteInhibitBit (bit 8) set?
     */
    if (checkAttrib && (adflags & ADFLAGS_HF)) {
        u_int16_t   bshort;

        ad_getattr(&ad, &bshort);
        if ((bshort & htons(ATTRBIT_NODELETE))) {
            ad_close( &ad, adflags );
            return(AFPERR_OLOCK);
        }
    }
    
    if ((adflags & ADFLAGS_HF) ) {
        /* FIXME we have a pb here because we want to know if a file is open 
         * there's a 'priority inversion' if you can't open the ressource fork RW
         * you can delete it if it's open because you can't get a write lock.
         * 
         * ADLOCK_FILELOCK means the whole ressource fork, not only after the 
         * metadatas
         *
         * FIXME it doesn't work for RFORK open read only and fork open without deny mode
         */
        if (ad_tmplock(&ad, ADEID_RFORK, ADLOCK_WR |ADLOCK_FILELOCK, 0, 0, 0) < 0 ) {
            ad_close( &ad, adflags );
            return( AFPERR_BUSY );
        }
    }

    if (ad_tmplock( &ad, ADEID_DFORK, ADLOCK_WR, 0, 0, 0 ) < 0) {
        err = AFPERR_BUSY;
    }
    else if (!(err = netatalk_unlink( ad_path( file, ADFLAGS_HF)) ) &&
             !(err = netatalk_unlink( file )) ) {
#ifdef CNID_DB /* get rid of entry */
        cnid_t id;
        if (vol && (id = cnid_get(vol->v_db, curdir->d_did, file, strlen(file)))) 
        {
            cnid_delete(vol->v_db, id);
        }
#endif /* CNID_DB */

    }
    ad_close( &ad, adflags );  /* ad_close removes locks if any */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end deletefile:");
#endif /* DEBUG */

    return err;
}

/* ------------------------------------ */
#ifdef CNID_DB
/* return a file id */
int afp_createid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         *st;
#if AD_VERSION > AD_VERSION1
    struct adouble	ad;
#endif
    struct vol		*vol;
    struct dir		*dir;
    char		*upath;
    int                 len;
    cnid_t		did, id;
    u_short		vid;
    struct path         *s_path;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_createid:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM);
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof(did);

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return( AFPERR_PARAM );
    }

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return afp_errno; /* was AFPERR_PARAM */
    }

    if ( path_isadir(s_path) ) {
        return( AFPERR_BADTYPE );
    }

    upath = s_path->u_name;
    switch (s_path->st_errno) {
        case 0:
             break; /* success */
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        case ENOENT:
            return AFPERR_NOOBJ;
        default:
            return AFPERR_PARAM;
    }
    st = &s_path->st;
    if ((id = cnid_lookup(vol->v_db, st, did, upath, len = strlen(upath)))) {
        memcpy(rbuf, &id, sizeof(id));
        *rbuflen = sizeof(id);
        return AFPERR_EXISTID;
    }

#if AD_VERSION > AD_VERSION1
    memset(&ad, 0, sizeof(ad));
    if (ad_open( upath, ADFLAGS_HF, O_RDONLY, 0, &ad ) >= 0) {
        memcpy(&id, ad_entry(&ad, ADEID_DID), sizeof(id));
        ad_close(&ad, ADFLAGS_HF);
    }
#endif /* AD_VERSION > AD_VERSION1 */

    if ((id = cnid_add(vol->v_db, st, did, upath, len, id)) != CNID_INVALID) {
        memcpy(rbuf, &id, sizeof(id));
        *rbuflen = sizeof(id);
        return AFP_OK;
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "ending afp_createid...:");
#endif /* DEBUG */

    switch (errno) {
    case EROFS:
        return AFPERR_VLOCK;
        break;
    case EPERM:
    case EACCES:
        return AFPERR_ACCESS;
        break;
    default:
        LOG(log_error, logtype_afpd, "afp_createid: cnid_add: %s", strerror(errno));
        return AFPERR_PARAM;
    }
}

/* ------------------------------
   resolve a file id */
int afp_resolveid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct dir		*dir;
    char		*upath;
    struct path         path;
    int                 err, buflen;
    cnid_t		id;
    u_int16_t		vid, bitmap;

    static char buffer[12 + MAXPATHLEN + 1];
    int len = 12 + MAXPATHLEN + 1;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_resolveid:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM);
    }

    memcpy(&id, ibuf, sizeof( id ));
    ibuf += sizeof(id);

    if (NULL == (upath = cnid_resolve(vol->v_db, &id, buffer, len)) ) {
        return AFPERR_NOID; /* was AFPERR_BADID, but help older Macs */
    }

    if (NULL == ( dir = dirlookup( vol, id )) ) {
        return AFPERR_NOID; /* idem AFPERR_PARAM */
    }
    path.u_name = upath;
    if (movecwd(vol, dir) < 0 || of_stat(&path) < 0) {
        switch (errno) {
        case EACCES:
        case EPERM:
            return AFPERR_ACCESS;
        case ENOENT:
            return AFPERR_NOID;
        default:
            return AFPERR_PARAM;
        }
    }
    /* directories are bad */
    if (S_ISDIR(path.st.st_mode))
        return AFPERR_BADTYPE;

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs( bitmap );
    path.m_name = utompath(vol, upath);
    if (AFP_OK != (err = getfilparams(vol, bitmap, &path , curdir, 
                            rbuf + sizeof(bitmap), &buflen))) {
        return err;
    }
    *rbuflen = buflen + sizeof(bitmap);
    memcpy(rbuf, ibuf, sizeof(bitmap));

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_resolveid:");
#endif /* DEBUG */

    return AFP_OK;
}

/* ------------------------------ */
int afp_deleteid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         st;
    struct vol		*vol;
    struct dir		*dir;
    char                *upath;
    int                 err;
    cnid_t		id;
    cnid_t		fileid;
    u_short		vid;
    static char buffer[12 + MAXPATHLEN + 1];
    int len = 12 + MAXPATHLEN + 1;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_deleteid:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM);
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&id, ibuf, sizeof( id ));
    ibuf += sizeof(id);
    fileid = id;

    if (NULL == (upath = cnid_resolve(vol->v_db, &id, buffer, len)) ) {
        return AFPERR_NOID;
    }

    if (NULL == ( dir = dirlookup( vol, id )) ) {
        return( AFPERR_PARAM );
    }

    err = AFP_OK;
    if ((movecwd(vol, dir) < 0) || (stat(upath, &st) < 0)) {
        switch (errno) {
        case EACCES:
        case EPERM:
            return AFPERR_ACCESS;
        case ENOENT:
            /* still try to delete the id */
            err = AFPERR_NOOBJ;
            break;
        default:
            return AFPERR_PARAM;
        }
    }
    else if (S_ISDIR(st.st_mode)) /* directories are bad */
        return AFPERR_BADTYPE;

    if (cnid_delete(vol->v_db, fileid)) {
        switch (errno) {
        case EROFS:
            return AFPERR_VLOCK;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        default:
            return AFPERR_PARAM;
        }
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_deleteid:");
#endif /* DEBUG */

    return err;
}
#endif /* CNID_DB */

#define APPLETEMP ".AppleTempXXXXXX"

int afp_exchangefiles(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         srcst, destst;
    struct vol		*vol;
    struct dir		*dir, *sdir;
    char		*spath, temp[17], *p;
    char                *supath, *upath;
    struct path         *path;
    int                 err;
    struct adouble	ads;
    struct adouble	add;
    struct adouble	*adsp;
    struct adouble	*addp;
    struct ofork	*s_of;
    struct ofork	*d_of;
    int                 crossdev;
    
#ifdef CNID_DB
    int                 slen, dlen;
#endif /* CNID_DB */
    u_int32_t		sid, did;
    u_int16_t		vid;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_exchangefiles:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM);
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    /* source and destination dids */
    memcpy(&sid, ibuf, sizeof(sid));
    ibuf += sizeof(sid);
    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(did);

    /* source file */
    if (NULL == (dir = dirlookup( vol, sid )) ) {
        return( AFPERR_PARAM );
    }

    if (NULL == ( path = cname( vol, dir, &ibuf )) ) {
        return afp_errno; /* was AFPERR_PARAM */
    }

    if ( path_isadir(path) ) {
        return( AFPERR_BADTYPE );   /* it's a dir */
    }

    upath = path->u_name;
    switch (path->st_errno) {
        case 0:
             break;
        case ENOENT:
            return AFPERR_NOID;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        default:
            return AFPERR_PARAM;
    }
    memset(&ads, 0, sizeof(ads));
    adsp = &ads;
    if ((s_of = of_findname(path))) {
            /* reuse struct adouble so it won't break locks */
            adsp = s_of->of_ad;
    }
    memcpy(&srcst, &path->st, sizeof(struct stat));
    /* save some stuff */
    sdir = curdir;
    spath = obj->oldtmp;
    supath = obj->newtmp;
    strcpy(spath, path->m_name);
    strcpy(supath, upath); /* this is for the cnid changing */
    p = absupath( vol, sdir, upath);
    if (!p) {
        /* pathname too long */
        return AFPERR_PARAM ;
    }

    /* look for the source cnid. if it doesn't exist, don't worry about
     * it. */
#ifdef CNID_DB
    sid = cnid_lookup(vol->v_db, &srcst, sdir->d_did, supath,
                      slen = strlen(supath));
#endif /* CNID_DB */

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return( AFPERR_PARAM );
    }

    if (NULL == ( path = cname( vol, dir, &ibuf )) ) {
        return( AFPERR_PARAM );
    }

    if ( path_isadir(path) ) {
        return( AFPERR_BADTYPE );
    }

    /* FPExchangeFiles is the only call that can return the SameObj
     * error */
    if ((curdir == sdir) && strcmp(spath, path->m_name) == 0)
        return AFPERR_SAMEOBJ;

    switch (path->st_errno) {
        case 0:
             break;
        case ENOENT:
            return AFPERR_NOID;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        default:
            return AFPERR_PARAM;
    }
    memset(&add, 0, sizeof(add));
    addp = &add;
    if ((d_of = of_findname( path))) {
            /* reuse struct adouble so it won't break locks */
            addp = d_of->of_ad;
    }
    memcpy(&destst, &path->st, sizeof(struct stat));

    /* they are not on the same device and at least one is open
    */
    crossdev = (srcst.st_dev != destst.st_dev);
    if ((d_of || s_of)  && crossdev)
        return AFPERR_MISC;
    
    upath = path->u_name;
#ifdef CNID_DB
    /* look for destination id. */
    did = cnid_lookup(vol->v_db, &destst, curdir->d_did, upath,
                      dlen = strlen(upath));
#endif /* CNID_DB */

    /* construct a temp name.
     * NOTE: the temp file will be in the dest file's directory. it
     * will also be inaccessible from AFP. */
    memcpy(temp, APPLETEMP, sizeof(APPLETEMP));
    if (!mktemp(temp))
        return AFPERR_MISC;

    /* now, quickly rename the file. we error if we can't. */
    if ((err = renamefile(p, temp, temp, vol_noadouble(vol), adsp)) < 0)
        goto err_exchangefile;
    of_rename(vol, s_of, sdir, spath, curdir, temp);

    /* rename destination to source */
    if ((err = renamefile(upath, p, spath, vol_noadouble(vol), addp)) < 0)
        goto err_src_to_tmp;
    of_rename(vol, d_of, curdir, path->m_name, sdir, spath);

    /* rename temp to destination */
    if ((err = renamefile(temp, upath, path->m_name, vol_noadouble(vol), adsp)) < 0)
        goto err_dest_to_src;
    of_rename(vol, s_of, curdir, temp, curdir, path->m_name);

#ifdef CNID_DB
    /* id's need switching. src -> dest and dest -> src. 
     * we need to re-stat() if it was a cross device copy.
    */
    if (sid) {
	cnid_delete(vol->v_db, sid);
    }
    if (did) {
	cnid_delete(vol->v_db, did);
    }
    if ((did && ( (crossdev && stat( upath, &srcst) < 0) || 
                cnid_update(vol->v_db, did, &srcst, curdir->d_did,upath, dlen) < 0))
       ||
       (sid && ( (crossdev && stat(p, &destst) < 0) ||
                cnid_update(vol->v_db, sid, &destst, sdir->d_did,supath, slen) < 0))
    ) {
        switch (errno) {
        case EPERM:
        case EACCES:
            err = AFPERR_ACCESS;
            break;
        default:
            err = AFPERR_PARAM;
        }
        goto err_temp_to_dest;
    }
#endif /* CNID_DB */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "ending afp_exchangefiles:");
#endif /* DEBUG */

    return AFP_OK;


    /* all this stuff is so that we can unwind a failed operation
     * properly. */
#ifdef CNID_DB
err_temp_to_dest:
#endif
    /* rename dest to temp */
    renamefile(upath, temp, temp, vol_noadouble(vol), adsp);
    of_rename(vol, s_of, curdir, upath, curdir, temp);

err_dest_to_src:
    /* rename source back to dest */
    renamefile(p, upath, path->m_name, vol_noadouble(vol), addp);
    of_rename(vol, d_of, sdir, spath, curdir, path->m_name);

err_src_to_tmp:
    /* rename temp back to source */
    renamefile(temp, p, spath, vol_noadouble(vol), adsp);
    of_rename(vol, s_of, curdir, temp, sdir, spath);

err_exchangefile:
    return err;
}
