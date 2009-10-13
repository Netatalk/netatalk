/*
 * $Id: comment.c,v 1.10 2009-10-13 22:55:37 didg Exp $
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
#include <errno.h>

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

void compush(struct papd_comment *comment)
{
    struct comstate	*cs;

    if (( cs = (struct comstate *)malloc( sizeof( struct comstate ))) ==
	    NULL ) {
	LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
	exit( 1 );
    }

    cs->cs_comment = comment;
    cs->cs_prev = comstate;
    cs->cs_flags = 0;
    comstate = cs;
}

int comswitch(struct papd_comment *comments, int (*handler)())
{
    struct papd_comment	*c, *comment = NULL;

    for ( c = comments; c->c_begin; c++ ) {
	if ( c->c_handler == handler ) {
	    comment = c;
	}
    }
    if ( comment == NULL || comment->c_handler != handler ) {
	LOG(log_error, logtype_papd, "comswitch: can't find handler!" );
	return( -1 );
    }
    compop();
    compush( comment );
    return( 0 );
}

int comcmp( char *start, char *stop, char *str,int how)
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

struct papd_comment *commatch( char *start, char *stop, struct papd_comment comments[])
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

char *comtoken( char *start, char *stop, char *pos, char *delim)
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
