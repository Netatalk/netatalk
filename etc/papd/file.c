/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/syslog.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

markline( start, stop, pf )
    char		**start, **stop;
    struct papfile	*pf;
{
    char		*p;

    if ( PF_BUFSIZ( pf ) == 0 && ( pf->pf_state & PF_EOF )) {
	return( 0 );
    }

    /* get a line */
    for ( p = pf->pf_cur; p < pf->pf_end; p++ ) {
	if ( *p == '\n' || *p == '\r' ) {
	    break;
	}
    }
    if ( p >= pf->pf_end ) {
	if ( pf->pf_state & PF_EOF ) {
	    APPEND( pf, "\n", 1 );
	} else {
	    return( -1 );
	}
    }

    *start = pf->pf_cur;
    *stop = p;
    if ( *stop == *start ) {
	return( 1 );			/* don't return len 0 lines */
    } else {
	return( *stop - *start );
    }
}

consumetomark( start, stop, pf )
    char		*start, *stop;
    struct papfile	*pf;
{
    if ( start != pf->pf_cur || pf->pf_cur > stop || stop > pf->pf_end ) {
	abort();
    }

    pf->pf_cur = stop + 1;		/* past the stop char */
    if ( pf->pf_cur > pf->pf_end ) {
	abort();
    }
    if ( pf->pf_cur == pf->pf_end ) {
	pf->pf_cur = pf->pf_end = pf->pf_buf;
    }

    return;
}

morespace( pf, data, len )
    struct papfile	*pf;
    char		*data;
    int			len;
{
    char		*nbuf;
    int			nsize;

    if ( pf->pf_cur != pf->pf_buf ) {			/* pull up */
	bcopy( pf->pf_cur, pf->pf_buf, PF_BUFSIZ( pf ));
	pf->pf_end = pf->pf_buf + PF_BUFSIZ( pf );
	pf->pf_cur = pf->pf_buf;
    }

    if ( pf->pf_end + len > pf->pf_buf + pf->pf_len ) {	/* make more space */
	nsize = (( pf->pf_len + len ) / PF_MORESPACE +
		(( pf->pf_len + len ) % PF_MORESPACE != 0 )) * PF_MORESPACE;
	if ( pf->pf_buf ) {
	    if (( nbuf = (char *)realloc( pf->pf_buf, nsize )) == 0 ) {
		exit( 1 );
	    }
	} else {
	    if (( nbuf = (char *)malloc( nsize )) == 0 ) {
		exit( 1 );
	    }
	}
	pf->pf_len = nsize;
	pf->pf_end = nbuf + ( pf->pf_end - pf->pf_buf );
	pf->pf_cur = nbuf + ( pf->pf_cur - pf->pf_buf );
	pf->pf_buf = nbuf;
    }

    bcopy( data, pf->pf_end, len );
    pf->pf_end += len;
}

spoolerror( out, str )
    struct papfile	*out;
    char		*str;
{
    char	*pserr1 = "%%[ Error: ";
    char	*pserr2 = " ]%%\n";

    if ( str == NULL ) {
	str = "Spooler error.";
    }

    APPEND( out, pserr1, strlen( pserr1 ));
    APPEND( out, str, strlen( str ));
    APPEND( out, pserr2, strlen( pserr2 ));
}
