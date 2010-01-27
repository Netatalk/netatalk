/*
 * $Id: nad.c,v 1.18 2010-01-27 21:27:53 didg Exp $
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
#include <dirent.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/volinfo.h>
#include <netatalk/endian.h>
#include "megatron.h"
#include "nad.h"

static struct volinfo	vol;
static char		hexdig[] = "0123456789abcdef";

static char mtou_buf[MAXPATHLEN + 1], utom_buf[MAXPATHLEN + 1];
static char *mtoupathcap(char *mpath)
{
    char	*m, *u, *umax;
    int		i = 0;

    m = mpath;
    u = mtou_buf;
    umax = u + sizeof(mtou_buf) - 4;
    while ( *m != '\0' && u < umax) {
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
    return( mtou_buf );
}


#define hextoint( c )	( isdigit( c ) ? c - '0' : c + 10 - 'a' )
#define islxdigit(x)	(!isupper(x)&&isxdigit(x))

static char *utompathcap( char *upath)
{
    char	*m, *u;
    int h;

    m = utom_buf;
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
    return( utom_buf );
}

static void euc2sjis( int *p1, int *p2) /* agrees w/ Samba on valid codes */
{
    int row_offset, cell_offset;
    unsigned char c1, c2;

    /* first convert EUC to ISO-2022 */
    c1 = *p1 & 0x7F;
    c2 = *p2 & 0x7F;

    /* now convert ISO-2022 to Shift-JIS */
    row_offset = c1 < 95 ? 112 : 176;
    cell_offset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

    *p1 = ((c1 + 1) >> 1) + row_offset;
    *p2 = c2 + cell_offset;
}

static void sjis2euc( int *p1, int *p2)  /* agrees w/ Samba on valid codes */
{
    int row_offset, cell_offset, adjust;
    unsigned char c1, c2;

    c1 = *p1;
    c2 = *p2;

    /* first convert Shift-JIS to ISO-2022 */
    adjust = c2 < 159;
    row_offset = c1 < 160 ? 112 : 176;
    cell_offset = adjust ? (c2 > 127 ? 32 : 31) : 126;

    c1 = ((c1 - row_offset) << 1) - adjust;
    c2 -= cell_offset;

    /* now convert ISO-2022 to EUC */
    *p1 = c1 | 0x80;
    *p2 = c2 | 0x80;
}

static char *mtoupatheuc( char *from)
{
    unsigned char *in, *out, *maxout;
    int p, p2, i = 0;

    in = (unsigned char *) from;
    out = (unsigned char *) mtou_buf;

    if( *in ) {
        maxout = out + sizeof( mtou_buf) - 3;

        while( out < maxout ) {
            p = *in++;

            if( ((0x81 <= p) && (p <= 0x9F))
             || ((0xE0 <= p) && (p <= 0xEF)) ) {
                /* JIS X 0208 */
                p2 = *in++;
                if( ((0x40 <= p2) && (p2 <= 0x7E))
                 || ((0x80 <= p2) && (p2 <= 0xFC)) )
                    sjis2euc( &p, &p2);
                *out++ = p;
                p = p2;

            } else if( (0xA1 <= p) && (p <= 0xDF) ) {
                *out++ = 0x8E;	/* halfwidth katakana */
            } else if( p < 0x80 ) {
#ifdef DOWNCASE
                p = ( isupper( p )) ? tolower( p ) : p;
#endif /* DOWNCASE */
            }
            if( ( p == '/') || ( i == 0 && p == '.' ) ) {
                *out++ = ':';
                *out++ = hexdig[ ( p & 0xf0 ) >> 4 ];
                p = hexdig[ p & 0x0f ];
            }
            i++;
            *out++ = p;
            if( p )
                continue;
            break;
        }
    } else {
        *out++ = '.';
        *out = 0;
    }

    return mtou_buf;
}

static char *utompatheuc( char *from)
{
    unsigned char *in, *out, *maxout;
    int p, p2;

    in = (unsigned char *) from;
    out = (unsigned char *) utom_buf;
    maxout = out + sizeof( utom_buf) - 3;

    while( out < maxout ) {
        p = *in++;

        if( (0xA1 <= p) && (p <= 0xFE) ) {	/* JIS X 0208 */
            p2 = *in++;
            if( (0xA1 <= p2) && (p2 <= 0xFE) )
                euc2sjis( &p, &p2);
            *out++ = p;
            p = p2;
        } else if( p == 0x8E ) {		/* halfwidth katakana */
            p = *in++;
        } else if( p < 0x80 ) {
#ifdef DOWNCASE
            p = ( isupper( p )) ? tolower( p ) : p;
#endif /* DOWNCASE */
        }
        if ( p == ':' && *(in) != '\0' && islxdigit( *(in)) &&
                *(in+1) != '\0' && islxdigit( *(in+1))) {
           p = hextoint( *in ) << 4;
           in++;
           p |= hextoint( *in );
           in++;
	}
        *out++ = p;
        if( p )
            continue;
        break;
    }

    return utom_buf;
}

static char *mtoupathsjis( char *from)
{
    unsigned char *in, *out, *maxout;
    int p, p2, i = 0;

    in = (unsigned char *) from;
    out = (unsigned char *) mtou_buf;

    if( *in ) {
        maxout = out + sizeof( mtou_buf) - 3;

        while( out < maxout ) {
            p = *in++;

            if( ((0x81 <= p) && (p <= 0x9F))
             || ((0xE0 <= p) && (p <= 0xEF)) ) {
                /* JIS X 0208 */
                p2 = *in++;
                *out++ = p;
                p = p2;

            } else if( (0xA1 <= p) && (p <= 0xDF) ) {
                ;	/* halfwidth katakana */
            } else if(p < 0x80 ) {
#ifdef DOWNCASE
                p = ( isupper( p )) ? tolower( p ) : p;
#endif /* DOWNCASE */
	    }
            if( ( p == '/') || ( i == 0 && p == '.' ) ) {
                *out++ = ':';
                *out++ = hexdig[ ( p & 0xf0 ) >> 4 ];
                p = hexdig[ p & 0x0f ];
            }
            i++;
            *out++ = p;
            if( p )
                continue;
            break;
        }
    } else {
        *out++ = '.';
        *out = 0;
    }

    return mtou_buf;
}

static char *utompathsjis( char *from)
{
    unsigned char *in, *out, *maxout;
    int p, p2;

    in = (unsigned char *) from;
    out = (unsigned char *) utom_buf;
    maxout = out + sizeof( utom_buf) - 3;

    while( out < maxout ) {
        p = *in++;

        if( (0xA1 <= p) && (p <= 0xFE) ) {	/* JIS X 0208 */
            p2 = *in++;
            *out++ = p;
            p = p2;
        } else if( p == 0x8E ) {		/* do nothing */
            ;
        } else if( p < 0x80 ) {
#ifdef DOWNCASE
           p = ( isupper( p )) ? tolower( p ) : p;
#endif /* DOWNCASE */
        }
        if ( p == ':' && *(in) != '\0' && islxdigit( *(in)) &&
                *(in+1) != '\0' && islxdigit( *(in+1))) {
           p = hextoint( *in ) << 4;
           in++;
           p |= hextoint( *in );
           in++;
	}
        *out++ = p;
        if( p )
            continue;
        break;
    }

    return utom_buf;
 }

static char *utompathiconv(char *upath)
{
    char        *m, *u;
    u_int16_t    flags = CONV_IGNORE | CONV_UNESCAPEHEX;
    size_t       outlen;
    static char	 mpath[MAXPATHLEN +2]; /* for convert_charset dest_len parameter +2 */

    m = mpath;
    outlen = strlen(upath);

#if 0
    if (vol->v_casefold & AFPVOL_UTOMUPPER)
        flags |= CONV_TOUPPER;
    else if (vol->v_casefold & AFPVOL_UTOMLOWER)
        flags |= CONV_TOLOWER;
#endif

    u = upath;

    /* convert charsets */
    if ((size_t)-1 == ( outlen = convert_charset ( vol.v_volcharset, vol.v_maccharset, vol.v_maccharset, u, outlen, mpath, MAXPATHLEN, &flags)) ) {
        fprintf( stderr, "Conversion from %s to %s for %s failed.", vol.v_volcodepage, vol.v_maccodepage, u);
        goto utompath_error;
    }

    if (flags & CONV_REQMANGLE) 
	goto utompath_error;

    return(m);

utompath_error:
    return(utompathcap( upath ));
}

static char *mtoupathiconv(char *mpath)
{
    char        *m, *u;
    size_t       inplen;
    size_t       outlen;
    u_int16_t    flags = 0;
    static char	 upath[MAXPATHLEN +2]; /* for convert_charset dest_len parameter +2 */

    if ( *mpath == '\0' ) {
        return( "." );
    }

    /* set conversion flags */
    if (!(vol.v_flags & AFPVOL_NOHEX))
        flags |= CONV_ESCAPEHEX;
    if (!(vol.v_flags & AFPVOL_USEDOTS))
        flags |= CONV_ESCAPEDOTS;

#if 0
    if ((vol->v_casefold & AFPVOL_MTOUUPPER))
        flags |= CONV_TOUPPER;
    else if ((vol->v_casefold & AFPVOL_MTOULOWER))
        flags |= CONV_TOLOWER;
#endif

    m = mpath;
    u = upath;

    inplen = strlen(m);
    outlen = MAXPATHLEN;

    if ((size_t)-1 == (outlen = convert_charset ( vol.v_maccharset, vol.v_volcharset, vol.v_maccharset, m, inplen, u, outlen, &flags)) ) {
        fprintf (stderr, "conversion from %s to %s for %s failed.", vol.v_maccodepage, vol.v_volcodepage, mpath);
        return(mtoupathcap( upath ));
    }

    return( upath );
}


 
char * (*_mtoupath) ( char *mpath) = mtoupathcap;
char * (*_utompath) ( char *upath) = utompathcap;

/* choose translators for optional character set */
void select_charset( int options)
{

    if( options & OPTION_EUCJP ) {
        _mtoupath = mtoupatheuc;
        _utompath = utompatheuc;
    } else if( options & OPTION_SJIS ) {
        _mtoupath = mtoupathsjis;
        _utompath = utompathsjis;
    } else {
        _mtoupath = mtoupathcap;
        _utompath = utompathcap;
    }
}


#if HEXOUTPUT
    int			hexfork[ NUMFORKS ];
#endif /* HEXOUTPUT */

static struct nad_file_data {
    char		macname[ MAXPATHLEN + 1 ];
    char		adpath[ 2 ][ MAXPATHLEN + 1];
    int			offset[ NUMFORKS ];
    struct adouble	ad;
} nad;

static void initvol(char *path)
{
    if (!loadvolinfo(path, &vol)) {
        vol_load_charsets(&vol);
        ad_init(&nad.ad, vol.v_adouble, 0);
        _mtoupath = mtoupathiconv;
        _utompath = utompathiconv;
    }
    else
        ad_init(&nad.ad, 0, 0);
}


int nad_open( char *path, int openflags, struct FHeader *fh, int options)
{
    struct stat		st;
    int			fork;

/*
 * Depending upon openflags, set up nad.adpath for the open.  If it 
 * is for write, then stat the current directory to get its mode.
 * Open the file.  Either fill or grab the adouble information.
 */
    select_charset( options);
    memset(&nad.ad, 0, sizeof(nad.ad));

    if ( openflags == O_RDONLY ) {
    	initvol(path);
	strcpy( nad.adpath[0], path );
	strcpy( nad.adpath[1], 
		nad.ad.ad_ops->ad_path( nad.adpath[0], ADFLAGS_HF ));
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
	initvol (".");
	strcpy( nad.macname, fh->name );
	strcpy( nad.adpath[0], mtoupath( nad.macname ));
	strcpy( nad.adpath[1], 
		nad.ad.ad_ops->ad_path( nad.adpath[0], ADFLAGS_HF ));
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

int nad_header_read(struct FHeader *fh)
{
    u_int32_t		temptime;
    struct stat		st;
    char 		*p;

#if 0
    memcpy( nad.macname, ad_entry( &nad.ad, ADEID_NAME ), 
	    ad_getentrylen( &nad.ad, ADEID_NAME ));
    nad.macname[ ad_getentrylen( &nad.ad, ADEID_NAME ) ] = '\0';
    strcpy( fh->name, nad.macname );
#endif

    /* just in case there's nothing in macname */
    if (*fh->name == '\0') {
      if ( NULL == (p = strrchr(nad.adpath[DATA], '/')) )
        p = nad.adpath[DATA];
      else p++;
#if 0      
      strcpy(fh->name, utompath(nad.adpath[DATA]));
#endif      
      strcpy(fh->name, utompath(p));
    }

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

int nad_header_write(struct FHeader *fh)
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
    ad_flush( &nad.ad );

    return( 0 );
}

static int		forkeid[] = { ADEID_DFORK, ADEID_RFORK };

ssize_t nad_read(int fork, char *forkbuf, size_t bufc)
{
    ssize_t		cc = 0;

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

ssize_t nad_write(int fork, char *forkbuf, size_t bufc)
{
    char		*buf_ptr;
    size_t		writelen;
    ssize_t		cc = 0;

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

int nad_close(int status)
{
    int			rv;
    if ( status == KEEP ) {
	if (( rv = ad_flush( &nad.ad )) < 0 ) {
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
