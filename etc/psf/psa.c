/*
 * $Id: psa.c,v 1.5 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * PostScript Accounting, psa.
 *
 * psa is invoked by psf, as output for a communication program.  The
 * communication program is expected to send a small program before and
 * after each job, which causes the page count to be emitted in a well
 * known format.  psa reads its input, looking for page counts and other
 * interesting data.  Any data that it doesn't understand, it emits to
 * stderr, the lpd log file.  Data that it does understand may be written
 * to a status file or logged.  Once all input has been received, psa
 * subtracts the beginning and end page counts, and log an accounting
 * record in the accounting file.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int ac, char **av)
{
    FILE	*af;
    char	*acc, *user, *host;
    char	buf[ 1024 ], *p, *end;
    int		cc, n, ipc = -1, fpc = -1;

    if ( ac != 4 ) {
	fprintf( stderr, "Usage:\t%s accounting-file user host\n", av[ 0 ] );
	exit( 2 );
    }

    acc = av[ 1 ];
    user = av[ 2 ];
    host = av[ 3 ];

    /*
     * Explain n = !n ...  Is there no beauty in truth?
     */
    while (( cc = read( 0, buf, sizeof( buf ))) > 0 ) {
	if ( ipc < 0 && *buf == '*' ) {
	    /* find initial pagecount */
	    for ( p = buf, end = buf + cc; p < end; p++ ) {
		if ( *p == '\n' || *p == '\r' ) {
		    break;
		}
	    }
	    if ( p == end ) {
		fprintf( stderr, "Can't find initial page count!\n" );
		exit( 2 );
	    }

	    p++;
	    ipc = atoi( buf + 1 );
	    cc -= ( p - buf );
	    if ( cc != 0 ) {
		bcopy( p, buf, cc );
	    }
	} else {
	    /* find final pagecount */
	    for ( p = buf + cc - 1; p >= buf; p-- ) {
		if ( *p != '\n' && *p != '\r' ) {
		    break;
		}
	    }
	    if ( p < buf ) {
		fprintf( stderr, "Can't find final page count!\n" );
		exit( 2 );
	    }

	    for ( ; p >= buf; p-- ) {
		if ( *p == '\n' || *p == '\r' ) {
		    break;
		}
	    }

	    if ( p < buf ) {
		p = buf;
	    } else {
		cc -= p - buf;
		p++;
	    }

	    if ( *p == '*' ) {
		n = atoi( p + 1 );
#define max(x,y)	((x)>(y)?(x):(y))
		fpc = max( n, fpc );
	    }
	}
	if ( cc != 0 && write( 2, buf, cc ) != cc ) {
	    fprintf( stderr, "write 1: 2 %p %d\n", buf, cc );
	    perror( "write" );
	    exit( 2 );
	}
    }
    if ( cc < 0 ) {
	perror( "read" );
	exit( 2 );
    }

    if ( ipc < 0 ) {
	fprintf( stderr, "Didn't find initial page count!\n" );
	exit( 2 );
    }

    if ( fpc < 0 ) {
	fprintf( stderr, "Didn't find final page count!\n" );
	exit( 2 );
    }

    /*
     * Write accounting record.
     */
    if (( af = fopen( acc, "a" )) != NULL ) {
	fprintf( af, "%7.2f\t%s:%s\n", (float)( fpc - ipc ), host, user );
    } else {
	perror( acc );
	exit( 2 );
    }

    exit( 0 );
}
