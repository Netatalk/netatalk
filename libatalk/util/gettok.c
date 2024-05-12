/*
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <pwd.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/globals.h>

static char	*l_curr;
static char	*l_end;

void initline( int len, char *line)
{
    l_curr = line;
    l_end = line + len;
}

#define ST_QUOTE	0
#define ST_WORD		1
#define ST_BEGIN	2

int
parseline(int len, char *token)
{
    char	*p, *e;
    int		state;

    state = ST_BEGIN;
    p = token;
    e = token + len;

    for (;;) {
        if ( l_curr > l_end ) {			/* end of line */
            *token = '\0';
            return( -1 );
        }

        switch ( *l_curr ) {
        case '"' :
            if ( state == ST_QUOTE ) {
                state = ST_WORD;
            } else {
                state = ST_QUOTE;
            }
            break;

        case '\0' :
        case '\t' :
        case '\n' :
        case ' ' :
            if ( state == ST_WORD ) {
                *p = '\0';
                return( p - token );
            }
            if ( state != ST_QUOTE ) {
                break;
            }
            /* FALL THROUGH */

        default :
            if ( state == ST_BEGIN ) {
                state = ST_WORD;
            }
            if ( p > e ) {			/* end of token */
                *token = '\0';
                return( -1 );
            }
            *p++ = *l_curr;
            break;
        }

        l_curr++;
    }
}
