/*
 * $Id: headers.c,v 1.10 2005-04-28 20:49:49 bfernhomberg Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h" 
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <string.h>
#include <stdio.h>

#include <netatalk/at.h>
#include <atalk/logger.h>

#include "file.h"
#include "comment.h"
#include "lp.h"

int ch_title( struct papfile *, struct papfile * );
int ch_for( struct papfile *, struct papfile * );


int ch_for( in, out )
	struct papfile	*in, *out _U_;
{
    char                *start, *stop, *p, *q, c;
    int                 linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );
    }

    stop = start + linelength;
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
	lp_for ( p );
        *q = c;
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

int ch_title( in, out )
    struct papfile	*in, *out _U_;
{
    char		*start, *stop, *p, *q, c;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

#ifdef DEBUG
    LOG(log_debug, logtype_papd, "Parsing %%Title");
#endif

    stop = start + linelength;
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

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

static int guess_creator ( char *creator )
{
	if (strstr(creator, "LaserWriter"))
		return 1;
	if (strstr(creator, "cgpdftops"))
		return 2;

	return 0;
}


int ch_creator( in, out )
    struct papfile	*in, *out _U_;
{
    char		*start, *stop, *p, *q, c;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    stop = start + linelength;
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
	in->origin = guess_creator ( p );
	lp_origin(in->origin);
	*q = c;
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

int ch_endcomm( in, out )
    struct papfile	*in, *out _U_;
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug, logtype_papd, "End Comment");
#endif
    in->pf_state |= PF_STW;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_starttranslate(in,out)
    struct papfile      *in, *out _U_;
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug, logtype_papd, "Start translate");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_endtranslate(in,out)
    struct papfile      *in, *out _U_;
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug, logtype_papd, "EndTranslate");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );
    }

    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_translateone(in,out)
    struct papfile      *in, *out _U_;
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug, logtype_papd, "TranslateOne");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}




/*
 * "Header" comments.
 */
struct papd_comment	headers[] = {
    { "%%Title:",			NULL,		ch_title,	0 },
    { "%%For:",				NULL,		ch_for,		0 },
    { "%%Creator:",			NULL,		ch_creator,	0 },
    { "%%EndComments",			NULL,		ch_endcomm,	0 },
    { "%%BeginFeature",			NULL,		ch_starttranslate,  0 },
    { "%%EndFeature",			NULL,		ch_endtranslate,  0 },
    { "%%BeginPageSetup",		NULL,		ch_starttranslate, 0 },
    { "%%EndPageSetup",			NULL,		ch_endtranslate, 0 },
#if 0
    { "%%BeginSetup",			NULL,		ch_translateone,  0 },
    { "%%EndSetup",			NULL,		ch_translateone,  0 },
    { "%%BeginProlog",			NULL,		ch_translateone,  0 },
    { "%%EndProlog",			NULL,		ch_translateone,  0 },
    { "%%Page:",			NULL,		ch_translateone, 0 },
    { "%%PageTrailer",			NULL,		ch_translateone, 0 },
    { "%%Trailer",			NULL,		ch_translateone, 0 },
    { "%%EOF",				NULL,		ch_translateone, 0 },
#endif
    { "%%",				NULL,		ch_translateone, 0 },
    { NULL,				NULL,		NULL,		0 },
};
