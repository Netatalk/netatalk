/*
 * $Id: ddp.c,v 1.3 2002-01-17 06:13:02 srittau Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/cmn_err.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/tihdr.h>
#include <sys/ddi.h>
#include <sys/ethernet.h>
#include <net/if.h>

#ifdef STDC_HEADERS
#include <strings.h>
#else
#include <string.h>
#endif

#include <netatalk/endian.h>
#include <netatalk/ddp.h>
#include <netatalk/at.h>

#include "if.h"
#include "sock.h"

    int
ddp_rput( struct atif_data *aid, mblk_t *m )
{
    struct atif_data	*daid;
    struct ddpehdr	*deh;
    struct sockaddr_at	sat;
    struct sock_data	*sd;

    if ( m->b_wptr - m->b_rptr < sizeof( struct ddpehdr )) {
	cmn_err( CE_NOTE, "ddp_rput short packet\n" );
	freemsg( m );
	return( EINVAL );
    }

    deh = (struct ddpehdr *)m->b_rptr;

    sat.sat_addr.s_net = deh->deh_dnet;
    sat.sat_addr.s_node = deh->deh_dnode;
    sat.sat_port = deh->deh_dport;

    if (( daid = if_dest( aid, &sat )) != NULL ) {
	if (( sd = sock_dest( daid, &sat )) != NULL ) {
	    if ( sd->sd_state != TS_IDLE ) {
		freemsg( m );
		return( EHOSTDOWN );
	    }
	    bzero( (caddr_t)&sat, sizeof( struct sockaddr_at ));
	    sat.sat_family = AF_APPLETALK;
	    sat.sat_addr.s_net = deh->deh_snet;
	    sat.sat_addr.s_node = deh->deh_snode;
	    sat.sat_port = deh->deh_sport;
	    adjmsg( m, sizeof( struct ddpehdr ));
	    t_unitdata_ind( WR( sd->sd_q ), m, &sat );
	} else {
	    /* toss it */
	    freemsg( m );
	    return( EHOSTDOWN );
	}
    } else {
	/* route it */
	freemsg( m );
	return( ENETUNREACH );
    }
    return( 0 );
}
