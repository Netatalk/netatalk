/*
 * $Id: fork.c,v 1.33 2002-08-26 08:57:50 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <atalk/logger.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>

#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/afp.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif

#include "fork.h"
#include "file.h"
#include "globals.h"
#include "directory.h"
#include "desktop.h"
#include "volume.h"

#define BYTELOCK_MAX 0x7FFFFFFFU

struct ofork		*writtenfork;
extern int getmetadata(struct vol *vol,
                 u_int16_t bitmap,
                 char *path, struct dir *dir, struct stat *st,
                 char *buf, int *buflen, struct adouble *adp, int attrbits );

static int getforkparams(ofork, bitmap, buf, buflen, attrbits )
struct ofork	*ofork;
u_int16_t		bitmap;
char		*buf;
int			*buflen;
const u_int16_t     attrbits;
{
#ifndef USE_LASTDID
    struct stat		lst, *lstp;
#endif /* !USE_LASTDID */
    struct stat		st;
    char		*upath;
    u_int32_t		aint;

    struct adouble	*adp;
    struct dir		*dir;
    struct vol		*vol;
    
    if ( ad_hfileno( ofork->of_ad ) == -1 ) {
        adp = NULL;
    } else {
        aint = ad_getentrylen( ofork->of_ad, ADEID_RFORK );
        if ( ad_refresh( ofork->of_ad ) < 0 ) {
            LOG(log_error, logtype_afpd, "getforkparams: ad_refresh: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }
        /* See afp_closefork() for why this is bad */
        ad_setentrylen( ofork->of_ad, ADEID_RFORK, aint );
        adp = ofork->of_ad;
    }

    /* can only get the length of the opened fork */
    if (((bitmap & (1<<FILPBIT_DFLEN)) && (ofork->of_flags & AFPFORK_RSRC)) ||
            ((bitmap & (1<<FILPBIT_RFLEN)) && (ofork->of_flags & AFPFORK_DATA))) {
        return( AFPERR_BITMAP );
    }

    vol = ofork->of_vol;
    dir = ofork->of_dir;

    if ( bitmap & ( 1<<FILPBIT_DFLEN | 1<<FILPBIT_FNUM |
                    (1 << FILPBIT_CDATE) | (1 << FILPBIT_MDATE) |
                    (1 << FILPBIT_BDATE))) {
        if ( ad_dfileno( ofork->of_ad ) == -1 ) {
            upath = mtoupath(vol, ofork->of_name);
            if (movecwd(vol, dir) < 0)
                return( AFPERR_NOOBJ );
            if ( stat( upath, &st ) < 0 )
                return( AFPERR_NOOBJ );
        } else {
            if ( fstat( ad_dfileno( ofork->of_ad ), &st ) < 0 ) {
                return( AFPERR_BITMAP );
            }
        }
    }
    return getmetadata(vol, bitmap, ofork->of_name, dir, &st, buf, buflen, adp, attrbits );    
}

/* -------------------------
*/
#define SHARE 0
#define EXCL  1
static int setforkmode(struct adouble *adp, int eid, int ofrefnum, int what, int mode)
{
    int lockmode;
    int lockop;
    
    /* NOTE: we can't write lock a read-only file. on those, we just
     * make sure that we have a read lock set. that way, we at least prevent
     * someone else from really setting a deny read/write on the file. 
     */
    lockmode = (ad_getoflags(adp, eid) & O_RDWR) ?ADLOCK_WR : ADLOCK_RD;
    lockop = (mode == EXCL)?lockmode:ADLOCK_RD;
    
    return ad_lock(adp, eid, lockop | ADLOCK_FILELOCK | ADLOCK_UPGRADE,
                        what, 1, ofrefnum);
}

/* -------------------------
*/
extern int ad_testlock(struct adouble *adp, int eid, int off);

static int getforkmode(struct adouble *adp, int eid, int what)
{
    return ad_testlock(adp, eid,  what);
}

/* -------------------------- 
   a lot of races, some can be remove. but I try first to get the semantic right
*/
#ifdef USE_FLOCK_LOCKS 
#error sorry, for now configure --with-flock-locks is broken...
#endif

static int afp_setmode(struct adouble *adp, int eid, int access, int ofrefnum)
{
    int ret;
    int readset;
    int writeset;
    int denyreadset;
    int denywriteset;
    int mode;    

    if (! (access & (OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD))) {
        return setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_NONE, SHARE);
    }

    if ((access & (OPENACC_RD | OPENACC_DRD))) {
        if ((readset = getforkmode(adp, eid, AD_FILELOCK_OPEN_RD)) <0)
            return readset;
        if ((denyreadset = getforkmode(adp, eid, AD_FILELOCK_DENY_RD)) <0)
            return denyreadset;

        if ((access & OPENACC_RD) && denyreadset) {
            errno = EACCES;
            return -1;
        }
        if ((access & OPENACC_DRD) && readset) {
            errno = EACCES;
            return -1;
        }   
        /* boolean logic is not enough, because getforkmode is not always telling the
         * true 
         */
        mode = ((access & OPENACC_DRD))?EXCL: SHARE;
        if ((access & OPENACC_RD)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_RD, mode);
            if (ret)
                return ret;
        }
        if ((access & OPENACC_DRD)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_DENY_RD, SHARE);
            if (ret)
                return ret;
        }
    }
    /* ------------same for writing -------------- */
    if ((access & (OPENACC_WR | OPENACC_DWR))) {
        if ((writeset = getforkmode(adp, eid, AD_FILELOCK_OPEN_WR)) <0)
            return writeset;
        if ((denywriteset = getforkmode(adp, eid, AD_FILELOCK_DENY_WR)) <0)
            return denywriteset;

        if ((access & OPENACC_WR) && denywriteset) {
            errno = EACCES;
            return -1;
        }
        if ((access & OPENACC_DWR) && writeset) {
            errno = EACCES;
            return -1;
        }   
        mode = ((access & OPENACC_DWR))?EXCL: SHARE;
        if ((access & OPENACC_WR)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_WR, mode);
            if (ret)
                return ret;
        }
        if ((access & OPENACC_DWR)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_DENY_WR, SHARE);
            if (ret)
                return ret;
        }
    }
    return 0;
}

/* ----------------------- */    
int afp_openfork(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct dir		*dir;
    struct ofork	*ofork, *opened;
    struct adouble      *adsame = NULL;
    int			buflen, ret, adflags, eid, lockop;
    u_int32_t           did;
    u_int16_t		vid, bitmap, access, ofrefnum, attrbits = 0;
    char		fork, *path, *upath;
    u_int16_t bshort;

    ibuf++;
    fork = *ibuf++;
    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof(vid);

    *rbuflen = 0;
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    if (( dir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );
    memcpy(&access, ibuf, sizeof( access ));
    access = ntohs( access );
    ibuf += sizeof( access );

    if ((vol->v_flags & AFPVOL_RO) && (access & OPENACC_WR)) {
        return AFPERR_VLOCK;
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    if ( fork == OPENFORK_DATA ) {
        eid = ADEID_DFORK;
        adflags = ADFLAGS_DF|ADFLAGS_HF;
    } else {
        eid = ADEID_RFORK;
        adflags = ADFLAGS_HF;
    }

    /* XXX: this probably isn't the best way to do this. the already
       open bits should really be set if the fork is opened by any
       program, not just this one. however, that's problematic to do
       if we can't write lock files somewhere. opened is also passed to 
       ad_open so that we can keep file locks together.
       FIXME: add the fork we are opening? 
    */
    if ((opened = of_findname(vol, curdir, path))) {
        attrbits = ((opened->of_ad->ad_df.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);
        attrbits |= ((opened->of_ad->ad_hf.adf_refcount > opened->of_ad->ad_df.adf_refcount)? ATTRBIT_ROPEN : 0);
                   
        adsame = opened->of_ad;
    }

    if (( ofork = of_alloc(vol, curdir, path, &ofrefnum, eid,
                           adsame)) == NULL ) {
        return( AFPERR_NFILE );
    }
    if (access & OPENACC_WR) {
        /* try opening in read-write mode */
        upath = mtoupath(vol, path);
        ret = AFPERR_NOOBJ;
        if (ad_open(upath, adflags, O_RDWR, 0, ofork->of_ad) < 0) {
            switch ( errno ) {
            case EROFS:
                ret = AFPERR_VLOCK;
            case EACCES:
                goto openfork_err;

                break;
            case ENOENT:
                {
                    struct stat st;

                    /* see if client asked for the data fork */
                    if (fork == OPENFORK_DATA) {
                        if (ad_open(upath, ADFLAGS_DF, O_RDWR, 0, ofork->of_ad) < 0) {
                            goto openfork_err;
                        }
                        adflags = ADFLAGS_DF;

                    } else if (stat(upath, &st) == 0) {
                        /* here's the deal. we only try to create the resource
                         * fork if the user wants to open it for write acess. */
                        if (ad_open(upath, adflags, O_RDWR | O_CREAT, 0666, ofork->of_ad) < 0)
                            goto openfork_err;
                    } else
                        goto openfork_err;
                }
                break;
            case EMFILE :
            case ENFILE :
                ret = AFPERR_NFILE;
                goto openfork_err;
                break;
            case EISDIR :
                ret = AFPERR_BADTYPE;
                goto openfork_err;
                break;
            default:
                LOG(log_error, logtype_afpd, "afp_openfork: ad_open: %s", strerror(errno) );
                ret = AFPERR_PARAM;
                goto openfork_err;
                break;
            }
        }
    } else {
        /* try opening in read-only mode */
        upath = mtoupath(vol, path);
        ret = AFPERR_NOOBJ;
        if (ad_open(upath, adflags, O_RDONLY, 0, ofork->of_ad) < 0) {
            switch ( errno ) {
            case EROFS:
                ret = AFPERR_VLOCK;
            case EACCES:
                /* check for a read-only data fork */
                if ((adflags != ADFLAGS_HF) &&
                        (ad_open(upath, ADFLAGS_DF, O_RDONLY, 0, ofork->of_ad) < 0))
                    goto openfork_err;

                adflags = ADFLAGS_DF;
                break;
            case ENOENT:
                {
                    struct stat st;

                    /* see if client asked for the data fork */
                    if (fork == OPENFORK_DATA) {
                        if (ad_open(upath, ADFLAGS_DF, O_RDONLY, 0, ofork->of_ad) < 0) {
                            goto openfork_err;
                        }
                        adflags = ADFLAGS_DF;

                    } else if (stat(upath, &st) != 0) {
                        goto openfork_err;
                    }
                }
                break;
            case EMFILE :
            case ENFILE :
                ret = AFPERR_NFILE;
                goto openfork_err;
                break;
            case EISDIR :
                ret = AFPERR_BADTYPE;
                goto openfork_err;
                break;
            default:
                LOG(log_error, logtype_afpd, "afp_openfork: ad_open: %s", strerror(errno) );
                goto openfork_err;
                break;
            }
        }
    }

    if ((adflags & ADFLAGS_HF) &&
            (ad_getoflags( ofork->of_ad, ADFLAGS_HF ) & O_CREAT)) {
        ad_setentrylen( ofork->of_ad, ADEID_NAME, strlen( path ));
        memcpy(ad_entry( ofork->of_ad, ADEID_NAME ), path,
               ad_getentrylen( ofork->of_ad, ADEID_NAME ));
        ad_flush( ofork->of_ad, adflags );
    }

    if (( ret = getforkparams(ofork, bitmap, rbuf + 2 * sizeof( u_int16_t ),
                              &buflen, attrbits )) != AFP_OK ) {
        ad_close( ofork->of_ad, adflags );
        goto openfork_err;
    }

    *rbuflen = buflen + 2 * sizeof( u_int16_t );
    bitmap = htons( bitmap );
    memcpy(rbuf, &bitmap, sizeof( u_int16_t ));
    rbuf += sizeof( u_int16_t );

    /* check  WriteInhibit bit, the test is done here, after some Mac trafic capture */
    ad_getattr(ofork->of_ad, &bshort);
    if ((bshort & htons(ATTRBIT_NOWRITE)) && (access & OPENACC_WR)) {
        ad_close( ofork->of_ad, adflags );
        of_dealloc( ofork );
        ofrefnum = 0;
        memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
        return(AFPERR_OLOCK);
    }

    /*
     * synchronization locks:
     */

    /* don't try to lock non-existent rforks. */
    if ((eid == ADEID_DFORK) || (ad_hfileno(ofork->of_ad) != -1)) {

        ret = afp_setmode(ofork->of_ad, eid, access, ofrefnum);
        /* can we access the fork? */
        if (ret < 0) {
            ret = errno;
            ad_close( ofork->of_ad, adflags );
            of_dealloc( ofork );
            switch (ret) {
            case EAGAIN: /* return data anyway */
            case EACCES:
            case EINVAL:
                ofrefnum = 0;
                memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
                return( AFPERR_DENYCONF );
                break;
            default:
                *rbuflen = 0;
                LOG(log_error, logtype_afpd, "afp_openfork: ad_lock: %s", strerror(errno) );
                return( AFPERR_PARAM );
            }
        }
        if ((access & OPENACC_WR))
            ofork->of_flags |= AFPFORK_ACCWR;
        if ((access & OPENACC_RD))
            ofork->of_flags |= AFPFORK_ACCRD;
    }

    memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
    return( AFP_OK );

openfork_err:
    of_dealloc( ofork );
    if (errno == EACCES)
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;
    return ret;
}

int afp_setforkparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    int32_t		size;
    u_int16_t		ofrefnum, bitmap;
    int                 err;

    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof( bitmap );
    memcpy(&size, ibuf, sizeof( size ));
    size = ntohl( size );

    *rbuflen = 0;
    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_setforkparams: of_find could not locate open fork refnum: %u", ofrefnum );
        return( AFPERR_PARAM );
    }

    if (ofork->of_vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0)
        return AFPERR_ACCESS;

    if (size < 0)
        return AFPERR_PARAM;

    if ((bitmap == (1<<FILPBIT_DFLEN)) && (ofork->of_flags & AFPFORK_DATA)) {
        err = ad_dtruncate( ofork->of_ad, size );
        if (err < 0)
            goto afp_setfork_err;
    } else if ((bitmap == (1<<FILPBIT_RFLEN)) &&
               (ofork->of_flags & AFPFORK_RSRC)) {
        ad_refresh( ofork->of_ad );
        err = ad_rtruncate(ofork->of_ad, size);
        if (err < 0)
            goto afp_setfork_err;

        if (ad_flush( ofork->of_ad, ADFLAGS_HF ) < 0) {
            LOG(log_error, logtype_afpd, "afp_setforkparams: ad_flush: %s",
                strerror(errno) );
            return( AFPERR_PARAM );
        }
    } else
        return AFPERR_BITMAP;

#ifdef AFS
    if ( flushfork( ofork ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_setforkparams: flushfork: %s", strerror(errno) );
    }
#endif /* AFS */

    return( AFP_OK );

afp_setfork_err:
    if (err == -2)
        return AFPERR_LOCK;
    else {
        switch (errno) {
        case EROFS:
            return AFPERR_VLOCK;
        case EPERM:
        case EACCES:
            return AFPERR_ACCESS;
        case EDQUOT:
        case EFBIG:
        case ENOSPC:
            return AFPERR_DFULL;
        default:
            return AFPERR_PARAM;
        }
    }
}

/* for this to work correctly, we need to check for locks before each
 * read and write. that's most easily handled by always doing an
 * appropriate check before each ad_read/ad_write. other things
 * that can change files like truncate are handled internally to those
 * functions. 
 */
#define ENDBIT(a)  ((a) & 0x80)
#define UNLOCKBIT(a) ((a) & 0x01)
int afp_bytelock(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    int32_t             offset, length;
    int                 eid;
    u_int16_t		ofrefnum;
    u_int8_t            flags;

    *rbuflen = 0;

    /* figure out parameters */
    ibuf++;
    flags = *ibuf; /* first bit = endflag, lastbit = lockflag */
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_bytelock: of_find");
        return( AFPERR_PARAM );
    }

    if ( ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else
        return AFPERR_PARAM;

    memcpy(&offset, ibuf, sizeof( offset ));
    offset = ntohl(offset);
    ibuf += sizeof(offset);

    memcpy(&length, ibuf, sizeof( length ));
    length = ntohl(length);
    if (length == 0xFFFFFFFF)
        length = BYTELOCK_MAX;
    else if (length <= 0) {
        return AFPERR_PARAM;
    } else if ((length >= AD_FILELOCK_BASE) &&
               (ad_hfileno(ofork->of_ad) == -1))
        return AFPERR_LOCK;

    if (ENDBIT(flags))
        offset += ad_size(ofork->of_ad, eid);

    if (offset < 0)    /* error if we have a negative offset */
        return AFPERR_PARAM;

    /* if the file is a read-only file, we use read locks instead of
     * write locks. that way, we can prevent anyone from initiating
     * a write lock. */
    if (ad_lock(ofork->of_ad, eid, UNLOCKBIT(flags) ? ADLOCK_CLR :
                ((ad_getoflags(ofork->of_ad, eid) & O_RDWR) ?
                 ADLOCK_WR : ADLOCK_RD), offset, length,
                ofork->of_refnum) < 0) {
        switch (errno) {
        case EACCES:
        case EAGAIN:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_LOCK;
            break;
        case ENOLCK:
            return AFPERR_NLOCK;
            break;
        case EINVAL:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_RANGEOVR;
            break;
        case EBADF:
        default:
            return AFPERR_PARAM;
            break;
        }
    }

    offset = htonl(offset);
    memcpy(rbuf, &offset, sizeof( offset ));
    *rbuflen = sizeof( offset );
    return( AFP_OK );
}
#undef UNLOCKBIT


static __inline__ int crlf( of )
struct ofork	*of;
{
    struct extmap	*em;

    if ( ad_hfileno( of->of_ad ) == -1 ||
            memcmp( ufinderi, ad_entry( of->of_ad, ADEID_FINDERI ),
                    8) == 0 ) {
        if (( em = getextmap( of->of_name )) == NULL ||
                memcmp( "TEXT", em->em_type, sizeof( em->em_type )) == 0 ) {
            return( 1 );
        } else {
            return( 0 );
        }
    } else {
        if ( memcmp( ufinderi,
                     ad_entry( of->of_ad, ADEID_FINDERI ), 4 ) == 0 ) {
            return( 1 );
        } else {
            return( 0 );
        }
    }
}


static __inline__ ssize_t read_file(struct ofork *ofork, int eid,
                                    int offset, u_char nlmask,
                                    u_char nlchar, char *rbuf,
                                    int *rbuflen, const int xlate)
{
    ssize_t cc;
    int eof = 0;
    char *p, *q;

    cc = ad_read(ofork->of_ad, eid, offset, rbuf, *rbuflen);
    if ( cc < 0 ) {
        LOG(log_error, logtype_afpd, "afp_read: ad_read: %s", strerror(errno) );
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }
    if ( cc < *rbuflen ) {
        eof = 1;
    }

    /*
     * Do Newline check.
     */
    if ( nlmask != 0 ) {
        for ( p = rbuf, q = p + cc; p < q; ) {
            if (( *p++ & nlmask ) == nlchar ) {
                break;
            }
        }
        if ( p != q ) {
            cc = p - rbuf;
            eof = 0;
        }
    }

    /*
     * If this file is of type TEXT, then swap \012 to \015.
     */
    if (xlate) {
        for ( p = rbuf, q = p + cc; p < q; p++ ) {
            if ( *p == '\012' ) {
                *p = '\015';
            } else if ( *p == '\015' ) {
                *p = '\012';
            }

        }
    }

    *rbuflen = cc;
    if ( eof ) {
        return( AFPERR_EOF );
    }
    return AFP_OK;
}

/* -----------------------------
 * with ddp, afp_read can return fewer bytes than in reqcount 
 * so return EOF only if read actually past end of file not
 * if offset +reqcount > size of file
 * e.g.:
 * getfork size ==> 10430
 * read fork offset 0 size 10752 ????  ==> 4264 bytes (without EOF)
 * read fork offset 4264 size 6128 ==> 4264 (without EOF)
 * read fork offset 9248 size 1508 ==> 1182 (EOF)
 * 10752 is a bug in Mac 7.5.x finder 
 *
 * with dsi, should we check that reqcount < server quantum? 
*/
int afp_read(obj, ibuf, ibuflen, rbuf, rbuflen)
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    off_t 		size;
    int32_t		offset, saveoff, reqcount, savereqcount;
    int			cc, err, eid, xlate = 0;
    u_int16_t		ofrefnum;
    u_char		nlmask, nlchar;

    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( u_short );

    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_read: of_find");
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCRD) == 0) {
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    memcpy(&offset, ibuf, sizeof( offset ));
    offset = ntohl( offset );
    ibuf += sizeof( offset );
    memcpy(&reqcount, ibuf, sizeof( reqcount ));
    reqcount = ntohl( reqcount );
    ibuf += sizeof( reqcount );

    nlmask = *ibuf++;
    nlchar = *ibuf++;

    /* if we wanted to be picky, we could add in the following
     * bit: if (afp_version == 11 && !(nlmask == 0xFF || !nlmask))
     */
    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    if ( ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
        xlate = (ofork->of_vol->v_flags & AFPVOL_CRLF) ? crlf(ofork) : 0;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else { /* fork wasn't opened. this should never really happen. */
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    /* zero request count */
    err = AFP_OK;
    if (!reqcount) {
        goto afp_read_err;
    }

    /* reqcount isn't always truthful. we need to deal with that. */
    size = ad_size(ofork->of_ad, eid);

    if (offset >= size) {
        err = AFPERR_EOF;
        goto afp_read_err;
    }

    savereqcount = reqcount;
    saveoff = offset;
    if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, saveoff, savereqcount) < 0) {
        err = AFPERR_LOCK;
        goto afp_read_err;
    }

#define min(a,b)	((a)<(b)?(a):(b))
    *rbuflen = min( reqcount, *rbuflen );
    err = read_file(ofork, eid, offset, nlmask, nlchar, rbuf, rbuflen,
                    xlate);
    if (err < 0)
        goto afp_read_done;

    /* dsi can stream requests. we can only do this if we're not checking
     * for an end-of-line character. oh well. */
    if ((obj->proto == AFPPROTO_DSI) && (*rbuflen < reqcount) && !nlmask) {
        DSI *dsi = obj->handle;

        if (obj->options.flags & OPTION_DEBUG) {
            printf( "(read) reply: %d/%d, %d\n", *rbuflen,
                    reqcount, dsi->clientID);
            bprint(rbuf, *rbuflen);
        }
        /* subtract off the offset */
        size -= offset;
        if (reqcount > size) {
    	   reqcount = size;
           err = AFPERR_EOF;
        }

        offset += *rbuflen;

        /* dsi_readinit() returns size of next read buffer. by this point,
         * we know that we're sending some data. if we fail, something
         * horrible happened. */
        if ((*rbuflen = dsi_readinit(dsi, rbuf, *rbuflen, reqcount, err)) < 0)
            goto afp_read_exit;

        /* due to the nature of afp packets, we have to exit if we get
           an error. we can't do this with translation on. */
#ifdef HAVE_SENDFILE_READ
        if (!(xlate || (obj->options.flags & OPTION_DEBUG))) {
            if (ad_readfile(ofork->of_ad, eid, dsi->socket, offset,
                            dsi->datasize) < 0) {
                if (errno == EINVAL)
                    goto afp_read_loop;
                else {
                    LOG(log_error, logtype_afpd, "afp_read: ad_readfile: %s", strerror(errno));
                    goto afp_read_exit;
                }
            }

            dsi_readdone(dsi);
            goto afp_read_done;
        }

afp_read_loop:
#endif /* HAVE_SENDFILE_READ */

        /* fill up our buffer. */
        while (*rbuflen > 0) {
            cc = read_file(ofork, eid, offset, nlmask, nlchar, rbuf,
                           rbuflen, xlate);
            if (cc < 0)
                goto afp_read_exit;

            offset += *rbuflen;
            if (obj->options.flags & OPTION_DEBUG) {
                printf( "(read) reply: %d, %d\n", *rbuflen, dsi->clientID);
                bprint(rbuf, *rbuflen);
            }

            /* dsi_read() also returns buffer size of next allocation */
            cc = dsi_read(dsi, rbuf, *rbuflen); /* send it off */
            if (cc < 0)
                goto afp_read_exit;
            *rbuflen = cc;
        }
        dsi_readdone(dsi);
        goto afp_read_done;

afp_read_exit:
        LOG(log_error, logtype_afpd, "afp_read: %s", strerror(errno));
        dsi_readdone(dsi);
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount);
        obj->exit(1);
    }

afp_read_done:
    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount);
    return err;

afp_read_err:
    *rbuflen = 0;
    return err;
}

int afp_flush(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol *vol;
    u_int16_t vid;

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    of_flush(vol);
    return( AFP_OK );
}

int afp_flushfork(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    u_int16_t		ofrefnum;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));

    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_flushfork: of_find");
        return( AFPERR_PARAM );
    }

    if ( flushfork( ofork ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_flushfork: %s", strerror(errno) );
    }

    return( AFP_OK );
}

/* this is very similar to closefork */
int flushfork( ofork )
struct ofork	*ofork;
{
    struct timeval tv;
    int len, err = 0, doflush = 0;

    if ( ad_dfileno( ofork->of_ad ) != -1 &&
            fsync( ad_dfileno( ofork->of_ad )) < 0 ) {
        LOG(log_error, logtype_afpd, "flushfork: dfile(%d) %s",
            ad_dfileno(ofork->of_ad), strerror(errno) );
        err = -1;
    }

    if ( ad_hfileno( ofork->of_ad ) != -1 ) {

        /* read in the rfork length */
        len = ad_getentrylen(ofork->of_ad, ADEID_RFORK);
        ad_refresh(ofork->of_ad);

        /* set the date if we're dirty */
        if ((ofork->of_flags & AFPFORK_DIRTY) &&
                (gettimeofday(&tv, NULL) == 0)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY|AD_DATE_UNIX, tv.tv_sec);
            ofork->of_flags &= ~AFPFORK_DIRTY;
            doflush++;
        }

        /* if we're actually flushing this fork, make sure to set the
         * length. otherwise, just use the stored length */
        if ((ofork->of_flags & AFPFORK_RSRC) &&
                (len != ad_getentrylen(ofork->of_ad, ADEID_RFORK))) {
            ad_setentrylen(ofork->of_ad, ADEID_RFORK, len);
            doflush++;
        }


        /* flush the header (if it is a resource fork) */
        if (ofork->of_flags & AFPFORK_RSRC)
            if (doflush && (ad_flush(ofork->of_ad, ADFLAGS_HF) < 0))
                err = -1;

        if (fsync( ad_hfileno( ofork->of_ad )) < 0)
            err = -1;

        if (err < 0)
            LOG(log_error, logtype_afpd, "flushfork: hfile(%d) %s",
                ad_hfileno(ofork->of_ad), strerror(errno) );
    }

    return( err );
}

int afp_closefork(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    struct timeval      tv;
    int			adflags, aint, doflush = 0;
    u_int16_t		ofrefnum;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));

    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_closefork: of_find");
        return( AFPERR_PARAM );
    }

    adflags = 0;
    if ((ofork->of_flags & AFPFORK_DATA) &&
            (ad_dfileno( ofork->of_ad ) != -1)) {
        adflags |= ADFLAGS_DF;
    }

    if ( ad_hfileno( ofork->of_ad ) != -1 ) {
        adflags |= ADFLAGS_HF;

        aint = ad_getentrylen( ofork->of_ad, ADEID_RFORK );
        ad_refresh( ofork->of_ad );
        if ((ofork->of_flags & AFPFORK_DIRTY) &&
                (gettimeofday(&tv, NULL) == 0)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY | AD_DATE_UNIX,
                       tv.tv_sec);
            doflush++;
        }

        /*
         * Only set the rfork's length if we're closing the rfork.
         */
        if ((ofork->of_flags & AFPFORK_RSRC) && aint !=
                ad_getentrylen( ofork->of_ad, ADEID_RFORK )) {
            ad_setentrylen( ofork->of_ad, ADEID_RFORK, aint );
            doflush++;
        }
        if ( doflush ) {
            ad_flush( ofork->of_ad, adflags );
        }
    }

    if ( ad_close( ofork->of_ad, adflags ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_closefork: ad_close: %s", strerror(errno) );
        return( AFPERR_PARAM );
    }

    of_dealloc( ofork );
    return( AFP_OK );
}


static __inline__ ssize_t write_file(struct ofork *ofork, int eid,
                                     off_t offset, char *rbuf,
                                     size_t rbuflen, const int xlate)
{
    char *p, *q;
    ssize_t cc;

    /*
     * If this file is of type TEXT, swap \015 to \012.
     */
    if (xlate) {
        for ( p = rbuf, q = p + rbuflen; p < q; p++ ) {
            if ( *p == '\015' ) {
                *p = '\012';
            } else if ( *p == '\012' ) {
                *p = '\015';
            }
        }
    }

    if (( cc = ad_write(ofork->of_ad, eid, offset, 0,
                        rbuf, rbuflen)) < 0 ) {
        switch ( errno ) {
        case EDQUOT :
        case EFBIG :
        case ENOSPC :
            return( AFPERR_DFULL );
        default :
            LOG(log_error, logtype_afpd, "afp_write: ad_write: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }
    }

    return cc;
}

/* FPWrite. NOTE: on an error, we always use afp_write_err as
 * the client may have sent us a bunch of data that's not reflected 
 * in reqcount et al. */
int afp_write(obj, ibuf, ibuflen, rbuf, rbuflen)
AFPObj              *obj;
char                *ibuf, *rbuf;
int                 ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    int32_t           	offset, saveoff, reqcount;
    int		        endflag, eid, xlate = 0, err = AFP_OK;
    u_int16_t		ofrefnum;
    ssize_t             cc;

    /* figure out parameters */
    ibuf++;
    endflag = ENDBIT(*ibuf);
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );
    memcpy(&offset, ibuf, sizeof( offset ));
    offset = ntohl( offset );
    ibuf += sizeof( offset );
    memcpy(&reqcount, ibuf, sizeof( reqcount ));
    reqcount = ntohl( reqcount );
    ibuf += sizeof( reqcount );

    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_write: of_find");
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
        err = AFPERR_ACCESS;
        goto afp_write_err;
    }

#ifdef AFS
    writtenfork = ofork;
#endif /* AFS */

    if ( ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
        xlate = (ofork->of_vol->v_flags & AFPVOL_CRLF) ? crlf(ofork) : 0;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        err = AFPERR_ACCESS; /* should never happen */
        goto afp_write_err;
    }

    if (endflag)
        offset += ad_size(ofork->of_ad, eid);

    /* handle bogus parameters */
    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    /* offset can overflow on 64-bit capable filesystems.
     * report disk full if that's going to happen. */
    if (offset + reqcount < 0) {
        err = AFPERR_DFULL;
        goto afp_write_err;
    }

    if (!reqcount) { /* handle request counts of 0 */
        err = AFP_OK;
        offset = htonl(offset);
        memcpy(rbuf, &offset, sizeof(offset));
        goto afp_write_err;
    }

    saveoff = offset;
    if (ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, saveoff,
                   reqcount) < 0) {
        err = AFPERR_LOCK;
        goto afp_write_err;
    }

    /* this is yucky, but dsi can stream i/o and asp can't */
    switch (obj->proto) {
#ifndef NO_DDP
    case AFPPROTO_ASP:
        if (asp_wrtcont(obj->handle, rbuf, rbuflen) < 0) {
            *rbuflen = 0;
            LOG(log_error, logtype_afpd, "afp_write: asp_wrtcont: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }

        if (obj->options.flags & OPTION_DEBUG) {
            printf("(write) len: %d\n", *rbuflen);
            bprint(rbuf, *rbuflen);
        }

        if ((cc = write_file(ofork, eid, offset, rbuf, *rbuflen,
                             xlate)) < 0) {
            *rbuflen = 0;
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount);
            return cc;
        }
        offset += cc;
        break;
#endif /* no afp/asp */

    case AFPPROTO_DSI:
        {
            DSI *dsi = obj->handle;

            /* find out what we have already and write it out. */
            cc = dsi_writeinit(dsi, rbuf, *rbuflen);
            if (!cc ||
                    (cc = write_file(ofork, eid, offset, rbuf, cc, xlate)) < 0) {
                dsi_writeflush(dsi);
                *rbuflen = 0;
                ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount);
                return cc;
            }
            offset += cc;

#if 0 /*def HAVE_SENDFILE_WRITE*/
            if (!(xlate || obj->options.flags & OPTION_DEBUG)) {
                if ((cc = ad_writefile(ofork->of_ad, eid, dsi->socket,
                                       offset, dsi->datasize)) < 0) {
                    switch (errno) {
                    case EDQUOT :
                    case EFBIG :
                    case ENOSPC :
                        cc = AFPERR_DFULL;
                        break;
                    default :
                        LOG(log_error, logtype_afpd, "afp_write: ad_writefile: %s", strerror(errno) );
                        goto afp_write_loop;
                    }
                    dsi_writeflush(dsi);
                    *rbuflen = 0;
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff,
                               reqcount);
                    return cc;
                }

                offset += cc;
                goto afp_write_done;
            }
#endif /* 0, was HAVE_SENDFILE_WRITE */

            /* loop until everything gets written. currently
                    * dsi_write handles the end case by itself. */
            while ((cc = dsi_write(dsi, rbuf, *rbuflen))) {
                if ( obj->options.flags & OPTION_DEBUG ) {
                    printf("(write) command cont'd: %d\n", cc);
                    bprint(rbuf, cc);
                }

                if ((cc = write_file(ofork, eid, offset, rbuf, cc, xlate)) < 0) {
                    dsi_writeflush(dsi);
                    *rbuflen = 0;
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff,
                               reqcount);
                    return cc;
                }
                offset += cc;
            }
        }
        break;
    }

    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount);
    if ( ad_hfileno( ofork->of_ad ) != -1 )
        ofork->of_flags |= AFPFORK_DIRTY;

    offset = htonl( offset );
#if defined(__GNUC__) && defined(HAVE_GCC_MEMCPY_BUG)
    bcopy(&offset, rbuf, sizeof(offset));
#else /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
    memcpy(rbuf, &offset, sizeof(offset));
#endif /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
    *rbuflen = sizeof(offset);
    return( AFP_OK );

afp_write_err:
    if (obj->proto == AFPPROTO_DSI) {
        dsi_writeinit(obj->handle, rbuf, *rbuflen);
        dsi_writeflush(obj->handle);
    }

    *rbuflen = (err == AFP_OK) ? sizeof(offset) : 0;
    return err;
}


int afp_getforkparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct ofork	*ofork;
    int			buflen, ret;
    u_int16_t		ofrefnum, bitmap;
    u_int16_t		attrbits = 0;

    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    *rbuflen = 0;
    if (( ofork = of_find( ofrefnum )) == NULL ) {
        LOG(log_error, logtype_afpd, "afp_getforkparams: of_find");
        return( AFPERR_PARAM );
    }
    attrbits = ((ofork->of_ad->ad_df.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);
    attrbits |= ((ofork->of_ad->ad_hf.adf_refcount > ofork->of_ad->ad_df.adf_refcount) ? ATTRBIT_ROPEN : 0);

    if (( ret = getforkparams( ofork, bitmap,
                               rbuf + sizeof( u_short ), &buflen, attrbits )) != AFP_OK ) {
        return( ret );
    }

    *rbuflen = buflen + sizeof( u_short );
    bitmap = htons( bitmap );
    memcpy(rbuf, &bitmap, sizeof( bitmap ));
    return( AFP_OK );
}

