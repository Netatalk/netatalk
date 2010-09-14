/*
 * $Id: asingle.c,v 1.14 2010-01-27 21:27:53 didg Exp $
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
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <atalk/adouble.h>
#include <netatalk/endian.h>
#include "asingle.h"
#include "megatron.h"

/*	String used to indicate standard input instead of a disk
	file.  Should be a string not normally used for a file
 */
#ifndef	STDIN
#	define	STDIN	"-"
#endif

/*	Yes and no
 */
#define NOWAY		0
#define SURETHANG	1

/*	This structure holds an entry description, consisting of three
	four byte entities.  The first is the Entry ID, the second is
	the File Offset and the third is the Length.
 */


/*	Both input and output routines use this struct and the
	following globals; therefore this module can only be used
	for one of the two functions at a time.
 */
static struct single_file_data {
    int			filed;
    char		path[ MAXPATHLEN + 1];
    struct ad_entry	entry[ ADEID_MAX ];
} 		single;

extern char	*forkname[];
static u_char	header_buf[ AD_HEADER_LEN ];

/* 
 * single_open must be called first.  pass it a filename that is supposed
 * to contain a AppleSingle file.  an single struct will be allocated and
 * somewhat initialized; single_filed is set.
 */

int single_open(char *singlefile, int flags, struct FHeader *fh, int options _U_)
{
    int			rc;

    if ( flags == O_RDONLY ) {
	if ( strcmp( singlefile, STDIN ) == 0 ) {
	    single.filed = fileno( stdin );
	} else if (( single.filed = open( singlefile, flags )) < 0 ) {
	    perror( singlefile );
	    return( -1 );
	}
	strncpy( single.path, singlefile, MAXPATHLEN );
#if DEBUG
	fprintf( stderr, "opened %s for read\n", single.path );
#endif /* DEBUG */
	if ((( rc = single_header_test()) > 0 ) && 
		( single_header_read( fh, rc ) == 0 )) {
	    return( 0 );
	}
	single_close( KEEP );
	return( -1 );
    }
    return( 0 );
}

/* 
 * single_close must be called before a second file can be opened using
 * single_open.  Upon successful completion, a value of 0 is returned.  
 * Otherwise, a value of -1 is returned.
 */

int single_close( int keepflag)
{
    if ( keepflag == KEEP ) {
	return( close( single.filed ));
    } else if ( keepflag == TRASH ) {
	if (( strcmp( single.path, STDIN ) != 0 ) && 
		( unlink( single.path ) < 0 )) {
	    perror ( single.path );
	}
	return( 0 );
    } else return( -1 );
}

/* 
 * single_header_read is called by single_open, and before any information
 * can read from the fh substruct.  it must be called before any of the
 * bytes of the other two forks can be read, as well.
 */

int single_header_read( struct FHeader *fh, int version)
{
/*
 * entry_buf is used for reading in entry descriptors, and for reading in
 * 	the actual entries of FILEINFO, FINDERINFO, and DATES.
 */
    u_char		entry_buf[ADEDLEN_FINDERI];
    u_int32_t		entry_id;
    u_int32_t		time_seconds;
    u_short		mask = 0xfcee;
    u_short		num_entries;
    int			n;
    int			readlen;
    int			date_entry = 0;
    off_t		pos;

/*
 * Go through and initialize the array of entry_info structs.  Read in the
 * number of entries, and then read in the info for each entry and save it
 * in the array.
 */

    for ( n = 0 ; n < ADEID_MAX; n++ ) {
	single.entry[ n ].ade_off = 0;
	single.entry[ n ].ade_len = 0;
    }
    memcpy( &num_entries, header_buf + 24, sizeof( num_entries ));
    num_entries = ntohs( num_entries );
#if DEBUG >= 2
    fprintf( stderr, "The number of entries is %d\n", num_entries );
#endif /* DEBUG */
    for ( ; num_entries > 0 ; num_entries-- ) {
	if ( read( single.filed, (char *)entry_buf, AD_ENTRY_LEN )
		!= AD_ENTRY_LEN ) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
	memcpy(&entry_id,  entry_buf, sizeof( entry_id ));
	entry_id = ntohl( entry_id );
	memcpy(&single.entry[ entry_id ].ade_off,
	       entry_buf + 4,
	       sizeof( single.entry[ entry_id ].ade_off ));
	single.entry[ entry_id ].ade_off =
		ntohl( single.entry[ entry_id ].ade_off );
	memcpy(&single.entry[ entry_id ].ade_len,
	       entry_buf + 8,
	       sizeof( single.entry[ entry_id ].ade_len ));
	single.entry[ entry_id ].ade_len =
		ntohl( single.entry[ entry_id ].ade_len );
#if DEBUG >= 2
	fprintf( stderr, "entry_id\t%d\n", entry_id );
	fprintf( stderr, "\toffset\t\t%d\n", single.entry[ entry_id ].ade_off );
	fprintf( stderr, "\tlength\t\t%d\n", single.entry[ entry_id ].ade_len );
#endif /* DEBUG */
    }

/*
 * Now that the entries have been identified, check to make sure
 * it is a Macintosh file if dealing with version two format file.
 */

    if ( version == 1 ) {
	if ( single.entry[ ADEID_FILEI ].ade_len > 0 )
	    date_entry = ADEID_FILEI;
    } else if ( version == 2 ) {
	if ( single.entry[ ADEID_FILEDATESI ].ade_len > 0 )
	    date_entry = ADEID_FILEDATESI;
    }
#if DEBUG
    fprintf( stderr, "date_entry = %d\n", date_entry );
#endif /* DEBUG */

/*
 * Go through and copy all the information you can get from 
 * the informational entries into the fh struct.  The ENTRYID_DATA
 * must be the last one done, because it leaves the file pointer in
 * the right place for the first read of the data fork.
 */
 
    if ( single.entry[ ADEID_NAME ].ade_off == 0 ) {
	fprintf( stderr, "%s has no name for the mac file.\n", single.path );
	return( -1 );
    } else {
	pos = lseek( single.filed, single.entry[ ADEID_NAME ].ade_off,
		SEEK_SET );
	readlen = single.entry[ ADEID_NAME ].ade_len > ADEDLEN_NAME ? 
	      ADEDLEN_NAME : single.entry[ ADEID_NAME ].ade_len;
	if ( read( single.filed, (char *)fh->name, readlen ) != readlen ) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
    }
    if (( single.entry[ ADEID_FINDERI ].ade_len < ADEDLEN_FINDERI ) ||
	    ( single.entry[ ADEID_FINDERI ].ade_off <= AD_HEADER_LEN )) {
	fprintf( stderr, "%s has bogus FinderInfo.\n", single.path );
	return( -1 );
    } else {
	pos = lseek( single.filed,
		single.entry[ ADEID_FINDERI ].ade_off, SEEK_SET );
	if ( read( single.filed, (char *)entry_buf, ADEDLEN_FINDERI) != ADEDLEN_FINDERI) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
	memcpy( &fh->finder_info.fdType, entry_buf + FINDERIOFF_TYPE,	
		sizeof( fh->finder_info.fdType ));
	memcpy( &fh->finder_info.fdCreator, entry_buf + FINDERIOFF_CREATOR,
		sizeof( fh->finder_info.fdCreator ));
	memcpy( &fh->finder_info.fdFlags, entry_buf +  FINDERIOFF_FLAGS,
		sizeof( fh->finder_info.fdFlags ));
	fh->finder_info.fdFlags = fh->finder_info.fdFlags & mask;
	memcpy( &fh->finder_info.fdLocation, entry_buf + FINDERIOFF_LOC,
		sizeof( fh->finder_info.fdLocation ));
	memcpy(&fh->finder_info.fdFldr, entry_buf + FINDERIOFF_FLDR,
		sizeof( fh->finder_info.fdFldr ));
	fh->finder_xinfo.fdScript = *(entry_buf + FINDERIOFF_SCRIPT);
	fh->finder_xinfo.fdXFlags = *(entry_buf + FINDERIOFF_XFLAGS);

#if DEBUG
	{
	    char		type[5];
	    char		creator[5];
 
	    strncpy( type, &fh->finder_info.fdType, 4 );
	    strncpy( creator, &fh->finder_info.fdCreator, 4 );
	    type[4] = creator[4] = '\0';
	    fprintf( stderr, "type is %s, creator is %s\n", type, creator );
	}
#endif /* DEBUG */
    }
    if (( single.entry[ ADEID_COMMENT ].ade_len == 0 ) || 
	    ( single.entry[ ADEID_COMMENT ].ade_off <= AD_HEADER_LEN )) {
	fh->comment[0] = '\0';
    } else {
	pos = lseek( single.filed, single.entry[ ADEID_COMMENT ].ade_off,
		SEEK_SET );
	readlen = single.entry[ ADEID_COMMENT ].ade_len > ADEDLEN_COMMENT
		? ADEDLEN_COMMENT : single.entry[ ADEID_COMMENT ].ade_len;
	if ( read( single.filed, (char *)fh->comment, readlen ) != readlen ) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
    }
/*
 * If date_entry is 7, we have an AppleSingle version one, do the 
 * appropriate stuff.  If it is 8, we have an AppleSingle version two,
 * do the right thing.  If date_entry is neither, just use the current date.
 * Unless I can't get the current date, in which case use time zero.
 */
    if (( date_entry < 7 ) || ( date_entry > 8 )) {
	if (( time_seconds = time( NULL )) == (u_int32_t)-1 ) {
	    time_seconds = AD_DATE_START;
	} else {
	    time_seconds = AD_DATE_FROM_UNIX(time_seconds);
	}
	memcpy(&fh->create_date, &time_seconds, sizeof( fh->create_date ));
	memcpy(&fh->mod_date, &time_seconds, sizeof( fh->mod_date ));
	fh->backup_date = AD_DATE_START;
    } else if ( single.entry[ date_entry ].ade_len != 16 ) {
	fprintf( stderr, "%s has bogus FileInfo or File Dates Info.\n", 
		single.path );
	return( -1 );
    } else if ( date_entry == ADEID_FILEI ) {
	pos = lseek( single.filed,
		single.entry[ date_entry ].ade_off, SEEK_SET );
	if ( read( single.filed, (char *)entry_buf, sizeof( entry_buf )) !=
		sizeof( entry_buf )) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
	memcpy( &fh->create_date, entry_buf + FILEIOFF_CREATE,
		sizeof( fh->create_date ));
	memcpy( &fh->mod_date, entry_buf + FILEIOFF_MODIFY,
		sizeof( fh->mod_date ));
	memcpy( &fh->backup_date, entry_buf + FILEIOFF_BACKUP,
		sizeof(fh->backup_date));
    } else if ( date_entry == ADEID_FILEDATESI ) {
	pos = lseek( single.filed,
		single.entry[ date_entry ].ade_off, SEEK_SET );
	if ( read( single.filed, (char *)entry_buf, sizeof( entry_buf )) !=
		sizeof( entry_buf )) {
	    perror( "Premature end of file :" );
	    return( -1 );
	}
	memcpy( &fh->create_date, entry_buf + FILEIOFF_CREATE,
		sizeof( fh->create_date ));
	memcpy( &fh->mod_date, entry_buf + FILEIOFF_MODIFY,
		sizeof( fh->mod_date ));
	memcpy( &fh->backup_date, entry_buf + FILEIOFF_BACKUP,
		sizeof(fh->backup_date));
    }
    if ( single.entry[ ADEID_RFORK ].ade_off == 0 ) {
	fh->forklen[RESOURCE] = 0;
    } else {
	fh->forklen[RESOURCE] =
		htonl( single.entry[ ADEID_RFORK ].ade_len );
    }
    if ( single.entry[ ADEID_DFORK ].ade_off == 0 ) {
	fh->forklen[ DATA ] = 0;
    } else {
	fh->forklen[ DATA ] = htonl( single.entry[ ADEID_DFORK ].ade_len );
	pos = lseek( single.filed, single.entry[ ADEID_DFORK ].ade_off, SEEK_SET );
    }

    return( 0 );
}

/*
 * single_header_test is called from single_open.  It checks certain
 * values of the file and determines if the file is an AppleSingle version
 * one file something else, and returns a one, or negative one to indicate
 * file type.
 *
 * The Magic Number of the file, the first four bytes, must be hex
 * 0x00051600.  Bytes 4 through 7 are the version number and must be hex
 * 0x00010000.  Bytes 8 through 23 identify the home file system, and we
 * are only interested in files from Macs.  Therefore these bytes must
 * contain hex 0x4d6163696e746f736820202020202020 which is ASCII
 * "Macintosh       " (that is seven blanks of padding).
 */
#define MACINTOSH	"Macintosh       "
static u_char		sixteennulls[] = { 0, 0, 0, 0, 0, 0, 0, 0,
				    0, 0, 0, 0, 0, 0, 0, 0 };

int single_header_test(void)
{
    ssize_t		cc;
    u_int32_t		templong;

    cc = read( single.filed, (char *)header_buf, sizeof( header_buf ));
    if ( cc < (ssize_t)sizeof( header_buf )) {
	perror( "Premature end of file :" );
	return( -1 );
    }

    memcpy( &templong, header_buf, sizeof( templong ));
    if ( ntohl( templong ) != AD_APPLESINGLE_MAGIC ) {
	fprintf( stderr, "%s is not an AppleSingle file.\n", single.path );
	return( -1 );
    }

    memcpy(&templong,  header_buf +  4, sizeof( templong ));
    templong = ntohl( templong );
    if ( templong == AD_VERSION1 ) {
	cc = 1;
	if ( memcmp( MACINTOSH, header_buf + 8, sizeof( MACINTOSH ) - 1 ) 
		!= 0 ) {
	    fprintf( stderr, "%s is not a Macintosh AppleSingle file.\n", 
		    single.path );
	    return( -1 );
	}
    } else if ( templong == AD_VERSION2 ) {
	cc = 2;
	if ( memcmp( sixteennulls, header_buf + 8, sizeof( sixteennulls ))
		!= 0 ) {
	    fprintf( stderr, 
		    "Warning:  %s may be a corrupt AppleSingle file.\n",
		    single.path );
	    return( -1 );
	}
    } else {
	fprintf( stderr, "%s is a version of AppleSingle I don't understand!\n",
		single.path );
	return( -1 );
    }

    return( cc );
}

/*
 * single_read is called until it returns zero for each fork.  When
 * it returns zero for the first fork, it seeks to the proper place
 * to read in the next, if there is one.  single_read must be called
 * enough times to return zero for each fork and no more.
 *
 */

ssize_t single_read( int fork, char *buffer, size_t length)
{
    u_int32_t		entry_id;
    char		*buf_ptr;
    size_t		readlen;
    ssize_t		cc = 1;
    off_t		pos;

    switch ( fork ) {
	case DATA :
	    entry_id = ADEID_DFORK;
	    break;
	case RESOURCE :
	    entry_id = ADEID_RFORK;
	    break;
	default :
	    return( -1 );
	    break;
    }

    if (single.entry[entry_id].ade_len > 0x7FFFFFFF) {
	fprintf(stderr, "single_read: Trying to read past end of fork!, ade_len == %u\n", single.entry[entry_id].ade_len);
	return -1;
    }
    if ( single.entry[ entry_id ].ade_len == 0 ) {
	if ( fork == DATA ) {
	    pos = lseek( single.filed,
		single.entry[ ADEID_RFORK ].ade_off, SEEK_SET );
	}
	return( 0 );
    }

    if ( single.entry[ entry_id ].ade_len < length ) {
	readlen = single.entry[ entry_id ].ade_len;
    } else {
	readlen = length;
    }

    buf_ptr = buffer;
    while (( readlen > 0 ) && ( cc > 0 )) {
	if (( cc = read( single.filed, buf_ptr, readlen )) > 0 ) {
	    readlen -= cc;
	    buf_ptr += cc;
	}
    }
    if ( cc >= 0 ) {
	cc = buf_ptr - buffer;
	single.entry[ entry_id ].ade_len -= cc;
    }

    return( cc );
}
