/*
 * $Id: nbp_unrgstr.c,v 1.6 2009-10-14 02:24:05 didg Exp $
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

int nbp_unrgstr(const char *obj,const char *type,const char  *zone, const struct at_addr *addr)
{
    struct sockaddr_at	to;
    struct nbphdr	nh;
    struct timeval	timeout;
    fd_set		readfd;
    struct servent	*se;
    char		*data;
    int			s, cc;
    SOCKLEN_T		namelen;


    memset(&to, 0, sizeof(to));
    if ((s = netddp_open(&to, NULL)) < 0)
	return -1;

    data = nbp_send;
    *data++ = DDPTYPE_NBP;
    nh.nh_op = NBPOP_UNRGSTR;
    nh.nh_cnt = 1;
    nh.nh_id = ++nbp_id;
    memcpy( data, &nh, SZ_NBPHDR );
    data += SZ_NBPHDR;

    memset(data, 0, SZ_NBPTUPLE);
    data += SZ_NBPTUPLE;

    if ( obj ) {
	if (( cc = strlen( obj )) > NBPSTRLEN ) return( -1 );
	*data++ = cc;
	memcpy( data, obj, cc );
	data += cc;
    } else {
	*data++ = 0;
    }

    if ( type ) {
	if (( cc = strlen( type )) > NBPSTRLEN ) return( -1 );
	*data++ = cc;
	memcpy( data, type, cc );
	data += cc;
    } else {
	*data++ = 0;
    }

    if ( zone ) {
	if (( cc = strlen( zone )) > NBPSTRLEN ) return( -1 );
	*data++ = cc;
	memcpy( data, zone, cc );
	data += cc;
    } else {
	*data++ = 0;
    }

    memset( &to, 0, sizeof( struct sockaddr_at ));
    to.sat_family = AF_APPLETALK;
    if (addr) 
      memcpy(&to.sat_addr, addr, sizeof(struct at_addr));
#ifdef BSD4_4
    to.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */

    if ( nbp_port == 0 ) {
	if (( se = getservbyname( "nbp", "ddp" )) == NULL ) {
	    nbp_port = 2;
	} else {
	    nbp_port = ntohs( se->s_port );
	}
    }
    to.sat_port = nbp_port;

    if ( netddp_sendto( s, nbp_send, data - nbp_send, 0,
			(struct sockaddr *)&to,
			sizeof( struct sockaddr_at )) < 0 ) {
        goto unregister_err;
    }

    FD_ZERO( &readfd );
    FD_SET( s, &readfd );
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (( cc = select( s + 1, &readfd, NULL, NULL, &timeout )) < 0 ) {
        goto unregister_err;
    }
    if ( cc == 0 ) {
	errno = ETIMEDOUT;
        goto unregister_err;
    }

    namelen = sizeof( struct sockaddr_at );
    if (( cc = netddp_recvfrom( s, nbp_recv, sizeof( nbp_recv ), 0,
			(struct sockaddr *)&to, &namelen )) < 0 ) {
        goto unregister_err;
    }
    netddp_close( s );

    data = nbp_recv;
    if ( *data++ != DDPTYPE_NBP ) {
	return( -1 );
    }
    memcpy( &nh, data, SZ_NBPHDR );
    if ( nh.nh_op != NBPOP_OK ) {
	return( -1 );
    }
    return( 0 );

unregister_err:
    netddp_close(s);
    return -1;
}
