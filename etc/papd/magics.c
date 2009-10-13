/*
 * $Id: magics.c,v 1.15 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/logger.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netatalk/at.h>

#include "file.h"
#include "comment.h"
#include "lp.h"

static int state=0;

static void parser_error(struct papfile *outfile)
{
                spoolerror( outfile, "Comments error, Ignoring job." );
		outfile->pf_state |= PF_EOF;
		lp_close();
}

int ps( struct papfile *infile, struct papfile *outfile, struct sockaddr_at *sat)
{
    char			*start;
    int				linelength, crlflength;
    struct papd_comment		*comment;

    for (;;) {
        if ( infile->pf_state & PF_STW ) {
		infile->pf_state &= ~PF_STW;
		/* set up spool file */
		if ( lp_open( outfile, sat ) < 0 && !state) {
		    LOG(log_error, logtype_papd, "lp_open failed" );
		    spoolerror( outfile, "Ignoring job." );
		}
		state = 1;
	}	
	if ( (comment = compeek()) ) {
	    switch( (*comment->c_handler)( infile, outfile, sat )) {
	    case CH_DONE :
		continue;

	    case CH_MORE :
		return( CH_MORE );

	    case CH_ERROR :
	        parser_error(outfile);
		return( 0 );

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

	    case -2:
	        parser_error(outfile);
		return( 0 );

	    case -1 :
		return( 0 );
	    }

	    if ( infile->pf_state & PF_BOT ) {
		if (( comment = commatch( start, start+linelength, magics )) != NULL ) {
		    compush( comment );
		    continue;	/* top of for (;;) */
		}
#if 0
		infile->pf_state &= ~PF_BOT;

		/* set up spool file */
		if ( lp_open( outfile, sat ) < 0 ) {
		    LOG(log_error, logtype_papd, "lp_open failed" );
		    spoolerror( outfile, "Ignoring job." );
		}
#endif
	    }

	    /* write to file */
	    lp_write( infile, start, linelength + crlflength );
	    CONSUME( infile, linelength + crlflength );
	}
    }
}

int cm_psquery( struct papfile *in, struct papfile *out, struct sockaddr_at *sat _U_)
{
    struct papd_comment	*comment;
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

        case -2 :
            return( CH_ERROR );
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

int cm_psadobe( struct papfile *in, struct papfile *out, struct sockaddr_at *sat _U_)
{
    char		*start;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();

    for (;;) {
	switch ( markline( in, &start, &linelength, &crlflength )) {
	case 0 :
	    /* eof on infile */
	    out->pf_state |= PF_EOF;
	    compop();
	    return( CH_DONE );

	case -1 :
	    return( CH_MORE );

        case -2 :
            return( CH_ERROR );
	}
	if ( in->pf_state & PF_BOT ) {
	    in->pf_state &= ~PF_BOT;
#if 0
	    if ( lp_open( out, sat ) < 0 ) {
		LOG(log_error, logtype_papd, "lp_open failed" );
		spoolerror( out, "Ignoring job." );
	    }
#endif
	} else {
	    if (( comment = commatch( start, start + linelength, headers )) != NULL ) {
		compush( comment );
		return( CH_DONE );
	    }
	}

	lp_write( in, start, linelength + crlflength );
	CONSUME( in, linelength + crlflength );
    }
}

char	*Query = "Query";

int cm_psswitch(struct papfile *in, struct papfile *out, struct sockaddr_at *sat _U_)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	/* eof on infile */
	out->pf_state |= PF_EOF;
	compop();
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
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

    if ( (size_t)(stop - p) >= strlen( Query ) &&
	    strncmp( p, Query, strlen( Query )) == 0 ) {
	if ( comswitch( magics, cm_psquery ) < 0 ) {
	    LOG(log_error, logtype_papd, "cm_psswitch: can't find psquery!" );
	    exit( 1 );
	}
    } else {
	if ( comswitch( magics, cm_psadobe ) < 0 ) {
	    LOG(log_error, logtype_papd, "cm_psswitch: can't find psadobe!" );
	    exit( 1 );
	}
    }
    return( CH_DONE );
}


struct papd_comment	magics[] = {
    { "%!PS-Adobe-3.0 Query",	NULL,			cm_psquery, C_FULL },
    { "%!PS-Adobe-3.0",		NULL,			cm_psadobe, C_FULL },
    { "%!PS-Adobe-",		NULL,			cm_psswitch,	0 },
    { NULL,			NULL,			NULL,		0 },
};
