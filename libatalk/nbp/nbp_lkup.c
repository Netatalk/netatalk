/*
 * $Id: nbp_lkup.c,v 1.5 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <netatalk/ddp.h>
#include <atalk/compat.h>
#include <atalk/nbp.h>
#include <atalk/netddp.h>
#include <atalk/ddp.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#include  "nbp_conf.h"

/* FIXME/SOCKLEN_T: socklen_t is a unix98 feature. */
#ifndef SOCKLEN_T
#define SOCKLEN_T unsigned int
#endif /* ! SOCKLEN_T */

int nbp_lookup( const char *obj, const char *type, const char *zone, struct nbpnve *nn,
    int			nncnt,
    const struct at_addr *ataddr)
{
    struct sockaddr_at	addr, from;
    struct timeval	tv, tv_begin, tv_end;
    fd_set		fds;
    struct nbpnve	nve;
    struct nbphdr	nh;
    struct nbptuple	nt;
    struct servent	*se;
    char		*data = nbp_send;
    SOCKLEN_T   	namelen;
    int			s, cnt, tries, sc, cc, i, c;

    memset(&addr, 0, sizeof(addr));
    memset(&from, 0, sizeof(from));
    if (ataddr) 
      memcpy(&addr.sat_addr, ataddr, sizeof(struct at_addr));
    if ((s = netddp_open(&addr, &from)) < 0)
      return -1;

    *data++ = DDPTYPE_NBP;
#ifdef MACOSX_SERVER
    nh.nh_op = from.sat_addr.s_node ? NBPOP_BRRQ : NBPOP_LKUP;
#else /* MACOSX_SERVER */
    nh.nh_op = NBPOP_BRRQ;
#endif /* MACOSX_SERVER */

    nh.nh_cnt = 1;
    nh.nh_id = ++nbp_id;
    memcpy( data, &nh, SZ_NBPHDR );
    data += SZ_NBPHDR;

    memset(&nt, 0, sizeof(nt));
    nt.nt_net = addr.sat_addr.s_net;
    nt.nt_node = addr.sat_addr.s_node;
    nt.nt_port = addr.sat_port;

    memcpy( data, &nt, SZ_NBPTUPLE);
    data += SZ_NBPTUPLE;

    if ( obj ) {
	if (( cc = strlen( obj )) > NBPSTRLEN ) goto lookup_err;
	*data++ = cc;
	memcpy( data, obj, cc );
	data += cc;
    } else {
	*data++ = 1;
	*data++ = '='; /* match anything */
    }

    if ( type ) {
	if (( cc = strlen( type )) > NBPSTRLEN ) goto lookup_err;
	*data++ = cc;
	memcpy( data, type, cc );
	data += cc;
    } else {
	*data++ = 1;
	*data++ = '='; /* match anything */
    }

    if ( zone ) {
	if (( cc = strlen( zone )) > NBPSTRLEN ) goto lookup_err;
	*data++ = cc;
	memcpy( data, zone, cc );
	data += cc;
    } else {
	*data++ = 1;
	*data++ = '*'; /* default zone */
    }

    if ( nbp_port == 0 ) {
      if (( se = getservbyname( "nbp", "ddp" )) == NULL ) {
	            nbp_port = 2;
      } else {
	            nbp_port = ntohs( se->s_port );
      }
    }

#ifdef MACOSX_SERVER
    if (from.sat_addr.s_node) {
      memcpy(&addr.sat_addr, &from.sat_addr, sizeof(addr.sat_addr));
    } else {
      addr.sat_addr.s_net = ATADDR_ANYNET;
      addr.sat_addr.s_node = ATADDR_BCAST;
    }
#endif /* MACOSX_SERVER */
    addr.sat_port = nbp_port;

    cnt = 0;
    tries = 3;
    sc = data - nbp_send;
    while ( tries > 0 ) {
	if ( netddp_sendto( s, nbp_send, sc, 0, (struct sockaddr *)&addr,
		sizeof( struct sockaddr_at )) < 0 ) {
	    goto lookup_err;
	}

	tv.tv_sec = 2L;
	tv.tv_usec = 0;

	for (;;) {
	    FD_ZERO( &fds );
	    FD_SET( s, &fds );
	    if ( gettimeofday( &tv_begin, NULL ) < 0 ) {
		goto lookup_err;
	    }
	    if (( c = select( s + 1, &fds, NULL, NULL, &tv )) < 0 ) {
		goto lookup_err;
	    }
	    if ( c == 0 || FD_ISSET( s, &fds ) == 0 ) {
		break;
	    }
	    if ( gettimeofday( &tv_end, NULL ) < 0 ) {
		goto lookup_err;
	    }
	    if ( tv_begin.tv_usec > tv_end.tv_sec ) {
		tv_end.tv_usec += 1000000;
		tv_end.tv_sec -= 1;
	    }
	    if (( tv.tv_usec -= ( tv_end.tv_usec - tv_begin.tv_usec )) < 0 ) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	    }
	    if (( tv.tv_sec -= ( tv_end.tv_sec - tv_begin.tv_sec )) < 0 ) {
		break;
	    }

	    namelen = sizeof( struct sockaddr_at );
	    if (( cc = netddp_recvfrom( s, nbp_recv, sizeof( nbp_recv ), 0,
		    (struct sockaddr *)&from, &namelen )) < 0 ) {
		goto lookup_err;
	    }

	    data = nbp_recv;
	    if ( *data++ != DDPTYPE_NBP ) {
		continue;
	    }
	    cc--;

	    memcpy( &nh, data, SZ_NBPHDR );
	    data += SZ_NBPHDR;
	    if ( nh.nh_op != NBPOP_LKUPREPLY ) {
		continue;
	    }
	    cc -= SZ_NBPHDR;

	    while (( i = nbp_parse( data, &nve, cc )) >= 0 ) {
		data += cc - i;
		cc = i;
		/*
		 * Check to see if nve is already in nn. If not,
		 * put it in, and increment cnt.
		 */
		for ( i = 0; i < cnt; i++ ) {
		    if ( nbp_match( &nve, &nn[ i ],
			    NBPMATCH_NOZONE|NBPMATCH_NOGLOB )) {
			break;
		    }
		}
		if ( i == cnt ) {
		    nn[ cnt++ ] = nve;
		}
		if ( cnt == nncnt ) {
		    tries = 0;
		    break;
		}
	    }
	    if ( cnt == nncnt ) {
		tries = 0;
		break;
	    }
	}
	tries--;
    }

    netddp_close(s);
    errno = 0;
    return( cnt );

lookup_err:
    netddp_close(s);
    return -1;
}
