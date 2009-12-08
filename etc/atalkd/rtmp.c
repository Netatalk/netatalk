/*
 * $Id: rtmp.c,v 1.17 2009-12-08 03:21:16 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <net/route.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>

#ifdef __svr4__
#include <sys/sockio.h>
#endif /* __svr4__ */

#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/rtmp.h>

#include "interface.h"
#include "gate.h"
#include "rtmp.h"
#include "zip.h"
#include "list.h"
#include "atserv.h"
#include "route.h"
#include "main.h"

void rtmp_delzonemap(struct rtmptab *rtmp)
{
    struct list		*lz, *flz, *lr, *flr;
    struct ziptab	*zt;

    lz = rtmp->rt_zt;
    while ( lz ) {					/* for each zone */
	zt = (struct ziptab *)lz->l_data;
	lr = zt->zt_rt;
	while ( lr ) {					/* for each route */
	    if ( (struct rtmptab *)lr->l_data == rtmp ) {
		if ( lr->l_prev == NULL ) {		/* head */
		    if ( lr->l_next == NULL ) {		/* last route in zone */
			if ( zt->zt_prev == NULL ) {
			    ziptab = zt->zt_next;
			} else {
			    zt->zt_prev->zt_next = zt->zt_next;
			}
			if ( zt->zt_next == NULL ) {
			    ziplast = zt->zt_prev;
			} else {
			    zt->zt_next->zt_prev = zt->zt_prev;
			}
			free( zt->zt_bcast );
			free( zt->zt_name );
			free( zt );
		    } else {
			zt->zt_rt = lr->l_next;
		    }
		} else {
		    lr->l_prev->l_next = lr->l_next;
		}
		if ( lr->l_next != NULL ) {
		    lr->l_next->l_prev = lr->l_prev;
		}
		flr = lr;
		lr = lr->l_next;
		free( flr );
	    } else {
		lr = lr->l_next;
	    }
	}
	flz = lz;
	lz = lz->l_next;
	free( flz );
    }
    rtmp->rt_zt = NULL;
}


/*
 * Complete configuration for phase 1 interface using RTMP information.
 */
static int rtmp_config( struct rtmp_head *rh, struct interface *iface)
{
    extern int		stabletimer;
    int cc;

    /*
     * If we're configuring a phase 2 interface, don't complete
     * configuration with RTMP.
     */
    if ( iface->i_flags & IFACE_PHASE2 ) {
	LOG(log_info, logtype_atalkd, "rtmp_config ignoring data" );
	return 0;
    }

    /*
     * Check our seed information, and reconfigure.
     */
    if ( rh->rh_net != iface->i_addr.sat_addr.s_net ) {
	if (( iface->i_flags & IFACE_SEED ) &&
		rh->rh_net != iface->i_caddr.sat_addr.s_net) {
	    LOG(log_error, logtype_atalkd, "rtmp_config net mismatch %u != %u",
		    ntohs( rh->rh_net ),
		    ntohs( iface->i_addr.sat_addr.s_net ));
	    return 1;
	}
	iface->i_addr.sat_addr.s_net = rh->rh_net;

	/*
	 * It is possible that we will corrupt our route database
	 * by just forcing this change.  XXX
	 */
	iface->i_rt->rt_firstnet = iface->i_rt->rt_lastnet = rh->rh_net;

	setaddr( iface, IFACE_PHASE1, iface->i_addr.sat_addr.s_net,
		iface->i_addr.sat_addr.s_node, rh->rh_net, rh->rh_net );
	stabletimer = UNSTABLE;
    }

    /* add addr to loopback route */
    if ((cc = looproute( iface, RTMP_ADD )) < 0 )
      return -1;

    if (cc) {
	LOG(log_error, logtype_atalkd, "rtmp_config: can't route %u.%u to loopback: %s",
		ntohs( iface->i_addr.sat_addr.s_net ),
		iface->i_addr.sat_addr.s_node,
		strerror(errno) );
    }

    LOG(log_info, logtype_atalkd, "rtmp_config configured %s", iface->i_name );
    iface->i_flags |= IFACE_CONFIG;
    if ( iface == ciface ) {
	ciface = ciface->i_next;
	bootaddr( ciface );
    }

    return 0;
}

/*
 * Delete rtmp from the per-interface in-use table, remove all
 * zone references, and remove the route from the kernel.
 */
static void rtmp_delinuse(struct rtmptab *rtmp)
{
    struct rtmptab	*irt;

    irt = rtmp->rt_gate->g_iface->i_rt;
    if ( irt->rt_inext == rtmp ) {			/* first */
	if ( rtmp->rt_iprev == rtmp ) {			/* only */
	    irt->rt_inext = NULL;
	} else {
	    irt->rt_inext = rtmp->rt_inext;
	    rtmp->rt_inext->rt_iprev = rtmp->rt_iprev;
	}
    } else {
	if ( rtmp->rt_inext == NULL ) {			/* last */
	    rtmp->rt_iprev->rt_inext = NULL;
	    irt->rt_inext->rt_iprev = rtmp->rt_iprev;
	} else {
	    rtmp->rt_iprev->rt_inext = rtmp->rt_inext;
	    rtmp->rt_inext->rt_iprev = rtmp->rt_iprev;
	}
    }
    rtmp->rt_iprev = NULL;
    rtmp->rt_inext = NULL;

    /* remove zone map */
    rtmp_delzonemap(rtmp);

    /* remove old route */
    gateroute( RTMP_DEL, rtmp );
}

/*
 * Add rtmp to the per-interface in-use table.  No verification is done...
 */
static void rtmp_addinuse( struct rtmptab *rtmp)
{
    struct rtmptab	*irt;

    gateroute( RTMP_ADD, rtmp );

    irt = rtmp->rt_gate->g_iface->i_rt;
    if ( irt->rt_inext == NULL ) {	/* empty list */
	rtmp->rt_inext = NULL;
	rtmp->rt_iprev = rtmp;
	irt->rt_inext = rtmp;
    } else {
	rtmp->rt_inext = irt->rt_inext;
	rtmp->rt_iprev = irt->rt_inext->rt_iprev;
	irt->rt_inext->rt_iprev = rtmp;
	irt->rt_inext = rtmp;
    }
}


/*
 * Change the zone mapping to replace "from" with "to".  This code assumes
 * the consistency of both the route -> zone map and the zone -> route map.
 * This is probably a bad idea.  How can we insure that the data is good
 * at this point?  What do we do if we get several copies of a route in
 * an RTMP packet?
 */
static int rtmp_copyzones( struct rtmptab *to,struct rtmptab *from)
{
    struct list		*lz, *lr;

    to->rt_zt = from->rt_zt;
    from->rt_zt = NULL;
    if ( from->rt_flags & RTMPTAB_HASZONES ) {
	to->rt_flags |= RTMPTAB_HASZONES;
    }
    for ( lz = to->rt_zt; lz; lz = lz->l_next ) {
	for ( lr = ((struct ziptab *)lz->l_data)->zt_rt; lr; lr = lr->l_next ) {
	    if ( (struct rtmptab *)lr->l_data == from ) {
		lr->l_data = (void *)to;	/* cast BS */
		break;
	    }
	}
	if ( lr == NULL ) {
	    LOG(log_error, logtype_atalkd, "rtmp_copyzones z -> r without r -> z, abort" );
	    return -1;
	}
    }

    return 0;
}


/*
 * Remove rtmp from the in-use table and the per-gate table.
 * Free any associated space.
 */
void rtmp_free( struct rtmptab *rtmp)
{
    struct gate		*gate;

    LOG(log_info, logtype_atalkd, "rtmp_free: %u-%u", ntohs(rtmp->rt_firstnet),
	   ntohs(rtmp->rt_lastnet));
    if ( rtmp->rt_iprev ) {
	rtmp_delinuse( rtmp );
    }

    /* remove from per-gate */
    gate = rtmp->rt_gate;
    if ( gate->g_rt == rtmp ) {				/* first */
	if ( rtmp->rt_prev == rtmp ) {			/* only */
	    gate->g_rt = NULL;
	} else {
	    gate->g_rt = rtmp->rt_next;
	    rtmp->rt_next->rt_prev = rtmp->rt_prev;
	}
    } else {
	if ( rtmp->rt_next == NULL ) {			/* last */
	    rtmp->rt_prev->rt_next = NULL;
	    gate->g_rt->rt_prev = rtmp->rt_prev;
	} else {
	    rtmp->rt_prev->rt_next = rtmp->rt_next;
	    rtmp->rt_next->rt_prev = rtmp->rt_prev;
	}
    }

    free( rtmp );
}


/*
 * Find a replacement for "replace".  If we can't find a replacement,
 * return 1.  If we do find a replacement, return 0. -1 on error.
 */
int rtmp_replace(struct rtmptab *replace)
{
    struct interface	*iface;
    struct gate		*gate;
    struct rtmptab	*rtmp, *found = NULL;

    LOG(log_info, logtype_atalkd, "rtmp_replace %u-%u", ntohs(replace->rt_firstnet),
	   ntohs(replace->rt_lastnet));
    for ( iface = interfaces; iface; iface = iface->i_next ) {
        if ((replace->rt_iface != iface) && 
	    ((iface->i_flags & IFACE_ISROUTER) == 0))
	  continue;

	for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
	    for ( rtmp = gate->g_rt; rtmp; rtmp = rtmp->rt_next ) {
		if ( rtmp->rt_firstnet == replace->rt_firstnet &&
			rtmp->rt_lastnet == replace->rt_lastnet ) {
		    if ( found == NULL || rtmp->rt_hops < found->rt_hops ) {
			found = rtmp;
		    }
		    break;
		}
	    }
	}
    }

    if ( found != replace ) {
	if (rtmp_copyzones( found, replace ) < 0)
	  return -1;
	rtmp_delinuse( replace );
	rtmp_addinuse( found );
	if ( replace->rt_state == RTMPTAB_BAD ) {
	    rtmp_free( replace );
	}
	return( 0 );
    } else {
	if ( replace->rt_hops == RTMPHOPS_POISON ) {
	    gateroute( RTMP_DEL, replace );
	}
	return( 1 );
    }
}


static int rtmp_new(struct rtmptab *rtmp)
{
    struct interface	*i;
    struct rtmptab	*r;
    extern int		newrtmpdata;

    newrtmpdata = 1;

    /*
     * Do we already have a gateway for this route?
     */
    for ( i = interfaces; i; i = i->i_next ) {
        if ((rtmp->rt_iface != i) && 
	    ((i->i_flags & IFACE_ISROUTER) == 0))
	  continue;

	for ( r = i->i_rt; r; r = r->rt_inext ) {
	    /* Should check RTMPTAB_EXTENDED here. XXX */
	    if (( ntohs( r->rt_firstnet ) <= ntohs( rtmp->rt_firstnet ) &&
		    ntohs( r->rt_lastnet ) >= ntohs( rtmp->rt_firstnet )) ||
		    ( ntohs( r->rt_firstnet ) <= ntohs( rtmp->rt_lastnet ) &&
		    ntohs( r->rt_lastnet ) >= ntohs( rtmp->rt_lastnet ))) {
		break;
	    }
	}
	if ( r ) {
	    break;
	}
    }

    /*
     * This part of this routine is almost never run.
     */
    if ( i ) {	/* can we get here without r being set? */
	if ( r->rt_firstnet != rtmp->rt_firstnet ||
		r->rt_lastnet != rtmp->rt_lastnet ) {
	    LOG(log_info, logtype_atalkd, "rtmp_new netrange mismatch %u-%u != %u-%u",
		    ntohs( r->rt_firstnet ), ntohs( r->rt_lastnet ),
		    ntohs( rtmp->rt_firstnet ), ntohs( rtmp->rt_lastnet ));
	    return 1;
	}

	/*
	 * Note that our whole methodology is wrong, if we want to do
	 * route "load balancing."  This entails changing our route
	 * each time we receive a tuple of equal value.  In fact, we can't
	 * do this, using our method, since we only check against in-use
	 * routes when a tuple is new from a router.
	 */
	if ( r->rt_hops < rtmp->rt_hops ) {
	    return 1;
	}

	if (rtmp_copyzones( rtmp, r ) < 0)
	  return -1;
	rtmp_delinuse( r );
    }

    rtmp_addinuse( rtmp );
    return 0;
}


int rtmp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
{
    struct rtmp_head	rh;
    struct rtmp_tuple	rt, xrt;
    struct gate		*gate;
    struct interface	*iface;
    struct interface 	*iface2;
    struct rtmptab	*rtmp;
    char		*end, packet[ ATP_BUFSIZ ];
    int                 cc;

    end = data + len;

    if ( data >= end ) {
	LOG(log_info, logtype_atalkd, "rtmp_packet no data" );
	return 1;
    }

    iface = ap->ap_iface;

    /* linux 2.6 sends broadcast queries to the first available socket 
       (in our case the last configured) 
       try to find the right one.
       Note: now a misconfigured or plugged router can broadcast
       a wrong route
    */
    for ( iface2 = interfaces; iface2; iface2 = iface2->i_next ) {
        if ( iface2->i_rt && from->sat_addr.s_net >= iface2->i_rt->rt_firstnet && 
                from->sat_addr.s_net <= iface2->i_rt->rt_lastnet) 
        {
              iface = iface2;
        }
    }
    /* end of linux 2.6 workaround */
    
    /* ignore our own packets */
    if ( from->sat_addr.s_net == iface->i_addr.sat_addr.s_net &&
	    from->sat_addr.s_node == iface->i_addr.sat_addr.s_node  ) {
	return 0;
    }

    switch( *data++ ) {
    case DDPTYPE_RTMPRD :
	/*
	 * Response and Data.
	 */
	if ( data + sizeof( struct rtmprdhdr ) > end ) {
	    LOG(log_info, logtype_atalkd, "rtmp_packet no data header" );
	    return 1;
	}
	memcpy( &rh, data, sizeof( struct rtmprdhdr ));
	data += sizeof( struct rtmprdhdr );

	/* check rh address against from address */
	if ( rh.rh_nodelen != 8 ) {
	    LOG(log_info, logtype_atalkd, "rtmp_packet bad node len (%d)", rh.rh_nodelen );
	    return 1;
	}
	if (( from->sat_addr.s_net != 0 && 
	      from->sat_addr.s_net != rh.rh_net ) ||
	      from->sat_addr.s_node != rh.rh_node ) {
	    LOG(log_info, logtype_atalkd, "rtmp_packet address mismatch" );
	    return 1;
	}

	if (( iface->i_flags & ( IFACE_ADDR|IFACE_CONFIG )) == IFACE_ADDR ) {
	    if ( iface->i_flags & IFACE_NOROUTER ) {
		/* remove addr to loopback route */
  	        if ((cc = looproute( iface, RTMP_DEL )) < 0) {
		  LOG(log_error, logtype_atalkd, "rtmp_packet: looproute");
		  return -1;
	        } 

		if (cc)
		  LOG(log_error, logtype_atalkd, "rtmp_packet: can't remove loopback: %s",
			  strerror(errno) );

		iface->i_flags &= ~IFACE_NOROUTER;
		iface->i_time = 0;
		LOG(log_info, logtype_atalkd, "rtmp_packet router has become available" );
	    }
	    if ( iface->i_flags & IFACE_PHASE1 ) {
	      if (rtmp_config( &rh, iface ) < 0) {
		  LOG(log_error, logtype_atalkd, "rtmp_packet: rtmp_config");
		  return -1;
	      }
	    } else if (zip_getnetinfo( iface ) < 0) {
	      LOG(log_error, logtype_atalkd, "rtmp_packet: zip_getnetinfo");
	      return -1;
	    }
	    return 0;
	}

	if (( iface->i_flags & IFACE_CONFIG ) == 0 ) {
	    return 0;
	}

	/*
	 * Parse first tuple.  For phase 2, verify that net is correct.
	 */
	if ( data + SZ_RTMPTUPLE > end ) {
	    LOG(log_info, logtype_atalkd, "rtmp_packet missing first tuple" );
	    return 1;
	}
	memcpy( &rt, data, SZ_RTMPTUPLE );
	data += SZ_RTMPTUPLE;

	if ( rt.rt_net == 0 ) {
	    if ( rt.rt_dist != 0x82 ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet bad phase 1 version" );
		return 1;
	    }

	    /*
	     * Grab the next tuple, since we don't want to pass the version
	     * number to the parsing code.  We're assuming that there are
	     * no extended tuples in this packet.
	     */
	    if ( data + SZ_RTMPTUPLE > end ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet missing second tuple" );
		return 1;
	    }
	    memcpy( &rt, data, SZ_RTMPTUPLE );
	    data += SZ_RTMPTUPLE;
	} else if ( rt.rt_dist & 0x80 ) {
	    if ( data + SZ_RTMPTUPLE > end ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet missing first range-end" );
		return 1;
	    }
	    memcpy( &xrt, data, SZ_RTMPTUPLE );
	    data += SZ_RTMPTUPLE;

	    if ( xrt.rt_dist != 0x82 ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet bad phase 2 version" );
		return 1;
	    }

	    /*
	     * Check for net range conflict.
	     */
	    if ( rt.rt_net != iface->i_rt->rt_firstnet ||
		    xrt.rt_net != iface->i_rt->rt_lastnet ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet interface mismatch" );
		return 1;
	    }
	} else {
#ifdef PHASE1NET
	    /*
	     * Gatorboxes put a net number in the first tuple, even on
	     * phase 1 nets.  This is wrong, but since we've got it, we
	     * might just as well check it.
	    if ( rt.rt_net != iface->i_rt->rt_firstnet ||
		    rt.rt_net != iface->i_rt->rt_lastnet ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet phase 1 interface mismatch" );
		return 1;
	    }
	     */
#else /* PHASE1NET */
	    LOG(log_info, logtype_atalkd, "rtmp_packet bad first tuple" );
	    return 1;
#endif /* PHASE1NET */
	}

	/*
	 * Find gateway.
	 */
	for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
	    if ( gate->g_sat.sat_addr.s_net == from->sat_addr.s_net &&
		    gate->g_sat.sat_addr.s_node == from->sat_addr.s_node ) {
		break;
	    }
	}
	if ( !gate ) {	/* new gateway */
	    if (( gate = (struct gate *)malloc( sizeof( struct gate ))) == NULL ) {
		LOG(log_error, logtype_atalkd, "rtmp_packet: malloc: %s", strerror(errno) );
		return -1;
	    }
	    gate->g_next = iface->i_gate;
	    gate->g_prev = NULL;
	    gate->g_rt = NULL;
	    gate->g_iface = iface;	/* need this? */
	    gate->g_sat = *from;
	    if ( iface->i_gate ) {
		iface->i_gate->g_prev = gate;
	    }
	    iface->i_gate = gate;
	    LOG(log_info, logtype_atalkd, "rtmp_packet gateway %u.%u up",
		    ntohs( gate->g_sat.sat_addr.s_net ),
		    gate->g_sat.sat_addr.s_node );
	}

	/*
	 * Reset the timeout on this gateway.  We'll remove the gateway
	 * entry, if the timeout gets to RTMPTAB_BAD.
	 */
	gate->g_state = RTMPTAB_GOOD;

	/*
	 * Parse remaining tuples.
	 */
	for (;;) {
	    /*
	     * Is route on this gateway?
	     */
	    for ( rtmp = gate->g_rt; rtmp; rtmp = rtmp->rt_next ) {
		if ( ntohs( rtmp->rt_firstnet ) <= ntohs( rt.rt_net ) &&
			ntohs( rtmp->rt_lastnet ) >= ntohs( rt.rt_net )) {
		    break;
		}
		if (( rt.rt_dist & 0x80 ) &&
			ntohs( rtmp->rt_firstnet ) <= ntohs( xrt.rt_net ) &&
			ntohs( rtmp->rt_lastnet ) >= ntohs( xrt.rt_net )) {
		    break;
		}
	    }

	    if ( rtmp ) {	/* found it */
		/*
		 * Check for range conflicts.  (This is getting a little
		 * ugly.)
		 */
		if ( rtmp->rt_firstnet != rt.rt_net ) {
		    LOG(log_info, logtype_atalkd, "rtmp_packet firstnet mismatch %u!=%u",
			    ntohs( rtmp->rt_firstnet ), ntohs( rt.rt_net ));
		    return 1;
		}
		if ( rt.rt_dist & 0x80 ) {
		    if (( rtmp->rt_flags & RTMPTAB_EXTENDED ) == 0 ) {
			LOG(log_info, logtype_atalkd, "rtmp_packet extended mismatch %u",
				ntohs( rtmp->rt_firstnet ));
			return 1;
		    }
		    if ( rtmp->rt_lastnet != xrt.rt_net ) {
			LOG(log_info, logtype_atalkd, "rtmp_packet lastnet mismatch %u!=%u",
			    ntohs( rtmp->rt_lastnet ), ntohs( xrt.rt_net ));
			return 1;
		    }
		} else {
		    if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
			LOG(log_info, logtype_atalkd, "rtmp_packet !extended mismatch %u",
				ntohs( rtmp->rt_firstnet ));
			return 1;
		    }
		    if ( rtmp->rt_lastnet != rt.rt_net ) {
			LOG(log_info, logtype_atalkd, "rtmp_packet lastnet mismatch %u!=%u",
			    ntohs( rtmp->rt_lastnet ), ntohs( rt.rt_net ));
			return 1;
		    }
		}

		rtmp->rt_state = RTMPTAB_GOOD;

		/*
		 * Check hop count.  If the count has changed, update
		 * the routing database.
		 */
		if (( rtmp->rt_hops != ( rt.rt_dist & 0x7f ) + 1 ) &&
			( rtmp->rt_hops != RTMPHOPS_POISON ||
			( rt.rt_dist & 0x7f ) + 1 <= RTMPHOPS_MAX )) {
		    if ( rtmp->rt_iprev ) {	/* route is in use */
			if ( rtmp->rt_hops > ( rt.rt_dist & 0x7f ) + 1 ) {
			    /*
			     * If this was POISON, we've deleted it from
			     * the kernel.  Add it back in.
			     */
			    if ( rtmp->rt_hops == RTMPHOPS_POISON ) {
				gateroute( RTMP_ADD, rtmp );
			    }
			    rtmp->rt_hops = ( rt.rt_dist & 0x7f ) + 1;
			} else {
			    /*
			     * Hop count has gone up for this route.
			     * Search for a new best route.  If we can't
			     * find one, just keep this route.  "poison"
			     * route are deleted in as_timer().
			     */
			    if (( rt.rt_dist & 0x7f ) + 1 > RTMPHOPS_MAX ) {
				rtmp->rt_hops = RTMPHOPS_POISON;
			    } else {
				rtmp->rt_hops = ( rt.rt_dist & 0x7f ) + 1;
			    }
			    if (rtmp_replace( rtmp ) < 0) {
			      LOG(log_error, logtype_atalkd, "rtmp_packet: rtmp_replace");
			      return -1;
			    }
			}
		    } else {			/* route not in use */
			rtmp->rt_hops = ( rt.rt_dist & 0x7f ) + 1;
			if ( rtmp->rt_hops > ( rt.rt_dist & 0x7f ) + 1 ) {
			  if (rtmp_new( rtmp ) < 0) {
			      LOG(log_error, logtype_atalkd, "rtmp_packet: rtmp_new");
			      return -1;
			  }
			}
		    }
		}

		/*
		 * Make the *next* node the head, since
		 * we're not likely to be asked for the same tuple twice
		 * in a row.
		 */
		if ( rtmp->rt_next != NULL ) {
		    gate->g_rt->rt_prev->rt_next = gate->g_rt;
		    gate->g_rt = rtmp->rt_next;
		    rtmp->rt_next = NULL;
		}
	    } else if (( rt.rt_dist & 0x7f ) + 1 > RTMPHOPS_MAX ) {
		LOG(log_info, logtype_atalkd, "rtmp_packet bad hop count from %u.%u for %u",
			ntohs( from->sat_addr.s_net ), from->sat_addr.s_node,
			ntohs( rt.rt_net ));
	    } else {		/* new for router */
		if (( rtmp = newrt(iface)) == NULL ) {
		    LOG(log_error, logtype_atalkd, "rtmp_packet: newrt: %s", strerror(errno) );
		    return -1;
		}
		rtmp->rt_firstnet = rt.rt_net;
		if ( rt.rt_dist & 0x80 ) {
		    rtmp->rt_lastnet = xrt.rt_net;
		    rtmp->rt_flags = RTMPTAB_EXTENDED;
		} else {
		    rtmp->rt_lastnet = rt.rt_net;
		}
		rtmp->rt_hops = ( rt.rt_dist & 0x7f ) + 1;
		rtmp->rt_state = RTMPTAB_GOOD;
		rtmp->rt_gate = gate;

		/*
		 * Add rtmptab entry to end of list (leave head alone).
		 */
		if ( gate->g_rt == NULL ) {
		    rtmp->rt_prev = rtmp;
		    gate->g_rt = rtmp;
		} else {
		    rtmp->rt_prev = gate->g_rt->rt_prev;
		    gate->g_rt->rt_prev->rt_next = rtmp;
		    gate->g_rt->rt_prev = rtmp;
		}
		
		if (rtmp_new( rtmp ) < 0) {
		    LOG(log_error, logtype_atalkd, "rtmp_packet: rtmp_new");
		    return -1;
		}
	    }

	    if ( data + SZ_RTMPTUPLE > end ) {
		break;
	    }
	    memcpy( &rt, data, SZ_RTMPTUPLE );
	    data += SZ_RTMPTUPLE;
	    if ( rt.rt_dist & 0x80 ) {
		if ( data + SZ_RTMPTUPLE > end ) {
		    LOG(log_info, logtype_atalkd, "rtmp_packet missing range-end" );
		    return 1;
		}
		memcpy( &xrt, data, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;
	    }
	}

	/*
	 * Make sure we've processed the whole packet.
	 */
	if ( data != end ) {
	    LOG(log_info, logtype_atalkd, "rtmp_packet length and count mismatch" );
	}
	break;

    case DDPTYPE_RTMPR :
	/*
	 * Request and RDR.
	 */
        if (((iface->i_flags & IFACE_ISROUTER) == 0) ||
	    iface->i_rt->rt_zt == NULL ||
	    ( iface->i_flags & IFACE_CONFIG ) == 0 ) {
	    return 0;
	}
	if ( *data == 1 ) {
	    data = packet;
	    *data++ = DDPTYPE_RTMPRD;
	    rh.rh_net = iface->i_addr.sat_addr.s_net;
	    rh.rh_nodelen = 8;
	    rh.rh_node = iface->i_addr.sat_addr.s_node;
	    memcpy( data, &rh, sizeof( struct rtmp_head ));
	    data += sizeof( struct rtmp_head );

	    if ( iface->i_flags & IFACE_PHASE2 ) {
		rt.rt_net = iface->i_rt->rt_firstnet;
		rt.rt_dist = 0x80;
		memcpy( data, &rt, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;

		rt.rt_net = iface->i_rt->rt_lastnet;
		rt.rt_dist = 0x82;
		memcpy( data, &rt, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;
	    }
	    if ( sendto( ap->ap_fd, packet, data - packet, 0,
		    (struct sockaddr *)from,
		    sizeof( struct sockaddr_at )) < 0 ) {
		LOG(log_error, logtype_atalkd, "as_timer sendto: %s", strerror(errno) );
	    }
	} else if ( *data == 2 || *data == 3 ) {
#ifdef DEBUG
	    printf( "rtmp_packet rdr (%d) from %u.%u\n",
		    *data, ntohs( from->sat_addr.s_net ),
		    from->sat_addr.s_node );
#endif /* DEBUG */
	} else {
	    LOG(log_info, logtype_atalkd, "rtmp_packet unknown request from %u.%u",
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
	}
	break;

    default :
	LOG(log_info, logtype_atalkd, "rtmp_packet bad ddp type from %u.%u",
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
	return 0;
    }

    return 0;
}

int rtmp_request( struct interface *iface)
{
    struct sockaddr_at	sat;
    struct atport	*ap;
    char		*data, packet[ 2 ];

    LOG(log_info, logtype_atalkd, "rtmp_request for %s", iface->i_name );

    for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	if ( ap->ap_packet == rtmp_packet ) {
	    break;
	}
    }
    if ( ap == NULL ) {
	LOG(log_error, logtype_atalkd, "rtmp_request can't find rtmp socket!" );
	return -1;
    }

    data = packet;
    *data++ = DDPTYPE_RTMPR;
    *data++ = RTMPROP_REQUEST;

    /*
     * There is a problem with the net zero "hint" hack.
     */
    memset( &sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = iface->i_addr.sat_addr.s_net;
    sat.sat_addr.s_node = ATADDR_BCAST;
    sat.sat_port = ap->ap_port;
    if ( sendto( ap->ap_fd, packet, data - packet, 0, (struct sockaddr *)&sat,
	    sizeof( struct sockaddr_at )) < 0 ) {
	LOG(log_error, logtype_atalkd, "rtmp_request sendto: %s", strerror(errno) );
	return -1;
    }
    return 0;
}


int looproute(struct interface *iface, unsigned int cmd)
{
    struct sockaddr_at	dst, loop;

    if ( cmd == RTMP_DEL && ( iface->i_flags & IFACE_LOOP ) == 0 ) {
	LOG(log_error, logtype_atalkd, "looproute panic no route" );
	return -1;
    }

    if ( cmd == RTMP_ADD && ( iface->i_flags & IFACE_LOOP )) {
	LOG(log_error, logtype_atalkd, "looproute panic two routes" );
	return -1;
    }

    memset( &dst, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    dst.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    dst.sat_family = AF_APPLETALK;
    dst.sat_addr.s_net = iface->i_addr.sat_addr.s_net;
    dst.sat_addr.s_node = iface->i_addr.sat_addr.s_node;
    memset( &loop, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    loop.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    loop.sat_family = AF_APPLETALK;
    loop.sat_addr.s_net = htons( ATADDR_ANYNET );
    loop.sat_addr.s_node = ATADDR_ANYNODE;

#ifndef BSD4_4
    if ( route( cmd,
		(struct sockaddr *) &dst,
		(struct sockaddr *) &loop,
		RTF_UP | RTF_HOST ) ) {
	return( 1 );
    }
#else /* ! BSD4_4 */
    if ( route( cmd,
	    	(struct sockaddr_at *) &dst,
		(struct sockaddr_at *) &loop,
		RTF_UP | RTF_HOST ) ) {
	return ( 1);
    }
#endif /* BSD4_4 */
    if ( cmd == RTMP_ADD ) {
	iface->i_flags |= IFACE_LOOP;
    }
    if ( cmd == RTMP_DEL ) {
	iface->i_flags &= ~IFACE_LOOP;
    }
    return( 0 );
}

int gateroute(unsigned int command, struct rtmptab *rtmp)
{
    struct sockaddr_at	dst, gate;
    unsigned short	net;

    if ( command == RTMP_DEL && ( rtmp->rt_flags & RTMPTAB_ROUTE ) == 0 ) {
	return( -1 );
    }
    if ( command == RTMP_ADD && ( rtmp->rt_flags & RTMPTAB_ROUTE )) {
	return( -1 );
    }

    net = ntohs( rtmp->rt_firstnet );
    /*
     * Since we will accept routes from gateways who advertise their
     * address as 0.YY, we must munge the gateway address we give to
     * the kernel.  Otherwise, we'll get a bunch of routes to the loop
     * back interface, and who wants that?
     */
    memset( &gate, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    gate.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    gate.sat_family = AF_APPLETALK;
    gate.sat_addr.s_net = rtmp->rt_gate->g_sat.sat_addr.s_net;
    gate.sat_addr.s_node = rtmp->rt_gate->g_sat.sat_addr.s_node;
    if ( gate.sat_addr.s_net == 0 ) {
	gate.sat_addr.s_net = net;
    }

    memset( &dst, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    dst.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    dst.sat_family = AF_APPLETALK;
    dst.sat_addr.s_node = ATADDR_ANYNODE;

    do {
	dst.sat_addr.s_net = htons( net );
#ifndef BSD4_4
	if ( route( command,
		    (struct sockaddr *) &dst,
		    (struct sockaddr *) &gate,
		    RTF_UP | RTF_GATEWAY )) {
	    LOG(log_error, logtype_atalkd, "route: %u -> %u.%u: %s", net,
		    ntohs( gate.sat_addr.s_net ), gate.sat_addr.s_node,
		    strerror(errno) );
	    continue;
	}
#else /* ! BSD4_4 */
	if ( route( command,
		    (struct sockaddr_at *) &dst,
		    (struct sockaddr_at *) &gate,
		    RTF_UP | RTF_GATEWAY )) {
	    LOG(log_error, logtype_atalkd, "route: %u -> %u.%u: %s", net,
	    	    ntohs( gate.sat_addr.s_net ), gate.sat_addr.s_node, strerror(errno) );
	    continue;
	}
#endif /* ! BSD4_4 */
    } while ( net++ < ntohs( rtmp->rt_lastnet ));

    if ( command == RTMP_ADD ) {
	rtmp->rt_flags |= RTMPTAB_ROUTE;
    }
    if ( command == RTMP_DEL ) {
	rtmp->rt_flags &= ~RTMPTAB_ROUTE;
    }

    return( 0 );
}

    struct rtmptab *
newrt(const struct interface *iface)
{
    struct rtmptab	*rtmp;

    if (( rtmp = (struct rtmptab *)calloc(1, sizeof(struct rtmptab))) == NULL ) {
	return( NULL );
    }

    rtmp->rt_iface = iface;
    return( rtmp );
}
