/*
 * $Id: fork.c,v 1.73 2010-03-30 12:55:26 franklahm Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include <string.h>
#include <errno.h>

#include <atalk/adouble.h>
#include <atalk/logger.h>

#include <sys/param.h>
#include <sys/socket.h>

#include <netatalk/at.h>

#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/globals.h>

#include "fork.h"
#include "file.h"
#include "directory.h"
#include "desktop.h"
#include "volume.h"

#ifdef DEBUG1
#define Debug(a) ((a)->options.flags & OPTION_DEBUG)
#else
#define Debug(a) (0)
#endif

#ifdef AFS
struct ofork *writtenfork;
#endif

static int getforkparams(struct ofork *ofork, u_int16_t bitmap, char *buf, size_t *buflen)
{
    struct path         path;
    struct stat		*st;

    struct adouble	*adp;
    struct dir		*dir;
    struct vol		*vol;
    

    /* can only get the length of the opened fork */
    if ( ( (bitmap & ((1<<FILPBIT_DFLEN) | (1<<FILPBIT_EXTDFLEN))) 
                  && (ofork->of_flags & AFPFORK_RSRC)) 
        ||
          ( (bitmap & ((1<<FILPBIT_RFLEN) | (1<<FILPBIT_EXTRFLEN))) 
                  && (ofork->of_flags & AFPFORK_DATA))) {
        return( AFPERR_BITMAP );
    }

    if ( ad_reso_fileno( ofork->of_ad ) == -1 ) { /* META ? */
        adp = NULL;
    } else {
        adp = ofork->of_ad;
    }

    vol = ofork->of_vol;
    dir = dirlookup(vol, ofork->of_did);

    if (NULL == (path.u_name = mtoupath(vol, of_name(ofork), dir->d_did, utf8_encoding()))) {
        return( AFPERR_MISC );
    }
    path.m_name = of_name(ofork);
    path.id = 0;
    st = &path.st;
    if ( bitmap & ( (1<<FILPBIT_DFLEN) | (1<<FILPBIT_EXTDFLEN) | 
                    (1<<FILPBIT_FNUM) | (1 << FILPBIT_CDATE) | 
                    (1 << FILPBIT_MDATE) | (1 << FILPBIT_BDATE))) {
        if ( ad_data_fileno( ofork->of_ad ) <= 0 ) {
            /* 0 is for symlink */
            if (movecwd(vol, dir) < 0)
                return( AFPERR_NOOBJ );
            if ( lstat( path.u_name, st ) < 0 )
                return( AFPERR_NOOBJ );
        } else {
            if ( fstat( ad_data_fileno( ofork->of_ad ), st ) < 0 ) {
                return( AFPERR_BITMAP );
            }
        }
    }
    return getmetadata(vol, bitmap, &path, dir, buf, buflen, adp );
}

/* ---------------------------- */
static off_t get_off_t(char **ibuf, int is64)
{
    u_int32_t             temp;
    off_t                 ret;

    ret = 0;
    memcpy(&temp, *ibuf, sizeof( temp ));
    ret = ntohl(temp); /* ntohl is unsigned */
    *ibuf += sizeof(temp);

    if (is64) {
        memcpy(&temp, *ibuf, sizeof( temp ));
        *ibuf += sizeof(temp);
        ret = ntohl(temp)| (ret << 32);
    }
    else {
    	ret = (int)ret;	/* sign extend */
    }
    return ret;
}

/* ---------------------- */
static int set_off_t(off_t offset, char *rbuf, int is64)
{
    u_int32_t  temp;
    int        ret;

    ret = 0;
    if (is64) {
        temp = htonl(offset >> 32);
        memcpy(rbuf, &temp, sizeof( temp ));
        rbuf += sizeof(temp);
        ret = sizeof( temp );
        offset &= 0xffffffff;
    }
    temp = htonl(offset);
    memcpy(rbuf, &temp, sizeof( temp ));
    ret += sizeof( temp );

    return ret;
}

/* ------------------------ 
*/
static int is_neg(int is64, off_t val)
{
    if (val < 0 || (sizeof(off_t) == 8 && !is64 && (val & 0x80000000U)))
    	return 1;
    return 0;
}

static int sum_neg(int is64, off_t offset, off_t reqcount) 
{
    if (is_neg(is64, offset +reqcount) ) 
   	return 1;
    return 0;
}

/* -------------------------
*/
static int setforkmode(struct adouble *adp, int eid, int ofrefnum, off_t what)
{
    return ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, what, 1, ofrefnum);
}

/* -------------------------
*/
int getforkmode(struct adouble *adp, int eid, off_t what)
{
    return ad_testlock(adp, eid,  what);
}

/* -------------------------
*/
static int fork_setmode(struct adouble *adp, int eid, int access, int ofrefnum)
{
    int ret;
    int readset;
    int writeset;
    int denyreadset;
    int denywriteset;

    if (! (access & (OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD))) {
        return setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_NONE);
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
        if ((access & OPENACC_RD)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_RD);
            if (ret)
                return ret;
        }
        if ((access & OPENACC_DRD)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_DENY_RD);
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
        if ((access & OPENACC_WR)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_OPEN_WR);
            if (ret)
                return ret;
        }
        if ((access & OPENACC_DWR)) {
            ret = setforkmode(adp, eid, ofrefnum, AD_FILELOCK_DENY_WR);
            if (ret)
                return ret;
        }
    }
    if ( access == (OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD)) {
        return ad_excl_lock(adp, eid);
    }
    return 0;
}

/* ----------------------- */
int afp_openfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct vol		*vol;
    struct dir		*dir;
    struct ofork	*ofork, *opened;
    struct adouble      *adsame = NULL;
    size_t		buflen;
    int			ret, adflags, eid;
    u_int32_t           did;
    u_int16_t		vid, bitmap, access, ofrefnum;
    char		fork, *path, *upath;
    struct stat         *st;
    u_int16_t           bshort;
    struct path         *s_path;
    
    ibuf++;
    fork = *ibuf++;
    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof(vid);

    *rbuflen = 0;
    if (NULL == ( vol = getvolbyvid( vid ))) {
        return( AFPERR_PARAM );
    }

    memcpy(&did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    if (NULL == ( dir = dirlookup( vol, did ))) {
	return afp_errno;    
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

    if (NULL == ( s_path = cname( vol, dir, &ibuf ))) {
	return get_afp_errno(AFPERR_PARAM);    
    }

    if (*s_path->m_name == '\0') {
       /* it's a dir ! */
       return  AFPERR_BADTYPE;
    }

    /* stat() data fork st is set because it's not a dir */
    switch ( s_path->st_errno ) {
    case 0:
        break;
    case ENOENT:
        return AFPERR_NOOBJ;
    case EACCES:
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;
    default:
        LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name, strerror(errno) );
        return AFPERR_PARAM;
    }
    /* FIXME should we check it first ? */
    upath = s_path->u_name;
    if (!vol_unix_priv(vol)) {
        if (check_access(upath, access ) < 0) {
            return AFPERR_ACCESS;
        }
    }
    else {
        if (file_access(s_path, access ) < 0) {
            return AFPERR_ACCESS;
        }
    }

    st   = &s_path->st;
    /* XXX: this probably isn't the best way to do this. the already
       open bits should really be set if the fork is opened by any
       program, not just this one. however, that's problematic to do
       if we can't write lock files somewhere. opened is also passed to 
       ad_open so that we can keep file locks together.
       FIXME: add the fork we are opening? 
    */
    if ((opened = of_findname(vol, s_path))) {
        adsame = opened->of_ad;
    }

    if ( fork == OPENFORK_DATA ) {
        eid = ADEID_DFORK;
        adflags = ADFLAGS_DF|ADFLAGS_HF;
    } else {
        eid = ADEID_RFORK;
        adflags = ADFLAGS_HF;
    }

    path = s_path->m_name;
    if (( ofork = of_alloc(vol, curdir, path, &ofrefnum, eid,
                           adsame, st)) == NULL ) {
        return( AFPERR_NFILE );
    }

    ret = AFPERR_NOOBJ;
    if (access & OPENACC_WR) {
        /* try opening in read-write mode */
        if (ad_open(upath, adflags, O_RDWR, 0, ofork->of_ad) < 0) {
            switch ( errno ) {
            case EROFS:
                ret = AFPERR_VLOCK;
            case EACCES:
                goto openfork_err;
                break;
            case ENOENT:
                if (fork == OPENFORK_DATA) {
                    /* try to open only the data fork */
                    if (ad_open(upath, ADFLAGS_DF, O_RDWR, 0, ofork->of_ad) < 0) {
                        goto openfork_err;
                    }
                    adflags = ADFLAGS_DF;
                }
                else {
                    /* here's the deal. we only try to create the resource
                    * fork if the user wants to open it for write acess. */
                    if (ad_open(upath, adflags, O_RDWR | O_CREAT, 0666, ofork->of_ad) < 0)
                        goto openfork_err;
                    ofork->of_flags |= AFPFORK_OPEN;
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
                LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name, strerror(errno) );
                ret = AFPERR_PARAM;
                goto openfork_err;
                break;
            }
        }
        else {
            /* the ressource fork is open too */
            ofork->of_flags |= AFPFORK_OPEN;
        }
    } else {
        /* try opening in read-only mode */
        ret = AFPERR_NOOBJ;
        if (ad_open(upath, adflags, O_RDONLY, 0, ofork->of_ad) < 0) {
            switch ( errno ) {
            case EROFS:
                ret = AFPERR_VLOCK;
            case EACCES:
                goto openfork_err;
                break;
            case ENOENT:
                /* see if client asked for a read only data fork */
                if (fork == OPENFORK_DATA) {
                    if (ad_open(upath, ADFLAGS_DF, O_RDONLY, 0, ofork->of_ad) < 0) {
                        goto openfork_err;
                    }
                    adflags = ADFLAGS_DF;
                }
                /* else we don't set AFPFORK_OPEN because there's no ressource fork file 
                 * We need to check AFPFORK_OPEN in afp_closefork(). eg fork open read-only
                 * then create in open read-write.
                 * FIXME , it doesn't play well with byte locking example:
                 * ressource fork open read only
                 * locking set on it (no effect, there's no file!)
                 * ressource fork open read write now
                */
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
                LOG(log_error, logtype_afpd, "afp_openfork('%s/%s'): ad_open: errno: %i (%s)",
                    getcwdpath, s_path->m_name, errno, strerror(errno) );
                goto openfork_err;
                break;
            }
        }
        else {
            /* the ressource fork is open too */
            ofork->of_flags |= AFPFORK_OPEN;
        }
    }

    if ((adflags & ADFLAGS_HF) && (ad_get_HF_flags( ofork->of_ad) & O_CREAT)) {
        if (ad_setname(ofork->of_ad, path)) {
            ad_flush( ofork->of_ad );
        }
    }

    if (( ret = getforkparams(ofork, bitmap, rbuf + 2 * sizeof( u_int16_t ),
                              &buflen )) != AFP_OK ) {
        ad_close( ofork->of_ad, adflags );
        goto openfork_err;
    }

    *rbuflen = buflen + 2 * sizeof( u_int16_t );
    bitmap = htons( bitmap );
    memcpy(rbuf, &bitmap, sizeof( u_int16_t ));
    rbuf += sizeof( u_int16_t );

    /* check  WriteInhibit bit if we have a ressource fork
     * the test is done here, after some Mac trafic capture 
     */
    if (ad_meta_fileno(ofork->of_ad) != -1) {   /* META */
        ad_getattr(ofork->of_ad, &bshort);
        if ((bshort & htons(ATTRBIT_NOWRITE)) && (access & OPENACC_WR)) {
            ad_close( ofork->of_ad, adflags );
            of_dealloc( ofork );
            ofrefnum = 0;
            memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
            return(AFPERR_OLOCK);
        }
    }

    /*
     * synchronization locks:
     */

    /* don't try to lock non-existent rforks. */
    if ((eid == ADEID_DFORK) || (ad_meta_fileno(ofork->of_ad) != -1)) { /* META */

        ret = fork_setmode(ofork->of_ad, eid, access, ofrefnum);
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
                LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_lock: %s", s_path->m_name, strerror(ret) );
                return( AFPERR_PARAM );
            }
        }
        if ((access & OPENACC_WR))
            ofork->of_flags |= AFPFORK_ACCWR;
    }
    /* the file may be open read only without ressource fork */
    if ((access & OPENACC_RD))
        ofork->of_flags |= AFPFORK_ACCRD;

    memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
    return( AFP_OK );

openfork_err:
    of_dealloc( ofork );
    if (errno == EACCES)
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;
    return ret;
}

int afp_setforkparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen, char *rbuf _U_, size_t *rbuflen)
{
    struct ofork	*ofork;
    off_t		size;
    u_int16_t		ofrefnum, bitmap;
    int                 err;
    int                 is64;
    int                 eid;
    off_t		st_size;
    
    ibuf += 2;

    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof( bitmap );

    *rbuflen = 0;
    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_setforkparams: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }

    if (ofork->of_vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0)
        return AFPERR_ACCESS;

    if ( ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else
        return AFPERR_PARAM;

    if ( ( (bitmap & ( (1<<FILPBIT_DFLEN) | (1<<FILPBIT_EXTDFLEN) )) 
                  && eid == ADEID_RFORK 
         ) ||
         ( (bitmap & ( (1<<FILPBIT_RFLEN) | (1<<FILPBIT_EXTRFLEN) )) 
                  && eid == ADEID_DFORK)) {
        return AFPERR_BITMAP;
    }
    
    is64 = 0;
    if ((bitmap & ( (1<<FILPBIT_EXTDFLEN) | (1<<FILPBIT_EXTRFLEN) ))) {
        if (afp_version >= 30) {
            is64 = 4;
        }
        else 
           return AFPERR_BITMAP;
    }

    if (ibuflen < 2+ sizeof(ofrefnum) + sizeof(bitmap) + is64 +4)
        return AFPERR_PARAM ;
    
    size = get_off_t(&ibuf, is64);

    if (size < 0)
        return AFPERR_PARAM; /* Some MacOS don't return an error they just don't change the size! */


    if (bitmap == (1<<FILPBIT_DFLEN) || bitmap == (1<<FILPBIT_EXTDFLEN)) {
    	st_size = ad_size(ofork->of_ad, eid);
    	err = -2;
    	if (st_size > size && 
    	      ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size -size, ofork->of_refnum) < 0) 
            goto afp_setfork_err;

        err = ad_dtruncate( ofork->of_ad, size );
        if (st_size > size)
 	    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size -size, ofork->of_refnum);  
        if (err < 0)
            goto afp_setfork_err;
    } else if (bitmap == (1<<FILPBIT_RFLEN) || bitmap == (1<<FILPBIT_EXTRFLEN)) {
        ad_refresh( ofork->of_ad );

    	st_size = ad_size(ofork->of_ad, eid);
    	err = -2;
    	if (st_size > size && 
    	       ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size -size, ofork->of_refnum) < 0) {
            goto afp_setfork_err;
	}
        err = ad_rtruncate(ofork->of_ad, size);
        if (st_size > size)
 	    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size -size, ofork->of_refnum);  
        if (err < 0)
            goto afp_setfork_err;

        if (ad_flush( ofork->of_ad ) < 0) {
            LOG(log_error, logtype_afpd, "afp_setforkparams(%s): ad_flush: %s", of_name(ofork), strerror(errno) );
            return AFPERR_PARAM;
        }
    } else
        return AFPERR_BITMAP;

#ifdef AFS
    if ( flushfork( ofork ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_setforkparams(%s): flushfork: %s", of_name(ofork), strerror(errno) );
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
            LOG(log_error, logtype_afpd, "afp_setforkparams: DISK FULL");
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


/* ---------------------- */
static int byte_lock(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen, int is64)
{
    struct ofork	*ofork;
    off_t               offset, length;
    int                 eid;
    u_int16_t		ofrefnum;
    u_int8_t            flags;
    int                 lockop;
    
    *rbuflen = 0;

    /* figure out parameters */
    ibuf++;
    flags = *ibuf; /* first bit = endflag, lastbit = lockflag */
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_bytelock: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }

    if ( ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else
        return AFPERR_PARAM;

    offset = get_off_t(&ibuf, is64);
    length = get_off_t(&ibuf, is64);

    /* FIXME AD_FILELOCK test is surely wrong */
    if (length == -1)
        length = BYTELOCK_MAX;
     else if (!length || is_neg(is64, length)) {
     	return AFPERR_PARAM;
     } else if ((length >= AD_FILELOCK_BASE) && -1 == (ad_reso_fileno(ofork->of_ad))) { /* HF ?*/
        return AFPERR_LOCK;
    }

    if (ENDBIT(flags)) {
        offset += ad_size(ofork->of_ad, eid);
        /* FIXME what do we do if file size > 2 GB and 
           it's not byte_lock_ext?
        */
    }
    if (offset < 0)    /* error if we have a negative offset */
        return AFPERR_PARAM;

    /* if the file is a read-only file, we use read locks instead of
     * write locks. that way, we can prevent anyone from initiating
     * a write lock. */
    lockop = UNLOCKBIT(flags) ? ADLOCK_CLR : ADLOCK_WR;
    if (ad_lock(ofork->of_ad, eid, lockop, offset, length,
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
    *rbuflen = set_off_t (offset, rbuf, is64);
    return( AFP_OK );
}

/* --------------------------- */
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
   return byte_lock ( obj, ibuf, ibuflen, rbuf, rbuflen , 0);
}

/* --------------------------- */
int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
   return byte_lock ( obj, ibuf, ibuflen, rbuf, rbuflen , 1);
}

#undef UNLOCKBIT

/* --------------------------- */
static int crlf(struct ofork *of)
{
    struct extmap	*em;

    if ( ad_meta_fileno( of->of_ad ) == -1 || !memcmp( ufinderi, ad_entry( of->of_ad, ADEID_FINDERI),8)) { /* META */
        /* no resource fork or no finderinfo, use our files extension mapping */
        if (!( em = getextmap( of_name(of) )) || memcmp( "TEXT", em->em_type, sizeof( em->em_type ))) {
            return 0;
        } 
        /* file type is TEXT */
        return 1;

    } else if ( !memcmp( "TEXT", ad_entry( of->of_ad, ADEID_FINDERI ), 4 )) {
        return 1;
    }
    return 0;
}


static ssize_t read_file(struct ofork *ofork, int eid,
                                    off_t offset, u_char nlmask,
                                    u_char nlchar, char *rbuf,
                                    size_t *rbuflen, const int xlate)
{
    ssize_t cc;
    int eof = 0;
    char *p, *q;

    cc = ad_read(ofork->of_ad, eid, offset, rbuf, *rbuflen);
    if ( cc < 0 ) {
        LOG(log_error, logtype_afpd, "afp_read(%s): ad_read: %s", of_name(ofork), strerror(errno) );
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }
    if ( (size_t)cc < *rbuflen ) {
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
static int read_fork(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen, int is64)
{
    struct ofork	*ofork;
    off_t		offset, saveoff, reqcount, savereqcount;
    ssize_t		cc, err;
    int			eid, xlate = 0;
    u_int16_t		ofrefnum;
    u_char		nlmask, nlchar;

    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( u_short );

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_read: of_find(%d) could not locate fork", ofrefnum );
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCRD) == 0) {
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }
    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    if (is64) {
        nlmask = nlchar = 0;
    }
    else {
        nlmask = *ibuf++;
        nlchar = *ibuf++;
    }
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

    LOG(log_debug, logtype_afpd, "afp_read(name: \"%s\", offset: %jd, reqcount: %jd)",
        of_name(ofork), (intmax_t)offset, (intmax_t)reqcount);

    savereqcount = reqcount;
    saveoff = offset;
    if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, saveoff, savereqcount,ofork->of_refnum) < 0) {
        err = AFPERR_LOCK;
        goto afp_read_err;
    }

    *rbuflen = MIN(reqcount, *rbuflen);
    LOG(log_debug, logtype_afpd, "afp_read(name: \"%s\", offset: %jd, reqcount: %jd): reading %jd bytes from file",
        of_name(ofork), (intmax_t)offset, (intmax_t)reqcount, (intmax_t)*rbuflen);
    err = read_file(ofork, eid, offset, nlmask, nlchar, rbuf, rbuflen, xlate);
    if (err < 0)
        goto afp_read_done;
    LOG(log_debug, logtype_afpd, "afp_read(name: \"%s\", offset: %jd, reqcount: %jd): got %jd bytes from file",
        of_name(ofork), (intmax_t)offset, (intmax_t)reqcount, (intmax_t)*rbuflen);

    /* dsi can stream requests. we can only do this if we're not checking
     * for an end-of-line character. oh well. */
    if ((obj->proto == AFPPROTO_DSI) && (*rbuflen < reqcount) && !nlmask) {
        DSI    *dsi = obj->handle;
        off_t  size;

        /* reqcount isn't always truthful. we need to deal with that. */
        size = ad_size(ofork->of_ad, eid);

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
        if ((cc = dsi_readinit(dsi, rbuf, *rbuflen, reqcount, err)) < 0)
            goto afp_read_exit;
        *rbuflen = cc;
        /* due to the nature of afp packets, we have to exit if we get
           an error. we can't do this with translation on. */
#ifdef WITH_SENDFILE 
        if (!(xlate || Debug(obj) )) {
            int fd;
                        
            fd = ad_readfile_init(ofork->of_ad, eid, &offset, 0);

            if (dsi_stream_read_file(dsi, fd, offset, dsi->datasize) < 0) {
                if (errno == EINVAL || errno == ENOSYS)
                    goto afp_read_loop;
                else {
                    LOG(log_error, logtype_afpd, "afp_read(%s): ad_readfile: %s", of_name(ofork), strerror(errno));
                    goto afp_read_exit;
                }
            }

            dsi_readdone(dsi);
            goto afp_read_done;
        }

afp_read_loop:
#endif 

        /* fill up our buffer. */
        while (*rbuflen > 0) {
            cc = read_file(ofork, eid, offset, nlmask, nlchar, rbuf,rbuflen, xlate);
            if (cc < 0)
                goto afp_read_exit;

            offset += *rbuflen;
            /* dsi_read() also returns buffer size of next allocation */
            cc = dsi_read(dsi, rbuf, *rbuflen); /* send it off */
            if (cc < 0)
                goto afp_read_exit;
            *rbuflen = cc;
        }
        dsi_readdone(dsi);
        goto afp_read_done;

afp_read_exit:
        LOG(log_error, logtype_afpd, "afp_read(%s): %s", of_name(ofork), strerror(errno));
        dsi_readdone(dsi);
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,ofork->of_refnum);
        obj->exit(EXITERR_CLNT);
    }

afp_read_done:
    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,ofork->of_refnum);
    return err;

afp_read_err:
    *rbuflen = 0;
    return err;
}

/* ---------------------- */
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* ---------------------- */
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/* ---------------------- */
int afp_flush(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol *vol;
    u_int16_t vid;

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&vid, ibuf, sizeof(vid));
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    of_flush(vol);
    return( AFP_OK );
}

int afp_flushfork(AFPObj *obj _U_, char	*ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct ofork	*ofork;
    u_int16_t		ofrefnum;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_flushfork: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }

    if ( flushfork( ofork ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_flushfork(%s): %s", of_name(ofork), strerror(errno) );
    }

    return( AFP_OK );
}

/*
  FIXME
  There is a lot to tell about fsync, fdatasync, F_FULLFSYNC.
  fsync(2) on OSX is implemented differently than on other platforms.
  see: http://mirror.linux.org.au/pub/linux.conf.au/2007/video/talks/278.pdf.
 */
int afp_syncfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct ofork        *ofork;
    u_int16_t           ofrefnum;

    *rbuflen = 0;
    ibuf += 2;

    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof( ofrefnum );

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afpd_syncfork: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }

    if ( flushfork( ofork ) < 0 ) {
	LOG(log_error, logtype_afpd, "flushfork(%s): %s", of_name(ofork), strerror(errno) );
	return AFPERR_MISC;
    }

    return( AFP_OK );
}

/* this is very similar to closefork */
int flushfork(struct ofork *ofork)
{
    struct timeval tv;

    int err = 0, doflush = 0;

    if ( ad_data_fileno( ofork->of_ad ) != -1 &&
            fsync( ad_data_fileno( ofork->of_ad )) < 0 ) {
        LOG(log_error, logtype_afpd, "flushfork(%s): dfile(%d) %s",
            of_name(ofork), ad_data_fileno(ofork->of_ad), strerror(errno) );
        err = -1;
    }

    if ( ad_reso_fileno( ofork->of_ad ) != -1 &&  /* HF */
	 (ofork->of_flags & AFPFORK_RSRC)) {

        /* read in the rfork length */
        ad_refresh(ofork->of_ad);

        /* set the date if we're dirty */
        if ((ofork->of_flags & AFPFORK_DIRTY) && !gettimeofday(&tv, NULL)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY|AD_DATE_UNIX, tv.tv_sec);
            ofork->of_flags &= ~AFPFORK_DIRTY;
            doflush++;
        }

        /* flush the header */
        if (doflush && ad_flush(ofork->of_ad) < 0)
                err = -1;

        if (fsync( ad_reso_fileno( ofork->of_ad )) < 0)
            err = -1;

        if (err < 0)
            LOG(log_error, logtype_afpd, "flushfork(%s): hfile(%d) %s",
                of_name(ofork), ad_reso_fileno(ofork->of_ad), strerror(errno) );
    }

    return( err );
}

int afp_closefork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct ofork	*ofork;
    u_int16_t		ofrefnum;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_closefork: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }
    if ( of_closefork( ofork ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_closefork(%s): of_closefork: %s", of_name(ofork), strerror(errno) );
        return( AFPERR_PARAM );
    }

    return( AFP_OK );
}


static ssize_t write_file(struct ofork *ofork, int eid,
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
            LOG(log_error, logtype_afpd, "write_file: DISK FULL");
            return( AFPERR_DFULL );
        case EACCES:
            return AFPERR_ACCESS;
        default :
            LOG(log_error, logtype_afpd, "afp_write(%s): ad_write: %s", of_name(ofork), strerror(errno) );
            return( AFPERR_PARAM );
        }
    }

    return cc;
}


/* FPWrite. NOTE: on an error, we always use afp_write_err as
 * the client may have sent us a bunch of data that's not reflected 
 * in reqcount et al. */
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen, int is64)
{
    struct ofork	*ofork;
    off_t           offset, saveoff, reqcount, oldsize, newsize;
    int		        endflag, eid, xlate = 0, err = AFP_OK;
    u_int16_t		ofrefnum;
    ssize_t             cc;

    /* figure out parameters */
    ibuf++;
    endflag = ENDBIT(*ibuf);
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );

    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_write: of_find(%d) could not locate fork", ofrefnum );
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

    oldsize = ad_size(ofork->of_ad, eid);
    if (endflag)
        offset += oldsize;

    /* handle bogus parameters */
    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    newsize = ((offset + reqcount) > oldsize) ? (offset + reqcount) : oldsize;

    /* offset can overflow on 64-bit capable filesystems.
     * report disk full if that's going to happen. */
     if (sum_neg(is64, offset, reqcount)) {
        LOG(log_error, logtype_afpd, "write_fork: DISK FULL");
        err = AFPERR_DFULL;
        goto afp_write_err;
    }

    if (!reqcount) { /* handle request counts of 0 */
        err = AFP_OK;
        *rbuflen = set_off_t (offset, rbuf, is64);
        goto afp_write_err;
    }

    saveoff = offset;
    if (ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, saveoff,
                   reqcount, ofork->of_refnum) < 0) {
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

#ifdef DEBUG1
        if (obj->options.flags & OPTION_DEBUG) {
            printf("(write) len: %d\n", *rbuflen);
            bprint(rbuf, *rbuflen);
        }
#endif
        if ((cc = write_file(ofork, eid, offset, rbuf, *rbuflen,
                             xlate)) < 0) {
            *rbuflen = 0;
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount, ofork->of_refnum);
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
            if (!cc || (cc = write_file(ofork, eid, offset, rbuf, cc, xlate)) < 0) {
                dsi_writeflush(dsi);
                *rbuflen = 0;
                ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount, ofork->of_refnum);
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
                               reqcount,  ofork->of_refnum);
                    return cc;
                }

                offset += cc;
                goto afp_write_done;
            }
#endif /* 0, was HAVE_SENDFILE_WRITE */

            /* loop until everything gets written. currently
                    * dsi_write handles the end case by itself. */
            while ((cc = dsi_write(dsi, rbuf, *rbuflen))) {
                if ((cc = write_file(ofork, eid, offset, rbuf, cc, xlate)) < 0) {
                    dsi_writeflush(dsi);
                    *rbuflen = 0;
                    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff,
                               reqcount,  ofork->of_refnum);
                    return cc;
                }
                offset += cc;
            }
        }
        break;
    }

    ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount,  ofork->of_refnum);
    if ( ad_meta_fileno( ofork->of_ad ) != -1 ) /* META */
        ofork->of_flags |= AFPFORK_DIRTY;

    /* we have modified any fork, remember until close_fork */
    ofork->of_flags |= AFPFORK_MODIFIED;

    /* update write count */
    ofork->of_vol->v_appended += (newsize > oldsize) ? (newsize - oldsize) : 0;

    *rbuflen = set_off_t (offset, rbuf, is64);
    return( AFP_OK );

afp_write_err:
    if (obj->proto == AFPPROTO_DSI) {
        dsi_writeinit(obj->handle, rbuf, *rbuflen);
        dsi_writeflush(obj->handle);
    }
    if (err != AFP_OK) {
        *rbuflen = 0;
    }
    return err;
}

/* ---------------------------- */
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/* ---------------------------- 
 * FIXME need to deal with SIGXFSZ signal
*/
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/* ---------------------------- */
int afp_getforkparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct ofork	*ofork;
    int             ret;
    u_int16_t		ofrefnum, bitmap;
    size_t          buflen;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof( ofrefnum ));
    ibuf += sizeof( ofrefnum );
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    *rbuflen = 0;
    if (NULL == ( ofork = of_find( ofrefnum )) ) {
        LOG(log_error, logtype_afpd, "afp_getforkparams: of_find(%d) could not locate fork", ofrefnum );
        return( AFPERR_PARAM );
    }

    if ( ad_meta_fileno( ofork->of_ad ) != -1 ) { /* META */
        if ( ad_refresh( ofork->of_ad ) < 0 ) {
            LOG(log_error, logtype_afpd, "getforkparams(%s): ad_refresh: %s", of_name(ofork), strerror(errno) );
            return( AFPERR_PARAM );
        }
    }

    if (AFP_OK != ( ret = getforkparams( ofork, bitmap,
                               rbuf + sizeof( u_short ), &buflen ))) {
        return( ret );
    }

    *rbuflen = buflen + sizeof( u_short );
    bitmap = htons( bitmap );
    memcpy(rbuf, &bitmap, sizeof( bitmap ));
    return( AFP_OK );
}

