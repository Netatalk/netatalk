/*
 * $Id: enumerate.c,v 1.40 2003-05-16 14:38:20 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include <netatalk/endian.h>
#include <atalk/afp.h>
#include <atalk/adouble.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB */
#include "desktop.h"
#include "directory.h"
#include "volume.h"
#include "globals.h"
#include "file.h"
#include "fork.h"
#include "filedir.h"

#define min(a,b)	((a)<(b)?(a):(b))

/* ---------------------------- */
struct dir *
            adddir( vol, dir, path)
struct vol	*vol;
struct dir	*dir;
struct path     *path;
{
    struct dir	*cdir, *edir;
    int		upathlen;
    char        *name;
    char        *upath;
    struct stat *st;
    int         deleted;

    upath = path->u_name;
    name  = path->m_name;    
    st    = &path->st;
    upathlen = strlen(upath);
    if ((cdir = dirnew(name, upath)) == NULL) {
        LOG(log_error, logtype_afpd, "adddir: malloc: %s", strerror(errno) );
        return NULL;
    }

    cdir->d_did = get_id(vol, NULL, st, dir->d_did, upath, upathlen);
    if (cdir->d_did == 0) 
        return NULL;

    if ((edir = dirinsert( vol, cdir ))) {
        /* it's not possible with LASTDID
           for CNID:
           - someone else have moved the directory.
           - it's a symlink inside the share.
           - it's an ID reused, the old directory was deleted but not
             the cnid record and the server've reused the inode for 
             the new dir.
           for HASH (we should get ride of HASH) 
           - someone else have moved the directory.
           - it's an ID reused as above
           - it's a hash duplicate and we are in big trouble
        */
        deleted = (edir->d_m_name == NULL);
        dirfreename(edir);
        edir->d_m_name = cdir->d_m_name;
        edir->d_u_name = cdir->d_u_name;
        free(cdir);
        cdir = edir;
        if (!cdir->d_parent || (cdir->d_parent == dir && !deleted))
            return cdir;
        /* the old was not in the same folder */
        if (!deleted)
            dirchildremove(cdir->d_parent, cdir);
    }

    /* parent/child directories */
    cdir->d_parent = dir;
    dirchildadd(dir, cdir);
    return( cdir );
}
/*
 * Struct to save directory reading context in. Used to prevent
 * O(n^2) searches on a directory.
 */
struct savedir {
    u_short	 sd_vid;
    u_int32_t	 sd_did;
    int		 sd_buflen;
    char	 *sd_buf;
    char	 *sd_last;
    unsigned int sd_sindex;
};
#define SDBUFBRK	2048

static int enumerate_loop(struct dirent *de, char *mname, void *data)
{
    struct savedir *sd = data; 
    char *start, *end;
    int  len,lenm;
    
    end = sd->sd_buf + sd->sd_buflen;
    len = strlen(de->d_name);
    *(sd->sd_last)++ = len;
    lenm = strlen(mname);
    if ( sd->sd_last + len +lenm + 4 > end ) {
        char *buf;

        start = sd->sd_buf;
        if (!(buf = realloc( sd->sd_buf, sd->sd_buflen +SDBUFBRK )) ) {
            LOG(log_error, logtype_afpd, "afp_enumerate: realloc: %s",
                        strerror(errno) );
            errno = ENOMEM;
            return -1;
        }
        sd->sd_buf = buf;
        sd->sd_buflen += SDBUFBRK;
        sd->sd_last = ( sd->sd_last - start ) + sd->sd_buf;
        end = sd->sd_buf + sd->sd_buflen;
    }

    memcpy( sd->sd_last, de->d_name, len + 1 );
    sd->sd_last += len + 1;

    *(sd->sd_last)++ = lenm;
    memcpy( sd->sd_last, mname, lenm + 1 );
    sd->sd_last += lenm + 1;
    
    return 0;
}

/* ----------------------------- 
 * FIXME: 
 * Doesn't work with dangling symlink
 * ie: 
 * - Move a folder with a dangling symlink in the trash
 * - empty the trash
 * afp_enumerate return an empty listing but offspring count != 0 in afp_getdirparams 
 * and the Mac doesn't try to call afp_delete!
 *
 * Another option for symlink
 * cf:
 * http://sourceforge.net/tracker/index.php?func=detail&aid=461938&group_id=8642&atid=108642
 * 
*/
char *check_dirent(const struct vol *vol, char *name)
{
    char *m_name = NULL;

    if (!strcmp(name, "..") || !strcmp(name, "."))
        return NULL;

    if (!(validupath(vol, name)))
        return NULL;

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name))
        return NULL;
    if (NULL == (m_name = utompath(vol, name, utf8_encoding()))) 
        return NULL;    

    /* now check against too big a file */
    if (strlen(m_name) > vol->max_filename)
        return NULL;

    return m_name;
}

/* ----------------------------- */
int 
for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
{
    DIR             *dp;
    struct dirent	*de;
    char            *m_name;
    int             ret;
    
    if (NULL == ( dp = opendir( name)) ) {
        return -1;
    }
    ret = 0;
    for ( de = readdir( dp ); de != NULL; de = readdir( dp )) {
        if (!(m_name = check_dirent(vol, de->d_name)))
            continue;

        ret++;
        if (fn && fn(de,m_name, data) < 0) {
           closedir(dp);
           return -1;
        }
    }
    closedir(dp);
    return ret;
}

/* ----------------------------- */
static int enumerate(obj, ibuf, ibuflen, rbuf, rbuflen, ext )
AFPObj       *obj;
char	     *ibuf, *rbuf;
unsigned int ibuflen, *rbuflen;
int     ext;
{
    static struct savedir	sd = { 0, 0, 0, NULL, NULL, 0 };
    struct vol			*vol;
    struct dir			*dir;
    int				did, ret, esz, len, first = 1;
    char                        *data, *start;
    u_int16_t			vid, fbitmap, dbitmap, reqcnt, actcnt = 0;
    u_int16_t			temp16;
    u_int32_t			sindex, maxsz, sz = 0;
    struct path                 *o_path;
    struct path                 s_path;
    int                         header;
        
    if ( sd.sd_buflen == 0 ) {
        if (( sd.sd_buf = (char *)malloc( SDBUFBRK )) == NULL ) {
            LOG(log_error, logtype_afpd, "afp_enumerate: malloc: %s", strerror(errno) );
            *rbuflen = 0;
            return AFPERR_MISC;
        }
        sd.sd_buflen = SDBUFBRK;
    }

    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        *rbuflen = 0;
        return (afp_errno == AFPERR_NOOBJ)?AFPERR_NODIR:afp_errno;
    }

    memcpy( &fbitmap, ibuf, sizeof( fbitmap ));
    fbitmap = ntohs( fbitmap );
    ibuf += sizeof( fbitmap );

    memcpy( &dbitmap, ibuf, sizeof( dbitmap ));
    dbitmap = ntohs( dbitmap );
    ibuf += sizeof( dbitmap );

    /* check for proper bitmaps -- the stuff in comments is for
     * variable directory ids. */
    if (!(fbitmap || dbitmap)
            /*|| (fbitmap & (1 << FILPBIT_PDID)) ||
              (dbitmap & (1 << DIRPBIT_PDID))*/) {
        *rbuflen = 0;
        return AFPERR_BITMAP;
    }

    memcpy( &reqcnt, ibuf, sizeof( reqcnt ));
    reqcnt = ntohs( reqcnt );
    ibuf += sizeof( reqcnt );

    if (ext == 2) {
        memcpy( &sindex, ibuf, sizeof( sindex ));
        sindex = ntohl( sindex );
        ibuf += sizeof( sindex );
    }
    else {
        memcpy( &temp16, ibuf, sizeof( temp16 ));
        sindex = ntohs( temp16 );
        ibuf += sizeof( temp16 );
    }

    if (!sindex) {
        *rbuflen = 0;
        return AFPERR_PARAM ;
    }

    if (ext == 2) {
        memcpy( &maxsz, ibuf, sizeof( maxsz ));
        maxsz = ntohl( maxsz );
        ibuf += sizeof( maxsz );
    }
    else {
        memcpy( &temp16, ibuf, sizeof( temp16 ));
        maxsz = ntohs( temp16 );
        ibuf += sizeof( temp16 );
    }
    
    header = (ext)?4:2;
    header *=sizeof( u_char );
    
    maxsz = min(maxsz, *rbuflen);
    o_path = cname( vol, dir, &ibuf );

    if (afp_errno == AFPERR_NOOBJ) 
        afp_errno = AFPERR_NODIR;

    *rbuflen = 0;
    if (NULL == o_path ) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }
    if ( *o_path->m_name != '\0') {
        /* it's a file or it's a dir and extendir() was unable to chdir in it */
        return path_error(o_path, AFPERR_NODIR );
    }

    data = rbuf + 3 * sizeof( u_int16_t );
    sz = 3 * sizeof( u_int16_t );	/* fbitmap, dbitmap, reqcount */

    /*
     * Read the directory into a pre-malloced buffer, stored
     *		len <name> \0
     * The end is indicated by a len of 0.
     */
    if ( sindex == 1 || curdir->d_did != sd.sd_did || vid != sd.sd_vid ) {
        sd.sd_last = sd.sd_buf;
        /* if dir was in the cache we don't have the inode */
        if (( !o_path->st_valid && stat( ".", &o_path->st ) < 0 ) ||
              (ret = for_each_dirent(vol, ".", enumerate_loop, (void *)&sd)) < 0) 
        {
            switch (errno) {
            case EACCES:
		return AFPERR_ACCESS;
            case ENOTDIR:
                return AFPERR_BADTYPE;
            case ENOMEM:
                return AFPERR_MISC;
            default:
                return AFPERR_NODIR;
            }
        }
        curdir->ctime  = o_path->st.st_ctime; /* play safe */
        curdir->offcnt = ret;
        *sd.sd_last = 0;

        sd.sd_last = sd.sd_buf;
        sd.sd_sindex = 1;

        sd.sd_vid = vid;
        sd.sd_did = curdir->d_did;
    }

    /*
     * Position sd_last as dictated by sindex.
     */
    if ( sindex < sd.sd_sindex ) {
        sd.sd_sindex = 1;
        sd.sd_last = sd.sd_buf;
    }
    while ( sd.sd_sindex < sindex ) {
        len = *(sd.sd_last)++;
        if ( len == 0 ) {
            sd.sd_did = 0;	/* invalidate sd struct to force re-read */
            return( AFPERR_NOOBJ );
        }
        sd.sd_last += len + 1;
        len = *(sd.sd_last)++;
        sd.sd_last += len + 1;
        sd.sd_sindex++;
    }

    while (( len = *(sd.sd_last)) != 0 ) {
        /*
         * If we've got all we need, send it.
         */
        if ( actcnt == reqcnt ) {
            break;
        }

        /*
         * Save the start position, in case we exceed the buffer
         * limitation, and have to back up one.
         */
        start = sd.sd_last;
        sd.sd_last++;

        if (*sd.sd_last == 0) {
            /* stat() already failed on this one */
            sd.sd_last += len + 1;
            len = *(sd.sd_last)++;
            sd.sd_last += len + 1;
            continue;
        }
        s_path.u_name = sd.sd_last;
        if (of_stat( &s_path) < 0 ) {
            /*
             * Somebody else plays with the dir, well it can be us with 
            * "Empty Trash..."
            */

            /* so the next time it won't try to stat it again
             * another solution would be to invalidate the cache with 
             * sd.sd_did = 0 but if it's not ENOENT error it will start again
             */
            *sd.sd_last = 0;
            sd.sd_last += len + 1;
            len = *(sd.sd_last)++;
            sd.sd_last += len + 1;
            curdir->offcnt--;		/* a little lie */
            continue;
        }

        sd.sd_last += len + 1;
        len = *(sd.sd_last)++;
        s_path.m_name = sd.sd_last;
        sd.sd_last += len + 1;
        /*
         * If a fil/dir is not a dir, it's a file. This is slightly
         * inaccurate, since that means /dev/null is a file, /dev/printer
         * is a file, etc.
         */
        if ( S_ISDIR(s_path.st.st_mode)) {
            if ( dbitmap == 0 ) {
                continue;
            }
            dir = dirsearch_byname(curdir, s_path.u_name);
            if (!dir) {
                if (s_path.m_name == NULL || (dir = adddir( vol, curdir, &s_path)) == NULL) {
                    return AFPERR_MISC;
                }
            }
            else {
                s_path.m_name = NULL;
            }
            if (AFP_OK != ( ret = getdirparams(vol, dbitmap, &s_path, dir,
                                     data + header , &esz ))) {
                return( ret );
            }

        } else {
            if ( fbitmap == 0 ) {
                continue;
            }
            if (s_path.m_name == NULL ) {
                return AFPERR_MISC;
            }
            if (AFP_OK != ( ret = getfilparams(vol, fbitmap, &s_path, curdir, 
                                     data + header , &esz )) ) {
                return( ret );
            }
        }

        /*
         * Make sure entry is an even length, possibly with a null
         * byte on the end.
         */
        if ( (esz + header) & 1 ) {
            *(data + header + esz ) = '\0';
            esz++;
        }

        /*
         * Check if we've exceeded the size limit.
         */
        if ( maxsz < sz + esz + header) {
            if (first) { /* maxsz can't hold a single reply */
                return AFPERR_PARAM;
            }
            sd.sd_last = start;
            break;
        }

        if (first)
            first = 0;

        sz += esz + header;
        if (ext) {
            temp16 = htons( esz + header );
            memcpy( data, &temp16, sizeof( temp16 ));
            data += sizeof(temp16);
        }
        else {
            *data++ = esz + header;
        }

        *data++ = S_ISDIR(s_path.st.st_mode) ? FILDIRBIT_ISDIR : FILDIRBIT_ISFILE;
        if (ext) {
             *data++ = 0;
        }
        data += esz;
        actcnt++;
    }

    if ( actcnt == 0 ) {
        sd.sd_did = 0;		/* invalidate sd struct to force re-read */
        return( AFPERR_NOOBJ );
    }
    sd.sd_sindex = sindex + actcnt;

    /*
     * All done, fill in misc junk in rbuf
     */
    fbitmap = htons( fbitmap );
    memcpy( rbuf, &fbitmap, sizeof( fbitmap ));
    rbuf += sizeof( fbitmap );
    dbitmap = htons( dbitmap );
    memcpy( rbuf, &dbitmap, sizeof( dbitmap ));
    rbuf += sizeof( dbitmap );
    actcnt = htons( actcnt );
    memcpy( rbuf, &actcnt, sizeof( actcnt ));
    rbuf += sizeof( actcnt );
    *rbuflen = sz;
    return( AFP_OK );
}

/* ----------------------------- */
int afp_enumerate(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj       *obj;
char	     *ibuf, *rbuf;
unsigned int ibuflen, *rbuflen;
{
    return enumerate(obj, ibuf,ibuflen ,rbuf,rbuflen , 0);
}

/* ----------------------------- */
int afp_enumerate_ext(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj       *obj;
char	     *ibuf, *rbuf;
unsigned int ibuflen, *rbuflen;
{
    return enumerate(obj, ibuf,ibuflen ,rbuf,rbuflen , 1);
}

/* ----------------------------- */
int afp_enumerate_ext2(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj       *obj;
char	     *ibuf, *rbuf;
unsigned int ibuflen, *rbuflen;
{
    return enumerate(obj, ibuf,ibuflen ,rbuf,rbuflen , 2);
}

