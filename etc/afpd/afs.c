/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef AFS

#include <string.h>
#include <sys/types.h>
#include <sys/syslog.h>
#include <netatalk/endian.h>
#include <netinet/in.h>
#include <afs/venus.h>
#include <afs/afsint.h>
#include <atalk/afp.h>
#include <unistd.h>

#include "globals.h"
#include "directory.h"
#include "volume.h"
#include "misc.h"

afs_getvolspace( vol, bfree, btotal, bsize )
    struct vol	*vol;
    VolSpace	*bfree, *btotal;
    u_int32_t   *bsize;
{
    struct ViceIoctl	vi;
    struct VolumeStatus	*vs;
    char		venuspace[ sizeof( struct VolumeStatus ) + 3 ];
    int			total, free;

    vi.in_size = 0;
    vi.out_size = sizeof( venuspace );
    vi.out = venuspace;
    if ( pioctl( vol->v_path, VIOCGETVOLSTAT, &vi, 1 ) < 0 ) {
	return( AFPERR_PARAM );
    }

    vs = (struct VolumeStatus *)venuspace;

    if ( vs->PartBlocksAvail > 0 ) {
	if ( vs->MaxQuota != 0 ) {
#define min(x,y)	(((x)<(y))?(x):(y))
	    free = min( vs->MaxQuota - vs->BlocksInUse, vs->PartBlocksAvail );
	} else {
	    free = vs->PartBlocksAvail;
	}
    } else {
	free = 0;
    }

    if ( vs->MaxQuota != 0 ) {
	total = free + vs->BlocksInUse;
    } else {
	total = vs->PartMaxBlocks;
    }

    *bsize = 1024;
    *bfree = (VolSpace) free * 1024;
    *btotal = (VolSpace) total * 1024;

    return( AFP_OK );
}

afp_getdiracl(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct ViceIoctl	vi;
    struct vol		*vol;
    struct dir		*dir;
    char		*path;
    u_int32_t		did;
    u_int16_t		vid;

    ibuf += 2;
    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( short );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (( dir = dirsearch( vol, did )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_NOOBJ );
    }
    if ( *path != '\0' ) {
	*rbuflen = 0;
	return( AFPERR_BITMAP );
    }

    vi.in_size = 0;
    vi.out_size = *rbuflen;
    vi.out = rbuf;
    if ( pioctl( ".", VIOCGETAL, &vi, 1 ) < 0 ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }
    *rbuflen = strlen( vi.out ) + 1;
    return( AFP_OK );
}

/*
 * Calculate the mode for a directory in AFS.  First, make sure the
 * directory is in AFS.  Could probably use something less heavy than
 * VIOCGETAL.  If the directory is on AFS, use access() calls to
 * estimate permission, a la mdw.
 */
afsmode( path, ma, dir )
    char		*path;
    struct maccess	*ma;
    struct dir		*dir;
{
    struct ViceIoctl	vi;
    char		buf[ 1024 ];

    if (( dir->d_flags & DIRF_FSMASK ) == DIRF_NOFS ) {
	vi.in_size = 0;
	vi.out_size = sizeof( buf );
	vi.out = buf;
	if ( pioctl( path, VIOCGETAL, &vi, 1 ) < 0 ) {
	    dir->d_flags |= DIRF_UFS;
	} else {
	    dir->d_flags |= DIRF_AFS;
	}
    }

    if (( dir->d_flags & DIRF_FSMASK ) != DIRF_AFS ) {
	return;
    }

    accessmode( upath, &ma, dir );

    return;
}

extern struct dir	*curdir;
/*
 * cmd | 0 | vid | did | pathtype | pathname | 0 | acl
 */
afp_setdiracl(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct ViceIoctl	vi;
    struct vol		*vol;
    struct dir		*dir;
    char		*path, *iend;
    u_int32_t		did;
    u_int16_t		vid;

    *rbuflen = 0;
    iend = ibuf + ibuflen;
    ibuf += 2;
    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( short );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (( dir = dirsearch( vol, did )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
	*rbuflen = 0;
	return( AFPERR_NOOBJ );
    }
    if ( *path != '\0' ) {
	*rbuflen = 0;
	return( AFPERR_BITMAP );
    }

    if ((int)ibuf & 1 ) {
	ibuf++;
    }

    vi.in_size = iend - ibuf;
    vi.in = ibuf;
    vi.out_size = 0;

    if ( pioctl( ".", VIOCSETAL, &vi, 1 ) < 0 ) {
	*rbuflen = 0;
	return( AFPERR_PARAM );
    }
    pioctl( ".AppleDouble", VIOCSETAL, &vi, 1 );
    if ( curdir->d_did  == DIRDID_ROOT ) {
	pioctl( ".AppleDesktop", VIOCSETAL, &vi, 1 );
    }

    return( AFP_OK );
}


#ifdef UAM_AFSKRB

#include <krb.h>
#include <des.h>
#include <afs/kauth.h>
#include <afs/kautils.h>

extern C_Block		seskey;
extern Key_schedule	seskeysched;

afp_afschangepw(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    char	name[ MAXKTCNAMELEN ], instance[ MAXKTCNAMELEN ];
    char	realm[ MAXKTCREALMLEN ];
    char	oldpw[ 9 ], newpw[ 9 ];
    int		len, rc;
    u_int16_t	clen;
    struct ktc_encryptionKey	oldkey, newkey;
    struct ktc_token		adtok;
    struct ubik_client		*conn;

    *rbuflen = 0;
    ++ibuf;
    len = (unsigned char) *ibuf++;
    ibuf[ len ] = '\0';
    *name = *instance = *realm = '\0';
    ka_ParseLoginName( ibuf, name, instance, realm );
    ucase( realm );
    if ( *realm == '\0' ) {
	if ( krb_get_lrealm( realm, 1 ) != KSUCCESS ) {
	    syslog( LOG_ERR, "krb_get_lrealm failed" );
	    return( AFPERR_BADUAM );
	}
    }

    if ( strlen( name ) < 2 || strlen( name ) > 18 ) {
	return( AFPERR_PARAM );
    }
    ibuf += len;

    memcpy( &clen, ibuf, sizeof( clen ));
    clen = ntohs( clen );
    if ( clen % 8 != 0 ) {
	return( AFPERR_PARAM );
    }

    ibuf += sizeof( short );
    pcbc_encrypt((C_Block *)ibuf, (C_Block *)ibuf,
	    clen, seskeysched, seskey, DES_DECRYPT );

    len = (unsigned char) *ibuf++;
    if ( len > 8 ) {
	return( AFPERR_PARAM );
    }
    memset( oldpw, 0, sizeof( oldpw ));
    memcpy( oldpw, ibuf, len );
    ibuf += len;
    oldpw[ len ] = '\0';

    len = (unsigned char) *ibuf++;
    if ( len > 8 ) {
	return( AFPERR_PARAM );
    }
    memset( newpw, 0, sizeof( newpw ));
    memcpy( newpw, ibuf, len );
    ibuf += len;
    newpw[ len ] = '\0';

    syslog( LOG_INFO,
	"changing password for <%s>.<%s>@<%s>", name, instance, realm );

    ka_StringToKey( oldpw, realm, &oldkey );
    memset( oldpw, 0, sizeof( oldpw ));
    ka_StringToKey( newpw, realm, &newkey );
    memset( newpw, 0, sizeof( newpw ));

    rc = ka_GetAdminToken( name, instance, realm, &oldkey, 60, &adtok, 0 );
    memset( &oldkey, 0, sizeof( oldkey ));
    switch ( rc ) {
	case 0:
	    break;
	case KABADREQUEST:
	    memset( &newkey, 0, sizeof( newkey ));
	    return( AFPERR_NOTAUTH );
	default:
	    memset( &newkey, 0, sizeof( newkey ));
	    return( AFPERR_BADUAM );
    }
    if ( ka_AuthServerConn( realm, KA_MAINTENANCE_SERVICE, &adtok, &conn )
		!= 0 ) {
	memset( &newkey, 0, sizeof( newkey ));
	return( AFPERR_BADUAM );
    }

    rc = ka_ChangePassword( name, instance, conn, 0, &newkey );
    memset( &newkey, 0, sizeof( newkey ));
    if ( rc != 0 ) {
	return( AFPERR_BADUAM );
    }

    syslog( LOG_DEBUG, "password changed succeeded" );
    return( AFP_OK );
}

#endif UAM_AFSKRB
#endif AFS
