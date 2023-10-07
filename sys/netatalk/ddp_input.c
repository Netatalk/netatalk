/*
 * $Id: ddp_input.c,v 1.4 2002-01-04 04:45:48 sibaz Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <atalk/logger.h>
#include <net/if.h>
#include <net/route.h>

#include "at.h"
#include "at_var.h"
#include "endian.h"
#include "ddp.h"
#include "ddp_var.h"

int		ddp_forward = 1;
int		ddp_firewall = 0;
extern int	ddp_cksum;
extern u_short	at_cksum();

/*
 * Could probably merge these two code segments a little better...
 */
atintr()
{
    struct elaphdr	*elhp, elh;
    struct ifnet	*ifp;
    struct mbuf		*m;
    struct at_ifaddr	*aa;
    int			s;

    for (;;) {
	s = splimp();

#ifdef BSD4_4
	IF_DEQUEUE( &atintrq2, m );
#else /* BSD4_4 */
	IF_DEQUEUEIF( &atintrq2, m, ifp );
#endif /* BSD4_4 */

	splx( s );

	if ( m == 0 ) {			/* no more queued packets */
	    break;
	}

#ifdef BSD4_4
	ifp = m->m_pkthdr.rcvif;
#endif /* BSD4_4 */
	for ( aa = at_ifaddr; aa; aa = aa->aa_next ) {
	    if ( aa->aa_ifp == ifp && ( aa->aa_flags & AFA_PHASE2 )) {
		break;
	    }
	}
	if ( aa == NULL ) {		/* ifp not an appletalk interface */
	    m_freem( m );
	    continue;
	}

	ddp_input( m, ifp, (struct elaphdr *)NULL, 2 );
    }

    for (;;) {
	s = splimp();

#ifdef BSD4_4
	IF_DEQUEUE( &atintrq1, m );
#else /* BSD4_4 */
	IF_DEQUEUEIF( &atintrq1, m, ifp );
#endif /* BSD4_4 */

	splx( s );

	if ( m == 0 ) {			/* no more queued packets */
	    break;
	}

#ifdef BSD4_4
	ifp = m->m_pkthdr.rcvif;
#endif /* BSD4_4 */
	for ( aa = at_ifaddr; aa; aa = aa->aa_next ) {
	    if ( aa->aa_ifp == ifp && ( aa->aa_flags & AFA_PHASE2 ) == 0 ) {
		break;
	    }
	}
	if ( aa == NULL ) {		/* ifp not an appletalk interface */
	    m_freem( m );
	    continue;
	}

	if ( m->m_len < SZ_ELAPHDR &&
		(( m = m_pullup( m, SZ_ELAPHDR )) == 0 )) {
	    ddpstat.ddps_tooshort++;
	    continue;
	}

	elhp = mtod( m, struct elaphdr *);
	m_adj( m, SZ_ELAPHDR );

	if ( elhp->el_type == ELAP_DDPEXTEND ) {
	    ddp_input( m, ifp, (struct elaphdr *)NULL, 1 );
	} else {
	    bcopy((caddr_t)elhp, (caddr_t)&elh, SZ_ELAPHDR );
	    ddp_input( m, ifp, &elh, 1 );
	}
    }
    return;
}

struct route	forwro;

ddp_input( m, ifp, elh, phase )
    struct mbuf		*m;
    struct ifnet	*ifp;
    struct elaphdr	*elh;
    int			phase;
{
    struct sockaddr_at	from, to;
    struct ddpshdr	*dsh, ddps;
    struct at_ifaddr	*aa;
    struct ddpehdr	*deh, ddpe;
#ifndef BSD4_4
    struct mbuf		*mp;
#endif /* BSD4_4 */
    struct ddpcb	*ddp;
    int			dlen, mlen;
    u_short		cksum;

    bzero( (caddr_t)&from, sizeof( struct sockaddr_at ));
    if ( elh ) {
	ddpstat.ddps_short++;

	if ( m->m_len < sizeof( struct ddpshdr ) &&
		(( m = m_pullup( m, sizeof( struct ddpshdr ))) == 0 )) {
	    ddpstat.ddps_tooshort++;
	    return;
	}

	dsh = mtod( m, struct ddpshdr *);
	bcopy( (caddr_t)dsh, (caddr_t)&ddps, sizeof( struct ddpshdr ));
	ddps.dsh_bytes = ntohl( ddps.dsh_bytes );
	dlen = ddps.dsh_len;

	to.sat_addr.s_net = 0;
	to.sat_addr.s_node = elh->el_dnode;
	to.sat_port = ddps.dsh_dport;
	from.sat_addr.s_net = 0;
	from.sat_addr.s_node = elh->el_snode;
	from.sat_port = ddps.dsh_sport;

	for ( aa = at_ifaddr; aa; aa = aa->aa_next ) {
	    if ( aa->aa_ifp == ifp && ( aa->aa_flags & AFA_PHASE2 ) == 0 &&
		    ( AA_SAT( aa )->sat_addr.s_node == to.sat_addr.s_node ||
		    to.sat_addr.s_node == ATADDR_BCAST )) {
		break;
	    }
	}
	if ( aa == NULL ) {
	    m_freem( m );
	    return;
	}
    } else {
	ddpstat.ddps_long++;

	if ( m->m_len < sizeof( struct ddpehdr ) &&
		(( m = m_pullup( m, sizeof( struct ddpehdr ))) == 0 )) {
	    ddpstat.ddps_tooshort++;
	    return;
	}

	deh = mtod( m, struct ddpehdr *);
	bcopy( (caddr_t)deh, (caddr_t)&ddpe, sizeof( struct ddpehdr ));
	ddpe.deh_bytes = ntohl( ddpe.deh_bytes );
	dlen = ddpe.deh_len;

	if (( cksum = ddpe.deh_sum ) == 0 ) {
	    ddpstat.ddps_nosum++;
	}

	from.sat_addr.s_net = ddpe.deh_snet;
	from.sat_addr.s_node = ddpe.deh_snode;
	from.sat_port = ddpe.deh_sport;
	to.sat_addr.s_net = ddpe.deh_dnet;
	to.sat_addr.s_node = ddpe.deh_dnode;
	to.sat_port = ddpe.deh_dport;

	if ( to.sat_addr.s_net == 0 ) {
	    for ( aa = at_ifaddr; aa; aa = aa->aa_next ) {
		if ( phase == 1 && ( aa->aa_flags & AFA_PHASE2 )) {
		    continue;
		}
		if ( phase == 2 && ( aa->aa_flags & AFA_PHASE2 ) == 0 ) {
		    continue;
		}
		if ( aa->aa_ifp == ifp &&
			( AA_SAT( aa )->sat_addr.s_node == to.sat_addr.s_node ||
			to.sat_addr.s_node == ATADDR_BCAST ||
			( ifp->if_flags & IFF_LOOPBACK ))) {
		    break;
		}
	    }
	} else {
	    for ( aa = at_ifaddr; aa; aa = aa->aa_next ) {
		if ( to.sat_addr.s_net == aa->aa_firstnet &&
			to.sat_addr.s_node == 0 ) {
		    break;
		}
		if (( ntohs( to.sat_addr.s_net ) < ntohs( aa->aa_firstnet ) ||
			ntohs( to.sat_addr.s_net ) > ntohs( aa->aa_lastnet )) &&
			( ntohs( to.sat_addr.s_net ) < 0xff00 ||
			ntohs( to.sat_addr.s_net ) > 0xfffe)) {
		    continue;
		}
		if ( to.sat_addr.s_node != AA_SAT( aa )->sat_addr.s_node &&
			to.sat_addr.s_node != ATADDR_BCAST ) {
		    continue;
		}
		break;
	    }
	}
    }

    /*
     * Adjust the length, removing any padding that may have been added
     * at a link layer.  We do this before we attempt to forward a packet,
     * possibly on a different media.
     */
#ifdef BSD4_4
    mlen = m->m_pkthdr.len;
#else /* BSD4_4 */
    for ( mlen = 0, mp = m; mp; mp = mp->m_next ) {
	mlen += mp->m_len;
    }
#endif /* BSD4_4 */
    if ( mlen < dlen ) {
	ddpstat.ddps_toosmall++;
	m_freem( m );
	return;
    }
    if ( mlen > dlen ) {
	m_adj( m, dlen - mlen );
    }

    /*
     * XXX Should we deliver broadcasts locally, also, or rely on the
     * link layer to give us a copy?  For the moment, the latter.
     */
    if ( aa == NULL || ( to.sat_addr.s_node == ATADDR_BCAST &&
	    aa->aa_ifp != ifp && ( ifp->if_flags & IFF_LOOPBACK ) == 0 )) {
	if ( ddp_forward == 0 ) {
	    m_freem( m );
	    return;
	}
	if ( forwro.ro_rt && ( satosat( &forwro.ro_dst )->sat_addr.s_net !=
		to.sat_addr.s_net ||
		satosat( &forwro.ro_dst )->sat_addr.s_node !=
		to.sat_addr.s_node )) {
	    RTFREE( forwro.ro_rt );
	    forwro.ro_rt = (struct rtentry *)0;
	}
	if ( forwro.ro_rt == (struct rtentry *)0 ||
	     forwro.ro_rt->rt_ifp == (struct ifnet *)0 ) {
#ifdef BSD4_4
	    forwro.ro_dst.sa_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
	    forwro.ro_dst.sa_family = AF_APPLETALK;
	    satosat( &forwro.ro_dst )->sat_addr.s_net = to.sat_addr.s_net;
	    satosat( &forwro.ro_dst )->sat_addr.s_node = to.sat_addr.s_node;
	    rtalloc( &forwro );
	}

	if ( to.sat_addr.s_net != satosat( &forwro.ro_dst )->sat_addr.s_net &&
		ddpe.deh_hops == DDP_MAXHOPS ) {
	    m_freem( m );
	    return;
	}

	if ( ddp_firewall &&
		( forwro.ro_rt == NULL || ( forwro.ro_rt->rt_ifp != ifp &&
		forwro.ro_rt->rt_ifp != at_ifaddr->aa_ifp ))) {
	    m_freem( m );
	    return;
	}

	ddpe.deh_hops++;
	ddpe.deh_bytes = htonl( ddpe.deh_bytes );
	bcopy( (caddr_t)&ddpe, (caddr_t)deh, sizeof( u_short ));
	if ( ddp_route( m, &forwro )) {
	    ddpstat.ddps_cantforward++;
	} else {
	    ddpstat.ddps_forward++;
	}
	return;
    }

#ifdef BSD4_4
    from.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    from.sat_family = AF_APPLETALK;

    if ( elh ) {
	m_adj( m, sizeof( struct ddpshdr ));
    } else {
	if ( ddp_cksum && cksum && cksum != at_cksum( m, sizeof( int ))) {
	    ddpstat.ddps_badsum++;
	    m_freem( m );
	    return;
	}
	m_adj( m, sizeof( struct ddpehdr ));
    }

    if (( ddp = ddp_search( &from, &to, aa )) == NULL ) {
	m_freem( m );
	return;
    }

    if ( sbappendaddr( &ddp->ddp_socket->so_rcv, (struct sockaddr *)&from,
	    m, (struct mbuf *)0 ) == 0 ) {
	ddpstat.ddps_nosockspace++;
	m_freem( m );
	return;
    }
    sorwakeup( ddp->ddp_socket );
}

m_printm( m )
    struct mbuf	*m;
{
    for (; m; m = m->m_next ) {
	bprint( mtod( m, char * ), m->m_len );
    }
}

#define BPXLEN	48
#define BPALEN	16
#include <ctype.h>
char	hexdig[] = "0123456789ABCDEF";

bprint( data, len )
    char	*data;
    int		len;
{
    char	xout[ BPXLEN ], aout[ BPALEN ];
    int		i = 0;

    bzero( xout, BPXLEN );
    bzero( aout, BPALEN );

    for ( ;; ) {
	if ( len < 1 ) {
	    if ( i != 0 ) {
		printf( "%s\t%s\n", xout, aout );
	    }
	    printf( "%s\n", "(end)" );
	    break;
	}

	xout[ (i*3) ] = hexdig[ ( *data & 0xf0 ) >> 4 ];
	xout[ (i*3) + 1 ] = hexdig[ *data & 0x0f ];

	if ( (u_char)*data < 0x7f && (u_char)*data > 0x20 ) {
	    aout[ i ] = *data;
	} else {
	    aout[ i ] = '.';
	}

	xout[ (i*3) + 2 ] = ' ';

	i++;
	len--;
	data++;

	if ( i > BPALEN - 2 ) {
	    printf( "%s\t%s\n", xout, aout );
	    bzero( xout, BPXLEN );
	    bzero( aout, BPALEN );
	    i = 0;
	    continue;
	}
    }
}
