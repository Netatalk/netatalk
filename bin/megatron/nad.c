/*
 * $Id: nad.c,v 1.9 2002-01-04 04:45:47 sibaz Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <atalk/logger.h>
#include <dirent.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <atalk/adouble.h>
#include <netatalk/endian.h>
#include "megatron.h"
#include "nad.h"

static char		hexdig[] = "0123456789abcdef";

char *mtoupath( mpath )
    char	*mpath;
{
    static char	upath[ MAXNAMLEN  + 1];
    char	*m, *u;
    int		i = 0;

    m = mpath;
    u = upath;
    while ( *m != '\0' ) {
#if AD_VERSION == AD_VERSION1
	if ( !isascii( *m ) || *m == '/' || ( i == 0 && *m == '.' )) {
#else /* AD_VERSION == AD_VERSION1 */
	if (!isprint(*m) || *m == '/' || ( i == 0 && (*m == '.' ))) {
#endif /* AD_VERSION == AD_VERSION1 */
	    *u++ = ':';
	    *u++ = hexdig[ ( *m & 0xf0 ) >> 4 ];
	    *u++ = hexdig[ *m & 0x0f ];
	} else {
#ifdef DOWNCASE
	    *u++ = ( isupper( *m )) ? tolower( *m ) : *m;
#else /* DOWNCASE */
	    *u++ = *m;
#endif /* DOWNCASE */
	}
	i++;
	m++;
    }
    *u = '\0';
    return( upath );
}


#define hextoint( c )	( isdigit( c ) ? c - '0' : c + 10 - 'a' )
#define islxdigit(x)	(!isupper(x)&&isxdigit(x))

char *utompath( upath )
    char	*upath;
{
    static char	mpath[ MAXNAMLEN  + 1];
    char	*m, *u;
    int h;

    m = mpath;
    u = upath;
    while ( *u != '\0' ) {
        if (*u == ':' && *(u + 1) != '\0' && islxdigit(*(u+1)) &&
	    *(u+2) != '\0' && islxdigit(*(u+2))) {
	  u++;
	  h = hextoint(*u) << 4;
	  u++;
	  h |= hextoint(*u);
	  *m = h;
	} else {
#ifdef DOWNCASE
	  *m = diatolower(*u);
#else /* DOWNCASE */
	  *m = *u;
#endif /* DOWNCASE */
	}
	u++;
	m++;
    }
    *m = '\0';
    return( mpath );
}

#if HEXOUTPUT
    int			hexfork[ NUMFORKS ];
#endif /* HEXOUTPUT */

struct nad_file_data {
    char		macname[ MAXPATHLEN + 1 ];
    char		adpath[ 2 ][ MAXPATHLEN + 1];
    int			offset[ NUMFORKS ];
    struct adouble	ad;
} nad;

int nad_open( path, openflags, fh, options )
    char		*path;
    int			openflags, options;
    struct FHeader	*fh;
{
    struct stat		st;
    int			fork;

/*
 * Depending upon openflags, set up nad.adpath for the open.  If it 
 * is for write, then stat the current directory to get its mode.
 * Open the file.  Either fill or grab the adouble information.
 */
    memset(&nad.ad, 0, sizeof(nad.ad));
    if ( openflags == O_RDONLY ) {
	strcpy( nad.adpath[0], path );
	strcpy( nad.adpath[1], 
		ad_path( nad.adpath[0], ADFLAGS_DF|ADFLAGS_HF ));
	for ( fork = 0 ; fork < NUMFORKS ; fork++ ) {
	    if ( stat( nad.adpath[ fork ], &st ) < 0 ) {
		if ( errno == ENOENT ) {
		    fprintf( stderr, "%s is not an adouble file.\n", path );
		} else {
		    perror( "stat of adouble file failed" );
		}
		return( -1 );
	    }
	}

#if DEBUG
    fprintf(stderr, "%s is adpath[0]\n", nad.adpath[0]);
    fprintf(stderr, "%s is adpath[1]\n", nad.adpath[1]);
#endif /* DEBUG */
	if ( ad_open( nad.adpath[ 0 ], ADFLAGS_DF|ADFLAGS_HF,
		openflags, (int)( st.st_mode & 0666 ), &nad.ad) < 0 ) {
	    perror( nad.adpath[ 0 ] );
	    return( -1 );
	}
	return( nad_header_read( fh ));

    } else {
	strcpy( nad.macname, fh->name );
	strcpy( nad.adpath[0], mtoupath( nad.macname ));
	strcpy( nad.adpath[1], 
		ad_path( nad.adpath[0], ADFLAGS_DF|ADFLAGS_HF ));
#if DEBUG
    fprintf(stderr, "%s\n", nad.macname);
    fprintf(stderr, "%s is adpath[0]\n", nad.adpath[0]);
    fprintf(stderr, "%s is adpath[1]\n", nad.adpath[1]);
#endif /* DEBUG */
	if ( stat( ".", &st ) < 0 ) {
	    perror( "stat of . failed" );
	    return( -1 );
	}
	(void)umask( 0 );
	if ( ad_open( nad.adpath[ 0 ], ADFLAGS_DF|ADFLAGS_HF,
		openflags, (int)( st.st_mode & 0666 ), &nad.ad) < 0 ) {
	    perror( nad.adpath[ 0 ] );
	    return( -1 );
	}
	return( nad_header_write( fh ));
    }
}

int nad_header_read( fh )
    struct FHeader	*fh;
{
    u_int32_t		temptime;
    struct stat		st;

    memcpy( nad.macname, ad_entry( &nad.ad, ADEID_NAME ), 
	    ad_getentrylen( &nad.ad, ADEID_NAME ));
    nad.macname[ ad_getentrylen( &nad.ad, ADEID_NAME ) ] = '\0';
    strcpy( fh->name, nad.macname );

    /* just in case there's nothing in macname */
    if (*fh->name == '\0')
      strcpy(fh->name, utompath(nad.adpath[DATA]));

    if ( stat( nad.adpath[ DATA ], &st ) < 0 ) {
	perror( "stat of datafork failed" );
	return( -1 );
    }
    fh->forklen[ DATA ] = htonl( st.st_size );
    fh->forklen[ RESOURCE ] = htonl( ad_getentrylen( &nad.ad, ADEID_RFORK ));
    fh->comment[0] = '\0';

#if DEBUG
    fprintf( stderr, "macname of file\t\t\t%.*s\n", strlen( fh->name ), 
	    fh->name );
    fprintf( stderr, "size of data fork\t\t%d\n", 
	    ntohl( fh->forklen[ DATA ] ));
    fprintf( stderr, "size of resource fork\t\t%d\n", 
	    ntohl( fh->forklen[ RESOURCE ] ));
    fprintf( stderr, "get info comment\t\t\"%s\"\n", fh->comment );
#endif /* DEBUG */

    ad_getdate(&nad.ad, AD_DATE_CREATE, &temptime);
    memcpy( &fh->create_date, &temptime, sizeof( temptime ));
    ad_getdate(&nad.ad, AD_DATE_MODIFY, &temptime);
    memcpy( &fh->mod_date, &temptime, sizeof( temptime ));
    ad_getdate(&nad.ad, AD_DATE_BACKUP, &temptime);
    memcpy( &fh->backup_date, &temptime, sizeof( temptime ));

#if DEBUG
    memcpy( &temptime, &fh->create_date, sizeof( temptime ));
    temptime = AD_DATE_TO_UNIX(temptime);
    fprintf( stderr, "create_date seconds\t\t%lu\n", temptime );
    memcpy( &temptime, &fh->mod_date, sizeof( temptime ));
    temptime = AD_DATE_TO_UNIX(temptime);
    fprintf( stderr, "mod_date seconds\t\t%lu\n", temptime );
    memcpy( &temptime, &fh->backup_date, sizeof( temptime ));
    temptime = AD_DATE_TO_UNIX(temptime);
    fprintf( stderr, "backup_date seconds\t\t%lu\n", temptime );
    fprintf( stderr, "size of finder_info\t\t%d\n", sizeof( fh->finder_info ));
#endif /* DEBUG */

    memcpy(&fh->finder_info.fdType,
	    ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_TYPE,
	   sizeof( fh->finder_info.fdType ));
    memcpy(&fh->finder_info.fdCreator,
	   ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_CREATOR,
	   sizeof( fh->finder_info.fdCreator ));
    memcpy(&fh->finder_info.fdFlags,
	   ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_FLAGS,
	   sizeof( fh->finder_info.fdFlags ));
    memcpy(&fh->finder_info.fdLocation,
	   ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_LOC,
	   sizeof( fh->finder_info.fdLocation ));
    memcpy(&fh->finder_info.fdFldr,
	  ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_FLDR,
	  sizeof( fh->finder_info.fdFldr ));
    memcpy(&fh->finder_xinfo.fdScript,
	   ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_SCRIPT,
	   sizeof(fh->finder_xinfo.fdScript));
    memcpy(&fh->finder_xinfo.fdXFlags,
	   ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_XFLAGS,
	   sizeof(fh->finder_xinfo.fdXFlags));

#if DEBUG
    {
	short		flags;
	fprintf( stderr, "finder_info.fdType\t\t%.*s\n", 
		sizeof( fh->finder_info.fdType ), &fh->finder_info.fdType );
	fprintf( stderr, "finder_info.fdCreator\t\t%.*s\n", 
		sizeof( fh->finder_info.fdCreator ),
		&fh->finder_info.fdCreator );
	fprintf( stderr, "nad type and creator\t\t%.*s\n\n", 
		sizeof( fh->finder_info.fdType ) + 
		sizeof( fh->finder_info.fdCreator ), 
		ad_entry( &nad.ad, ADEID_FINDERI ));
	memcpy(&flags, ad_entry( &nad.ad, ADEID_FINDERI ) + 
	       FINDERIOFF_FLAGS, sizeof( flags ));
	fprintf( stderr, "nad.ad flags\t\t\t%x\n", flags );
	fprintf( stderr, "fh flags\t\t\t%x\n", fh->finder_info.fdFlags );
	fprintf(stderr, "fh script\t\t\t%x\n", fh->finder_xinfo.fdScript);
	fprintf(stderr, "fh xflags\t\t\t%x\n", fh->finder_xinfo.fdXFlags);
    }
#endif /* DEBUG */

    nad.offset[ DATA ] = nad.offset[ RESOURCE ] = 0;

    return( 0 );

}

int nad_header_write( fh )
    struct FHeader	*fh;
{
    u_int32_t		temptime;

    ad_setentrylen( &nad.ad, ADEID_NAME, strlen( nad.macname ));
    memcpy( ad_entry( &nad.ad, ADEID_NAME ), nad.macname, 
	    ad_getentrylen( &nad.ad, ADEID_NAME ));
    ad_setentrylen( &nad.ad, ADEID_COMMENT, strlen( fh->comment ));
    memcpy( ad_entry( &nad.ad, ADEID_COMMENT ), fh->comment, 
	    ad_getentrylen( &nad.ad, ADEID_COMMENT ));
    ad_setentrylen( &nad.ad, ADEID_RFORK, ntohl( fh->forklen[ RESOURCE ] ));

#if DEBUG
    fprintf( stderr, "ad_getentrylen\n" );
    fprintf( stderr, "ADEID_FINDERI\t\t\t%d\n", 
	    ad_getentrylen( &nad.ad, ADEID_FINDERI ));
    fprintf( stderr, "ADEID_RFORK\t\t\t%d\n", 
	    ad_getentrylen( &nad.ad, ADEID_RFORK ));
    fprintf( stderr, "ADEID_NAME\t\t\t%d\n",
	    ad_getentrylen( &nad.ad, ADEID_NAME ));
    fprintf( stderr, "ad_entry of ADEID_NAME\t\t%.*s\n",
	    ad_getentrylen( &nad.ad, ADEID_NAME ), 
	    ad_entry( &nad.ad, ADEID_NAME ));
    fprintf( stderr, "ADEID_COMMENT\t\t\t%d\n",
	     ad_getentrylen( &nad.ad, ADEID_COMMENT ));
#endif /* DEBUG */

    memcpy( &temptime, &fh->create_date, sizeof( temptime ));
    ad_setdate(&nad.ad, AD_DATE_CREATE, temptime);
    memcpy( &temptime, &fh->mod_date, sizeof( temptime ));
    ad_setdate(&nad.ad, AD_DATE_MODIFY, temptime);

#if DEBUG
    ad_getdate(&nad.ad, AD_DATE_CREATE, &temptime);
    temptime = AD_DATE_TO_UNIX(temptime);
    fprintf(stderr, "FILEIOFF_CREATE seconds\t\t%ld\n", temptime );
    ad_getdate(&nad.ad, AD_DATE_MODIFY, &temptime);
    temptime = AD_DATE_TO_UNIX(temptime);
    fprintf(stderr, "FILEIOFF_MODIFY seconds\t\t%ld\n", temptime );
#endif /* DEBUG */

    memset( ad_entry( &nad.ad, ADEID_FINDERI ), 0, ADEDLEN_FINDERI );
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_TYPE, 
	    &fh->finder_info.fdType, sizeof( fh->finder_info.fdType ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_CREATOR,
	   &fh->finder_info.fdCreator, sizeof( fh->finder_info.fdCreator ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_FLAGS,
	    &fh->finder_info.fdFlags, sizeof( fh->finder_info.fdFlags ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_LOC,
	    &fh->finder_info.fdLocation,sizeof( fh->finder_info.fdLocation ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_FLDR,
	    &fh->finder_info.fdFldr, sizeof( fh->finder_info.fdFldr ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_SCRIPT,
	    &fh->finder_xinfo.fdScript, sizeof( fh->finder_xinfo.fdScript ));
    memcpy( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_XFLAGS,
	    &fh->finder_xinfo.fdXFlags, sizeof( fh->finder_xinfo.fdXFlags));


#if DEBUG
    {
	short		flags;
	memcpy(&flags, ( ad_entry( &nad.ad, ADEID_FINDERI ) + FINDERIOFF_FLAGS),
		sizeof( flags ));
	fprintf( stderr, "nad.ad flags\t\t\t%x\n", flags );
	fprintf( stderr, "fh flags\t\t\t%x\n", fh->finder_info.fdFlags );
	fprintf( stderr, "fh xflags\t\t\t%x\n", fh->finder_xinfo.fdXFlags );
	fprintf( stderr, "type and creator\t\t%.*s\n\n", 
		sizeof( fh->finder_info.fdType ) + 
		sizeof( fh->finder_info.fdCreator ),
		ad_entry( &nad.ad, ADEID_FINDERI ));
    }
#endif /* DEBUG */

#if HEXOUTPUT
    hexfork[ DATA ] = open( "datafork", O_WRONLY|O_CREAT, 0622 );
    hexfork[ RESOURCE ] = open( "resfork", O_WRONLY|O_CREAT, 0622 );
#endif /* HEXOUTPUT */

    nad.offset[ DATA ] = nad.offset[ RESOURCE ] = 0;
    ad_flush( &nad.ad, ADFLAGS_DF|ADFLAGS_HF );

    return( 0 );
}

int			forkeid[] = { ADEID_DFORK, ADEID_RFORK };

int nad_read( fork, forkbuf, bufc )
    int			fork;
    char		*forkbuf;
    int			bufc;
{
    int			cc = 0;

#if DEBUG
    fprintf( stderr, "Entering nad_read\n" );
#endif /* DEBUG */

    if (( cc = ad_read( &nad.ad, forkeid[ fork ], nad.offset[ fork ], 
	    forkbuf, bufc)) < 0 )  {
	perror( "Reading the appledouble file:" );
	return( cc );
    }
    nad.offset[ fork ] += cc;

#if DEBUG
    fprintf( stderr, "Exiting nad_read\n" );
#endif /* DEBUG */

    return( cc );
}

int nad_write( fork, forkbuf, bufc )
    int			fork;
    char		*forkbuf;
    int			bufc;
{
    char		*buf_ptr;
    int			writelen;
    int			cc = 0;

#if DEBUG
    fprintf( stderr, "Entering nad_write\n" );
#endif /* DEBUG */

#if HEXOUTPUT
    write( hexfork[ fork ], forkbuf, bufc );
#endif /* HEXOUTPUT */

    writelen = bufc;
    buf_ptr = forkbuf;

    while (( writelen > 0 ) && ( cc >= 0 )) {
	cc =  ad_write( &nad.ad, forkeid[ fork ], nad.offset[ fork ], 
		0, buf_ptr, writelen);
	nad.offset[ fork ] += cc;
	buf_ptr += cc;
	writelen -= cc;
    }
    if ( cc < 0 ) {
	perror( "Writing the appledouble file:" );
	return( cc );
    }

    return( bufc );
}

int nad_close( status )
int			status;
{
    int			rv;
    if ( status == KEEP ) {
	if (( rv = ad_flush( &nad.ad, ADFLAGS_DF|ADFLAGS_HF )) < 0 ) {
	    fprintf( stderr, "nad_close rv for flush %d\n", rv );
	    return( rv );
	}
	if (( rv = ad_close( &nad.ad, ADFLAGS_DF|ADFLAGS_HF )) < 0 ) {
	    fprintf( stderr, "nad_close rv for close %d\n", rv );
	    return( rv );
	}
    } else if ( status == TRASH ) {
	if ( unlink( nad.adpath[ 0 ] ) < 0 ) {
	    perror ( nad.adpath[ 0 ] );
	}
	if ( unlink( nad.adpath[ 1 ] ) < 0 ) {
	    perror ( nad.adpath[ 1 ] );
	}
	return( 0 );
    } else return( -1 );
    return( 0 );
}
