/*
 * $Id: showppd.c,v 1.7 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netatalk/at.h>
#include <atalk/atp.h>

#include "ppd.h"

extern struct ppd_font		*ppd_fonts;

extern struct ppd_feature	ppd_features[];


int main(int ac, char **av)
{
    struct ppd_feature	*pfe;
    struct ppd_font	*pfo;

    if ( ac != 2 ) {
	fprintf( stderr, "Usage:\t%s ppdfile\n", av[ 0 ] );
	exit( 1 );
    }

    read_ppd( av[ 1 ], 0 );
    for ( pfo = ppd_fonts; pfo; pfo = pfo->pd_next ) {
	printf( "Font: %s\n", pfo->pd_font );
    }
    for ( pfe = ppd_features; pfe->pd_name; pfe++ ) {
	printf( "Feature: %s %s\n", pfe->pd_name, (pfe->pd_value)?pfe->pd_value:"NULL" );
    }

    exit ( 0 );
}
