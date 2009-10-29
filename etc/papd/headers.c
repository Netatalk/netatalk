/*
 * $Id: headers.c,v 1.14 2009-10-29 13:38:15 didg Exp $
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
#include <stdlib.h>

#include <netatalk/at.h>
#include <atalk/logger.h>

#include "file.h"
#include "comment.h"
#include "lp.h"

int ch_title( struct papfile *, struct papfile * );
int ch_for( struct papfile *, struct papfile * );

static char *get_text(char *start, int linelength)
{
    char *p, *q;
    char *t, *ret;
    char *stop;
    
    /* 1023 is arbitrary 255 max for comment but some may be escape \xxx and space and keyword */

    if (linelength > 1023)
        return NULL;

    t = ret = calloc(1, linelength +1);

    if (!ret)
        return NULL;

    stop = start + linelength;
    for ( p = start; p < stop; p++ ) {
        if ( *p == ':' ) {
            p++;
            break;
        }
    }
    
    for ( ; p < stop; p++ ) {
        if (*p != ' ' && *p != '\t') {
            break;
        }
    }

    if ( p < stop && *p == '(' ) {
        int count;
        /* start with ( then it's a <text> */ 
        p++;
        for ( q = p, count = 1; q < stop; q++, t++ ) {
            if (*q == '(') {
              count++;
            }
            else if ( *q == ')' ) {
                count--;
                if (!count) {
                    break;
                }
            }
            *t = *q;
        }
    }
    else {
        /* it's a textline */
        for ( q = p; q < stop; q++, t++ ) {
            *t = *q;
        }
    }
    return ret;
}

int ch_for( struct papfile *in, struct papfile *out _U_)
{
    char                *start, *cmt;
    int                 linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    cmt = get_text(start, linelength);

    if ( cmt ) {
	lp_for ( cmt );
	free(cmt);
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

int ch_title( struct papfile *in, struct papfile *out _U_)
{
    char		*start, *cmt;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "Parsing %%Title");
#endif

    cmt = get_text(start, linelength);

    if ( cmt ) {
	lp_job( cmt );
	free(cmt);
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


int ch_creator( struct papfile *in, struct papfile *out _U_)
{
    char		*start, *cmt;
    int			linelength, crlflength;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    cmt = get_text(start, linelength);

    if ( cmt ) {
	in->origin = guess_creator ( cmt );
	free(cmt);
	lp_origin(in->origin);
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return( CH_DONE );
}

int ch_endcomm( struct papfile *in, struct papfile *out _U_)
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "End Comment");
#endif
    in->pf_state |= PF_STW;

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
	return( 0 );

    case -1 :
	return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_starttranslate( struct papfile *in, struct papfile *out _U_)
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "Start translate");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write( in, start, linelength + crlflength );
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_endtranslate(struct papfile *in, struct papfile *out _U_)
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "EndTranslate");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );

    case -2 :
        return( CH_ERROR );
    }

    lp_write( in, start, linelength + crlflength );
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME( in, linelength + crlflength );
    return ( CH_DONE);
}

int ch_translateone( struct papfile *in, struct papfile *out _U_)
{
    char                *start;
    int                 linelength, crlflength;

#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "TranslateOne");
#endif

    switch ( markline( in, &start, &linelength, &crlflength )) {
    case 0 :
        return( 0 );

    case -1 :
        return( CH_MORE );

    case -2 :
        return( CH_ERROR );
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
