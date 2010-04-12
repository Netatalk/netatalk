/*
 * $Id: psorder.c,v 1.10 2010-04-12 14:28:47 franklahm Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include "pa.h"
#include "psorder.h"

#include <atalk/paths.h>

/*
 *			Global Variables
 */

static u_char			psbuf[ 8192 ];
static struct psinfo_st		psinfo;
static int			orderflag, forceflag;

static void
filecleanup( int errorcode, int tfd, char *tfile)
{

/*
	Close and unlink the temporary file.
 */

    if ( tfd != 0 ) {
	if ( close( tfd ) != 0 ) {
	    perror( tfile );
	    exit( errorcode );
	}
	if ( unlink( tfile ) != 0 ) {
	    perror( tfile );
	    exit( errorcode );
	}
    }

    exit( errorcode );
}

static void
filesetup( char *inputfile, int *infd, char *tfile, int *tfd)
{
    struct stat		st;
    char		*template = _PATH_TMPPAGEORDER;

    if ( strcmp( inputfile, STDIN ) != 0 ) {
	if ( stat( inputfile, &st ) < 0 ) {
	    perror( inputfile );
	    filecleanup( -1, -1, "" );
	}
	if ( st.st_mode & S_IFMT & S_IFDIR ) {
	    fprintf( stderr, "%s is a directory.\n", inputfile );
	    filecleanup( 0, -1, "" );
	}
	if (( *infd = open( inputfile, O_RDONLY, 0600 )) < 0 ) {
	    perror( inputfile );
	    filecleanup( -1, -1, "" );
	}
    } else {
	*infd = 0;
    }

#if DEBUG
    fprintf( stderr, "Input file or stdin and stdout opened.\n" );
    fprintf( stderr, "Input file descriptor is %d .\n", *infd );
#endif /* DEBUG */

/*
	make temporary file
 */

#if defined(NAME_MAX)
    (void *)strncpy( tfile, template, NAME_MAX );
#else
    (void *)strncpy( tfile, template, MAXNAMLEN );
#endif
    if (( *tfd = mkstemp( tfile )) == -1 ) {
	fprintf( stderr, "can't create temporary file %s\n", tfile );
	filecleanup( -1, -1, "" );
    }

#if DEBUG
    fprintf( stderr, "Temporary file %s created and opened.\n", tfile );
    fprintf( stderr, "Temporary file descriptor is %d .\n", *tfd );
#endif /* DEBUG */

    psinfo.firstpage = NULL;
    psinfo.lastpage = NULL;
    psinfo.trailer = 0;
    psinfo.pages.offset = 0;
    psinfo.pages.end = 0;
    psinfo.pages.num[0] = '\0';
    psinfo.pages.order[0] = '\0';

    return;
}

static struct pspage_st
*getpspage(off_t off)
{
    struct pspage_st	*newpspage;

    newpspage = (struct pspage_st *)malloc( sizeof( struct pspage_st )); 
    if ( newpspage != NULL ) {
	newpspage->offset = off;
	newpspage->nextpage = NULL;
	*newpspage->lable = '\0';
	*newpspage->ord = '\0';
    }
    return( newpspage );
}

static int
handletok(off_t count, char *token)
{
    int			incdoc = 0;
    struct pspage_st	*newpage;
    char		*tmp;

    if (( strncmp( PENDDOC, token, strlen( PENDDOC )) == 0 ) && incdoc ) {
	incdoc--;
#if DEBUG
	fprintf( stderr, "found an EndDoc\n" );
#endif /* DEBUG */

    } else if ( strncmp( PBEGINDOC, token, strlen( PBEGINDOC )) == 0 ) {
	incdoc++;
#if DEBUG
	fprintf( stderr, "found a BeginDoc\n" );
#endif /* DEBUG */

    } else if ( !incdoc && 
	    ( strncmp( PPAGE, token, strlen( PPAGE )) == 0 )) {
#if DEBUG
	fprintf( stderr, "found a Page\n" );
#endif /* DEBUG */
	if (( newpage = getpspage( count )) == NULL ) {
	    return( -1 );
	}
	if ( psinfo.firstpage == NULL ) {
	    newpage->prevpage = NULL;
	    psinfo.firstpage = newpage;
	} else {
	    newpage->prevpage = psinfo.lastpage;
	    psinfo.lastpage->nextpage = newpage;
	}
	psinfo.lastpage = newpage;
	while ( *token++ != ':' );
	if (( tmp = strtok( token, WHITESPACE )) != NULL ) {
	    (void)strncpy( newpage->lable, tmp, NUMLEN );
	    if (( tmp = strtok( NULL, WHITESPACE )) != NULL ) {
		(void)strncpy( newpage->ord, tmp, ORDLEN );
	    }
	}
#if DEBUG
	fprintf( stderr, "page lable %s, page ord %s\n", newpage->lable,
		newpage->ord );
#endif /* DEBUG */

    } else if ( !incdoc && 
	    ( strncmp( PPAGES, token, strlen( PPAGES )) == 0 )) {
#if DEBUG
	fprintf( stderr, "found a Pages\n" );
#endif /* DEBUG */
	psinfo.pages.offset = count;
	psinfo.pages.end = strlen( token ) + count;
	while ( *token++ != ':' );
	while ( isspace( *token )) token++;
	if ( strncmp( ATEND, token, strlen( ATEND )) == 0 ) {
#if DEBUG
	    fprintf( stderr, "it is a Pages: (atend)\n" );
#endif /* DEBUG */
	    psinfo.pages.offset = 0;
	    psinfo.pages.end = 0;
	} else {
	    if (( tmp = strtok( token, WHITESPACE )) != NULL ) {
		(void)strncpy( psinfo.pages.num, tmp, NUMLEN );
		if (( tmp = strtok( NULL, WHITESPACE )) != NULL ) {
		    (void)strncpy( psinfo.pages.order, tmp, ORDERLEN );
		}
	    }
#if DEBUG
	    fprintf( stderr, "number of pages %s\n", psinfo.pages.num );
	    fprintf( stderr, "order control number %s\n", psinfo.pages.order );
#endif /* DEBUG */
	}

    } else if ( !incdoc && 
	    ( strncmp( PTRAILER, token, strlen( PTRAILER )) == 0 )) {
#if DEBUG
	fprintf( stderr, "found the Trailer\n" );
#endif /* DEBUG */
	if  ( psinfo.trailer == 0 ) {
	    psinfo.trailer = count;
	}
    }

    return( 0 );
}

static void
readps(int inputfd, int tempfd, char *tempfile)
{
    off_t		ccread = 0;
    off_t		ccmatch;
    char		*curtok = NULL;
    FILE		*tempstream;
    pa_buf_t		*pb;
    int			n;
    char		c = -1;
    char		pc, cc = 0;

    pb = pa_init( inputfd );
    if (( tempstream = fdopen( tempfd, "w" )) == NULL ) {
	perror( "fdopen fails for tempfile" );
	filecleanup( -1, tempfd, tempfile );
    }

    if (( c = pa_getchar( pb )) != 0 ) {
	ccread++;
	(void)putc( c, tempstream );
	pc = 0;
	cc = c;
	pa_match( pb );
	n = strlen( PPSADOBE );
	for ( ; ( n > 0 ) && (( c = pa_getchar( pb )) != 0 ) ; n-- ) {
	    ccread++ ;
	    (void)putc( c, tempstream );
	    pc = cc;
	    cc = c;
	}
	curtok = pa_gettok( pb );
    }
#if DEBUG
    fprintf( stderr, "%s\n", curtok );
#endif /* DEBUG */

/*
 * not postscript
 */
    if ( strcmp( curtok, PPSADOBE ) != 0 ) {
#if DEBUG
    fprintf( stderr, "in the not postscript section of readps\n" );
#endif /* DEBUG */
	while (( c = pa_getchar( pb )) != 0 ) {
	    ccread++;
	    (void)putc( c, tempstream );
	    pc = cc;
	    cc = c;
	}

	(void)fflush( tempstream );
	return;
    }

/*
 * postscript
 */
#if DEBUG
    fprintf( stderr, "in the postscript section of readps\n" );
#endif /* DEBUG */
    while (( c = pa_getchar( pb )) != 0 ) {
	ccread++;
	(void)putc( c, tempstream );
	pc = cc;
	cc = c;
	if ((( pc == '\r' ) || ( pc == '\n' )) && ( cc == '%' )) {
#if DEBUG
	    fprintf( stderr, "supposed start of match, cc = %c\n", cc );
#endif /* DEBUG */
	    pa_match( pb );
	    ccmatch = ccread - 1;
	    while ( ( c = pa_getchar( pb ) ) ) {
		if ( c != 0 ) {
		    ccread++;
		    (void)putc( c, tempstream );
		    pc = cc;
		    cc = c;
		}
		if (( c == '\r' ) || ( c == '\n' ) || ( cc == '\0' )) {
		    curtok = pa_gettok( pb );
#if DEBUG
		    fprintf( stderr, "%s\n", curtok );
#endif /* DEBUG */
		    if ( handletok( ccmatch, curtok ) < 0 ) {
			perror( "malloc died" );
			filecleanup( -1, tempfd, tempfile );
		    }
		    break;
		}
		if ( c == 0 ) break;
	    }
	    if ( c == 0 ) break;
	}
    }

    (void)fflush( tempstream );
    return;
}

static void
temp2out(int tempfd, char *tempfile, off_t length)
{
    int			ccread;
    int			ccwrite;
    int			size;

    while ( length > 0 ) {
	if ( length > sizeof( psbuf )) {
	    size = sizeof( psbuf );
	} else size = length;
	if (( ccread = read( tempfd, psbuf, size )) > 0 ) {
	    size = ccread;
	    while ( ccread > 0 ) {
		ccwrite = write( 1, psbuf, ccread );
		if ( ccwrite < 0 ) {
		    perror( "stdout" );
		    filecleanup( ccwrite, tempfd, tempfile );
		} else {
		    ccread -= ccwrite;
		}
	    }
	}
	if ( ccread < 0 ) {
	    perror( "temporary file" );
	    filecleanup( ccread, tempfd, tempfile );
	}
	length -= size;
    }
}

static void
writelable(int tempfd, char *tempfile, char *lable)
{
    char		line[256];
    int			ccwrite;
    int			linelen;
    char		*argone;
    char		*argtwo;

    if ( strcmp( lable, PPAGES ) == 0 ) {
	argone = psinfo.pages.num;
	argtwo = psinfo.pages.order;
    } else {
	argone = argtwo = NULL;
    }
    (void)sprintf( line, "%s %s %s", lable, argone, argtwo );
    linelen = strlen( line );

    ccwrite = write( 1, line, linelen );
    if ( ccwrite < 0 ) {
	perror( "stdout" );
	filecleanup( ccwrite, tempfd, tempfile );
    } else {
	linelen -= ccwrite;
    }
}

static void
writeps(int tempfd, char *tempfile)
{
    struct stat		st;
    off_t		endofpage;
    int			order;

    if ( stat( tempfile, &st ) < 0 ) {
	perror( "stat failed" );
	filecleanup( -1, tempfd, tempfile );
    }
    if ( psinfo.trailer == 0 ) {
	endofpage = st.st_size;
    } else endofpage = psinfo.trailer;

    if (( psinfo.firstpage == NULL ) || 
	    ( psinfo.firstpage == psinfo.lastpage )) {
	order = FORWARD;
    } else if ( psinfo.pages.offset == 0 ) {
	order = orderflag;
    } else if (( strncmp( psinfo.pages.order, "", ORDERLEN ) == 0 ) ||
	    ( strncmp( psinfo.pages.order, "1", ORDERLEN ) == 0 )) {
	order = orderflag;
	if ( order == REVERSE ) strcpy( psinfo.pages.order, "-1" );
    } else if ( strncmp( psinfo.pages.order, "-1", ORDERLEN ) == 0 ) {
	if ( orderflag == FORWARD ) {
	    order = REVERSE;
	    strcpy( psinfo.pages.order, "1" );
	} else order = FORWARD;
    } else if (( strncmp( psinfo.pages.order, "0", ORDERLEN ) == 0 ) &&
	    forceflag ) {
	order = orderflag;
    } else order = FORWARD;

    if ( order == FORWARD ) {
	temp2out( tempfd, tempfile, st.st_size );
    } else {
/*
 *	output the header stuff and rewrite the $$Pages line
 *	if it is in the header and not %%Pages: (atend)
 */
	if ( psinfo.firstpage->offset > 0 ) {
	    if (( psinfo.firstpage->offset > psinfo.pages.offset ) &&
		    ( psinfo.pages.offset != 0 )) {
		temp2out( tempfd, tempfile, psinfo.pages.offset );
		writelable( tempfd, tempfile, PPAGES );
		if ( lseek( tempfd, psinfo.pages.end, SEEK_SET ) < 0 ) {
		    perror( tempfile );
		    filecleanup( -1, tempfd, tempfile );
		}
		temp2out( tempfd, tempfile, 
			psinfo.firstpage->offset - psinfo.pages.end );
	    } else temp2out( tempfd, tempfile, psinfo.firstpage->offset );
	}
/*
 *	output the pages, last to first
 */
	while ( psinfo.lastpage != NULL ) {
	    if ( lseek( tempfd, psinfo.lastpage->offset, SEEK_SET ) < 0 ) {
		perror( tempfile );
		filecleanup( -1, tempfd, tempfile );
	    }
	    temp2out( tempfd, tempfile, endofpage - psinfo.lastpage->offset );
	    endofpage = psinfo.lastpage->offset;
	    psinfo.lastpage = psinfo.lastpage->prevpage;
	    if ( psinfo.lastpage != NULL ) {
		(void)free( psinfo.lastpage->nextpage );
		psinfo.lastpage->nextpage = NULL;
	    }
	}
/*
 *	output the trailer stuff and rewrite the $$Pages line
 *	if it is in the trailer
 */
	if ( psinfo.trailer != 0 ) {
	    if ( lseek( tempfd, psinfo.trailer, SEEK_SET ) < 0 ) {
		perror( tempfile );
		filecleanup( -1, tempfd, tempfile );
	    }
	    if ( psinfo.trailer < psinfo.pages.offset ) {
		temp2out( tempfd, tempfile,
			psinfo.pages.offset - psinfo.trailer );
		writelable( tempfd, tempfile, PPAGES );
		if ( lseek( tempfd, psinfo.pages.end, SEEK_SET ) < 0 ) {
		    perror( tempfile );
		    filecleanup( -1, tempfd, tempfile );
		}
		temp2out( tempfd, tempfile, st.st_size - psinfo.pages.end );
	    } else temp2out( tempfd, tempfile, st.st_size - psinfo.trailer );
	}
    }

    return;
}

static int
psorder(char *path)
{
    int			tempfd;
    int			inputfd;
#if defined(NAME_MAX)
    char		tempfile[NAME_MAX];
#else
    char		tempfile[MAXNAMLEN];
#endif

    filesetup( path, &inputfd, tempfile, &tempfd );
    readps( inputfd, tempfd, tempfile );
    if ( lseek( tempfd, REWIND, SEEK_SET ) < 0 ) {
	perror( tempfile );
	filecleanup( -1, tempfd, tempfile );
    }
    writeps( tempfd, tempfile );
    filecleanup( 0, tempfd, tempfile );
    return( 0 );
}

int main(int argc, char **argv)
{
    extern int	optind;
    char	*progname;
    int		errflag = 0;
    int		c;

    while (( c = getopt( argc, argv, OPTSTR )) != -1 ) {
	switch ( c ) {
	case REVCHAR:
	    if ( orderflag ) errflag++;
	    else orderflag = REVERSE;
	    break;
	case FORWCHAR:
	    if ( orderflag ) errflag++;
	    else orderflag = FORWARD;
	    break;
	case FORCECHAR:
	    if ( forceflag ) errflag++;
	    else forceflag++;
	    break;
	}
    }
    if ( errflag ) {
	if (( progname = strrchr( argv[ 0 ], '/' )) == NULL ) {
	    progname = argv[ 0 ];
	} else progname++;
	fprintf( stderr, "usage: %s [-duf] [sourcefile]\n", progname );
	return( -1 );
    } else if ( !orderflag ) orderflag = FORWARD;

    if ( optind >= argc ) {
	return( psorder( STDIN ));
    }
    return( psorder( argv[ optind ] ));
}

