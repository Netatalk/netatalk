/*
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

#include <sys/types.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <net/if.h>
#include <net/if_llc.h>
#include <net/if_to_proto.h>
#include <net/netisr.h>
#include <netinet/in.h>
#undef s_net
#include <netinet/if_ether.h>

#include "at.h"
#include "at_var.h"

extern u_char	at_org_code[ 3 ];
extern u_char	aarp_org_code[ 3 ];

/*
 * This is the magic input routine, for all AppleTalk related packets.
 * It will pick up *all* packets received, on all interfaces, apparently.
 * If it turns out that receiving all packets in this fashion causes
 * DLI to not receive packets what it should, we may need to call DLI
 * directly from within the AppleTalk input routines.  Ick.
 */
struct mbuf *
ddp_ifinput( m, ifp, inq, eh )
    struct mbuf		*m;
    struct ifnet	*ifp;
    struct ifqueue	**inq;
    struct ether_header	*eh;
{
    struct llc		llc;
    struct if_family	*ifam;

    switch ( eh->ether_type ) {
    case ETHERTYPE_AT :
	*inq = &atintrq1;
	smp_lock( &(*inq)->lk_ifqueue, LK_RETRY );
	schednetisr( NETISR_AT );
	return( m );

    case ETHERTYPE_AARP :
	aarpinput( ifp, m );
	return( 0 );

    default :
	if ( eh->ether_type <= ETHERMTU ) {		/* ieee802 */
	    if ( m->m_len < sizeof( struct llc )) {
		break;
	    }

	    bcopy( mtod( m, caddr_t ), &llc, sizeof( struct llc ));
	    if ( llc.llc_dsap != LLC_SNAP_LSAP ||
		    llc.llc_ssap != LLC_SNAP_LSAP ||
		    llc.llc_control != LLC_UI ) {
		break;
	    }

	    if ( bcmp( llc.llc_org_code, at_org_code,
		    sizeof( at_org_code )) == 0 &&
		    ntohs( llc.llc_ether_type ) == ETHERTYPE_AT ) {
		m_adj( m, sizeof( struct llc ));
		*inq = &atintrq2;
		smp_lock( &(*inq)->lk_ifqueue, LK_RETRY );
		schednetisr( NETISR_AT );
		return( m );
	    }

	    if ( bcmp( llc.llc_org_code, aarp_org_code,
		    sizeof( aarp_org_code )) == 0 &&
		    ntohs( llc.llc_ether_type ) == ETHERTYPE_AARP ) {
		m_adj( m, sizeof( struct llc ));
		aarpinput( ifp, m );
		return( 0 );
	    }
	}
    }

    /*
     * Check is anyone else wants this packet.
     */
    for ( ifam = if_family; ifam->domain != -1; ifam++ ) {
	if (( eh->ether_type == ifam->if_type || ifam->if_type == -1 ) &&
		ifam->prswitch &&
		ifam->prswitch->pr_ifinput != (int (*)())ddp_ifinput ) {
	    break;
	}
    }
    if ( ifam->domain != -1 && ifam->prswitch->pr_ifinput ) {
	return( (struct mbuf *)(*ifam->prswitch->pr_ifinput)( m, ifp,
		inq, eh ));
    }

    m_freem( m );
    return( 0 );
}

/*
 * Fill in type and odst. odst is the media output address, i.e.
 * the MAC layer address. Type is the MAC type. Should be 0 to
 * indicate IEEE addressing.
 *
 * Stupidly enough, there's no way to say "can't send this now."
 * So, we just let the first packet go into the air. Not much
 * else to be done, except maybe bitch at DEC. Note: we're not
 * passing the mbuf to aarpresolve() -- that way it doesn't get
 * mfree-ed twice.
 */
ddp_ifoutput( ifp, m, dst, type, odst )
    struct ifnet	*ifp;
    struct mbuf		*m;
    struct sockaddr_at	*dst;
    short		*type;
    char		*odst;
{
    struct at_ifaddr	*aa;
    struct llc		*llc;
    struct mbuf		*m0;

    if ( !aarpresolve( ifp, 0, dst, odst )) {
	*type = 0xffff;
	return( 0 );
    }

    if (( aa = (struct at_ifaddr *)at_ifawithnet( dst, ifp->if_addrlist ))
	    == 0 ) {
	*type = 0xffff;
	return( 0 );
    }

    if ( aa->aa_flags & AFA_PHASE2 ) {
	/*
	 * This code needs to be modeled after the similar code in
	 * at_sun.c -- you can't just MGET() and bcopy(), since we might be
	 * dealing with mbufs which are really pages.
	 */
	MGET( m0, M_WAIT, MT_HEADER );
	if ( m0 == 0 ) {
	    *type = 0xffff;
	    return( 0 );
	}
	m0->m_next = m->m_next;
	m0->m_off = m->m_off;
	m0->m_len = m->m_len;
	bcopy( mtod( m, caddr_t ), mtod( m0, caddr_t ), m->m_len );
	m->m_next = m0;
	m->m_off = MMINOFF;
	m->m_len = sizeof( struct llc );

	llc = mtod( m, struct llc *);
	llc->llc_dsap = llc->llc_ssap = LLC_SNAP_LSAP;
	llc->llc_control = LLC_UI;
	bcopy( at_org_code, llc->llc_org_code, sizeof( at_org_code ));
	llc->llc_ether_type = htons( ETHERTYPE_AT );

	/*
	 * Set the type to be the length of the packet, instead of 0.
	 * Ultrix used to put the length in the packet when we set type
	 * to 0, however, now we do it ourselves.
	 */
	for ( *type = 0; m; m = m->m_next ) {
	    *type += m->m_len;
	}
    } else {
	*type = ETHERTYPE_AT;
    }

    return( 1 );
}

ddp_ifioctl( ifp, cmd, data )
    struct ifnet	*ifp;
    int			cmd;
    caddr_t		data;
{
    switch( cmd ) {
    case SIOCSIFADDR :
	aarpwhohas((struct arpcom *)ifp,
		&AA_SAT((struct ifaddr *)data)->sat_addr );
	break;
    default :
	return( EINVAL );
    }
    return( 0 );
}
