/* $Id: sock.c,v 1.2 2002-01-17 07:11:13 srittau Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/tihdr.h>
#include <sys/ethernet.h>
#include <net/if.h>

#ifdef STDC_HEADERS
#include <strings.h>
#else
#include <string.h>
#endif

#include <netatalk/at.h>

#include "if.h"
#include "sock.h"

static struct sock_data	*sockets = NULL;

    struct sock_data *
sock_alloc( queue_t *q )
{
    struct sock_data	*sd;

    if (( sd = kmem_alloc( sizeof( struct sock_data ), KM_SLEEP )) == NULL ) {
	return( NULL );
    }
    sd->sd_state = TS_UNBND;
    sd->sd_q = q;
    sd->sd_next = sd->sd_prev = NULL;
    bzero( (caddr_t)&sd->sd_sat, sizeof( struct sockaddr_at ));

    sd->sd_next = sockets;
    if ( sockets != NULL ) {
	sockets->sd_prev = sd;
    }
    sockets = sd;

    return( sd );
}

    void
sock_free( struct sock_data *sd )
{
    if ( sd == sockets ) {
	sockets = sd->sd_next;
    }
    if ( sd->sd_next != NULL ) {
	sd->sd_next->sd_prev = sd->sd_prev;
    }
    if ( sd->sd_prev != NULL ) {
	sd->sd_prev->sd_next = sd->sd_next;
    }
    kmem_free( sd, sizeof( struct sock_data ));
    return;
}

    struct sock_data *
sock_dest( struct atif_data *aid, struct sockaddr_at *sat )
{
    struct sock_data	*sd;

    for ( sd = sockets; sd != NULL; sd = sd->sd_next ) {
	if ( sat->sat_port == sd->sd_sat.sat_port &&
		/* huh? */
		aid->aid_sat.sat_addr.s_net == sd->sd_sat.sat_addr.s_net &&
		( sat->sat_addr.s_node == sd->sd_sat.sat_addr.s_node ||
		sat->sat_addr.s_node == ATADDR_BCAST )) {
	    break;
	}
    }
    return( sd );
}

/*
 * This is a change in semantics.  The port must be ATADDR_ANYPORT for
 * ATADDR_ANYNET/NODE to not mean the loopback.
 */
    int
sock_bind( struct sock_data *sd, struct sockaddr_at *sat )
{
    struct atif_data	*paid;
    struct sock_data	*psd;
    struct sockaddr_at	psat;
    u_short		port;

    psat = *sat;
    if ( psat.sat_family != AF_APPLETALK ) {
	cmn_err( CE_CONT, "sock_bind non-AppleTalk\n" );
	return( EPROTOTYPE );
    }

    if ( psat.sat_port == ATADDR_ANYPORT ) {
	if ( psat.sat_addr.s_net == ATADDR_ANYNET &&
		psat.sat_addr.s_node == ATADDR_ANYNODE ) {
	    /* chose primary interface */
	    if (( paid = if_primary()) == NULL ) {
		return( EADDRNOTAVAIL );
	    }
	    psat.sat_addr.s_net = paid->aid_sat.sat_addr.s_net;
	    psat.sat_addr.s_node = paid->aid_sat.sat_addr.s_node;
	}

	/* pick unused port */
	for ( port = ATPORT_RESERVED; port < ATPORT_LAST; port++ ) {
	    for ( psd = sockets; psd != NULL; psd = psd->sd_next ) {
		if ( port == psd->sd_sat.sat_port &&
			psat.sat_addr.s_net == psd->sd_sat.sat_addr.s_net &&
			psat.sat_addr.s_node == psd->sd_sat.sat_addr.s_node ) {
		    break;
		}
	    }
	    if ( psd == NULL ) {
		break;
	    }
	}
	if ( psd != NULL ) {
	    return( EADDRINUSE );
	}
	psat.sat_port = port;
    }

    sd->sd_sat = psat;
    sd->sd_state = TS_IDLE;
    return( 0 );
}
