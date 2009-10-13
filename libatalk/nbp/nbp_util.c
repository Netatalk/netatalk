/*
 * $Id: nbp_util.c,v 1.5 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>

#include <atalk/nbp.h>
#include <atalk/ddp.h>
#include <atalk/util.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#include  "nbp_conf.h"

char		nbp_send[ 1024 ];
char		nbp_recv[ 1024 ];
u_char		nbp_port = 0;
unsigned char   nbp_id = 0;

int nbp_parse(char *data, struct nbpnve *nn, int len)
{
    struct nbptuple	nt;

    memcpy( &nt, data, SZ_NBPTUPLE);
    data += SZ_NBPTUPLE;
    len -= SZ_NBPTUPLE;
    if ( len < 0 ) {
	return( -1 );
    }

#ifdef BSD4_4
    nn->nn_sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    nn->nn_sat.sat_family = AF_APPLETALK;
    nn->nn_sat.sat_addr.s_net = nt.nt_net;
    nn->nn_sat.sat_addr.s_node = nt.nt_node;
    nn->nn_sat.sat_port = nt.nt_port;

    nn->nn_objlen = *data++;
    len -= nn->nn_objlen + 1;
    if ( len < 0 ) {
	return( -1 );
    }
    if ( nn->nn_objlen > NBPSTRLEN ) {
	return( -1 );
    }
    memcpy( nn->nn_obj, data, nn->nn_objlen );
    data += nn->nn_objlen;

    nn->nn_typelen = *data++;
    len -= nn->nn_typelen + 1;
    if ( len < 0 ) {
	return( -1 );
    }
    if ( nn->nn_typelen > NBPSTRLEN ) {
	return( 1 );
    }
    memcpy( nn->nn_type, data, nn->nn_typelen );

    data += nn->nn_typelen;
    nn->nn_zonelen = *data++;
    len -= nn->nn_zonelen + 1;
    if ( len < 0 ) {
	return( -1 );
    }
    if ( nn->nn_zonelen > NBPSTRLEN ) {
	return( 1 );
    }
    memcpy( nn->nn_zone, data, nn->nn_zonelen );

    return( len );
}

#define NBPM_OBJ	(1<<1)
#define NBPM_TYPE	(1<<2)
#define NBPM_ZONE	(1<<3)

int nbp_match(struct nbpnve *n1, struct nbpnve *n2, int flags)
{
    int			match = 0;

    if ( flags & NBPMATCH_NOZONE ) {
	match |= NBPM_ZONE;
    }

    if ( !( flags & NBPMATCH_NOGLOB )) {
	if ( n1->nn_objlen == 1 && n1->nn_obj[0] == '=' ) {
	    match |= NBPM_OBJ;
	}
	if ( n1->nn_typelen == 1 && n1->nn_type[0] == '=' ) {
	    match |= NBPM_TYPE;
	}
    }

    if ( !( match & NBPM_OBJ )) {
	if ( n1->nn_objlen != n2->nn_objlen ||
		strndiacasecmp( n1->nn_obj, n2->nn_obj, n1->nn_objlen )) {
	    return( 0 );
	}
    }
    if ( !( match & NBPM_TYPE )) {
	if ( n1->nn_typelen != n2->nn_typelen ||
		strndiacasecmp( n1->nn_type, n2->nn_type, n1->nn_typelen )) {
	    return( 0 );
	}
    }
    if ( !( match & NBPM_ZONE )) {
	if ( n1->nn_zonelen != n2->nn_zonelen ||
		strndiacasecmp( n1->nn_zone, n2->nn_zone, n1->nn_zonelen )) {
	    return( 0 );
	}
    }

    return( 1 );
}

int nbp_name(const char  *name, char **objp,char **typep,char **zonep)
{
    static char	buf[ 32 + 1 + 32 + 1 + 32 + 1 ];
    char	*p;

    if ( name ) {
	if ( strlen( name ) + 1 > sizeof( buf )) {
	    return( -1 );
	}
	strcpy( buf, name );

	if (( p = strrchr( buf, '@' )) != NULL ) {
	    *p++ = '\0';
	    *zonep = p;
	}
	if (( p = strrchr( buf, ':' )) != NULL ) {
	    *p++ = '\0';
	    *typep = p;
	}
	if ( *buf != '\0' ) {
	    *objp = buf;
	}
    }

    return( 0 );
}
