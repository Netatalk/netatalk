/*
 * $Id: comment.c,v 1.7 2002-01-04 04:45:47 sibaz Exp $
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
#include <stdlib.h>
#include <string.h>

#include "comment.h"

struct comstate	*comstate;

char	*comcont = "%%+";

void compop( void )
{
    struct comstate	*cs;

    cs = comstate;
    comstate = cs->cs_prev;
    free( cs );
}

void compush( comment )
    struct papd_comment	*comment;
{
    struct comstate	*cs;

    if (( cs = (struct comstate *)malloc( sizeof( struct comstate ))) ==
	    NULL ) {
	LOG(log_error, logtype_default, "malloc: %m" );
	exit( 1 );
    }

    cs->cs_comment = comment;
    cs->cs_prev = comstate;
    cs->cs_flags = 0;
    comstate = cs;
}

int comswitch( comments, handler )
    struct papd_comment	*comments;
    int			(*handler)();
{
    struct papd_comment	*c, *comment = NULL;

    for ( c = comments; c->c_begin; c++ ) {
	if ( c->c_handler == handler ) {
	    comment = c;
	}
    }
    if ( comment == NULL || comment->c_handler != handler ) {
	LOG(log_error, logtype_default, "comswitch: can't find handler!" );
	return( -1 );
    }
    compop();
    compush( comment );
    return( 0 );
}

int comcmp( start, stop, str, how )
    char	*start, *stop, *str;
    int		how;
{
    int		cc, len;

    len = stop - start;
    cc = strlen( str );
    if ( how & C_FULL ) {
	if ( (cc == len) && (strncmp( str, start, cc ) == 0) ) {
	    return( 0 );
	}
    } else {
	if ( (cc <= len) && (strncmp( str, start, cc ) == 0) ) {
	    return( 0 );
	}
    }

    return( 1 );
}

struct papd_comment *commatch( start, stop, comments )
    char		*start, *stop;
    struct papd_comment	comments[];
{
    struct papd_comment	*comment;

    for ( comment = comments; comment->c_begin; comment++ ) {
	if ( comcmp( start, stop, comment->c_begin, comment->c_flags ) == 0 ) {
	    break;
	}
    }
    if ( comment->c_begin ) {
	return( comment );
    } else {
	return( NULL );
    }
}

char *comtoken( start, stop, pos, delim )
    char	*start, *stop, *pos, *delim;
{
    if ( pos < start || pos > stop ) {
	abort();
    }

    for ( ; pos < stop; pos++ ) {
	if ( index( delim, *pos )) {
	    break;
	}
    }
    if ( ++pos < stop ) {
	return( pos );
    } else {
	return( NULL );
    }
}
