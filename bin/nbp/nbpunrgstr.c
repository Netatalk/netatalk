/*
 * $Id: nbpunrgstr.c,v 1.4 2001-06-29 14:14:46 rufustfirefly Exp $
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
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/util.h>
#include <atalk/nbp.h>

void Usage( av0 )
    char	*av0;
{
    char	*p;

    if (( p = strrchr( av0, '/' )) == 0 ) {
	p = av0;
    } else {
	p++;
    }

    fprintf( stderr, "Usage: %s [ -A address ] obj:type@zone\n", p );
    exit( 1 );
}

int main( ac, av )
    int		ac;
    char	**av;
{
    char		*Obj = 0, *Type = 0, *Zone = 0;
    struct at_addr      addr;
    int                 c;

    extern char		*optarg;
    extern int		optind;
    
    memset(&addr, 0, sizeof(addr));
    while ((c = getopt(ac, av, "A:")) != EOF) {
      switch (c) {
      case 'A':
	if (!atalk_aton(optarg, &addr)) {
	  fprintf(stderr, "Bad address.\n");
	  exit(1);
	}
	break;
      default:
	Usage(av[0]);
	break;
      }
    }

    if (ac - optind != 1) {
	Usage( av[ 0 ] );
    }

    /*
     * Get the name. If Type or Obj aren't specified, error.
     */
    if ( nbp_name( av[optind], &Obj, &Type, &Zone ) || !Obj || !Type ) {
	Usage( av[ 0 ] );
    }

    if ( nbp_unrgstr( Obj, Type, Zone, &addr ) < 0 ) {
	fprintf( stderr, "Can't unregister %s:%s@%s\n", Obj, Type,
		Zone ? Zone : "*" );
	exit( 1 );
    }

    return 0;
}
