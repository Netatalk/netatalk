/*
 * $Id: at_sun.c,v 1.2 2001-08-06 13:39:30 rufustfirefly Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/errno.h>

#include <net/if_arp.h>
#include <net/if.h>
#include <net/route.h>
#include <net/af.h>
#include <net/netisr.h>

#include <sun/vddrv.h>

#include <netinet/in.h>
#undef s_net
#include <netinet/if_ether.h>

#include <netatalk/at.h>
#include <netatalk/at_var.h>
#include <netatalk/ddp_var.h>
#include <netatalk/phase2.h>

struct vdlat {
    int		vda_magic;
    char	*vda_name;
}	atvdl = {
    VDMAGIC_USER, "netatalk"
};

struct ifqueue	*atef_input();
int		atef_output();
extern int	atintr();

extern u_char	aarp_org_code[ 3 ];
extern u_char	at_org_code[ 3 ];

/*
 * Entries in this table are inserted into the ether_families linked
 * list at the beginnning. As such, they will be searched by the input
 * and output routines opposite to the order here.
 *
 * In order to do both phase 1 and phase 2 during output, we have a
 * special entry (the AF_APPLETALK entry) whose ethertype field is
 * changed by the output function, to reflect the actual link layer
 * to be used. This ether_family entry is never seen during Ethernet
 * input, since the earlier entry captures all packets (it is seen
 * during loopback input, in that the input function is called directly
 * on loopback output).
 */
struct ether_family	ether_atalk[] = {
    {
	AF_APPLETALK,	0,	/* Changed by atef_output() */
	atef_input,	atef_output,	atintr,
    },
    {
	0,		ETHERTYPE_AARP,
	atef_input,	0,		0,
    },
    {
	0,		ETHERTYPE_AT,
	atef_input,	0,		atintr,
    },
    {
	0,		EF_8023_TYPE,
	atef_input,	0,		atintr,
    },
};
int	ether_atalkN = sizeof( ether_atalk ) / sizeof( ether_atalk[ 0 ] );

extern struct ether_family	*ether_families;

extern int			atalk_hash(), atalk_netmatch();
extern int			null_hash(), null_netmatch();

xxxinit( cmd, vdd, vdi, vds )
    unsigned int	cmd;
    struct vddrv	*vdd;
    addr_t		vdi;
    struct vdstat	*vds;
{
    struct ether_family	*ef;
    struct domain	*dom;
    struct protosw	*pr;
    int			i;

    switch( cmd ) {
	case VDLOAD :
	    vdd->vdd_vdtab = (struct vdlinkage *)&atvdl;

	    /*
	     * Register with the network-interface layer (ethernet).
	     */
	    for ( i = 0; i < ether_atalkN; i++ ) {
		ether_register( &ether_atalk[ i ] );
	    }

	    /*
	     * Register with the socket layer.
	     */
	    atalkdomain.dom_next = domains;
	    domains = &atalkdomain;
	    if ( atalkdomain.dom_init ) {
		(*atalkdomain.dom_init)();
	    }
	    for ( pr = atalkdomain.dom_protosw;
		    pr < atalkdomain.dom_protoswNPROTOSW; pr++ ) {
		if ( pr->pr_init ) {
		    (*pr->pr_init)();
		}
	    }

	    /*
	     * Cobble ourselves into the routing table.
	     */
	    afswitch[ AF_APPLETALK ].af_hash = atalk_hash;
	    afswitch[ AF_APPLETALK ].af_netmatch = atalk_netmatch;
	    return( 0 );

	case VDUNLOAD :
	    /*
	     * Make sure that there are no open appletalk sockets.
	     */
	    if ( ddpcb != NULL ) {
		return( EMFILE );
	    }

	    /*
	     * There is no ether_unregister(), so we'll have to do it
	     * our selves...
	     */
	    for ( i = 0; i < ether_atalkN; i++ ) {
		if ( ether_families == &ether_atalk[ i ] ) {
		    ether_families = ether_families->ef_next;
		    continue;
		} else {
		    for ( ef = ether_families; ef->ef_next; ef = ef->ef_next ) {
			if ( ef->ef_next == &ether_atalk[ i ] ) {
			    ef->ef_next = ef->ef_next->ef_next;
			    break;
			}
		    }
		}
	    }

	    /*
	     * Remove aarp timers and held packets.
	     */
	    aarp_clean();

	    /*
	     * Remove AppleTalk interface addresses.
	     */
	    aa_clean();

	    /*
	     * Remove our routines from the routing table.
	     */
	    afswitch[ AF_APPLETALK ].af_hash = null_hash;
	    afswitch[ AF_APPLETALK ].af_netmatch = null_netmatch;

	    /*
	     * Remove atalkdomain from the domains list.
	     * Unlikely, but someone may have registered after us.
	     */
	    if ( domains == &atalkdomain ) {
		domains = domains->dom_next;
	    } else {
		for ( dom = domains; dom->dom_next; dom = dom->dom_next ) {
		    if ( dom->dom_next == &atalkdomain ) {
			dom->dom_next = dom->dom_next->dom_next;
			break;
		    }
		}
	    }
	    return( 0 );

	case VDSTAT :
	    return( 0 );
	default :
	    return( EIO );
    }
}

/*
 * Input routine for netatalk on suns.  There are five possible
 * packets.  First, packets received on the loopback interface
 * are immediately sent to the phase 1 interrupt queue (this will
 * have to change if we ever do a phase 2 only version).  Second,
 * IEEE802 packet are sent to either the aarpinput() routine or
 * the phase 2 interrupt queue.  Finally, DIX packets are sent
 * to either aarpinput() or the phase 1 interrupt queue.
 */
    struct ifqueue *
atef_input( ifp, m, header )
    struct ifnet	*ifp;
    struct mbuf		*m;
    struct ether_header	*header;
{
    struct llc		llc;
    struct mbuf		*n = 0;

    /*
     * Check first for LOOPBACK flag, since loopback code passes NULL for
     * the header.
     */
    if ( ifp->if_flags & IFF_LOOPBACK ) {
	return( &atintrq2 );
    }

    /*
     * Before SunOS 4.1, the ether_type was passed as is from the
     * packet.  After SunOS 4.1, the ether_type is swapped in
     * do_protocol(), before the ether_family routines are called.
     */
#if defined( sun ) && defined( i386 )
    header->ether_type = ntohs( header->ether_type );
#endif /* sun i386 */

    if ( header->ether_type <= ETHERMTU ) {	/* IEEE802 */
	/*
	 * We need to remove the interface pointer from the beginning of this
	 * packet.  We can't always use IF_ADJ(), since it can (and will,
	 * very often) MFREE() the first mbuf in our chain.  If IF_ADJ()
	 * would free the first mbuf, we just advance our pointer to the
	 * next mbuf.  Since our calling routine passes m by value, we're
	 * not actually losing m.  Later, we don't need to put the interface
	 * pointer back on, since the caller still has it in its copy of m.
	 */
	if ( m->m_len == sizeof( struct ifnet * )) {
	    n = m;
	    m = m->m_next;
	} else {
	    IF_ADJ( m );
	}

	/*
	 * We can't call m_pullup(), since we need to preserve
	 * the value of m.
	 */
	if ( m->m_len < sizeof( struct llc )) {
printf( "atef_input size llc\n" );
	    ( n ) ? m_freem( n ) : m_freem( m );
	    return( 0 );
	}
	bcopy( mtod( m, caddr_t ), &llc, sizeof( struct llc ));
	if ( llc.llc_dsap != LLC_SNAP_LSAP || llc.llc_ssap != LLC_SNAP_LSAP ||
		llc.llc_control != LLC_UI ) {
	    ( n ) ? m_freem( n ) : m_freem( m );
	    return( 0 );
	}

	/*
	 * See IF_ADJ() above.  Here we prepend ifp to the mbuf chain.  If we
	 * didn't remove it earlier, we don't replace it here.
	 */
	if ( n ) {
	    m_adj( m, sizeof( struct llc ));
	} else {
	    m_adj( m, sizeof( struct llc ) - sizeof( struct ifnet *));
	    if ( m->m_len < sizeof( struct ifnet * )) {
printf( "atef_input too small!\n" );
		m_freem( m );
		return( 0 );
	    }
	    *mtod( m, struct ifnet ** ) = ifp;
	}

	if ( ntohs( llc.llc_ether_type ) == ETHERTYPE_AT &&
		bcmp( llc.llc_org_code, at_org_code,
		sizeof( at_org_code )) == 0 ) {
	    return( &atintrq2 );
	}

	/* do we really want to pass m, here?  what happened to n? XXX */
	if ( ntohs( llc.llc_ether_type ) == ETHERTYPE_AARP &&
		bcmp( llc.llc_org_code, aarp_org_code,
		sizeof( aarp_org_code )) == 0 ) {
	    aarpinput( ifp, n ? n : m );
	    return( 0 );
	}

    } else {					/* DIX */
	switch ( header->ether_type ) {
	case ETHERTYPE_AT :
	    return( &atintrq1 );

	case ETHERTYPE_AARP :
	    aarpinput( ifp, m );
	    return( 0 );
	}
    }

    ( n ) ? m_freem( n ) : m_freem( m );
    return( 0 );
}

/*
 * If the destination is on a 802.3 wire, do phase 2 encapsulation,
 * adding the 802.2 and SNAP headers.  Always fill in the edst with the
 * ethernet address of the destination.
 */
atef_output( dst, m, ifp, edst )
    struct sockaddr_at	*dst;
    struct mbuf		*m;
    struct ifnet	*ifp;
    struct ether_addr	*edst;
{
    struct at_ifaddr	*aa;
    struct mbuf		*m0;
    struct llc		llc;
    int			s;

    s = splimp();
    if ( !aarpresolve( ifp, m, dst, edst )) {
	(void) splx( s );
	return( 1 );
    }
    (void) splx( s );

    /*
     * ifaddr is the first thing in at_ifaddr
     */
    if (( aa = (struct at_ifaddr *)at_ifawithnet( dst, ifp->if_addrlist ))
	    == 0 ) {
	m_freem( m );
	return( 1 );
    }

    /*
     * In the phase 2 case, we need to prepend an mbuf for the llc header.
     * Since we must preserve the value of m, which is passed to us by
     * value, we m_copy() the first mbuf, and use it for our llc header.
     *
     * We could worry about leaving space for the ether header, but
     * since we'll have to go through all sorts of hoops, including a
     * possibly large copy, there's really no sense.
     */
    if ( aa->aa_flags & AFA_PHASE2 ) {
	if ( M_HASCL( m ) || m->m_off - MMINOFF < sizeof( struct llc )) {
	    if (( m0 = m_copy( m, 0, m->m_len )) == 0 ) {
		m_freem( m );
		return( 1 );
	    }
	    if ( M_HASCL( m )) {	/* m is a cluster */
		int s = splimp();

		mclput( m );
		splx( s );
	    }

	    m0->m_next = m->m_next;
	    m->m_next = m0;
	    m->m_off = MMAXOFF - sizeof( struct llc );
	    m->m_len = sizeof( struct llc );
	} else {
	    m->m_off -= sizeof( struct llc );
	    m->m_len += sizeof( struct llc );
	}

	llc.llc_dsap = llc.llc_ssap = LLC_SNAP_LSAP;
	llc.llc_control = LLC_UI;
	bcopy( at_org_code, llc.llc_org_code, sizeof( at_org_code ));
	llc.llc_ether_type = htons( ETHERTYPE_AT );
	bcopy( &llc, mtod( m, caddr_t ), sizeof( struct llc ));
	ether_atalk[ 0 ].ef_ethertype = EF_8023_TYPE;
	return( 0 );
    } else {
	ether_atalk[ 0 ].ef_ethertype = ETHERTYPE_AT;
	return( 0 );
    }
}
