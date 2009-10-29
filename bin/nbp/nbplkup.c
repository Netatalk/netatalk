/*
 * $Id: nbplkup.c,v 1.9 2009-10-29 11:35:57 didg Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/nbp.h>
#include <atalk/util.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include <atalk/unicode.h>

static char *Obj = "=";
static char *Type = "=";
static char *Zone = "*";

static void Usage(char *av0)
{
    char	*p;

    if (( p = strrchr( av0, '/' )) == NULL ) {
	p = av0;
    } else {
	p++;
    }

    printf( "Usage:\t%s [ -A address ] [ -r responses] [-m Mac charset] [ obj:type@zone ]\n", p );
    exit( 1 );
}

int main(int ac, char **av)
{
    struct nbpnve	*nn;
    char		*name;
    int			i, c, nresp = 1000;
    struct at_addr      addr;
    char		*obj = NULL;
    size_t		obj_len;
    charset_t		chMac = CH_MAC;
    char *		convname;

    extern char		*optarg;
    extern int		optind;

    memset(&addr, 0, sizeof(addr));
    while (( c = getopt( ac, av, "r:A:m:" )) != EOF ) {
	switch ( c ) {
	case 'A':
  	    if (!atalk_aton(optarg, &addr)) {
	        fprintf(stderr, "Bad address.\n");
		exit(1);
	    }
	    break;
	case 'r' :
	    nresp = atoi( optarg );
	    break;
        case 'm':
            if ((charset_t)-1 == (chMac = add_charset(optarg)) ) {
	        fprintf(stderr, "Invalid Mac charset.\n");
		exit(1);
	    }
            break;

	default :
	    Usage( av[ 0 ] );
	    exit( 1 );
	}
    }

    if (( nn = (struct nbpnve *)malloc( nresp * sizeof( struct nbpnve )))
	    == NULL ) {
	perror( "malloc" );
	exit( 1 );
    }

    if ( ac - optind > 1 ) {
	Usage( av[ 0 ] );
	exit( 1 );
    }

    /*
     * Get default values from the environment. We need to copy out
     * the results, here, since nbp_name returns it's parameters
     * in static space, and we'll clobber them when we call it again
     * later.
     */
    if (( name = getenv( "NBPLKUP" )) != NULL ) {
	if ( nbp_name( name, &Obj, &Type, &Zone )) {
	    fprintf( stderr,
		    "Environment variable syntax error: NBPLKUP = %s\n",
		    name );
	    exit( 1 );
	}

	if (( name = (char *)malloc( strlen( Obj ) + 1 )) == NULL ) {
	    perror( "malloc" );
	    exit( 1 );
	}
	strcpy( name, Obj );
	Obj = name;

	if (( name = (char *)malloc( strlen( Type ) + 1 )) == NULL ) {
	    perror( "malloc" );
	    exit( 1 );
	}
	strcpy( name, Type );
	Type = name;

	if (( name = (char *)malloc( strlen( Zone ) + 1 )) == NULL ) {
	    perror( "malloc" );
	    exit( 1 );
	}
	strcpy( name, Zone );
	Zone = name;

    }

    if ( ac - optind == 1 ) {
	if ((size_t)(-1) == convert_string_allocate( CH_UNIX, chMac,
                           av[ optind ], -1, &convname))
            convname = av[ optind ];

	if ( nbp_name( convname, &Obj, &Type, &Zone )) {
	    Usage( av[ 0 ] );
	    exit( 1 );
	}
    }

    if (( c = nbp_lookup( Obj, Type, Zone, nn, nresp, &addr)) < 0 ) {
	perror( "nbp_lookup" );
	exit( -1 );
    }
    for ( i = 0; i < c; i++ ) {
	
	if ((size_t)(-1) == (obj_len = convert_string_allocate( chMac, 
                       CH_UNIX, nn[ i ].nn_obj, nn[ i ].nn_objlen, &obj)) ) {
            obj_len = nn[ i ].nn_objlen;
            if (( obj = strdup(nn[ i ].nn_obj)) == NULL ) {
	        perror( "strdup" );
	        exit( 1 );
	    }
        }

	printf( "%31.*s:%-34.*s %u.%u:%u\n",
		(int)obj_len, obj,
		nn[ i ].nn_typelen, nn[ i ].nn_type,
		ntohs( nn[ i ].nn_sat.sat_addr.s_net ),
		nn[ i ].nn_sat.sat_addr.s_node,
		nn[ i ].nn_sat.sat_port );

	free(obj);
    }

    free(nn);
    return 0;
}
