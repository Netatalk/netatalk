/*
 * $Id: desktop.c,v 1.26 2003-04-09 06:05:24 didg Exp $
 *
 * See COPYRIGHT.
 *
 * bug:
 * afp_XXXcomment are (the only) functions able to open
 * a ressource fork when there's no data fork, eg after
 * it was removed with samba.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/logger.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/afp.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#include <dirent.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_FCNTL_H
#include <unistd.h>
#endif /* HAVE_FCNTL_H */

#include "volume.h"
#include "directory.h"
#include "fork.h"
#include "globals.h"
#include "desktop.h"

#ifdef FILE_MANGLING
#include "mangle.h"
#endif /* CNID_DB */

#ifdef AFP3x
#include <iconv.h>
#endif

int afp_opendt(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    u_int16_t	vid;

    ibuf += 2;

    memcpy( &vid, ibuf, sizeof(vid));
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }

    memcpy( rbuf, &vid, sizeof(vid));
    *rbuflen = sizeof(vid);
    return( AFP_OK );
}

int afp_closedt(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    *rbuflen = 0;
    return( AFP_OK );
}

struct savedt	si = { { 0, 0, 0, 0 }, -1, 0 };

static int iconopen( vol, creator, flags, mode )
struct vol	*vol;
u_char	creator[ 4 ];
{
    char	*dtf, *adt, *adts;

    if ( si.sdt_fd != -1 ) {
        if ( memcmp( si.sdt_creator, creator, sizeof( CreatorType )) == 0 &&
                si.sdt_vid == vol->v_vid ) {
            return 0;
        }
        close( si.sdt_fd );
        si.sdt_fd = -1;
    }

    dtf = dtfile( vol, creator, ".icon" );

    if (( si.sdt_fd = open( dtf, flags, ad_mode( dtf, mode ))) < 0 ) {
        if ( errno == ENOENT && ( flags & O_CREAT )) {
            if (( adts = strrchr( dtf, '/' )) == NULL ) {
                return -1;
            }
            *adts = '\0';
            if (( adt = strrchr( dtf, '/' )) == NULL ) {
                return -1;
            }
            *adt = '\0';
            (void) ad_mkdir( dtf, DIRBITS | 0777 );
            *adt = '/';
            (void) ad_mkdir( dtf, DIRBITS | 0777 );
            *adts = '/';

            if (( si.sdt_fd = open( dtf, flags, ad_mode( dtf, mode ))) < 0 ) {
                LOG(log_error, logtype_afpd, "iconopen: open %s: %s", dtf, strerror(errno) );
                return -1;
            }
        } else {
            return -1;
        }
    }

    memcpy( si.sdt_creator, creator, sizeof( CreatorType ));
    si.sdt_vid = vol->v_vid;
    si.sdt_index = 1;
    return 0;
}

int afp_addicon(obj, ibuf, ibuflen, rbuf, rbuflen)
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct iovec	iov[ 2 ];
    u_char		fcreator[ 4 ], imh[ 12 ], irh[ 12 ], *p;
    int			itype, cc = AFP_OK, iovcnt = 0, buflen;
    u_int32_t           ftype, itag;
    u_int16_t		bsize, rsize, vid;

    buflen = *rbuflen;
    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
        cc = AFPERR_PARAM;
        goto addicon_err;
    }

    memcpy( fcreator, ibuf, sizeof( fcreator ));
    ibuf += sizeof( fcreator );

    memcpy( &ftype, ibuf, sizeof( ftype ));
    ibuf += sizeof( ftype );

    itype = (unsigned char) *ibuf;
    ibuf += 2;

    memcpy( &itag, ibuf, sizeof( itag ));
    ibuf += sizeof( itag );

    memcpy( &bsize, ibuf, sizeof( bsize ));
    bsize = ntohs( bsize );

    if ( si.sdt_fd != -1 ) {
        (void)close( si.sdt_fd );
        si.sdt_fd = -1;
    }

    if (iconopen( vol, fcreator, O_RDWR|O_CREAT, 0666 ) < 0) {
        cc = AFPERR_NOITEM;
        goto addicon_err;
    }

    if (lseek( si.sdt_fd, (off_t) 0L, SEEK_SET ) < 0) {
        close(si.sdt_fd);
        si.sdt_fd = -1;
        LOG(log_error, logtype_afpd, "afp_addicon: lseek: %s", strerror(errno) );
        cc = AFPERR_PARAM;
        goto addicon_err;
    }

    /*
     * Read icon elements until we find a match to replace, or
     * we get to the end to insert.
     */
    p = imh;
    memcpy( p, &itag, sizeof( itag ));
    p += sizeof( itag );
    memcpy( p, &ftype, sizeof( ftype ));
    p += sizeof( ftype );
    *p++ = itype;
    *p++ = 0;
    bsize = htons( bsize );
    memcpy( p, &bsize, sizeof( bsize ));
    bsize = ntohs( bsize );
    while (( cc = read( si.sdt_fd, irh, sizeof( irh ))) > 0 ) {
        memcpy( &rsize, irh + 10, sizeof( rsize ));
        rsize = ntohs( rsize );
        /*
         * Is this our set of headers?
         */
        if ( memcmp( irh, imh, sizeof( irh ) - sizeof( u_short )) == 0 ) {
            /*
             * Is the size correct?
             */
            if ( bsize != rsize )
                cc = AFPERR_ITYPE;
            break;
        }

        if ( lseek( si.sdt_fd, (off_t) rsize, SEEK_CUR ) < 0 ) {
            LOG(log_error, logtype_afpd, "afp_addicon: lseek: %s", strerror(errno) );
            cc = AFPERR_PARAM;
        }
    }

    /*
     * Some error occurred, return.
     */
addicon_err:
    if ( cc < 0 ) {
        LOG(log_error, logtype_afpd, "afp_addicon: %s", strerror(errno) );
        if (obj->proto == AFPPROTO_DSI) {
            dsi_writeinit(obj->handle, rbuf, buflen);
            dsi_writeflush(obj->handle);
        }
        return cc;
    }


    switch (obj->proto) {
#ifndef NO_DDP
    case AFPPROTO_ASP:
        buflen = bsize;
        if ((asp_wrtcont(obj->handle, rbuf, &buflen) < 0) || buflen != bsize)
            return( AFPERR_PARAM );

        if (obj->options.flags & OPTION_DEBUG) {
            printf("(write) len: %d\n", buflen);
            bprint(rbuf, buflen);
        }

        /*
         * We're at the end of the file, add the headers, etc.  */
        if ( cc == 0 ) {
            iov[ 0 ].iov_base = (caddr_t)imh;
            iov[ 0 ].iov_len = sizeof( imh );
            iov[ 1 ].iov_base = rbuf;
            iov[ 1 ].iov_len = bsize;
            iovcnt = 2;
        }

        /*
         * We found an icon to replace.
         */
        if ( cc > 0 ) {
            iov[ 0 ].iov_base = rbuf;
            iov[ 0 ].iov_len = bsize;
            iovcnt = 1;
        }

        if ( writev( si.sdt_fd, iov, iovcnt ) < 0 ) {
            LOG(log_error, logtype_afpd, "afp_addicon: writev: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }
        break;
#endif /* no afp/asp */      
    case AFPPROTO_DSI:
        {
            DSI *dsi = obj->handle;

            iovcnt = dsi_writeinit(dsi, rbuf, buflen);

            /* add headers at end of file */
            if ((cc == 0) && (write(si.sdt_fd, imh, sizeof(imh)) < 0)) {
                LOG(log_error, logtype_afpd, "afp_addicon: write: %s", strerror(errno));
                dsi_writeflush(dsi);
                return AFPERR_PARAM;
            }

            if ((cc = write(si.sdt_fd, rbuf, iovcnt)) < 0) {
                LOG(log_error, logtype_afpd, "afp_addicon: write: %s", strerror(errno));
                dsi_writeflush(dsi);
                return AFPERR_PARAM;
            }

            while ((iovcnt = dsi_write(dsi, rbuf, buflen))) {
                if ( obj->options.flags & OPTION_DEBUG ) {
                    printf("(write) command cont'd: %d\n", iovcnt);
                    bprint(rbuf, iovcnt);
                }

                if ((cc = write(si.sdt_fd, rbuf, iovcnt)) < 0) {
                    LOG(log_error, logtype_afpd, "afp_addicon: write: %s", strerror(errno));
                    dsi_writeflush(dsi);
                    return AFPERR_PARAM;
                }
            }
        }
        break;
    }

    close( si.sdt_fd );
    si.sdt_fd = -1;
    return( AFP_OK );
}

u_char	utag[] = { 0, 0, 0, 0 };
u_char	ucreator[] = { 'U', 'N', 'I', 'X' };
u_char	utype[] = { 'T', 'E', 'X', 'T' };
short	usize = 256;
u_char	uicon[] = {
    0x1F, 0xFF, 0xFC, 0x00, 0x10, 0x00, 0x06, 0x00,
    0x10, 0x00, 0x05, 0x00, 0x10, 0x00, 0x04, 0x80,
    0x10, 0x00, 0x04, 0x40, 0x10, 0x00, 0x04, 0x20,
    0x10, 0x00, 0x07, 0xF0, 0x10, 0x00, 0x00, 0x10,
    0x10, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x10,
    0x10, 0x00, 0x00, 0x10, 0x10, 0x80, 0x02, 0x10,
    0x11, 0x80, 0x03, 0x10, 0x12, 0x80, 0x02, 0x90,
    0x12, 0x80, 0x02, 0x90, 0x14, 0x80, 0x02, 0x50,
    0x14, 0x87, 0xC2, 0x50, 0x14, 0x58, 0x34, 0x50,
    0x14, 0x20, 0x08, 0x50, 0x12, 0x16, 0xD0, 0x90,
    0x11, 0x01, 0x01, 0x10, 0x12, 0x80, 0x02, 0x90,
    0x12, 0x9C, 0x72, 0x90, 0x14, 0x22, 0x88, 0x50,
    0x14, 0x41, 0x04, 0x50, 0x14, 0x49, 0x24, 0x50,
    0x14, 0x55, 0x54, 0x50, 0x14, 0x5D, 0x74, 0x50,
    0x14, 0x5D, 0x74, 0x50, 0x12, 0x49, 0x24, 0x90,
    0x12, 0x22, 0x88, 0x90, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFC, 0x00, 0x1F, 0xFF, 0xFE, 0x00,
    0x1F, 0xFF, 0xFF, 0x00, 0x1F, 0xFF, 0xFF, 0x80,
    0x1F, 0xFF, 0xFF, 0xC0, 0x1F, 0xFF, 0xFF, 0xE0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF0,
};

int afp_geticoninfo(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    u_char	fcreator[ 4 ], ih[ 12 ];
    u_int16_t	vid, iindex, bsize;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( fcreator, ibuf, sizeof( fcreator ));
    ibuf += sizeof( fcreator );
    memcpy( &iindex, ibuf, sizeof( iindex ));
    iindex = ntohs( iindex );

    if ( memcmp( fcreator, ucreator, sizeof( ucreator )) == 0 ) {
        if ( iindex > 1 ) {
            return( AFPERR_NOITEM );
        }
        memcpy( ih, utag, sizeof( utag ));
        memcpy( ih + sizeof( utag ), utype, sizeof( utype ));
        *( ih + sizeof( utag ) + sizeof( utype )) = 1;
        *( ih + sizeof( utag ) + sizeof( utype ) + 1 ) = 0;
        memcpy( ih + sizeof( utag ) + sizeof( utype ) + 2, &usize,
                sizeof( usize ));
        memcpy( rbuf, ih, sizeof( ih ));
        *rbuflen = sizeof( ih );
        return( AFP_OK );
    }

    if ( iconopen( vol, fcreator, O_RDONLY, 0 ) < 0) {
        return( AFPERR_NOITEM );
    }

    if ( iindex < si.sdt_index ) {
        if ( lseek( si.sdt_fd, (off_t) 0L, SEEK_SET ) < 0 ) {
            return( AFPERR_PARAM );
        }
        si.sdt_index = 1;
    }

    /*
     * Position to the correct spot.
     */
    for (;;) {
        if ( read( si.sdt_fd, ih, sizeof( ih )) != sizeof( ih )) {
            close( si.sdt_fd );
            si.sdt_fd = -1;
            return( AFPERR_NOITEM );
        }
        memcpy( &bsize, ih + 10, sizeof( bsize ));
        bsize = ntohs(bsize);
        if ( lseek( si.sdt_fd, (off_t) bsize, SEEK_CUR ) < 0 ) {
            LOG(log_error, logtype_afpd, "afp_iconinfo: lseek: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }
        if ( si.sdt_index == iindex ) {
            memcpy( rbuf, ih, sizeof( ih ));
            *rbuflen = sizeof( ih );
            return( AFP_OK );
        }
        si.sdt_index++;
    }
}


int afp_geticon(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    off_t       offset;
    int		rc, buflen;
    u_char	fcreator[ 4 ], ftype[ 4 ], itype, ih[ 12 ];
    u_int16_t	vid, bsize, rsize;

    buflen = *rbuflen;
    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( fcreator, ibuf, sizeof( fcreator ));
    ibuf += sizeof( fcreator );
    memcpy( ftype, ibuf, sizeof( ftype ));
    ibuf += sizeof( ftype );
    itype = (unsigned char) *ibuf++;
    ibuf++;
    memcpy( &bsize, ibuf, sizeof( bsize ));
    bsize = ntohs( bsize );

    if ( memcmp( fcreator, ucreator, sizeof( ucreator )) == 0 &&
            memcmp( ftype, utype, sizeof( utype )) == 0 &&
            itype == 1 &&
            bsize <= usize ) {
        memcpy( rbuf, uicon, bsize);
        *rbuflen = bsize;
        return( AFP_OK );
    }

    if ( iconopen( vol, fcreator, O_RDONLY, 0 ) < 0) {
        return( AFPERR_NOITEM );
    }

    if ( lseek( si.sdt_fd, (off_t) 0L, SEEK_SET ) < 0 ) {
        close(si.sdt_fd);
        si.sdt_fd = -1;
        LOG(log_error, logtype_afpd, "afp_geticon: lseek: %s", strerror(errno));
        return( AFPERR_PARAM );
    }

    si.sdt_index = 1;
    offset = 0;
    while (( rc = read( si.sdt_fd, ih, sizeof( ih ))) > 0 ) {
        si.sdt_index++;
        offset += sizeof(ih);
        if ( memcmp( ih + sizeof( int ), ftype, sizeof( ftype )) == 0 &&
                *(ih + sizeof( int ) + sizeof( ftype )) == itype ) {
            break;
        }
        memcpy( &rsize, ih + 10, sizeof( rsize ));
        rsize = ntohs( rsize );
        if ( lseek( si.sdt_fd, (off_t) rsize, SEEK_CUR ) < 0 ) {
            LOG(log_error, logtype_afpd, "afp_geticon: lseek: %s", strerror(errno) );
            return( AFPERR_PARAM );
        }
        offset += rsize;
    }

    if ( rc < 0 ) {
        LOG(log_error, logtype_afpd, "afp_geticon: read: %s", strerror(errno));
        return( AFPERR_PARAM );
    }

    if ( rc == 0 ) {
        return( AFPERR_NOITEM );
    }

    memcpy( &rsize, ih + 10, sizeof( rsize ));
    rsize = ntohs( rsize );
#define min(a,b)	((a)<(b)?(a):(b))
    rc = min( bsize, rsize );

    if ((obj->proto == AFPPROTO_DSI) && (buflen < rc)) {
        DSI *dsi = obj->handle;
        struct stat st;
        off_t size;

        size = (fstat(si.sdt_fd, &st) < 0) ? 0 : st.st_size;
        if (size < rc + offset) {
            return AFPERR_PARAM;
        }

        if ((*rbuflen = dsi_readinit(dsi, rbuf, buflen, rc, AFP_OK)) < 0)
            goto geticon_exit;

        /* do to the streaming nature, we have to exit if we encounter
         * a problem. much confusion results otherwise. */
        while (*rbuflen > 0) {
#if defined(SENDFILE_FLAVOR_LINUX) || defined(SENDFILE_FLAVOR_BSD)
            if (!obj->options.flags & OPTION_DEBUG) {
#ifdef SENDFILE_FLAVOR_LINUX
                if (sendfile(dsi->socket, si.sdt_fd, &offset, dsi->datasize) < 0)
                    goto geticon_exit;
#endif /* SENDFILE_FLAVOR_LINUX */

#ifdef SENDFILE_FLAVOR_BSD
                if (sendfile(si.sdt_fd, dsi->socket, offset, rc, NULL, NULL, 0) < 0)
                    goto geticon_exit;
#endif /* SENDFILE_FLAVOR_BSD */

                goto geticon_done;
            }
#endif /* SENDFILE_FLAVOR_LINUX || SENDFILE_FLAVOR_BSD */

            buflen = read(si.sdt_fd, rbuf, *rbuflen);
            if (buflen < 0)
                goto geticon_exit;

            if (obj->options.flags & OPTION_DEBUG) {
                printf( "(read) reply: %d, %d\n", buflen, dsi->clientID);
                bprint(rbuf, buflen);
            }

            /* dsi_read() also returns buffer size of next allocation */
            buflen = dsi_read(dsi, rbuf, buflen); /* send it off */
            if (buflen < 0)
                goto geticon_exit;

            *rbuflen = buflen;
        }

geticon_done:
        dsi_readdone(dsi);
        return AFP_OK;

geticon_exit:
        LOG(log_info, logtype_afpd, "afp_geticon: %s", strerror(errno));
        dsi_readdone(dsi);
        obj->exit(1);
        return AFP_OK;

    } else {
        if ( read( si.sdt_fd, rbuf, rc ) < rc ) {
            return( AFPERR_PARAM );
        }
        *rbuflen = rc;
        return AFP_OK;
    }
}


static char		hexdig[] = "0123456789abcdef";
char *dtfile(const struct vol *vol, u_char creator[], char *ext )
{
    static char	path[ MAXPATHLEN + 1];
    char	*p;
    unsigned int i;

    strcpy( path, vol->v_path );
    strcat( path, "/.AppleDesktop/" );
    for ( p = path; *p != '\0'; p++ )
        ;

    if ( !isprint( creator[ 0 ] ) || creator[ 0 ] == '/' ) {
        *p++ = hexdig[ ( creator[ 0 ] & 0xf0 ) >> 4 ];
        *p++ = hexdig[ creator[ 0 ] & 0x0f ];
    } else {
        *p++ = creator[ 0 ];
    }

    *p++ = '/';

    for ( i = 0; i < sizeof( CreatorType ); i++ ) {
        if ( !isprint( creator[ i ] ) || creator[ i ] == '/' ) {
            *p++ = hexdig[ ( creator[ i ] & 0xf0 ) >> 4 ];
            *p++ = hexdig[ creator[ i ] & 0x0f ];
        } else {
            *p++ = creator[ i ];
        }
    }
    *p = '\0';
    strcat( path, ext );

    return( path );
}

/* ---------------------------
 * mpath is only a filename 
*/
static char  upath[ MAXPATHLEN + 1];
static char  mpath[ MAXPATHLEN + 1];
static char  ucs2[ MAXPATHLEN + 1];

static char *old_mtoupath(const struct vol *vol, char *mpath)
{
    char	*m, *u;
    int		 i = 0;
    int          changed = 0;
        
    m = mpath;
    u = upath;
    while ( *m != '\0' ) {
        /* handle case conversion first */
        if (vol->v_casefold & AFPVOL_MTOUUPPER)
            *m = diatoupper( *m );
        else if (vol->v_casefold & AFPVOL_MTOULOWER)
            *m = diatolower( *m );

        /* we have a code page. we only use the ascii range
         * if we have map ascii specified. */
        if (vol->v_mtoupage && ((*m & 0x80) ||
                                vol->v_flags & AFPVOL_MAPASCII)) {
            *u = vol->v_mtoupage->map[(unsigned char) *m].value;
            changed = 1;
            if (!*u && *m) {
                /* if conversion failed, encode in hex
                 * to prevent silly truncation
                 * H.P. Jansen <hpj@urpla.net> */
#ifdef DEBUG
                LOG(log_debug, logtype_afpd, "mtoupath: hex encode: 0x%x", (unsigned char) *m);
#endif
                *u++ = ':';
                *u++ = hexdig[ ( *m & 0xf0 ) >> 4 ];
                *u = hexdig[ *m & 0x0f ];
            }
        } else {
#if AD_VERSION == AD_VERSION1
            if ((((vol->v_flags & AFPVOL_NOHEX) == 0) &&
                    (!isascii(*m) || *m == '/')) ||
                    (((vol->v_flags & AFPVOL_USEDOTS) == 0) &&
                     ( i == 0 && (*m == '.' )))) {
#else 
            if ((((vol->v_flags & AFPVOL_NOHEX) == 0) &&
                    (!isprint(*m) || *m == '/')) ||
                    (((vol->v_flags & AFPVOL_USEDOTS) == 0) &&
                     ( i == 0 && (*m == '.' )))) {
#endif
                /* do hex conversion. */
                *u++ = ':';
                *u++ = hexdig[ ( *m & 0xf0 ) >> 4 ];
                *u = hexdig[ *m & 0x0f ];
                changed = 1;
            } else
                *u = *m;
        }
        u++;
        i++;
        m++;
    }
    *u = '\0';

#ifdef DEBUG
    LOG(log_debug, logtype_afpd, "mtoupath: '%s':'%s'", mpath, upath);
#endif /* DEBUG */

    return( (changed)?upath:mpath );
}

/* ---------------------------- */
#define hextoint( c )	( isdigit( c ) ? c - '0' : c + 10 - 'a' )
#define islxdigit(x)	(!isupper(x)&&isxdigit(x))

static char *old_utompath(const struct vol *vol, char *upath)
{
    char        *m, *u;
    int          h;
    int          changed = 0;

    /* do the hex conversion */
    u = upath;
    m = mpath;
    while ( *u != '\0' ) {
        /* we have a code page */
        if (vol->v_utompage && ((*u & 0x80) ||
                                (vol->v_flags & AFPVOL_MAPASCII))) {
            *m = vol->v_utompage->map[(unsigned char) *u].value;
            changed = 1;
        } else
            if ( *u == ':' && *(u+1) != '\0' && islxdigit( *(u+1)) &&
                    *(u+2) != '\0' && islxdigit( *(u+2))) {
                ++u;
                h = hextoint( *u ) << 4;
                ++u;
                h |= hextoint( *u );
                *m = h;
                changed = 1;
            } else
                *m = *u;

        /* handle case conversion */
        if (vol->v_casefold & AFPVOL_UTOMLOWER)
            *m = diatolower( *m );
        else if (vol->v_casefold & AFPVOL_UTOMUPPER)
            *m = diatoupper( *m );

        u++;
        m++;
    }
    *m = '\0';
    m = mpath;

#ifdef FILE_MANGLING
    m = mangle(vol, mpath, upath, 0);
    if (m != mpath) {
        changed = 1;
    }
#endif /* FILE_MANGLING */

#ifdef DEBUG
    LOG(log_debug, logtype_afpd, "utompath: '%s':'%s'", upath, mpath);
#endif /* DEBUG */

    return((changed)? m:upath );
}

/* --------------- */
extern unsigned int do_precomposition(unsigned int base, unsigned int comb);

static char comp[MAXPATHLEN +1];

static char *precompose(u_int16_t  *name, size_t inplen, size_t *outlen)
{
size_t i;
u_int16_t base, comb;
u_int16_t *in, *out;
u_int16_t result;

    if (!inplen || (inplen & 1) || inplen > sizeof(comp)/sizeof(u_int16_t))
        return NULL;
    i = 0;
    in  = name;
    out = (u_int16_t *)comp;
    *outlen = 0;
    
    base = *in;
    while (1) {
        i += 2;
        in++;
        if (i == inplen) {
           *out = base;
           *outlen += 2;
           return comp;
        }
        comb = *in;
        if (comb >= 0x300 && (result = do_precomposition(base, comb))) {
           *out = result;
           out++;
           *outlen += 2;
           i += 2;
           in++;
           if (i == inplen) 
              return comp;
           base = *in;
        }
        else {
           *out = base;
           out++;
           *outlen += 2;
           base = comb;
        }
    }
}

/* --------------- */
extern unsigned int do_decomposition(unsigned int base);

static char *decompose(u_int16_t  *name, size_t inplen, size_t *outlen)
{
size_t i;
u_int16_t base;
u_int16_t *in, *out;
unsigned int result;

    if (!inplen || (inplen & 1))
        return NULL;
    i = 0;
    in  = name;
    out = (u_int16_t *)comp;
    *outlen = 0;
    
    while (i < inplen) {
        if (*outlen >= sizeof(comp)/sizeof(u_int16_t) +2) {
            return NULL;
        }
        base = *in;
        if ((result = do_decomposition(base))) {
           *out = result  >> 16;
           out++;
           *outlen += 2;
           *out = result & 0xffff;
           out++;
           *outlen += 2;
        }
        else {
           *out = base;
           out++;
           *outlen += 2;
        }
        i += 2;
        in++;
     }
     return comp;
}

/* --------------------------- */
char *mtoupath(const struct vol *vol, char *mpath, int utf8)
{
    char	*m, *u, *r;
    int		 i = 0;
    size_t       inplen;
    size_t       outlen;
        
    if ( *mpath == '\0' ) {
        return( "." );
    }

#ifdef FILE_MANGLING
    m = demangle(vol, mpath);
    if (m != mpath) {
        return m;
    }
#endif /* FILE_MANGLING */

    if (!vol_utf8(vol))
    	return old_mtoupath(vol, mpath);

    m = mpath;
    u = upath;
    while ( *m != '\0' ) {
        if ( (!(vol->v_flags & AFPVOL_NOHEX) && *m == '/') ||
             (!(vol->v_flags & AFPVOL_USEDOTS) && i == 0 && *m == '.') ||
             (!utf8 && (unsigned char)*m == 0xf0) /* Apple logo */
        ) {
          /* do hex conversion. */
          *u++ = ':';
          *u++ = hexdig[ ( *m & 0xf0 ) >> 4 ];
          *u = hexdig[ *m & 0x0f ];
        } else
           *u = *m;
        u++;
        i++;
        m++;
    }
    *u = '\0';
    u = upath;
#ifdef AFP3x
    inplen = strlen(u);
    outlen = MAXPATHLEN;
    r = ucs2;
    if (!utf8) {
        if ((size_t)(-1) == iconv(vol->v_mactoutf8, 0,0,0,0) )
            return NULL;
        /* assume precompose */
        if ((size_t)(-1) == iconv(vol->v_mactoutf8, &u, &inplen, &r, &outlen))
            return NULL;
        u = ucs2;
    }
    else { 
        if ((size_t)(-1) == iconv(vol->v_utf8toucs2, 0,0,0,0) )
            return NULL;

        if ((size_t)(-1) == iconv(vol->v_utf8toucs2, &u, &inplen, &r, &outlen))
            return NULL;

        u = precompose((u_int16_t *)ucs2, MAXPATHLEN -outlen, &inplen);

        if ((size_t)(-1) == iconv(vol->v_ucs2toutf8, 0,0,0,0))
            return NULL;
            
        outlen = MAXPATHLEN;
        r = upath;
        if ((size_t)(-1) == iconv(vol->v_ucs2toutf8, &u, &inplen, &r, &outlen))
            return NULL;
        u = upath;
    }
    u[MAXPATHLEN -outlen] = 0;
#endif
#ifdef DEBUG
    LOG(log_debug, logtype_afpd, "mtoupath: '%s':'%s'", mpath, upath);
#endif /* DEBUG */
    return( u );
}

/* --------------- */
char *utompath(const struct vol *vol, char *upath, int utf8)
{
    char        *m, *u, *r;
    int          h;
    int          mangleflag = 0;
    size_t       inplen;
    size_t       outlen;

    if (!vol_utf8(vol))
    	return old_utompath(vol, upath);
    /* do the hex conversion */
    u = upath;
    m = mpath;
    while ( *u != '\0' ) {
        if ( *u == ':' && islxdigit( *(u+1)) && islxdigit( *(u+2))) {
            ++u;
            h = hextoint( *u ) << 4;
            ++u;
            h |= hextoint( *u );
            *m = h;
        } else
            *m = *u;
        u++;
        m++;
    }
    *m = '\0';
    m = mpath;
#ifdef AFP3x    
    if ((size_t)(-1) == iconv(vol->v_utf8toucs2, 0,0,0,0) )
        return NULL;
    inplen = strlen(mpath);
    outlen = MAXPATHLEN;
    r = ucs2;
    if ((size_t)(-1) == iconv(vol->v_utf8toucs2, &m, &inplen, &r, &outlen))
        return NULL;

    if (utf8) {
        if ( NULL == (m = decompose((u_int16_t *)ucs2, MAXPATHLEN -outlen, &inplen)))
            return NULL;

        if ((size_t)(-1) == iconv(vol->v_ucs2toutf8, 0,0,0,0))
            return NULL;
            
        outlen = MAXPATHLEN;
        r = mpath;
        if ((size_t)(-1) == iconv(vol->v_ucs2toutf8, &m, &inplen, &r, &outlen))
            return NULL;
    }
    else {
        m = precompose((u_int16_t *)ucs2, MAXPATHLEN -outlen, &inplen);

        if ((size_t)(-1) == iconv(vol->v_ucs2tomac, 0,0,0,0))
            return NULL;
            
        outlen = MAXPATHLEN;
        r = mpath;
        if ((size_t)(-1) == iconv(vol->v_ucs2tomac, &m, &inplen, &r, &outlen)) {
            switch (errno) {
            case EILSEQ:
                if (outlen != MAXPATHLEN) {
                    mangleflag = 1;
                }
            default:
                return NULL;
            }
        }
    }
    mpath[MAXPATHLEN -outlen] = 0;
#endif
#ifdef FILE_MANGLING
    m = mangle(vol, mpath, upath, mangleflag);
#else
    if (mangleflag)
        return NULL;
    m = mpath;
#endif /* FILE_MANGLING */

    return(m);
}

/* ----------------------------- */
int afp_addcomment(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad, *adp;
    struct vol		*vol;
    struct dir		*dir;
    struct ofork        *of;
    struct path         *path;
    char                *name, *upath;
    int			clen;
    u_int32_t           did;
    u_int16_t		vid;
    int                 isadir;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
	return afp_errno;
    }

    if (NULL == ( path = cname( vol, dir, &ibuf )) ) {
	return get_afp_errno(AFPERR_NOOBJ);
    }

    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    clen = (u_char)*ibuf++;
    clen = min( clen, 199 );

    upath = path->u_name;
    if (check_access(upath, OPENACC_WR ) < 0) {
        return AFPERR_ACCESS;
    }

    isadir = path_isadir(path);
    if (isadir || !(of = of_findname(path))) {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    } else
        adp = of->of_ad;

    if (ad_open( upath , vol_noadouble(vol) |
                 (( isadir) ? ADFLAGS_HF|ADFLAGS_DIR : ADFLAGS_HF),
                 O_RDWR|O_CREAT, 0666, adp) < 0 ) {
        return( AFPERR_ACCESS );
    }

    if ( (ad_getoflags( adp, ADFLAGS_HF ) & O_CREAT) ) {
        if ( *path->m_name == '\0' ) {
            name = curdir->d_m_name;
        } else {
            name = path->m_name;
        }
        ad_setentrylen( adp, ADEID_NAME, strlen( name ));
        memcpy( ad_entry( adp, ADEID_NAME ), name,
                ad_getentrylen( adp, ADEID_NAME ));
    }

    ad_setentrylen( adp, ADEID_COMMENT, clen );
    memcpy( ad_entry( adp, ADEID_COMMENT ), ibuf, clen );
    ad_flush( adp, ADFLAGS_HF );
    ad_close( adp, ADFLAGS_HF );
    return( AFP_OK );
}

int afp_getcomment(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad, *adp;
    struct vol		*vol;
    struct dir		*dir;
    struct ofork        *of;
    struct path         *s_path;
    char		*upath;
    u_int32_t		did;
    u_int16_t		vid;
    int                 isadir;
    
    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
	return afp_errno;
    }

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
	return get_afp_errno(AFPERR_NOOBJ);
    }

    upath = s_path->u_name;
    isadir = path_isadir(s_path);
    if (isadir || !(of = of_findname(s_path))) {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    } else
        adp = of->of_ad;
    if ( ad_open( upath,
                  ( isadir) ? ADFLAGS_HF|ADFLAGS_DIR : ADFLAGS_HF,
                  O_RDONLY, 0666, adp) < 0 ) {
        return( AFPERR_NOITEM );
    }

    /*
     * Make sure the AD file is not bogus.
     */
    if ( ad_getentrylen( adp, ADEID_COMMENT ) < 0 ||
            ad_getentrylen( adp, ADEID_COMMENT ) > 199 ) {
        ad_close( adp, ADFLAGS_HF );
        return( AFPERR_NOITEM );
    }

    *rbuf++ = ad_getentrylen( adp, ADEID_COMMENT );
    memcpy( rbuf, ad_entry( adp, ADEID_COMMENT ),
            ad_getentrylen( adp, ADEID_COMMENT ));
    *rbuflen = ad_getentrylen( adp, ADEID_COMMENT ) + 1;
    ad_close( adp, ADFLAGS_HF );

    /* return AFPERR_NOITEM if len == 0 ? */
    return( AFP_OK );
}

int afp_rmvcomment(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct adouble	ad, *adp;
    struct vol		*vol;
    struct dir		*dir;
    struct ofork        *of;
    struct path         *s_path;
    char		*upath;
    u_int32_t		did;
    u_int16_t		vid;
    int                 isadir;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
	return afp_errno;
    }

    if (NULL == ( s_path = cname( vol, dir, &ibuf ))) {
	return get_afp_errno(AFPERR_NOOBJ);
    }

    upath = s_path->u_name;
    if (check_access(upath, OPENACC_WR ) < 0) {
        return AFPERR_ACCESS;
    }

    isadir = path_isadir(s_path);
    if (isadir || !(of = of_findname(s_path))) {
        memset(&ad, 0, sizeof(ad));
        adp = &ad;
    } else
        adp = of->of_ad;

    if ( ad_open( upath,
                   (isadir) ? ADFLAGS_HF|ADFLAGS_DIR : ADFLAGS_HF,
                  O_RDWR, 0, adp) < 0 ) {
        switch ( errno ) {
        case ENOENT :
            return( AFPERR_NOITEM );
        case EACCES :
            return( AFPERR_ACCESS );
        default :
            return( AFPERR_PARAM );
        }
    }

    ad_setentrylen( adp, ADEID_COMMENT, 0 );
    ad_flush( adp, ADFLAGS_HF );
    ad_close( adp, ADFLAGS_HF );
    return( AFP_OK );
}
