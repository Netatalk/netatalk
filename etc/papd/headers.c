/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/syslog.h>
#include <sys/param.h>
#include <stdio.h>

#include "file.h"
#include "comment.h"

ch_title( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p, *q, c;
    struct comment	*comment = compeek();

    switch ( markline( &start, &stop, in )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    for ( p = start; p < stop; p++ ) {
	if ( *p == ':' ) {
	    break;
	}
    }

    for ( ; p < stop; p++ ) {
	if ( *p == '(' ) {
	    break;
	}
    }

    for ( q = p; q < stop; q++ ) {
	if ( *q == ')' ) {
	    break;
	}
    }

    if ( q < stop && p < stop ) {
	p++;
	c = *q;
	*q = '\0';
	lp_job( p );
	*q = c;
    }

    *stop = '\n';
    lp_write( start, stop - start + 1 );
    compop();
    consumetomark( start, stop, in );
    return( CH_DONE );
}

/*
 * "Header" comments.
 */
struct comment	headers[] = {
    { "%%Title:",			0,		ch_title,	0 },
    { 0 },
};
