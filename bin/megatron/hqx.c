/*
 * $Id: hqx.c,v 1.18 2010-01-27 21:27:53 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/param.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <netinet/in.h>

#include <atalk/adouble.h>
#include <netatalk/endian.h>

#include "megatron.h"
#include "nad.h"
#include "hqx.h"
#include "updcrc.h"

#define HEXOUTPUT	0

/*	String used to indicate standard input instead of a disk
	file.  Should be a string not normally used for a file
 */
#ifndef	STDIN
#	define	STDIN	"-"
#endif /* ! STDIN */

/*	Yes and no
 */
#define NOWAY		0
#define SURETHANG	1

/*	Looking for the first or any other line of a binhex file
 */
#define	FIRST		0
#define OTHER		1

/*	This is the binhex run length encoding character
 */
#define RUNCHAR		0x90

/*	These are field sizes in bytes of various pieces of the
	binhex header
 */
#define	BHH_VERSION		1
#define	BHH_TCSIZ		8
#define	BHH_FLAGSIZ		2
#define	BHH_DATASIZ		4
#define	BHH_RESSIZ		4
#define BHH_CRCSIZ		2
#define BHH_HEADSIZ		21

#if HEXOUTPUT
FILE		*rawhex, *expandhex;
#endif /* HEXOUTPUT */

static struct hqx_file_data {
    u_int32_t		forklen[ NUMFORKS ];
    u_short		forkcrc[ NUMFORKS ];
    char		path[ MAXPATHLEN + 1];
    u_short		headercrc;
    int			filed;
} 		hqx;

extern char	*forkname[];
static u_char	hqx7_buf[8192];
static u_char	*hqx7_first;
static u_char	*hqx7_last;
static int	first_flag;

/* 
hqx_open must be called first.  pass it a filename that is supposed
to contain a binhqx file.  an hqx struct will be allocated and
somewhat initialized; hqx_fd is set.  skip_junk is called from
here; skip_junk leaves hqx7_first and hqx7_last set.
 */

int hqx_open(char *hqxfile, int flags, struct FHeader *fh, int options)
{
    int			maxlen;

#if DEBUG
    fprintf( stderr, "megatron: entering hqx_open\n" );
#endif /* DEBUG */
    select_charset( options);
    if ( flags == O_RDONLY ) {

#if HEXOUTPUT
	rawhex = fopen( "rawhex.unhex", "w" );
	expandhex = fopen( "expandhex.unhex", "w" );
#endif /* HEXOUTPUT */

	first_flag = 0;

	if ( strcmp( hqxfile, STDIN ) == 0 ) {
	    hqx.filed = fileno( stdin );
	} else if (( hqx.filed = open( hqxfile, O_RDONLY )) < 0 ) {
	    perror( hqxfile );
	    return( -1 );
	}

	if ( skip_junk( FIRST ) == 0 ) {
	    if ( hqx_header_read( fh ) == 0 ) {
#if DEBUG
		off_t	pos;

		pos = lseek( hqx.filed, 0, SEEK_CUR );
		fprintf( stderr, "megatron: current position is %ld\n", pos );
#endif /* DEBUG */
		return( 0 );
	    }
	}
	hqx_close( KEEP );
	fprintf( stderr, "%s\n", hqxfile );
	return( -1 );
    } else {
	maxlen = sizeof( hqx.path ) -1;
	strncpy( hqx.path, fh->name, maxlen );
	strncpy( hqx.path, mtoupath( hqx.path ), maxlen );
	strncat( hqx.path, ".hqx", maxlen - strlen( hqx.path ));
	if (( hqx.filed = open( hqx.path, flags, 0666 )) < 0 ) {
	    perror( hqx.path );
	    return( -1 );
	}
	if ( hqx_header_write( fh ) != 0 ) {
	    hqx_close( TRASH );
	    fprintf( stderr, "%s\n", hqx.path );
	    return( -1 );
	}
	return( 0 );
    }
}

/* 
 * hqx_close must be called before a second file can be opened using
 * hqx_open.  Upon successful completion, a value of 0 is returned.  
 * Otherwise, a value of -1 is returned.
 */

int hqx_close(int keepflag)
{
    if ( keepflag == KEEP ) {
	return( close( hqx.filed ));
    } else if ( keepflag == TRASH ) {
	if (( strcmp( hqx.path, STDIN ) != 0 ) && ( unlink( hqx.path ) < 0 )) {
	    perror( hqx.path );
	}
	return( 0 );
    } else return( -1 );
}

/*
 * hqx_read is called until it returns zero for each fork.  when it is
 * and finds that there is zero left to give, it reads in and compares
 * the crc with the calculated one, and returns zero if all is well.
 * it returns negative is the crc was bad or if has been called too many
 * times for the same fork.  hqx_read must be called enough times to
 * return zero and no more than that.
 */

ssize_t hqx_read(int fork, char *buffer, size_t length)
{
    u_short		storedcrc;
    size_t		readlen;
    size_t		cc;

#if DEBUG >= 3
    {
	off_t	pos;
	pos = lseek( hqx.filed, 0, SEEK_CUR );
	fprintf( stderr, "hqx_read: current position is %ld\n", pos );
    }
    fprintf( stderr, "hqx_read: fork is %s\n", forkname[ fork ] );
    fprintf( stderr, "hqx_read: remaining length is %d\n", hqx.forklen[fork] );
#endif /* DEBUG >= 3 */

    if (hqx.forklen[fork] > 0x7FFFFFFF) {
	fprintf(stderr, "This should never happen, dude!, fork length == %u\n", hqx.forklen[fork]);
	return -1;
    }

    if ( hqx.forklen[ fork ] == 0 ) {
	cc = hqx_7tobin( (char *)&storedcrc, sizeof( storedcrc ));
	if ( cc == sizeof( storedcrc )) {
	    storedcrc = ntohs ( storedcrc );
#if DEBUG >= 4
    fprintf( stderr, "hqx_read: storedcrc\t\t%x\n", storedcrc );
    fprintf( stderr, "hqx_read: observed crc\t\t%x\n\n", hqx.forkcrc[fork] );
#endif /* DEBUG >= 4 */
	    if ( storedcrc == hqx.forkcrc[ fork ] ) {
		return( 0 );
	    }
	    fprintf( stderr, "hqx_read: Bad %s fork crc, dude\n", 
		    forkname[ fork ] );
	}
	return( -1 );
    }

    if ( hqx.forklen[ fork ] < length ) {
	readlen = hqx.forklen[ fork ];
    } else {
	readlen = length;
    }
#if DEBUG >= 3
    fprintf( stderr, "hqx_read: readlen is %d\n", readlen );
#endif /* DEBUG >= 3 */

    cc = hqx_7tobin( buffer, readlen );
    if ( cc > 0 ) {
	hqx.forkcrc[ fork ] = 
		updcrc( hqx.forkcrc[ fork ], (u_char *)buffer, cc );
	hqx.forklen[ fork ] -= cc;
    }
#if DEBUG >= 3
    fprintf( stderr, "hqx_read: chars read is %d\n", cc );
#endif /* DEBUG >= 3 */
    return( cc );
}

/* 
 * hqx_header_read is called by hqx_open, and before any information can
 * read from the hqx_header substruct.  it must be called before any
 * of the bytes of the other two forks can be read, as well.
 * returns a negative number if it was unable to pull enough information
 * to fill the hqx_header fields.
 */

int hqx_header_read(struct FHeader *fh)
{
    char		*headerbuf, *headerptr;
    u_int32_t		time_seconds;
    u_short		mask;
    u_short		header_crc;
    char		namelen;

#if HEXOUTPUT
    int		headerfork;
    headerfork = open( "headerfork", O_WRONLY|O_CREAT, 0622 );
#endif /* HEXOUTPUT */

    mask = htons( 0xfcee );
    hqx.headercrc = 0;

    if ( hqx_7tobin( &namelen, sizeof( namelen )) == 0 ) {
	fprintf( stderr, "Premature end of file :" );
	return( -2 );
    }
    hqx.headercrc = updcrc( hqx.headercrc, (u_char *)&namelen, 
	    sizeof( namelen ));

#if HEXOUTPUT
    write( headerfork, &namelen, sizeof( namelen ));
#endif /* HEXOUTPUT */

    if (( headerbuf = 
	    (char *)malloc( (unsigned int)( namelen + BHH_HEADSIZ ))) == NULL ) {
	return( -1 );
    }
    if ( hqx_7tobin( headerbuf, ( namelen + BHH_HEADSIZ )) == 0 ) {
	free( headerbuf );
	fprintf( stderr, "Premature end of file :" );
	return( -2 );
    }
    headerptr = headerbuf;
    hqx.headercrc = updcrc( hqx.headercrc, 
	    (u_char *)headerbuf, ( namelen + BHH_HEADSIZ - BHH_CRCSIZ ));

#if HEXOUTPUT
    write( headerfork, headerbuf, ( namelen + BHH_HEADSIZ ));
#endif /* HEXOUTPUT */

/*
 * stuff from the hqx file header
 */

    memcpy( fh->name, headerptr, (int)namelen );
    headerptr += namelen;
    headerptr += BHH_VERSION;
    memcpy(&fh->finder_info,  headerptr, BHH_TCSIZ );
    headerptr += BHH_TCSIZ;
    memcpy(&fh->finder_info.fdFlags,  headerptr, BHH_FLAGSIZ );
    fh->finder_info.fdFlags = fh->finder_info.fdFlags & mask;
    headerptr += BHH_FLAGSIZ;
    memcpy(&fh->forklen[ DATA ],  headerptr, BHH_DATASIZ );
    hqx.forklen[ DATA ] = ntohl( fh->forklen[ DATA ] );
    headerptr += BHH_DATASIZ;
    memcpy( &fh->forklen[ RESOURCE ], headerptr, BHH_RESSIZ );
    hqx.forklen[ RESOURCE ] = ntohl( fh->forklen[ RESOURCE ] );
    headerptr += BHH_RESSIZ;
    memcpy(&header_crc,  headerptr, BHH_CRCSIZ );
    headerptr += BHH_CRCSIZ;
    header_crc = ntohs( header_crc );

/*
 * stuff that should be zero'ed out
 */

    fh->comment[0] = '\0';
    fh->finder_info.fdLocation = 0;
    fh->finder_info.fdFldr = 0;

#if DEBUG >= 5
    {
	short		flags;

	fprintf( stderr, "Values read by hqx_header_read\n" );
	fprintf( stderr, "name length\t\t%d\n", namelen );
	fprintf( stderr, "file name\t\t%s\n", fh->name );
	fprintf( stderr, "get info comment\t%s\n", fh->comment );
	fprintf( stderr, "type\t\t\t%.*s\n", sizeof( fh->finder_info.fdType ),
		&fh->finder_info.fdType );
	fprintf( stderr, "creator\t\t\t%.*s\n", 
		sizeof( fh->finder_info.fdCreator ), 
		&fh->finder_info.fdCreator );
	memcpy( &flags, &fh->finder_info.fdFlags, sizeof( flags ));
	flags = ntohs( flags );
	fprintf( stderr, "flags\t\t\t%x\n", flags );
	fprintf( stderr, "data fork length\t%ld\n", hqx.forklen[DATA] );
	fprintf( stderr, "resource fork length\t%ld\n", hqx.forklen[RESOURCE] );
	fprintf( stderr, "header_crc\t\t%x\n", header_crc );
	fprintf( stderr, "observed crc\t\t%x\n", hqx.headercrc );
	fprintf( stderr, "\n" );
    }
#endif /* DEBUG >= 5 */

/*
 * create and modify times are figured from right now
 */

    time_seconds = AD_DATE_FROM_UNIX(time( NULL ));
    memcpy( &fh->create_date, &time_seconds, 
	    sizeof( fh->create_date ));
    memcpy( &fh->mod_date, &time_seconds, 
	    sizeof( fh->mod_date ));
    fh->backup_date = AD_DATE_START;

/*
 * stuff that should be zero'ed out
 */

    fh->comment[0] = '\0';
    memset( &fh->finder_info.fdLocation, 0, 
	    sizeof( fh->finder_info.fdLocation ));
    memset( &fh->finder_info.fdFldr, 0, sizeof( fh->finder_info.fdFldr ));

    hqx.forkcrc[ DATA ] = 0;
    hqx.forkcrc[ RESOURCE ] = 0;

    free( headerbuf );
    if ( header_crc != hqx.headercrc ) {
	fprintf( stderr, "Bad Header crc, dude :" );
	return( -3 );
    }
    return( 0 );
}

/*
 * hqx_header_write.
 */

int hqx_header_write(struct FHeader *fh _U_)
{
    return( -1 );
}

/*
 * hqx7_fill is called from skip_junk and hqx_7tobin.  it pulls from the
 * binhqx file into the hqx7 buffer.  returns number of bytes read
 * or a zero for end of file.
 * it sets the pointers to the hqx7 buffer up to point to the valid data.
 */

ssize_t hqx7_fill(u_char *hqx7_ptr)
{
    ssize_t		cc;
    size_t		cs;

    cs = hqx7_ptr - hqx7_buf;
    if ( cs >= sizeof( hqx7_buf )) return( -1 );
    hqx7_first = hqx7_ptr;
    cc = read( hqx.filed, (char *)hqx7_first, ( sizeof( hqx7_buf ) - cs ));
    if ( cc < 0 ) {
	perror( "" );
	return( cc );
    }
    hqx7_last = ( hqx7_first + cc );
    return( cc );
}

/*
char tr[] = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
	     0 123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
	     0                1               2               3 
Input characters are translated to a number between 0 and 63 by direct
array lookup.  0xFF signals a bad character.  0xFE is signals a legal
character that should be skipped, namely '\n', '\r'.  0xFD signals ':'.
0xFC signals a whitespace character.
*/

static const u_char hqxlookup[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFE, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFC, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0xFF, 0xFF,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0xFF,
    0x14, 0x15, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0xFF,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0xFF,
    0x2C, 0x2D, 0x2E, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0xFF,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0xFF, 0xFF,
    0x3D, 0x3E, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/*
 * skip_junk is called from hqx_open.  it skips over junk in the file until
 * it comes to a line containing a valid first line of binhqx encoded file.
 * returns a 0 for success, negative if it never finds good data.
 * pass a FIRST when looking for the first valid binhex line, a value of 
 * OTHER when looking for any subsequent line.
 */

int skip_junk(int line)
{
    int			found = NOWAY;
    int			stopflag;
    int			nc = 0;
    u_char		c;
    u_char		prevchar;

    if ( line == FIRST ) {
	if ( hqx7_fill( hqx7_buf  ) <= 0 ) {
	    fprintf( stderr, "Premature end of file :" );
	    return( -1 );
	}
    }

    while ( found == NOWAY ) {
	if ( line == FIRST ) {
	    if ( *hqx7_first == ':' ) {
		nc = c = 0;
		stopflag = NOWAY;
		hqx7_first++;
		while (( stopflag == NOWAY ) && 
			( nc < ( hqx7_last - hqx7_first ))) {
		    switch ( c = hqxlookup[ hqx7_first[ nc ]] ) {
			case 0xFC :
			case 0xFF :
			case 0xFE :
			case 0xFD :
			    stopflag = SURETHANG;
			    break;
			default :
			    nc++;
			    break;
		    }
		}
		if (( nc > 30 ) && ( nc < 64 ) &&
			(( c == 0xFE ) || ( c == 0xFD ))) found = SURETHANG;
	    } else {
		hqx7_first++;
	    }
	} else {
	    if (( prevchar = hqxlookup[ *hqx7_first ] ) == 0xFE ) {
		nc = c = 0;
		stopflag = NOWAY;
		hqx7_first++;
		while (( stopflag == NOWAY ) && 
			( nc < ( hqx7_last - hqx7_first ))) {
		    switch ( c = hqxlookup[ hqx7_first[ nc ]] ) {
			case 0xFC :
			case 0xFE :
			    if (( prevchar == 0xFC ) || ( prevchar == 0xFE )) {
				nc++;
				break;
			    }
			case 0xFF :
			case 0xFD :
			    stopflag = SURETHANG;
			    break;
			default :
			    prevchar = c;
			    nc++;
			    break;
		    }
		}
		if ( c == 0xFD ) {
		    found = SURETHANG;
		} else if (( nc > 30 ) && ( c == 0xFE )) {
		    found = SURETHANG;
		}
	    } else {
		hqx7_first++;
	    }
	}

	if (( hqx7_last - hqx7_first ) == nc ) {
	    if ( line == FIRST ) {
		*hqx7_buf = ':';
	    } else *hqx7_buf = '\n';
	    memcpy(hqx7_buf + 1, hqx7_first, nc );
	    hqx7_first = hqx7_buf + ( ++nc );
	    if ( hqx7_fill( hqx7_first ) <= 0 ) {
		fprintf( stderr, "Premature end of file :" );
		return( -1 );
	    }
	    hqx7_first = hqx7_buf;
	}
    }

    return( 0 );
}

/* 
 * hqx_7tobin is used to read the data, converted to binary.  It is
 * called by hqx_header_read to get the header information, and must be
 * called to get the data for each fork, and the crc data for each
 * fork.  it has the same basic calling structure as unix read.  the
 * number of valid bytes read is returned.  It does buffering so as to
 * return the requested length of data every time, unless the end of
 * file is reached.
 */

size_t hqx_7tobin( char *outbuf, size_t datalen)
{
    static u_char	hqx8[3];
    static int		hqx8i;
    static u_char	prev_hqx8;
    static u_char	prev_out;
    static u_char	prev_hqx7;
    static int		eofflag;
    u_char		hqx7[4];
    int			hqx7i = 0;
    char		*out_first;
    char		*out_last;

#if DEBUG
    fprintf( stderr, "hqx_7tobin: datalen entering %d\n", datalen );
    fprintf( stderr, "hqx_7tobin: hqx8i entering %d\n", hqx8i );
#endif /* DEBUG */

    if ( first_flag == 0 ) {
	prev_hqx8 = 0;
	prev_hqx7 = 0;
	prev_out = 0;
	hqx8i = 3;
	first_flag = 1;
	eofflag = 0;
    }

#if DEBUG
    fprintf( stderr, "hqx_7tobin: hqx8i entering %d\n", hqx8i );
#endif /* DEBUG */

    out_first = outbuf;
    out_last = out_first + datalen;

    while (( out_first < out_last ) && ( eofflag == 0 )) {

	if ( hqx7_first == hqx7_last ) {
	    if ( hqx7_fill( hqx7_buf ) == 0 ) {
		eofflag = 1;
		continue;
	    }
	}

	if ( hqx8i > 2 ) {

	    while (( hqx7i < 4 ) && ( hqx7_first < hqx7_last )) {
		hqx7[ hqx7i ] = hqxlookup[ *hqx7_first ];
		switch ( hqx7[ hqx7i ] ) {
		    case 0xFC :
			if (( prev_hqx7 == 0xFC ) || ( prev_hqx7 == 0xFE )) {
			    hqx7_first++;
			    break;
			}
		    case 0xFD :
		    case 0xFF :
			eofflag = 1;
			while ( hqx7i < 4 ) {
			    hqx7[ hqx7i++ ] = 0;
			}
			break;
		    case 0xFE :
			prev_hqx7 = hqx7[ hqx7i ];
			if ( skip_junk( OTHER ) < 0 ) {
			    fprintf( stderr, "\n" );
			    eofflag = 1;
			    while ( hqx7i < 4 ) {
				hqx7[ hqx7i++ ] = 0; }
			}
			break;
		    default :
			prev_hqx7 = hqx7[ hqx7i++ ];
			hqx7_first++;
			break;
		}
	    }
	    
	    if ( hqx7i == 4 ) {
		hqx8[ 0 ] = (( hqx7[ 0 ] << 2 ) | ( hqx7[ 1 ] >> 4 ));
		hqx8[ 1 ] = (( hqx7[ 1 ] << 4 ) | ( hqx7[ 2 ] >> 2 ));
		hqx8[ 2 ] = (( hqx7[ 2 ] << 6 ) | ( hqx7[ 3 ] ));
		hqx7i = hqx8i = 0;
	    }
	}

	while (( hqx8i < 3 ) && ( out_first < out_last )) {

#if HEXOUTPUT
	    putc( hqx8i, rawhex );
            putc( hqx8[ hqx8i ], rawhex );
#endif /* HEXOUTPUT */

	    if ( prev_hqx8 == RUNCHAR ) {
		if ( hqx8[ hqx8i ] == 0 ) {
		    *out_first = prev_hqx8;
#if HEXOUTPUT
		    putc( *out_first, expandhex );
#endif /* HEXOUTPUT */
		    prev_out = prev_hqx8;
		    out_first++;
		}
		while (( out_first < out_last ) && ( hqx8[ hqx8i ] > 1 )) {
		    *out_first = prev_out;
#if HEXOUTPUT
		    putc( *out_first, expandhex );
#endif /* HEXOUTPUT */
		    hqx8[ hqx8i ]--;
		    out_first++;
		}
		if ( hqx8[ hqx8i ] < 2 ) {
		    prev_hqx8 = hqx8[ hqx8i ];
		    hqx8i++;
		}
		continue;
	    }

	    prev_hqx8 = hqx8[ hqx8i ];
	    if ( prev_hqx8 != RUNCHAR ) {
		*out_first = prev_hqx8;
#if HEXOUTPUT
		putc( *out_first, expandhex );
#endif /* HEXOUTPUT */
		prev_out = prev_hqx8;
		out_first++;
	    }
	    hqx8i++;

	}

    }
    return( out_first - outbuf );
}
