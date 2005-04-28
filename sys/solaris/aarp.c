/*
 * $Id: aarp.c,v 1.4 2005-04-28 20:50:07 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/byteorder.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ethernet.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <netinet/arp.h>
#include <net/if.h>

#ifdef STDC_HEADERS
#include <strings.h>
#else
#include <string.h>
#endif

#include <netatalk/at.h>
#include <netatalk/aarp.h>
#include <netatalk/phase2.h>

#include "if.h"

struct aarplist {
    struct aarplist	*aal_next, *aal_prev;
    struct at_addr	aal_addr;
    u_char		aal_hwaddr[ ETHERADDRL ];
    u_char		aal_age;
    u_char		aal_flags;
    mblk_t		*aal_m;
};

    struct aarplist *
aarp_find( struct atif_data *aid, ushort net, unchar node )
{
    struct aarplist	*aal;

    for ( aal = aid->aid_aarplist; aal != NULL; aal = aal->aal_next ) {
	if ( aal->aal_addr.s_net == net && aal->aal_addr.s_node == node ) {
	    break;
	}
    }
    return( aal );
}

    struct aarplist *
aarp_alloc( struct atif_data *aid, ushort net, unchar node )
{
    struct aarplist	*aal;

    for ( aal = aid->aid_aarplist; aal != NULL; aal = aal->aal_next ) {
	if ( aal->aal_addr.s_net == net && aal->aal_addr.s_node == node ) {
	    return( aal );
	}
    }

    if ( aid->aid_aarpflist == NULL ) {
        if (( aal = (struct aarplist *)kmem_alloc( sizeof( struct aarplist ),
                KM_NOSLEEP )) == NULL ) {
            return( NULL );
        }
    } else {
	aal = aid->aid_aarpflist;
	aid->aid_aarpflist = aal->aal_next;
	if ( aid->aid_aarpflist != NULL ) {
	    aid->aid_aarpflist->aal_prev = NULL;
	}
    }

    aal->aal_addr.s_net = net;
    aal->aal_addr.s_node = node;
    bzero( aal->aal_hwaddr, sizeof( aal->aal_hwaddr ));
    aal->aal_age = 0;
    aal->aal_flags = 0;
    aal->aal_m = NULL;

    aal->aal_next = aid->aid_aarplist;
    aal->aal_prev = NULL;
    if ( aid->aid_aarplist != NULL ) {
	aid->aid_aarplist->aal_prev = aal;
    }
    aid->aid_aarplist = aal;

    return( aal );
}

/*
 * Move entry to free list.
 */
    void
aarp_free( struct atif_data *aid, struct aarplist *aal )
{
    if ( aal->aal_next != NULL ) {
	aal->aal_next->aal_prev = aal->aal_prev;
    }
    if ( aal->aal_prev != NULL ) {
	aal->aal_prev->aal_next = aal->aal_next;
    }
    if ( aid->aid_aarplist == aal ) {
	aid->aid_aarplist = aal->aal_next;
    }

    if ( aal->aal_m != NULL ) {
	freemsg( aal->aal_m );
	aal->aal_m = NULL;
    }

    aal->aal_prev = NULL;
    aal->aal_next = aid->aid_aarpflist;
    if ( aid->aid_aarpflist != NULL ) {
	aid->aid_aarpflist->aal_prev = aal;
    }
    aid->aid_aarpflist = aal;
    return;
}

    void
aarp_timeout( void *ptr )
{
    struct atif_data 	*aid = (struct atif_data *) ptr;
    struct aarplist	*aal, *p;

    aid->aid_aarptimeo = qtimeout( aid->aid_q, aarp_timeout,
	    (caddr_t)aid, 60 * hz );
    for ( aal = aid->aid_aarplist; aal != NULL; aal = p ) {
	p = aal->aal_next;
	if ( ++aal->aal_age < (( aal->aal_flags ) ? 5 : 3 )) {
	    continue;
	}
	aarp_free( aid, aal );
    }
    return;
}

    void
aarp_init( struct atif_data *aid )
{
    aid->aid_aarptimeo = qtimeout( aid->aid_q, aarp_timeout,
	    (caddr_t)aid, 60 * hz );
    return;
}

    void
aarp_clean( struct atif_data *aid )
{
    struct aarplist *aal, *p;

    if ( aid->aid_aarptimeo != 0 ) {
	quntimeout( aid->aid_q, aid->aid_aarptimeo );
	aid->aid_aarptimeo = 0;
    }

    for ( aal = aid->aid_aarplist; aal != NULL; aal = p ) {
	p = aal->aal_next;
	if ( aal->aal_m != NULL ) {
	    freemsg( aal->aal_m );
	    aal->aal_m = NULL;
	}
	kmem_free( aal, sizeof( struct aarplist ));
    }
    aid->aid_aarplist = NULL;

    for ( aal = aid->aid_aarpflist; aal != NULL; aal = p ) {
	p = aal->aal_next;
	if ( aal->aal_m != NULL ) {
	    freemsg( aal->aal_m );
	    aal->aal_m = NULL;
	}
	kmem_free( aal, sizeof( struct aarplist ));
    }
    aid->aid_aarpflist = NULL;

    return;
}

    int
aarp_rput( queue_t *q, mblk_t *m )
{
    struct atif_data	*aid = (struct atif_data *)q->q_ptr;
    struct ether_aarp	*ea;
    struct aarplist	*aal;
    ushort		tpnet, spnet, op;

    if ( m->b_wptr - m->b_rptr < sizeof( struct ether_aarp )) {
	cmn_err( CE_NOTE, "aarp_rput short packet\n" );
	goto done;
    }

    ea = (struct ether_aarp *)m->b_rptr;

    if ( ea->aarp_hrd != htons( AARPHRD_ETHER ) ||
	    ea->aarp_pro != htons( ETHERTYPE_AT ) ||
	    ea->aarp_hln != sizeof( ea->aarp_sha ) ||
	    ea->aarp_pln != sizeof( ea->aarp_spu )) {
	cmn_err( CE_NOTE, "aarp_rput bad constants\n" );
	goto done;
    }

    if ( bcmp( ea->aarp_sha, aid->aid_hwaddr, sizeof( ea->aarp_sha )) == 0 ) {
	goto done;
    }

    op = ntohs( ea->aarp_op );
    bcopy( ea->aarp_tpnet, &tpnet, sizeof( tpnet ));
    bcopy( ea->aarp_spnet, &spnet, sizeof( spnet ));

    if ( aid->aid_flags & AIDF_PROBING ) {
	if ( tpnet == aid->aid_sat.sat_addr.s_net &&
		ea->aarp_tpnode == aid->aid_sat.sat_addr.s_node ) {
	    aid->aid_flags &= ~AIDF_PROBING;
	    aid->aid_flags |= AIDF_PROBEFAILED;
	    cmn_err( CE_NOTE, "aarp_rput probe collision %s\n", aid->aid_name );
	}
    } else {
	if ( tpnet == aid->aid_sat.sat_addr.s_net &&
		ea->aarp_tpnode == aid->aid_sat.sat_addr.s_node ) {
	    switch ( op ) {
	    case AARPOP_REQUEST :
		aal = aarp_alloc( aid, spnet, ea->aarp_spnode );
		bcopy( ea->aarp_sha, aal->aal_hwaddr, sizeof( ea->aarp_sha ));
		aal->aal_age = 0;
		aal->aal_flags = 1;		/* complete */
	    case AARPOP_PROBE :
		aarp_send( aid, AARPOP_RESPONSE, ea->aarp_sha,
			spnet, ea->aarp_spnode );
		break;

	    case AARPOP_RESPONSE :
		if (( aal =
			aarp_find( aid, spnet, ea->aarp_spnode )) == NULL ) {
		    break;
		}
		bcopy( ea->aarp_sha, aal->aal_hwaddr, sizeof( ea->aarp_sha ));
		aal->aal_age = 0;
		aal->aal_flags = 1;		/* complete */
		if ( aal->aal_m != NULL ) {
		    dl_unitdata_req( WR( q ), aal->aal_m, ETHERTYPE_AT,
			    aal->aal_hwaddr );
		    aal->aal_m = NULL;
		}
		break;

	    default :
		cmn_err( CE_NOTE, "aarp_rput bad op %X\n", op );
		break;
	    }
	} else {
	    switch ( op ) {
	    case AARPOP_REQUEST :
		break;
	    case AARPOP_PROBE :
		if (( aal =
			aarp_find( aid, spnet, ea->aarp_spnode )) != NULL ) {
		    aarp_free( aid, aal );
		}
		break;

	    case AARPOP_RESPONSE :
		cmn_err( CE_NOTE, "aarp_rput someone using our address\n" );
		break;

	    default :
		cmn_err( CE_NOTE, "aarp_rput bad op %X\n", op );
		break;
	    }
	}
    }

done :
    freemsg( m );
    return( 0 );
}

    void
aarp_send( struct atif_data *aid, int op, caddr_t hwaddr,
	ushort net, unchar node )
{
    mblk_t		*m;
    struct ether_aarp	*ea;

    if (( m = allocb( sizeof( struct ether_aarp ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct ether_aarp );
    ea = (struct ether_aarp *)m->b_rptr;
    bzero( (caddr_t)ea, sizeof( struct ether_aarp ));

    ea->aarp_hrd = htons( AARPHRD_ETHER );
    ea->aarp_pro = htons( ETHERTYPE_AT );
    ea->aarp_hln = sizeof( ea->aarp_sha );
    ea->aarp_pln = sizeof( ea->aarp_spu );
    ea->aarp_op = htons( op );
    bcopy( aid->aid_hwaddr, ea->aarp_sha, sizeof( ea->aarp_sha ));

    if ( hwaddr == NULL ) {
	bzero( ea->aarp_tha, sizeof( ea->aarp_tha ));
    } else {
	bcopy( hwaddr, ea->aarp_tha, sizeof( ea->aarp_tha ));
    }

    ea->aarp_tpnode = node;
    bcopy( &aid->aid_sat.sat_addr.s_net, ea->aarp_spnet,
	    sizeof( ea->aarp_spnet ));
    bcopy( &net, ea->aarp_tpnet, sizeof( ea->aarp_tpnet ));
    ea->aarp_spnode = aid->aid_sat.sat_addr.s_node;
    ea->aarp_tpnode = node;

    if ( hwaddr == NULL ) {
	dl_unitdata_req( WR( aid->aid_q ), m, ETHERTYPE_AARP,
		at_multicastaddr );
    } else {
	dl_unitdata_req( WR( aid->aid_q ), m, ETHERTYPE_AARP, hwaddr );
    }
    return;
}

    int
aarp_resolve( struct atif_data *aid, mblk_t *m, struct sockaddr_at *sat )
{
    struct aarplist	*aal;

    if ( sat->sat_addr.s_node == ATADDR_BCAST ) {
	dl_unitdata_req( WR( aid->aid_q ), m, ETHERTYPE_AT, at_multicastaddr );
	return( 0 );
    }

    if (( aal = aarp_alloc( aid, sat->sat_addr.s_net, sat->sat_addr.s_node )) ==
	    NULL ) {
	freemsg( m );
	return( ENOMEM );
    }
    aal->aal_age = 0;

    if ( aal->aal_flags ) {	/* complete */
	dl_unitdata_req( WR( aid->aid_q ), m, ETHERTYPE_AT, aal->aal_hwaddr );
    } else {
	/* send aarp request */
	if ( aal->aal_m != NULL ) {
	    freemsg( aal->aal_m );
	}
	/* either freed above, in timeout, or sent in aarp_rput() */
	aal->aal_m = m;	
	aarp_send( aid, AARPOP_REQUEST, NULL,
		sat->sat_addr.s_net, sat->sat_addr.s_node );
    }
    return( 0 );
}
