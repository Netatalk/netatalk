/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/syslog.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>

#include "file.h"
#include "comment.h"

ps( infile, outfile )
    struct papfile	*infile, *outfile;
{
    char			*start, *stop;
    struct comment		*comment;

    for (;;) {
	if ( comment = compeek()) {
	    switch( (*comment->c_handler)( infile, outfile )) {
	    case CH_DONE :
		continue;

	    case CH_MORE :
		return( CH_MORE );

	    default :
		return( CH_ERROR );
	    }

	} else {
	    switch ( markline( &start, &stop, infile )) {
	    case 0 :
		/* eof on infile */
		outfile->pf_state |= PF_EOF;
		lp_close();
		return( 0 );

	    case -1 :
		return( 0 );
	    }

	    if ( infile->pf_state & PF_BOT ) {
		if (( comment = commatch( start, stop, magics )) != NULL ) {
		    compush( comment );
		    continue;	/* top of for (;;) */
		}
		infile->pf_state &= ~PF_BOT;

		/* set up spool file */
		if ( lp_open( outfile ) < 0 ) {
		    syslog( LOG_ERR, "lp_open failed" );
		    spoolerror( outfile, "Ignoring job." );
		}
	    }

	    /* write to file */
	    *stop = '\n';
	    lp_write( start, stop - start + 1 );
	    consumetomark( start, stop, infile );
	}
    }
}

cm_psquery( in, out )
    struct papfile	*in, *out;
{
    struct comment	*comment;
    char		*start, *stop;

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    /* eof on infile */
	    out->pf_state |= PF_EOF;
	    compop();
	    return( CH_DONE );

	case -1 :
	    return( CH_MORE );
	}

	if ( in->pf_state & PF_BOT ) {
	    in->pf_state &= ~PF_BOT;
	} else {
	    if (( comment = commatch( start, stop, queries )) != NULL ) {
		compush( comment );
		return( CH_DONE );
	    }
	}

	consumetomark( start, stop, in );
    }
}

cm_psadobe( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( &start, &stop, in )) {
	case 0 :
	    /* eof on infile */
	    out->pf_state |= PF_EOF;
	    compop();
	    return( CH_DONE );

	case -1 :
	    return( CH_MORE );
	}

	if ( in->pf_state & PF_BOT ) {
	    in->pf_state &= ~PF_BOT;
	    if ( lp_open( out ) < 0 ) {
		syslog( LOG_ERR, "lp_open failed" );
		spoolerror( out, "Ignoring job." );
	    }
	} else {
	    if (( comment = commatch( start, stop, headers )) != NULL ) {
		compush( comment );
		return( CH_DONE );
	    }
	}

	*stop = '\n';
	lp_write( start, stop - start + 1 );
	consumetomark( start, stop, in );
    }
}

char	*Query = "Query";

cm_psswitch( in, out )
    struct papfile	*in, *out;
{
    char		*start, *stop, *p;
    struct comment	*comment = compeek();

    switch ( markline( &start, &stop, in )) {
    case 0 :
	/* eof on infile */
	out->pf_state |= PF_EOF;
	compop();
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    for ( p = start; p < stop; p++ ) {
	if ( *p == ' ' || *p == '\t' ) {
	    break;
	}
    }
    for ( ; p < stop; p++ ) {
	if ( *p != ' ' && *p != '\t' ) {
	    break;
	}
    }

    if ( stop - p >= strlen( Query ) &&
	    strncmp( p, Query, strlen( Query )) == 0 ) {
	if ( comswitch( magics, cm_psquery ) < 0 ) {
	    syslog( LOG_ERR, "cm_psswitch: can't find psquery!" );
	    exit( 1 );
	}
    } else {
	if ( comswitch( magics, cm_psadobe ) < 0 ) {
	    syslog( LOG_ERR, "cm_psswitch: can't find psadobe!" );
	    exit( 1 );
	}
    }
    return( CH_DONE );
}

struct comment	magics[] = {
    { "%!PS-Adobe-3.0 Query",	0,			cm_psquery, C_FULL },
    { "%!PS-Adobe-3.0",		0,			cm_psadobe, C_FULL },
    { "%!PS-Adobe-",		0,			cm_psswitch,	0 },
    { 0 },
};
