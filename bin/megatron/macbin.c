/*
 * $Id: macbin.c,v 1.15 2010-01-27 21:27:53 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/param.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

#include <atalk/adouble.h>
#include <netatalk/endian.h>
#include "megatron.h"
#include "macbin.h"
#include "updcrc.h"

/* This allows megatron to generate .bin files that won't choke other
   well-known converter apps. It also makes sure that checksums
   always match. (RLB) */
#define MACBINARY_PLAY_NICE_WITH_OTHERS

/*	String used to indicate standard input instead of a disk
	file.  Should be a string not normally used for a file
 */
#ifndef	STDIN
#	define	STDIN	"-"
#endif /* STDIN */

/*	Yes and no
 */
#define NOWAY		0
#define SURETHANG	1

/*	Size of a macbinary file header
 */
#define HEADBUFSIZ	128

/*	Both input and output routines use this struct and the
	following globals; therefore this module can only be used
	for one of the two functions at a time.
 */
static struct bin_file_data {
    u_int32_t		forklen[ NUMFORKS ];
    char		path[ MAXPATHLEN + 1];
    int			filed;
    u_short		headercrc;
    time_t              gmtoff; /* to convert from/to localtime */
} 		bin;

extern char	*forkname[];
static u_char	head_buf[HEADBUFSIZ];

/* 
 * bin_open must be called first.  pass it a filename that is supposed
 * to contain a macbinary file.  an bin struct will be allocated and
 * somewhat initialized; bin_filed is set.
 */

int bin_open(char *binfile, int flags, struct FHeader *fh, int options)
{
    int			maxlen;
    int			rc;
    time_t              t;
    struct tm           *tp;

#if DEBUG
    fprintf( stderr, "entering bin_open\n" );
#endif /* DEBUG */

    /* call localtime so that we get the timezone offset */
    bin.gmtoff = 0;
#ifndef NO_STRUCT_TM_GMTOFF
    time(&t);
    tp = localtime(&t);
    if (tp)
        bin.gmtoff = tp->tm_gmtoff;
#endif /* ! NO_STRUCT_TM_GMTOFF */

    if ( flags == O_RDONLY ) { /* input */
	if ( strcmp( binfile, STDIN ) == 0 ) {
	    bin.filed = fileno( stdin );
	} else if (( bin.filed = open( binfile, flags )) < 0 ) {
	    perror( binfile );
	    return( -1 );
	}
#if DEBUG
	fprintf( stderr, "opened %s for read\n", binfile );
#endif /* DEBUG */
	if ((( rc = test_header() ) > 0 ) && 
		( bin_header_read( fh, rc ) == 0 )) {
	    return( 0 );
	}
	fprintf( stderr, "%s is not a macbinary file.\n", binfile );
	return( -1 );
    } else { /* output */
        if (options & OPTION_STDOUT) 
	  bin.filed = fileno(stdout);
	else {
	  maxlen = sizeof( bin.path ) - 1;
#if DEBUG
	  fprintf( stderr, "sizeof bin.path\t\t\t%d\n", sizeof( bin.path ));
	  fprintf( stderr, "maxlen \t\t\t\t%d\n", maxlen );
#endif /* DEBUG */
	  strncpy( bin.path, fh->name, maxlen );
	  strncpy( bin.path, mtoupath( bin.path ), maxlen );
	  strncat( bin.path, ".bin", maxlen - strlen( bin.path ));
	  if (( bin.filed = open( bin.path, flags, 0666 )) < 0 ) {
	    perror( bin.path );
	    return( -1 );
	  }
#if DEBUG
	  fprintf( stderr, "opened %s for write\n", 
		   (options & OPTION_STDOUT) ? "(stdout)" : bin.path );
#endif /* DEBUG */
	}

	if ( bin_header_write( fh ) != 0 ) {
	    bin_close( TRASH );
	    fprintf( stderr, "%s\n", bin.path );
	    return( -1 );
	}
	return( 0 );
    }
}

/* 
 * bin_close must be called before a second file can be opened using
 * bin_open.  Upon successful completion, a value of 0 is returned.  
 * Otherwise, a value of -1 is returned.
 */

int bin_close(int keepflag)
{
#if DEBUG
    fprintf( stderr, "entering bin_close\n" );
#endif /* DEBUG */
    if ( keepflag == KEEP ) {
	return( close( bin.filed ));
    } else if ( keepflag == TRASH ) {
	if (( strcmp( bin.path, STDIN ) != 0 ) && 
		( unlink( bin.path ) < 0 )) {
	    perror ( bin.path );
	}
	return( 0 );
    } else return( -1 );
}

/*
 * bin_read is called until it returns zero for each fork.  when it is
 * and finds that there is zero left to give, it seeks to the position
 * of the next fork (if there is one ).
 * bin_read must be called enough times to
 * return zero and no more than that.
 */

ssize_t bin_read( int fork, char *buffer, size_t length)
{
    char		*buf_ptr;
    size_t		readlen;
    ssize_t		cc = 1;
    off_t		pos;

#if DEBUG >= 3
    fprintf( stderr, "bin_read: fork is %s\n", forkname[ fork ] );
    fprintf( stderr, "bin_read: remaining length is %d\n", bin.forklen[fork] );
#endif /* DEBUG >= 3 */

    if (bin.forklen[fork] > 0x7FFFFFFF) {
	fprintf(stderr, "This should never happen, dude! fork length == %u\n", bin.forklen[fork]);
	return -1;
    }

    if ( bin.forklen[ fork ] == 0 ) {
	if ( fork == DATA ) {
	    pos = lseek( bin.filed, 0, SEEK_CUR );
#if DEBUG
	    fprintf( stderr, "current position is %ld\n", pos );
#endif /* DEBUG */
	    pos %= HEADBUFSIZ;
	    if (pos != 0) {
	      pos = lseek( bin.filed, HEADBUFSIZ - pos, SEEK_CUR );
	    }
#if DEBUG
	    fprintf( stderr, "current position is %ld\n", pos );
#endif /* DEBUG */
	}
	return( 0 );
    }

    if ( bin.forklen[ fork ] < length ) {
	readlen = bin.forklen[ fork ];
    } else {
	readlen = length;
    }
#if DEBUG >= 3
    fprintf( stderr, "bin_read: readlen is %d\n", readlen );
    fprintf( stderr, "bin_read: cc is %d\n", cc );
#endif /* DEBUG >= 3 */

    buf_ptr = buffer;
    while (( readlen > 0 ) && ( cc > 0 )) {
	if (( cc = read( bin.filed, buf_ptr, readlen )) > 0 ) {
#if DEBUG >= 3
	    fprintf( stderr, "bin_read: cc is %d\n", cc );
#endif /* DEBUG >= 3 */
	    readlen -= cc;
	    buf_ptr += cc;
	}
    }
    if ( cc >= 0 ) {
	cc = buf_ptr - buffer;
	bin.forklen[ fork ] -= cc;
    }

#if DEBUG >= 3
    fprintf( stderr, "bin_read: chars read is %d\n", cc );
#endif /* DEBUG >= 3 */
    return( cc );
}

/*
 * bin_write 
 */

ssize_t bin_write(int fork, char *buffer, size_t length)
{
    char		*buf_ptr;
    ssize_t		writelen;
    ssize_t		cc = 0;
    off_t		pos;
    u_char		padchar = 0x7f;
		/* Not sure why, but it seems this must be 0x7f to match
		   other converters, not 0. (RLB) */

#if DEBUG >= 3
    fprintf( stderr, "bin_write: fork is %s\n", forkname[ fork ] );
    fprintf( stderr, "bin_write: remaining length is %d\n", bin.forklen[fork] );
#endif /* DEBUG >= 3 */

    if (( fork == RESOURCE ) && ( bin.forklen[ DATA ] != 0 )) {
	fprintf( stderr, "Forklength error.\n" );
	return( -1 );
    }

    buf_ptr = (char *)buffer;
    if ( bin.forklen[ fork ] >= length ) {
	writelen = length;
    } else {
	fprintf( stderr, "Forklength error.\n" );
	return( -1 );
    }

#if DEBUG >= 3
    fprintf( stderr, "bin_write: write length is %d\n", writelen );
#endif /* DEBUG >= 3 */

    while (( writelen > 0 ) && ( cc >= 0 )) {
	cc = write( bin.filed, buf_ptr, writelen );
	buf_ptr += cc;
	writelen -= cc;
    }
    if ( cc < 0 ) {
	perror( "Couldn't write to macbinary file:" );
	return( cc );
    }

    bin.forklen[fork] -= length;

/*
 * add the padding at end of data and resource forks
 */

    if ( bin.forklen[ fork ] == 0 ) {
	pos = lseek( bin.filed, 0, SEEK_CUR );
#if DEBUG
	fprintf( stderr, "current position is %ld\n", pos );
#endif /* DEBUG */
	pos %= HEADBUFSIZ;
	if (pos != 0) { /* pad only if we need to */
	  pos = lseek( bin.filed, HEADBUFSIZ - pos - 1, SEEK_CUR );
	  if ( write( bin.filed, &padchar, 1 ) != 1 ) {
	    perror( "Couldn't write to macbinary file:" );
	    return( -1 );
	  }
	}
#if DEBUG
	  fprintf( stderr, "current position is %ld\n", pos );
#endif /* DEBUG */
    }

#if DEBUG
	fprintf( stderr, "\n" );
#endif /* DEBUG */

    return( length );
}

/* 
 * bin_header_read is called by bin_open, and before any information can
 * read from the fh substruct.  it must be called before any
 * of the bytes of the other two forks can be read, as well.
 */

int bin_header_read(struct FHeader *fh, int revision)
{
    u_short		mask;

/*
 * Set the appropriate finder flags mask for the type of macbinary
 * file it is, and copy the extra macbinary II stuff from the header.
 * If it is not a macbinary file revision of I or II, then return
 * negative.
 */

    switch ( revision ) {
        case 3:
	case 2 :
	    mask = htons( 0xfcee );
	    memcpy(&fh->finder_info.fdFlags + 1, head_buf + 101,1 );
	    break;
	case 1 :
	    mask = htons( 0xfc00 );
	    break;
	default :
	    return( -1 );
	    break;
    }

/*
 * Go through and copy all the stuff you can get from the 
 * MacBinary header into the fh struct.  What fun!
 */

    memcpy(fh->name, head_buf +  2, head_buf[ 1 ] );
    memcpy(&fh->create_date, head_buf +  91, 4 );
    fh->create_date = MAC_DATE_TO_UNIX(fh->create_date) - bin.gmtoff;
    fh->create_date = AD_DATE_FROM_UNIX(fh->create_date);
    memcpy( &fh->mod_date, head_buf +  95, 4 );
    fh->mod_date = MAC_DATE_TO_UNIX(fh->mod_date) - bin.gmtoff;
    fh->mod_date = AD_DATE_FROM_UNIX(fh->mod_date);
    fh->backup_date = AD_DATE_START;
    memcpy( &fh->finder_info, head_buf +  65, 8 );

#ifndef MACBINARY_PLAY_NICE_WITH_OTHERS /* (RLB) */
    memcpy( &fh->finder_info.fdFlags, head_buf + 73, 1 );
    fh->finder_info.fdFlags &= mask;
#else /* ! MACBINARY_PLAY_NICE_WITH_OTHERS */
	memcpy( &fh->finder_info.fdFlags, head_buf + 73, 2 );
#endif /* ! MACBINARY_PLAY_NICE_WITH_OTHERS */

    memcpy(&fh->finder_info.fdLocation, head_buf + 75, 4 );
    memcpy(&fh->finder_info.fdFldr, head_buf +  79, 2 );
    memcpy(&fh->forklen[ DATA ],  head_buf + 83, 4 );
    bin.forklen[ DATA ] = ntohl( fh->forklen[ DATA ] );
    memcpy(&fh->forklen[ RESOURCE ],  head_buf +  87, 4 );
    bin.forklen[ RESOURCE ] = ntohl( fh->forklen[ RESOURCE ] );
    fh->comment[0] = '\0';

    if (revision == 3) {
      fh->finder_xinfo.fdScript = *(head_buf + 106);
      fh->finder_xinfo.fdXFlags = *(head_buf + 107);
    }

#if DEBUG >= 5
    {
	short		flags;
	long		flags_long;

	fprintf( stderr, "Values read by bin_header_read\n" );
	fprintf( stderr, "name length\t\t%d\n", head_buf[ 1 ] );
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

	/* Show fdLocation too (RLB) */
	memcpy( &flags_long, &fh->finder_info.fdLocation,
		sizeof( flags_long ));
	flags_long = ntohl( flags_long );
	fprintf( stderr, "location flags\t\t%lx\n", flags_long );

	fprintf( stderr, "data fork length\t%ld\n", bin.forklen[DATA] );
	fprintf( stderr, "resource fork length\t%ld\n", bin.forklen[RESOURCE] );
	fprintf( stderr, "\n" );
    }
#endif /* DEBUG >= 5 */

    return( 0 );
}

/* 
 * bin_header_write is called by bin_open, and relies on information
 * from the fh substruct.  it must be called before any
 * of the bytes of the other two forks can be written, as well.
 * bin_header_write and bin_header_read are opposites.
 */

int bin_header_write(struct FHeader *fh)
{
    char		*write_ptr;
    u_int32_t           t;
    int			wc;
    int			wr;

    memset(head_buf, 0, sizeof( head_buf ));
    head_buf[ 1 ] = (u_char)strlen( fh->name );
    memcpy( head_buf + 2, fh->name, head_buf[ 1 ] );
    memcpy( head_buf + 65, &fh->finder_info, 8 );

#ifndef MACBINARY_PLAY_NICE_WITH_OTHERS /* (RLB) */
    memcpy( head_buf + 73, &fh->finder_info.fdFlags, 1 );
#else /* ! MACBINARY_PLAY_NICE_WITH_OTHERS */
    memcpy( head_buf + 73, &fh->finder_info.fdFlags, 2 );
#endif /* ! MACBINARY_PLAY_NICE_WITH_OTHERS */

    memcpy( head_buf + 75, &fh->finder_info.fdLocation, 4 );
    memcpy( head_buf + 79, &fh->finder_info.fdFldr, 2 );
    memcpy( head_buf + 83, &fh->forklen[ DATA ], 4 );
    memcpy( head_buf + 87, &fh->forklen[ RESOURCE ], 4 );
    t = AD_DATE_TO_UNIX(fh->create_date) + bin.gmtoff;
    t = MAC_DATE_FROM_UNIX(t);
    memcpy( head_buf + 91, &t, sizeof(t) );
    t = AD_DATE_TO_UNIX(fh->mod_date) + bin.gmtoff;
    t = MAC_DATE_FROM_UNIX(t);
    memcpy( head_buf + 95, &t, sizeof(t) );
    memcpy( head_buf + 101, &fh->finder_info.fdFlags + 1, 1);

    /* macbinary III */
    memcpy( head_buf + 102, "mBIN", 4);
    *(head_buf + 106) = fh->finder_xinfo.fdScript;
    *(head_buf + 107) = fh->finder_xinfo.fdXFlags;
    head_buf[ 122 ] = 130;

    head_buf[ 123 ] = 129;

    bin.headercrc = htons( updcrc( (u_short) 0, head_buf, 124 ));
    memcpy(head_buf + 124, &bin.headercrc, sizeof( bin.headercrc ));

    bin.forklen[ DATA ] = ntohl( fh->forklen[ DATA ] );
    bin.forklen[ RESOURCE ] = ntohl( fh->forklen[ RESOURCE ] );

#if DEBUG >= 5
    {
	short	flags;
	long	flags_long;

	fprintf( stderr, "Values written by bin_header_write\n" );
	fprintf( stderr, "name length\t\t%d\n", head_buf[ 1 ] );
	fprintf( stderr, "file name\t\t%s\n", (char *)&head_buf[ 2 ] );
	fprintf( stderr, "type\t\t\t%.4s\n", (char *)&head_buf[ 65 ] );
	fprintf( stderr, "creator\t\t\t%.4s\n", (char *)&head_buf[ 69 ] );

	memcpy( &flags, &fh->finder_info.fdFlags, sizeof( flags ));
	flags = ntohs( flags );
	fprintf( stderr, "flags\t\t\t%x\n", flags );

	/* Show fdLocation too (RLB) */
	memcpy( &flags_long, &fh->finder_info.fdLocation,
		sizeof( flags_long ));
	flags_long = ntohl( flags_long );
	fprintf( stderr, "location flags\t\t%ldx\n", flags_long );

	fprintf( stderr, "data fork length\t%ld\n", bin.forklen[DATA] );
	fprintf( stderr, "resource fork length\t%ld\n", bin.forklen[RESOURCE] );
	fprintf( stderr, "\n" );
    }
#endif /* DEBUG >= 5 */

    write_ptr = (char *)head_buf;
    wc = sizeof( head_buf );
    wr = 0;
    while (( wc > 0 ) && ( wr >= 0 )) {
	wr = write( bin.filed, write_ptr, wc );
	write_ptr += wr;
	wc -= wr;
    }
    if ( wr < 0 ) {
	perror( "Couldn't write macbinary header:" );
	return( wr );
    }

    return( 0 );
}

/*
 * test_header is called from bin_open.  it checks certain values of
 * the first 128 bytes, determines if the file is a MacBinary,
 * MacBinary II, MacBinary III, or non-MacBinary file, and returns a
 * one, two, three or negative one to indicate the file type.
 *
 * If the signature at 102 is equal to "mBIN," then it's a MacBinary
 * III file. Bytes 0 and 74 must be zero for the file to be any type
 * of MacBinary.  If the crc of bytes 0 through 123 equals the value
 * at offset 124 then it is a MacBinary II.  If not, then if byte 82
 * is zero, byte 2 is a valid value for a mac filename length (between
 * one and sixty-three), and bytes 101 through 125 are all zero, then
 * the file is a MacBinary. 
 *
 * NOTE: apple's MacBinary II files have a non-zero value at byte 74.
 * so, the check for byte 74 isn't very useful.
 */

int test_header(void)
{
    const char          zeros[25] = "";
    ssize_t		cc;
    u_short		header_crc;
    u_char		namelen;

#if DEBUG
    fprintf( stderr, "entering test_header\n" );
#endif /* DEBUG */

    cc = read( bin.filed, (char *)head_buf, sizeof( head_buf ));
    if ( cc < sizeof( head_buf )) {
	perror( "Premature end of file :" );
	return( -1 );
    }

#if DEBUG
    fprintf( stderr, "was able to read HEADBUFSIZ bytes\n" );
#endif /* DEBUG */

    /* check for macbinary III header */
    if (memcmp(head_buf + 102, "mBIN", 4) == 0)
        return 3;

    /* check for macbinary II even if only one of the bytes is zero */
    if (( head_buf[ 0 ] == 0 ) || ( head_buf[ 74 ] == 0 )) {
#if DEBUG
      fprintf( stderr, "byte 0 and 74 are both zero\n" );
#endif /* DEBUG */
      bin.headercrc = updcrc( (u_short) 0, head_buf, 124 );
      memcpy(&header_crc, head_buf + 124, sizeof( header_crc ));
      header_crc = ntohs( header_crc );
      if ( header_crc == bin.headercrc ) {
	return( 2 );
      }

#if DEBUG
      fprintf( stderr, "header crc didn't pan out\n" );
#endif /* DEBUG */
    }

    /* now see if we have a macbinary file. */
    if ( head_buf[ 82 ] != 0 ) {
	return( -1 );
    }
    memcpy( &namelen, head_buf + 1, sizeof( namelen ));
#if DEBUG
    fprintf( stderr, "name length is %d\n", namelen );
#endif /* DEBUG */
    if (( namelen < 1 ) || ( namelen > 63 )) {
	return( -1 );
    }

    /* bytes 101 - 125 should be zero */
    if (memcmp(head_buf + 101, zeros, sizeof(zeros)) != 0)
        return -1;

    /* macbinary forks aren't larger than 0x7FFFFF */
    /* we allow forks to be larger, breaking the specs */
    memcpy(&cc, head_buf + 83, sizeof(cc));
    cc = ntohl(cc);
    if (cc > 0x7FFFFFFF)
        return -1;
    memcpy(&cc, head_buf + 87, sizeof(cc));
    cc = ntohl(cc);
    if (cc > 0x7FFFFFFF)
        return -1;


#if DEBUG
    fprintf( stderr, "byte 82 is zero and name length is cool\n" );
#endif /* DEBUG */

    return( 1 );
}
