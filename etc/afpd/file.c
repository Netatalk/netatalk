/*
 * $Id: file.c,v 1.43 2002-03-24 17:43:39 jmarcus Exp $
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

/* check for mtab DID code */
#ifdef DID_MTAB
#include "parse_mtab.h"
#endif /* DID_MTAB */

#ifdef FORCE_UIDGID
#warning UIDGID
#include "uid.h"
#endif /* FORCE_UIDGID */

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

int getfilparams(struct vol *vol,
                 u_int16_t bitmap,
                 char *path, struct dir *dir, struct stat *st,
                 char *buf, int *buflen )
{
#ifndef USE_LASTDID
    struct stat		hst, lst, *lstp;
#else /* USE_LASTDID */
    struct stat		hst;
#endif /* USE_LASTDID */
    struct adouble	ad, *adp;
    struct ofork        *of;
    struct extmap	*em;
    char		*data, *nameoff = NULL, *upath;
    int			bit = 0, isad = 1;
    u_int32_t		aint;
    u_int16_t		ashort;
    u_char              achar, fdType[4];

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin getfilparams:");
#endif /* DEBUG */

    upath = mtoupath(vol, path);
    if ((of = of_findname(vol, curdir, path))) {
        adp = of->of_ad;
    } else {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    }

    if ( ad_open( upath, ADFLAGS_HF, O_RDONLY, 0, adp) < 0 ) {
        isad = 0;
    } else if ( fstat( ad_hfileno( adp ), &hst ) < 0 ) {
        LOG(log_error, logtype_afpd, "getfilparams fstat: %s", strerror(errno) );
    }

    data = buf;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch ( bit ) {
        case FILPBIT_ATTR :
            if ( isad ) {
                ad_getattr(adp, &ashort);
            } else if (*upath == '.') {
                ashort = htons(ATTRBIT_INVISIBLE);
            } else
                ashort = 0;
            memcpy(data, &ashort, sizeof( ashort ));
            data += sizeof( u_short );
            break;

        case FILPBIT_PDID :
            memcpy(data, &dir->d_did, sizeof( u_int32_t ));
            data += sizeof( u_int32_t );
            break;

        case FILPBIT_CDATE :
            if (!isad || (ad_getdate(adp, AD_DATE_CREATE, &aint) < 0))
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_MDATE :
            if ( isad && (ad_getdate(adp, AD_DATE_MODIFY, &aint) == 0)) {
                if ((st->st_mtime > AD_DATE_TO_UNIX(aint)) &&
                        (hst.st_mtime < st->st_mtime)) {
                    aint = AD_DATE_FROM_UNIX(st->st_mtime);
                }
            } else {
                aint = AD_DATE_FROM_UNIX(st->st_mtime);
            }
            memcpy(data, &aint, sizeof( int ));
            data += sizeof( int );
            break;

        case FILPBIT_BDATE :
            if (!isad || (ad_getdate(adp, AD_DATE_BACKUP, &aint) < 0))
                aint = AD_DATE_START;
            memcpy(data, &aint, sizeof( int ));
            data += sizeof( int );
            break;

        case FILPBIT_FINFO :
            if (isad)
                memcpy(data, ad_entry(adp, ADEID_FINDERI), 32);
            else {
                memcpy(data, ufinderi, 32);
                if (*upath == '.') { /* make it invisible */
                    ashort = htons(FINDERINFO_INVISIBLE);
                    memcpy(data + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
                }
            }

            if ((!isad || (memcmp(ad_entry(adp, ADEID_FINDERI),
                                  ufinderi, 8 ) == 0)) &&
                    (em = getextmap( path ))) {
                memcpy(data, em->em_type, sizeof( em->em_type ));
                memcpy(data + 4, em->em_creator, sizeof(em->em_creator));
            }
            data += 32;
            break;

        case FILPBIT_LNAME :
            nameoff = data;
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
            if (isad)
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
#ifdef DID_MTAB
                aint = htonl( afpd_st_cnid ( lstp ) );
#else /* DID_MTAB */
                aint = htonl(CNID(lstp, 1));
#endif /* DID_MTAB */
#endif /* USE_LASTDID */
            }

            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_DFLEN :
            aint = htonl( st->st_size );
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case FILPBIT_RFLEN :
            if ( isad ) {
                aint = htonl( ad_getentrylen( adp, ADEID_RFORK ));
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
               <shirsch@ibm.net> */
        case FILPBIT_PDINFO :
            if ( isad ) {
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
            break;

        default :
            if ( isad ) {
                ad_close( adp, ADFLAGS_HF );
            }
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( nameoff ) {
        ashort = htons( data - buf );
        memcpy(nameoff, &ashort, sizeof( ashort ));
        if ((aint = strlen( path )) > MACFILELEN)
            aint = MACFILELEN;
        *data++ = aint;
        memcpy(data, path, aint );
        data += aint;
    }
    if ( isad ) {
        ad_close( adp, ADFLAGS_HF );
    }
    *buflen = data - buf;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end getfilparams:");
#endif /* DEBUG */

    return( AFP_OK );
}

int afp_createfile(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         st;
    struct adouble	ad, *adp;
    struct vol		*vol;
    struct dir		*dir;
    struct ofork        *of;
    char		*path, *upath;
    int			creatf, did, openf, retvalue = AFP_OK;
    u_int16_t		vid;
#ifdef FORCE_UIDGID
    uidgidset		*uidgid;
#endif /* FORCE_UIDGID */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_createfile:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf++;
    creatf = (unsigned char) *ibuf++;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did));
    ibuf += sizeof( did );

    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    upath = mtoupath(vol, path);

    /* check for illegal bits in the unix filename */
    if (!wincheck(vol, upath))
        return AFPERR_PARAM;

    if ((vol->v_flags & AFPVOL_NOHEX) && strchr(upath, '/'))
        return AFPERR_PARAM;

    if (!validupath(vol, upath))
        return AFPERR_EXIST;

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, upath))
        return AFPERR_EXIST;

    if ((of = of_findname(vol, curdir, path))) {
        adp = of->of_ad;
    } else {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    }
    if ( creatf) {
        /* on a hard create, fail if file exists and is open */
        if ((stat(upath, &st) == 0) && of)
            return AFPERR_BUSY;
        openf = O_RDWR|O_CREAT|O_TRUNC;
    } else {
        openf = O_RDWR|O_CREAT|O_EXCL;
    }

#ifdef FORCE_UIDGID

    /* preserve current euid, egid */
    save_uidgid ( uidgid );

    /* perform all switching of users */
    set_uidgid ( vol );

#endif /* FORCE_UIDGID */

    if ( ad_open( upath, vol_noadouble(vol)|ADFLAGS_DF|ADFLAGS_HF,
                  openf, 0666, adp) < 0 ) {
        switch ( errno ) {
        case EEXIST :
#ifdef FORCE_UIDGID
            /* bring everything back to old euid, egid */
            restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_EXIST );
        case EACCES :
#ifdef FORCE_UIDGID
            /* bring everything back to old euid, egid */
            restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_ACCESS );
        case ENOENT:
            /* on noadouble volumes, just creating the data fork is ok */
            if (vol_noadouble(vol) && (stat(upath, &st) == 0))
                goto createfile_done;
            /* fallthrough */
        default :
#ifdef FORCE_UIDGID
            /* bring everything back to old euid, egid */
            restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */
            return( AFPERR_PARAM );
        }
    }

    ad_setentrylen( adp, ADEID_NAME, strlen( path ));
    memcpy(ad_entry( adp, ADEID_NAME ), path,
           ad_getentrylen( adp, ADEID_NAME ));
    ad_flush( adp, ADFLAGS_DF|ADFLAGS_HF );
    ad_close( adp, ADFLAGS_DF|ADFLAGS_HF );

createfile_done:

#ifdef DROPKLUDGE
    if (vol->v_flags & AFPVOL_DROPBOX) {
        retvalue = matchfile2dirperms(upath, vol, did);
    }
#endif /* DROPKLUDGE */

    setvoltime(obj, vol );

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_createfile");
#endif /* DEBUG */

#ifdef FORCE_UIDGID
    /* bring everything back to old euid, egid */
    restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */

    return (retvalue);
}

int afp_setfilparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    char	*path;
    int		did, rc;
    u_int16_t	vid, bitmap;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_setfilparams:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    if (( rc = setfilparams(vol, path, bitmap, ibuf )) == AFP_OK ) {
        setvoltime(obj, vol );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_setfilparams:");
#endif /* DEBUG */

    return( rc );
}


int setfilparams(struct vol *vol,
                 char *path, u_int16_t bitmap, char *buf )
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

#ifdef FORCE_UIDGID
    uidgidset		*uidgid;

    uidgid = malloc(sizeof(uidgidset));
#endif /* FORCE_UIDGID */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin setfilparams:");
#endif /* DEBUG */

    upath = mtoupath(vol, path);
    if ((of = of_findname(vol, curdir, path))) {
        adp = of->of_ad;
    } else {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    }

#ifdef FORCE_UIDGID
    save_uidgid ( uidgid );
    set_uidgid ( vol );
#endif /* FORCE_UIDGID */

    if (ad_open( upath, vol_noadouble(vol) | ADFLAGS_HF,
                 O_RDWR|O_CREAT, 0666, adp) < 0) {
        /* for some things, we don't need an adouble header */
        if (bitmap & ~(1<<FILPBIT_MDATE)) {
#ifdef FORCE_UIDGID
            restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */
            return vol_noadouble(vol) ? AFP_OK : AFPERR_ACCESS;
        }
        isad = 0;
    } else if ((ad_getoflags( adp, ADFLAGS_HF ) & O_CREAT) ) {
        ad_setentrylen( adp, ADEID_NAME, strlen( path ));
        memcpy(ad_entry( adp, ADEID_NAME ), path,
               ad_getentrylen( adp, ADEID_NAME ));
    }

    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch(  bit ) {
        case FILPBIT_ATTR :
            memcpy(&ashort, buf, sizeof( ashort ));
            ad_getattr(adp, &bshort);
            if ( ntohs( ashort ) & ATTRBIT_SETCLR ) {
                bshort |= htons( ntohs( ashort ) & ~ATTRBIT_SETCLR );
            } else {
                bshort &= ~ashort;
            }
            ad_setattr(adp, bshort);
            buf += sizeof( ashort );
            break;

        case FILPBIT_CDATE :
            memcpy(&aint, buf, sizeof(aint));
            ad_setdate(adp, AD_DATE_CREATE, aint);
            buf += sizeof( aint );
            break;

        case FILPBIT_MDATE :
            memcpy(&aint, buf, sizeof( aint ));
            if (isad)
                ad_setdate(adp, AD_DATE_MODIFY, aint);
            ut.actime = ut.modtime = AD_DATE_TO_UNIX(aint);
            utime(upath, &ut);
            buf += sizeof( aint );
            break;

        case FILPBIT_BDATE :
            memcpy(&aint, buf, sizeof(aint));
            ad_setdate(adp, AD_DATE_BACKUP, aint);
            buf += sizeof( aint );
            break;

        case FILPBIT_FINFO :
            if ((memcmp( ad_entry( adp, ADEID_FINDERI ), ufinderi, 8 ) == 0)
                    && (em = getextmap( path )) &&
                    (memcmp(buf, em->em_type, sizeof( em->em_type )) == 0) &&
                    (memcmp(buf + 4, em->em_creator,
                            sizeof( em->em_creator )) == 0)) {
                memcpy(buf, ufinderi, 8 );
            }
            memcpy(ad_entry( adp, ADEID_FINDERI ), buf, 32 );
            buf += 32;
            break;

            /* Client needs to set the ProDOS file info for this file.
               Use defined strings for the simple cases, and convert
               all else into pXYY per Inside Appletalk.  Always set
               the creator as "pdos". <shirsch@ibm.net> */
        case FILPBIT_PDINFO :
            achar = *buf;
            buf += 2;
            memcpy(&ashort, buf, sizeof( ashort ));
            ashort = ntohs( ashort );
            buf += 2;

            switch ( (unsigned int) achar )
            {
            case 0x04 :
                fdType = ( u_char *) "TEXT";
                break;

            case 0xff :
                fdType = ( u_char *) "PSYS";
                break;

            case 0xb3 :
                fdType = ( u_char *) "PS16";
                break;

            case 0x00 :
                fdType = ( u_char *) "BINA";
                break;

            default :
                xyy[0] = ( u_char ) 'p';
                xyy[1] = achar;
                xyy[2] = ( u_char ) ( ashort >> 8 ) & 0xff;
                xyy[3] = ( u_char ) ashort & 0xff;
                fdType = xyy;
                break;
            }

            memcpy(ad_entry( adp, ADEID_FINDERI ), fdType, 4 );
            memcpy(ad_entry( adp, ADEID_FINDERI ) + 4, "pdos", 4 );
            break;


        default :
            err = AFPERR_BITMAP;
            goto setfilparam_done;
        }

        bitmap = bitmap>>1;
        bit++;
    }

setfilparam_done:
    if (isad) {
        ad_flush( adp, ADFLAGS_HF );
        ad_close( adp, ADFLAGS_HF );

#ifdef FORCE_UIDGID
        restore_uidgid ( uidgid );
#endif /* FORCE_UIDGID */

    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end setfilparams:");
#endif /* DEBUG */

    return err;
}

/*
 * renamefile and copyfile take the old and new unix pathnames
 * and the new mac name.
 * NOTE: if we have to copy a file instead of renaming it, locks
 *       will break.
 */
int renamefile(src, dst, newname, noadouble )
char	*src, *dst, *newname;
const int         noadouble;
{
    struct adouble	ad;
    char		adsrc[ MAXPATHLEN + 1];
    int			len, rc;

    /*
     * Note that this is only checking the existance of the data file,
     * not the header file.  The thinking is that if the data file doesn't
     * exist, but the header file does, the right thing to do is remove
     * the data file silently.
     */

    /* existence check moved to afp_moveandrename */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin renamefile:");
#endif /* DEBUG */

    if ( rename( src, dst ) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EPERM:
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        case EXDEV :			/* Cross device move -- try copy */
            if (( rc = copyfile(src, dst, newname, noadouble )) != AFP_OK ) {
                deletefile( dst, 0 );
                return( rc );
            }
            return deletefile( src, 0);
        default :
            return( AFPERR_PARAM );
        }
    }

    strcpy( adsrc, ad_path( src, 0 ));
    rc = 0;
rename_retry:
    if (rename( adsrc, ad_path( dst, 0 )) < 0 ) {
        struct stat st;

        switch ( errno ) {
        case ENOENT :
            /* check for a source appledouble header. if it exists, make
             * a dest appledouble directory and do the rename again. */
            memset(&ad, 0, sizeof(ad));
            if (rc || stat(adsrc, &st) ||
                    (ad_open(dst, ADFLAGS_HF, O_RDWR | O_CREAT, 0666, &ad) < 0))
                return AFP_OK;
            rc++;
            ad_close(&ad, ADFLAGS_HF);
            goto rename_retry;
        case EPERM:
        case EACCES :
            return( AFPERR_ACCESS );
        case EROFS:
            return AFPERR_VLOCK;
        default :
            return( AFPERR_PARAM );
        }
    }

    memset(&ad, 0, sizeof(ad));
    if ( ad_open( dst, ADFLAGS_HF, O_RDWR, 0666, &ad) < 0 ) {
        switch ( errno ) {
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

    len = strlen( newname );
    ad_setentrylen( &ad, ADEID_NAME, len );
    memcpy(ad_entry( &ad, ADEID_NAME ), newname, len );
    ad_flush( &ad, ADFLAGS_HF );
    ad_close( &ad, ADFLAGS_HF );

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end renamefile:");
#endif /* DEBUG */

    return( AFP_OK );
}

int afp_copyfile(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*dir;
    char	*newname, *path, *p;
    u_int32_t	sdid, ddid;
    int		plen, err, retvalue = AFP_OK;
    u_int16_t	svid, dvid;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_copyfile:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&svid, ibuf, sizeof( svid ));
    ibuf += sizeof( svid );
    if (( vol = getvolbyvid( svid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy(&sdid, ibuf, sizeof( sdid ));
    ibuf += sizeof( sdid );
    if (( dir = dirsearch( vol, sdid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy(&dvid, ibuf, sizeof( dvid ));
    ibuf += sizeof( dvid );
    memcpy(&ddid, ibuf, sizeof( ddid ));
    ibuf += sizeof( ddid );

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }
    if ( *path == '\0' ) {
        return( AFPERR_BADTYPE );
    }

    /* don't allow copies when the file is open.
     * XXX: the spec only calls for read/deny write access.
     *      however, copyfile doesn't have any of that info,
     *      and locks need to stay coherent. as a result,
     *      we just balk if the file is opened already. */
    if (of_findname(vol, curdir, path))
        return AFPERR_DENYCONF;

    newname = obj->newtmp;
    strcpy( newname, path );

    p = ctoupath( vol, curdir, newname );

    if (( vol = getvolbyvid( dvid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    if (( dir = dirsearch( vol, ddid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }
    if ( *path != '\0' ) {
        return( AFPERR_BADTYPE );
    }

    /* one of the handful of places that knows about the path type */
    if ( *ibuf++ != 2 ) {
        return( AFPERR_PARAM );
    }
    if (( plen = (unsigned char)*ibuf++ ) != 0 ) {
        strncpy( newname, ibuf, plen );
        newname[ plen ] = '\0';
    }

    if ( (err = copyfile(p, mtoupath(vol, newname ), newname,
                         vol_noadouble(vol))) < 0 ) {
        return err;
    }

    setvoltime(obj, vol );

#ifdef DROPKLUDGE
    if (vol->v_flags & AFPVOL_DROPBOX) {
        retvalue=matchfile2dirperms(newname, vol, sdid);
    }
#endif /* DROPKLUDGE */

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

/* XXX: this needs to use ad_open and ad_lock. so, we need to
 * pass in vol and path */
int copyfile(src, dst, newname, noadouble )
char	*src, *dst, *newname;
const int   noadouble;
{
    struct adouble	ad;
    struct stat         st;
    char		filebuf[8192];
    int			sfd, dfd, len, err = AFP_OK;
    ssize_t             cc;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin copyfile:");
#endif /* DEBUG */

    if (newname) {
        if ((sfd = open( ad_path( src, ADFLAGS_HF ), O_RDONLY, 0 )) < 0 ) {
            switch ( errno ) {
            case ENOENT :
                break; /* just copy the data fork */
            case EACCES :
                return( AFPERR_ACCESS );
            default :
                return( AFPERR_PARAM );
            }
        } else {
            if (( dfd = open( ad_path( dst, ADFLAGS_HF ), O_WRONLY|O_CREAT,
                              ad_mode( ad_path( dst, ADFLAGS_HF ), 0666 ))) < 0 ) {
                close( sfd );
                switch ( errno ) {
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

            /* copy the file */
#ifdef SENDFILE_FLAVOR_LINUX
            if (fstat(sfd, &st) == 0) {
                if ((cc = sendfile(dfd, sfd, NULL, st.st_size)) < 0) {
                    switch (errno) {
                    case EDQUOT:
                    case EFBIG:
                    case ENOSPC:
                        err = AFPERR_DFULL;
                        break;
                    case EROFS:
                        err = AFPERR_VLOCK;
                        break;
                    default:
                        err = AFPERR_PARAM;
                    }
                }
                goto copyheader_done;
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

copyheader_done:
            close(sfd);
            close(dfd);
            if (err < 0) {
                unlink(ad_path(dst, ADFLAGS_HF));
                return err;
            }
        }
    }

    /* data fork copying */
    if (( sfd = open( src, O_RDONLY, 0 )) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOOBJ );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }

    if (( dfd = open( dst, O_WRONLY|O_CREAT, ad_mode( dst, 0666 ))) < 0 ) {
        close( sfd );
        switch ( errno ) {
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

#ifdef SENDFILE_FLAVOR_LINUX
    if (fstat(sfd, &st) == 0) {
        if ((cc = sendfile(dfd, sfd, NULL, st.st_size)) < 0) {
            switch (errno) {
            case EDQUOT:
            case EFBIG:
            case ENOSPC:
                err = AFPERR_DFULL;
                break;
            default:
                err = AFPERR_PARAM;
            }
        }
        goto copydata_done;
    }
#endif /* SENDFILE_FLAVOR_LINUX */

    while (1) {
        if ((cc = read( sfd, filebuf, sizeof( filebuf ))) < 0) {
            if (errno == EINTR)
                continue;

            err = AFPERR_PARAM;
            break;
        }

        if (!cc || ((err = copy_all(dfd, filebuf, cc)) < 0)) {
            break;
        }
    }

copydata_done:
    close(sfd);
    close(dfd);
    if (err < 0) {
        unlink(ad_path(dst, ADFLAGS_HF));
        unlink(dst);
        return err;
    }

    if (newname) {
        memset(&ad, 0, sizeof(ad));
        if ( ad_open( dst, noadouble | ADFLAGS_HF, O_RDWR|O_CREAT,
                      0666, &ad) < 0 ) {
            switch ( errno ) {
            case ENOENT :
                return noadouble ? AFP_OK : AFPERR_NOOBJ;
            case EACCES :
                return( AFPERR_ACCESS );
            case EROFS:
                return AFPERR_VLOCK;
            default :
                return( AFPERR_PARAM );
            }
        }

        len = strlen( newname );
        ad_setentrylen( &ad, ADEID_NAME, len );
        memcpy(ad_entry( &ad, ADEID_NAME ), newname, len );
        ad_flush( &ad, ADFLAGS_HF );
        ad_close( &ad, ADFLAGS_HF );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end copyfile:");
#endif /* DEBUG */

    return( AFP_OK );
}


/* -----------------------------------
   checkAttrib:   1 check kFPDeleteInhibitBit 
   ie deletfile called from afp_delete
   
*/
int deletefile( file, checkAttrib )
char		*file;
int         checkAttrib;
{
    struct adouble	ad;
    int			adflags, err = AFP_OK;
    int			locktype = ADLOCK_WR;
    int			openmode = O_RDWR;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin deletefile:");
#endif /* DEBUG */

    while(1) {
        /*
         * If can't open read/write then try again read-only.  If it's open
         * read-only, we must do a read lock instead of a write lock.
         */
        /* try to open both at once */
        adflags = ADFLAGS_DF|ADFLAGS_HF;
        memset(&ad, 0, sizeof(ad));
        if ( ad_open( file, adflags, openmode, 0, &ad ) < 0 ) {
            switch (errno) {
            case ENOENT:
                adflags = ADFLAGS_DF;
                /* that failed. now try to open just the data fork */
                memset(&ad, 0, sizeof(ad));
                if ( ad_open( file, adflags, openmode, 0, &ad ) < 0 ) {
                    switch (errno) {
                    case ENOENT:
                        return AFPERR_NOOBJ;
                    case EACCES:
                        if(openmode == O_RDWR) {
                            openmode = O_RDONLY;
                            locktype = ADLOCK_RD;
                            continue;
                        } else {
                            return AFPERR_ACCESS;
                        }
                    case EROFS:
                        return AFPERR_VLOCK;
                    default:
                        return AFPERR_PARAM;
                    }
                }
                break;

            case EACCES:
                if(openmode == O_RDWR) {
                    openmode = O_RDONLY;
                    locktype = ADLOCK_RD;
                    continue;
                } else {
                    return AFPERR_ACCESS;
                }
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

    if ((adflags & ADFLAGS_HF) &&
            (ad_tmplock(&ad, ADEID_RFORK, locktype, 0, 0) < 0 )) {
        ad_close( &ad, adflags );
        return( AFPERR_BUSY );
    }

    if (ad_tmplock( &ad, ADEID_DFORK, locktype, 0, 0 ) < 0) {
        err = AFPERR_BUSY;
        goto delete_unlock;
    }

    if ( unlink( ad_path( file, ADFLAGS_HF )) < 0 ) {
        switch ( errno ) {
        case EPERM:
        case EACCES :
            err = AFPERR_ACCESS;
            goto delete_unlock;
        case EROFS:
            err = AFPERR_VLOCK;
            goto delete_unlock;
        case ENOENT :
            break;
        default :
            err = AFPERR_PARAM;
            goto delete_unlock;
        }
    }

    if ( unlink( file ) < 0 ) {
        switch ( errno ) {
        case EPERM:
        case EACCES :
            err = AFPERR_ACCESS;
            break;
        case EROFS:
            err = AFPERR_VLOCK;
            break;
        case ENOENT :
            break;
        default :
            err = AFPERR_PARAM;
            break;
        }
    }

delete_unlock:
    if (adflags & ADFLAGS_HF)
        ad_tmplock(&ad, ADEID_RFORK, ADLOCK_CLR, 0, 0);
    ad_tmplock(&ad, ADEID_DFORK, ADLOCK_CLR, 0, 0);
    ad_close( &ad, adflags );

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end deletefile:");
#endif /* DEBUG */

    return err;
}


#ifdef CNID_DB
/* return a file id */
int afp_createid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         st;
    struct adouble	ad;
    struct vol		*vol;
    struct dir		*dir;
    char		*path, *upath;
    int                 len;
    cnid_t		did, id;
    u_short		vid;

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_createid:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM);
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof(did);

    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if ( *path == '\0' ) {
        return( AFPERR_BADTYPE );
    }

    upath = mtoupath(vol, path);
    if (stat(upath, &st) < 0) {
        switch (errno) {
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        case ENOENT:
            return AFPERR_NOOBJ;
        default:
            return AFPERR_PARAM;
        }
    }

    if (id = cnid_lookup(vol->v_db, &st, did, upath, len = strlen(upath))) {
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

    if (id = cnid_add(vol->v_db, &st, did, upath, len, id) != CNID_INVALID) {
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

/* resolve a file id */
int afp_resolveid(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct stat         st;
    struct vol		*vol;
    struct dir		*dir;
    char		*upath;
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

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM);
    }

    memcpy(&id, ibuf, sizeof( id ));
    ibuf += sizeof(id);

    if ((upath = cnid_resolve(vol->v_db, &id, buffer, len)) == NULL) {
        return AFPERR_BADID;
    }

    if (( dir = dirlookup( vol, id )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if ((movecwd(vol, dir) < 0) || (stat(upath, &st) < 0)) {
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
    if (S_ISDIR(st.st_mode))
        return AFPERR_BADTYPE;

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs( bitmap );

    if ((err = getfilparams(vol, bitmap, utompath(vol, upath), curdir, &st,
                            rbuf + sizeof(bitmap), &buflen)) != AFP_OK)
        return err;

    *rbuflen = buflen + sizeof(bitmap);
    memcpy(rbuf, ibuf, sizeof(bitmap));

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_resolveid:");
#endif /* DEBUG */

    return AFP_OK;
}

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

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM);
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy(&id, ibuf, sizeof( id ));
    ibuf += sizeof(id);
    fileid = id;

    if ((upath = cnid_resolve(vol->v_db, &id, buffer, len)) == NULL) {
        return AFPERR_NOID;
    }

    if (( dir = dirlookup( vol, id )) == NULL ) {
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

    /* directories are bad */
    if (S_ISDIR(st.st_mode))
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
    char		*spath, temp[17], *path, *p;
    char                *supath, *upath;
    int                 err;
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

    if (( vol = getvolbyvid( vid )) == NULL ) {
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
    if ((dir = dirsearch( vol, sid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if ( *path == '\0' ) {
        return( AFPERR_BADTYPE );
    }

    upath = mtoupath(vol, path);
    if (stat(upath, &srcst) < 0) {
        switch (errno) {
        case ENOENT:
            return AFPERR_NOID;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        default:
            return AFPERR_PARAM;
        }
    }

    /* save some stuff */
    sdir = curdir;
    spath = obj->oldtmp;
    supath = obj->newtmp;
    strcpy(spath, path);
    strcpy(supath, upath); /* this is for the cnid changing */
    p = ctoupath( vol, sdir, spath);

    /* look for the source cnid. if it doesn't exist, don't worry about
     * it. */
#ifdef CNID_DB
    sid = cnid_lookup(vol->v_db, &srcst, sdir->d_did, supath,
                      slen = strlen(supath));
#endif /* CNID_DB */

    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if ( *path == '\0' ) {
        return( AFPERR_BADTYPE );
    }

    /* FPExchangeFiles is the only call that can return the SameObj
     * error */
    if ((curdir == sdir) && strcmp(spath, path) == 0)
        return AFPERR_SAMEOBJ;

    upath = mtoupath(vol, path);
    if (stat(upath, &destst) < 0) {
        switch (errno) {
        case ENOENT:
            return AFPERR_NOID;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        default:
            return AFPERR_PARAM;
        }
    }

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
    if ((err = renamefile(p, temp, temp, vol_noadouble(vol))) < 0)
        goto err_exchangefile;
    of_rename(vol, sdir, spath, curdir, temp);

    /* rename destination to source */
    if ((err = renamefile(path, p, spath, vol_noadouble(vol))) < 0)
        goto err_src_to_tmp;
    of_rename(vol, curdir, path, sdir, spath);

    /* rename temp to destination */
    if ((err = renamefile(temp, upath, path, vol_noadouble(vol))) < 0)
        goto err_dest_to_src;
    of_rename(vol, curdir, temp, curdir, path);

#ifdef CNID_DB
    /* id's need switching. src -> dest and dest -> src. */
    if (sid && (cnid_update(vol->v_db, sid, &destst, curdir->d_did,
                            upath, dlen) < 0)) {
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

    if (did && (cnid_update(vol->v_db, did, &srcst, sdir->d_did,
                            supath, slen) < 0)) {
        switch (errno) {
        case EPERM:
        case EACCES:
            err = AFPERR_ACCESS;
            break;
        default:
            err = AFPERR_PARAM;
        }

        if (sid)
            cnid_update(vol->v_db, sid, &srcst, sdir->d_did, supath, slen);
        goto err_temp_to_dest;
    }
#endif /* CNID_DB */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "ending afp_exchangefiles:");
#endif /* DEBUG */

    return AFP_OK;


    /* all this stuff is so that we can unwind a failed operation
     * properly. */
err_temp_to_dest:
    /* rename dest to temp */
    renamefile(upath, temp, temp, vol_noadouble(vol));
    of_rename(vol, curdir, upath, curdir, temp);

err_dest_to_src:
    /* rename source back to dest */
    renamefile(p, upath, path, vol_noadouble(vol));
    of_rename(vol, sdir, spath, curdir, path);

err_src_to_tmp:
    /* rename temp back to source */
    renamefile(temp, p, spath, vol_noadouble(vol));
    of_rename(vol, curdir, temp, sdir, spath);

err_exchangefile:
    return err;
}
