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

ps( infile, outfile, sat )
    struct papfile	*infile, *outfile;
    struct sockaddr_at	*sat;
{
    char			*start;
    int				linelength, crlflength;
    struct comment		*comment;

    for (;;) {
	if ( comment = compeek()) {
	    switch( (*comment->c_handler)( infile, outfile, sat )) {
	    case CH_DONE :
		continue;

	    case CH_MORE :
		return( CH_MORE );

	    default :
		return( CH_ERROR );
	    }

	} else {
	    switch ( markline( infile, &start, &linelength, &crlflength )) {
	    case 0 :
		/* eof on infile */
		outfile->pf_state |= PF_EOF;
		lp_close();
		return( 0 );

	    case -1 :
		return( 0 );
	    }

	    if ( infile->pf_state & PF_BOT ) {
		if (( comment = commatch( start, start+linelength, magics )) != NULL ) {
		    compush( comment );
		    continue;	/* top of for (;;) */
		}
		infile->pf_state &= ~PF_BOT;

		/* set up spool file */
		if ( lp_open( outfile, sat ) < 0 ) {
		    syslog( LOG_ERR, "lp_open failed" );
		    spoolerror( outfile, "Ignoring job." );
		}
	    }

	    /* write to file */
	    lp_write( start, linelength + crlflength );
	    CONSUME( infile, linelength + crlflength );
	}
    }
}

cm_psquery( in, out, sat )
    struct papfile	*in, *out;
    struct sockaddr_at	*sat;
{
    struct comment	*comment;
    char		*start;
    int			linelength, crlflength;

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
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
	    if (( comment = commatch( start, start+linelength, queries )) != NULL ) {
		compush( comment );
		return( CH_DONE );
	    }
	}

	CONSUME( in, linelength + crlflength );
    }
}

cm_psadobe( in, out, sat )
    struct papfile	*in, *out;
    struct sockaddr_at	*sat;
{
    char		*start;
    int			linelength, crlflength;
    struct comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
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
	    if ( lp_open( out, sat ) < 0 ) {
		syslog( LOG_ERR, "lp_open failed" );
		spoolerror( out, "Ignoring job." );
	    }
	} else {
	    if (( comment = commatch( start, start + linelength, headers )) != NULL ) {
		compush( comment );
		return( CH_DONE );
	    }
	}

	lp_write( start, linelength + crlflength );
	CONSUME( in, linelength + crlflength );
    }
}

char	*Query = "Query";

cm_psswitch( in, out, sat )
    struct papfile	*in, *out;
    struct sockaddr_at	*sat;
{
    char		*start, *stop, *p;
    int			linelength, crlflength;
    struct comment	*comment = compeek();

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	/* eof on infile */
	out->pf_state |= PF_EOF;
	compop();
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    stop = start + linelength;
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
