/* $Id: if.c,v 1.3 2005-04-28 20:50:07 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stream.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/byteorder.h>
#include <sys/ethernet.h>
#include <sys/ddi.h>
#include <net/if.h>
#include <netinet/arp.h>

#ifdef STDC_HEADERS
#include <strings.h>
#else
#include <string.h>
#endif

#include <netatalk/at.h>
#include <netatalk/aarp.h>

#include "if.h"
#include "rt.h"
#include "ioc.h"

static struct atif_data	*interfaces = NULL;

    struct atif_data *
if_primary()
{
    return( interfaces );
}

    struct atif_data *
if_alloc( queue_t *q )
{
    struct atif_data	*aid;

    if (( aid = kmem_zalloc( sizeof( struct atif_data ), KM_SLEEP )) == NULL ) {
	return( NULL );
    }
    aid->aid_q = q;
    aid->aid_state = DL_UNATTACHED;

    return( aid );
}

/*
 * Name an interface, insert it in our list of interfaces.  If this is the
 * first interface, create the loopback interface.  If it's not the first
 * interfaces, keep the first interface the same, i.e. the first configured
 * interface should be the primary interface.
 */
    int
if_name( struct atif_data *aid, char *name, ulong ppa )
{
    sprintf( aid->aid_name, "%s%ld", name, ppa );

    if ( interfaces == NULL ) {		/* create fake loopback */
	if (( interfaces = if_alloc( NULL )) == NULL ) {
	    return( ENOMEM );
	}
	strcpy( interfaces->aid_name, "lo0" );
	interfaces->aid_state = DL_IDLE;
	bzero( interfaces->aid_hwaddr, sizeof( interfaces->aid_hwaddr ));
	interfaces->aid_flags = AIDF_LOOPBACK;
	interfaces->aid_c.c_type = 0;

	aid->aid_next = interfaces;
	aid->aid_next->aid_prev = aid;
	interfaces = aid;
    } else {
	aid->aid_next = interfaces->aid_next;
	aid->aid_prev = interfaces;
	aid->aid_next->aid_prev = aid;
	interfaces->aid_next = aid;
    }

    aarp_init( aid );
    return( 0 );
}

    void
if_free( struct atif_data *aid )
{
    if ( aid->aid_c.c_type != 0 ) {
	cmn_err( CE_NOTE, "if_free context %x\n", aid->aid_c.c_type );
	return;
    }

    aarp_clean( aid );

    if ( aid->aid_next != NULL ) {
	aid->aid_next->aid_prev = aid->aid_prev;
    }
    if ( aid->aid_prev != NULL ) {
	aid->aid_prev->aid_next = aid->aid_next;
    }
    if ( aid == interfaces ) {
	interfaces = aid->aid_next;
    }
    kmem_free( aid, sizeof( struct atif_data ));

    if ( interfaces != NULL && interfaces->aid_next == NULL ) {
	kmem_free( interfaces, sizeof( struct atif_data ));
	interfaces = NULL;
    }
    return;
}

    int
if_getaddr( char *name, struct sockaddr_at *sat )
{
    struct atif_data	*aid;

    for ( aid = interfaces; aid != NULL; aid = aid->aid_next ) {
	if ( strcmp( name, aid->aid_name ) == 0 ) {
	    break;
	}
    }
    if ( aid == NULL ) {
	return( EADDRNOTAVAIL );
    }

    bcopy( &aid->aid_sat, sat, sizeof( struct sockaddr_at ));
    return( 0 );
}

    int
if_addmulti( queue_t *q, mblk_t *m, char *name, struct sockaddr *sa )
{
    struct atif_data	*aid;

    for ( aid = interfaces; aid != NULL; aid = aid->aid_next ) {
	if ( strcmp( name, aid->aid_name ) == 0 ) {
	    break;
	}
    }
    if ( aid == NULL ) {
	return( EADDRNOTAVAIL );
    }

    if ( aid->aid_c.c_type != 0 ) {
	cmn_err( CE_NOTE, "if_addmulti context %x\n", aid->aid_c.c_type );
	return( EINVAL );
    }

    aid->aid_c.c_type = SIOCADDMULTI;
    aid->aid_c.c_u.u_multi.um_q = q;
    aid->aid_c.c_u.u_multi.um_m = m;
    dl_enabmulti_req( WR( aid->aid_q ), sa->sa_data );

    return( 0 );
}

    void
if_pickaddr( void *ptr )
{
    struct atif_data *aid = (struct atif_data*) ptr;

    if ( aid->aid_c.c_type != SIOCSIFADDR ) {
	cmn_err( CE_NOTE, "if_pickaddr context %x\n", aid->aid_c.c_type );
	return;
    }

    if ( aid->aid_flags & AIDF_PROBEFAILED ) {
	aid->aid_flags &= ~AIDF_PROBEFAILED;
	/* choose new address */
	for (;;) {
	    if ( aid->aid_c.c_u.u_addr.ua_nodecnt == 0 ) {
		/* if we've exausted all addresses, fail */
		if ( aid->aid_c.c_u.u_addr.ua_netcnt == 0 ) {
		    ioc_error_ack( aid->aid_c.c_u.u_addr.ua_q,
			    aid->aid_c.c_u.u_addr.ua_m, EADDRINUSE );
		    aid->aid_c.c_type = 0;
		    aid->aid_c.c_u.u_addr.ua_q = NULL;
		    aid->aid_c.c_u.u_addr.ua_m = NULL;
		    aid->aid_c.c_u.u_addr.ua_probecnt = 0;
		    aid->aid_c.c_u.u_addr.ua_netcnt = 0;
		    aid->aid_c.c_u.u_addr.ua_nodecnt = 0;
		} else {
		    aid->aid_c.c_u.u_addr.ua_nodecnt = 256;
		    aid->aid_c.c_u.u_addr.ua_netcnt--;
		    if ( ntohs(aid->aid_sat.sat_addr.s_net) >
			    ntohs(aid->aid_nr.nr_lastnet) ) {
			aid->aid_sat.sat_addr.s_net = aid->aid_nr.nr_firstnet;
		    } else
		      aid->aid_sat.sat_addr.s_net = 
			htons(ntohs(aid->aid_sat.sat_addr.s_net) + 1);
		}
	    } else {
		aid->aid_sat.sat_addr.s_node++;
		aid->aid_c.c_u.u_addr.ua_nodecnt--;
		if ( aid->aid_sat.sat_addr.s_node == 0 || 
			aid->aid_sat.sat_addr.s_node == 255 || 
			aid->aid_sat.sat_addr.s_node == 254 ) {
		    continue;
		}
		break;
	    }
	}
    }
    if ( aid->aid_c.c_u.u_addr.ua_probecnt-- <= 0 ) {
	aid->aid_flags &= ~AIDF_PROBING;
	/* worked, send ioctl reponse */
	ioc_ok_ack( aid->aid_c.c_u.u_addr.ua_q, aid->aid_c.c_u.u_addr.ua_m, 0 );
	aid->aid_c.c_type = 0;
	aid->aid_c.c_u.u_addr.ua_q = NULL;
	aid->aid_c.c_u.u_addr.ua_m = NULL;
	aid->aid_c.c_u.u_addr.ua_probecnt = 0;
	aid->aid_c.c_u.u_addr.ua_netcnt = 0;
	aid->aid_c.c_u.u_addr.ua_nodecnt = 0;
	return;
    }

    aarp_send( aid, AARPOP_PROBE, NULL,
	    aid->aid_sat.sat_addr.s_net, aid->aid_sat.sat_addr.s_node );
    qtimeout( aid->aid_q, if_pickaddr, (caddr_t)aid, hz / 5 );
}

    int
if_setaddr( queue_t *q, mblk_t *m, char *name, struct sockaddr_at *sat )
{
    struct atif_data	*aid;
    struct netrange	nr;
    ulong		time;

    for ( aid = interfaces; aid != NULL; aid = aid->aid_next ) {
	if ( strcmp( name, aid->aid_name ) == 0 ) {
	    break;
	}
    }
    if ( aid == NULL ) {
	return( EADDRNOTAVAIL );
    }

    if ( aid->aid_c.c_type != 0 ) {
	cmn_err( CE_NOTE, "if_setaddr context %x\n", aid->aid_c.c_type );
	return( EINVAL );
    }

    bcopy( sat->sat_zero, &nr, sizeof( struct netrange ));

    if ( aid->aid_flags & AIDF_LOOPBACK ) {
	aid->aid_sat = *sat;
	aid->aid_nr = nr;

	/* routes? */

	ioc_ok_ack( q, m, 0 );
	return( 0 );
    }

    drv_getparm( TIME, &time );
    if ( sat->sat_addr.s_net == ATADDR_ANYNET ) {
	if ( nr.nr_lastnet == nr.nr_firstnet ) {
	    sat->sat_addr.s_net = nr.nr_firstnet;
	} else {
	    sat->sat_addr.s_net = htons(ntohs(nr.nr_firstnet) + time %
		    (ntohs(nr.nr_lastnet) - ntohs(nr.nr_firstnet)));
	}
    } else {
	if ( ntohs( sat->sat_addr.s_net ) < ntohs( nr.nr_firstnet ) ||
		ntohs( sat->sat_addr.s_net ) > ntohs( nr.nr_lastnet )) {
	    return( EINVAL );
	}
    }

    if ( sat->sat_addr.s_node == ATADDR_ANYNODE ) {
	sat->sat_addr.s_node = time;
    }

    aid->aid_flags |= AIDF_PROBING;
    aid->aid_sat = *sat;
    aid->aid_nr = nr;

    aid->aid_c.c_type = SIOCSIFADDR;
    aid->aid_c.c_u.u_addr.ua_q = q;
    aid->aid_c.c_u.u_addr.ua_m = m;
    aid->aid_c.c_u.u_addr.ua_probecnt = 10;
    aid->aid_c.c_u.u_addr.ua_netcnt = ntohs(nr.nr_lastnet) - 
      ntohs(nr.nr_firstnet);
    aid->aid_c.c_u.u_addr.ua_nodecnt = 256;
    qtimeout( aid->aid_q, if_pickaddr, (caddr_t)aid, 0 );
    return( 0 );
}

/*
 * These three routines are all a big mess...
 */
    struct atif_data *
if_dest( struct atif_data *aid, struct sockaddr_at *sat )
{
    struct atif_data	*dest;

    for ( dest = interfaces; dest != NULL; dest = dest->aid_next ) {
	if ((( sat->sat_addr.s_net == 0 && aid == dest ) ||
		( ntohs(sat->sat_addr.s_net) >= 
		  ntohs(dest->aid_nr.nr_firstnet) &&
		ntohs(sat->sat_addr.s_net) <= 
		  ntohs(dest->aid_nr.nr_lastnet) )) &&
		( sat->sat_addr.s_node == dest->aid_sat.sat_addr.s_node ||
		sat->sat_addr.s_node == ATADDR_BCAST )) {
	    break;
	}
    }
    if ( dest == NULL ) {
	return( NULL );
    }
    return( dest );
}

    struct atif_data *
if_withaddr( struct sockaddr_at *sat )
{
    struct atif_data	*dest;

    for ( dest = interfaces; dest != NULL; dest = dest->aid_next ) {
	if ( sat->sat_addr.s_net == dest->aid_sat.sat_addr.s_net &&
		sat->sat_addr.s_node == dest->aid_sat.sat_addr.s_node ) {
	    break;
	}
    }
    return( dest );
}

    struct atif_data *
if_withnet( struct sockaddr_at *sat )
{
    struct atif_data	*dest;

    for ( dest = interfaces; dest != NULL; dest = dest->aid_next ) {
	if ( ntohs(sat->sat_addr.s_net) >= ntohs(dest->aid_nr.nr_firstnet) &&
		ntohs(sat->sat_addr.s_net) <= ntohs(dest->aid_nr.nr_lastnet)) {
	    break;
	}
    }
    return( dest );
}

    int
if_route( struct atif_data *aid, mblk_t *m, struct sockaddr_at *sat )
{
    struct sockaddr_at	gate;

    if ( sat->sat_addr.s_net == 0 ) {
	if ( sat->sat_addr.s_node == 0 ) {
	    aid = if_withaddr( sat );
	}
	if ( aid == NULL ) {
	    freemsg( m );
	    return( ENETUNREACH );
	}
	gate = *sat;
    } else {
	if ( rt_gate( sat, &gate ) < 0 ) {	/* no route */
	    gate = *sat;
	}
	if (( aid = if_withaddr( &gate )) == NULL ) {
	    if (( aid = if_withnet( &gate )) == NULL ) {
		freemsg( m );
		return( ENETUNREACH );
	    }
	}
    }

    if ( aid->aid_flags & AIDF_LOOPBACK ) {
	return( ddp_rput( aid, m ));
    } else {
	/* the aarp layer is expected to send broadcast packets appropriately */
	return( aarp_resolve( aid, m, &gate ));
    }
}
