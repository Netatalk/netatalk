/*
 * $Id: zip.c,v 1.15 2009-12-13 00:31:50 didg Exp $
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
#include <sys/param.h>
#include <sys/types.h>
#include <atalk/logger.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
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
#include <atalk/zip.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atserv.h"
#include "interface.h"
#include "gate.h"
#include "zip.h"
#include "rtmp.h"
#include "list.h"
#include "multicast.h"
#include "main.h"

struct ziptab	*ziptab = NULL, *ziplast = NULL;


static int zonecheck(struct rtmptab *rtmp, struct interface *iface)
{
    struct list		*l;
    struct ziptab	*czt, *zt;
    int			cztcnt, ztcnt;

    if (( iface->i_flags & IFACE_SEED ) == 0 ) {
	return( 0 );
    }

    for ( cztcnt = 0, czt = iface->i_czt; czt; czt = czt->zt_next, cztcnt++ ) {
	for ( l = rtmp->rt_zt; l; l = l->l_next ) {
	    zt = (struct ziptab *)l->l_data;
	    if ( czt->zt_len == zt->zt_len &&
		    !strndiacasecmp( czt->zt_name, zt->zt_name, czt->zt_len )) {
		break;
	    }
	}
	if ( l == NULL ) {
	    LOG(log_error, logtype_atalkd, "zonecheck: %.*s not in zone list", czt->zt_len,
		    czt->zt_name );
	    return( -1 );	/* configured zone not found in net zones */
	}
    }

    for ( ztcnt = 0, l = rtmp->rt_zt; l; l = l->l_next, ztcnt++ )
	;

    if ( cztcnt != ztcnt ) {
	LOG(log_error, logtype_atalkd, "zonecheck: %d configured zones, %d zones found",
		cztcnt, ztcnt );
	return( -1 );		/* more net zones than configured zones */
    }

    return( 0 );
}


int zip_packet(struct atport *ap,struct sockaddr_at *from, char *data, int len)
{
    struct ziphdr	zh;
    struct atphdr	ah;
    struct interface	*iface;
    struct gate		*gate;
    struct rtmptab	*rtmp = NULL;
    struct list		*l;
    struct ziptab	*zt;
    u_short		firstnet, lastnet, index, nz;
    char		*end, zname[ 32 ], packet[ ATP_BUFSIZ ], *nzones, *lastflag;
    char		*reply, *rend, *ziphdr;
    int			zlen, n, zipop, rcnt, qcnt, zcnt, zsz;
    extern int		stabletimer;

    end = data + len;

    if ( data >= end ) {
	LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
	return 1;
    }

    /* get interface */
    iface = ap->ap_iface;

    switch( *data++ ) {
    case DDPTYPE_ZIP :
	if ( data + sizeof( struct ziphdr ) > end ) {
	    LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
	    return 1;
	}
	memcpy( &zh, data, sizeof( struct ziphdr ));
	data += sizeof( struct ziphdr );

	switch ( zh.zh_op ) {
	case ZIPOP_QUERY :
	    /* set up reply */
	    reply = packet;
	    rend = packet + sizeof( packet );
	    *reply++ = DDPTYPE_ZIP;
	    ziphdr = reply;
	    reply += 2;
	    rcnt = 0;

	    qcnt = zh.zh_count;

	    while ( data + sizeof( u_short ) <= end && qcnt-- > 0 ) {
		memcpy( &firstnet, data, sizeof( u_short ));
		data += sizeof( u_short );

		/*
		 * Look for the given network number (firstnet).
		 * Perhaps we could do better than brute force?
		 */
		for ( iface = interfaces; iface; iface = iface->i_next ) {
		    for ( rtmp = iface->i_rt; rtmp; rtmp = rtmp->rt_inext ) {
			if ( firstnet == rtmp->rt_firstnet ) {
			    break;
			}
		    }
		    if ( rtmp ) {
			break;
		    }
		}
		if ( rtmp == NULL ) {
		    continue;
		}

		/*
		 * Count the number of zones in this list, and the
		 * number of byte it will consume in a reply.
		 */
		for ( zsz = 0, zcnt = 0, l = rtmp->rt_zt; l; l = l->l_next ) {
		    zcnt++;
		    zt = (struct ziptab *)l->l_data;
		    zsz += sizeof( u_short ) + 1 + zt->zt_len;
		}

		/*
		 * We might send this list in the current reply, as the
		 * first thing in the next reply, or as an extended packet.
		 */
		if ( reply + zsz > rend ) {
		    if ( rcnt > 0 ) {
			zh.zh_op = ZIPOP_REPLY;
			zh.zh_cnt = rcnt;
			memcpy( ziphdr, &zh, sizeof( struct ziphdr ));
			if ( sendto( ap->ap_fd, packet, reply - packet, 0,
				(struct sockaddr *)from,
				sizeof( struct sockaddr_at )) < 0 ) {
			    LOG(log_error, logtype_atalkd, "zip reply sendto: %s",
				    strerror(errno) );
			}

			reply = packet + 3;
			rcnt = 0;
		    }

		    if ( reply + zsz > rend ) {
			/* ereply */
			for ( l = rtmp->rt_zt; l; l = l->l_next, rcnt++ ) {
			    zt = (struct ziptab *)l->l_data;
			    if ( reply + sizeof( u_short ) + 1 + zt->zt_len >
				    rend ) {
				zh.zh_op = ZIPOP_EREPLY;
				zh.zh_cnt = zcnt;
				memcpy( ziphdr, &zh, sizeof( struct ziphdr ));
				if ( sendto( ap->ap_fd, packet, reply - packet,
					0, (struct sockaddr *)from,
					sizeof( struct sockaddr_at )) < 0 ) {
				    LOG(log_error, logtype_atalkd, "zip reply sendto: %s",
					    strerror(errno) );
				}

				reply = packet + 3;
				rcnt = 0;
			    }

			    memcpy( reply, &firstnet, sizeof( u_short ));
			    reply += sizeof( u_short );
			    *reply++ = zt->zt_len;
			    memcpy( reply, zt->zt_name, zt->zt_len );
			    reply += zt->zt_len;
			}

			if ( rcnt > 0 ) {
			    zh.zh_op = ZIPOP_EREPLY;
			    zh.zh_cnt = zcnt;
			    memcpy( ziphdr, &zh, sizeof( struct ziphdr ));
			    if ( sendto( ap->ap_fd, packet, reply - packet, 0,
				    (struct sockaddr *)from,
				    sizeof( struct sockaddr_at )) < 0 ) {
				LOG(log_error, logtype_atalkd, "zip reply sendto: %s",
					strerror(errno) );
			    }

			    reply = packet + 3;
			    rcnt = 0;
			}
			continue;
		    }
		}

		for ( l = rtmp->rt_zt; l; l = l->l_next, rcnt++ ) {
		    zt = (struct ziptab *)l->l_data;
		    memcpy( reply, &firstnet, sizeof( u_short ));
		    reply += sizeof( u_short );
		    *reply++ = zt->zt_len;
		    memcpy( reply, zt->zt_name, zt->zt_len );
		    reply += zt->zt_len;
		}
	    }

	    if ( rcnt > 0 ) {
		zh.zh_op = ZIPOP_REPLY;
		zh.zh_cnt = rcnt;
		memcpy( ziphdr, &zh, sizeof( struct ziphdr ));
		if ( sendto( ap->ap_fd, packet, reply - packet, 0,
			(struct sockaddr *)from,
			sizeof( struct sockaddr_at )) < 0 ) {
		    LOG(log_error, logtype_atalkd, "zip reply sendto: %s",
			    strerror(errno) );
		}
	    }
	    break;

	case ZIPOP_REPLY :
	    for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
		if (( from->sat_addr.s_net == 0 ||
			gate->g_sat.sat_addr.s_net == from->sat_addr.s_net ) &&
			gate->g_sat.sat_addr.s_node == from->sat_addr.s_node ) {
		    break;
		}
	    }
	    if ( gate == NULL ) {
		LOG(log_info, logtype_atalkd, "zip reply from non-gateway %u.%u", 
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
		return 1;
	    }

	    rtmp = NULL;

	    do {
		if ( data + sizeof( u_short ) + 1 > end ) {	/* + strlen */
		    LOG(log_info, logtype_atalkd, "zip reply short (%d)", len );
		    return 1;
		}
		memcpy( &firstnet, data, sizeof( u_short ));
		data += sizeof( u_short );

		if ( rtmp && rtmp->rt_firstnet != firstnet ) {
		    /* XXX */
		    if ( rtmp->rt_gate == NULL &&
			    zonecheck( rtmp, gate->g_iface ) != 0 ) {
			LOG(log_error, logtype_atalkd, "zip_packet seed zonelist mismatch" );
			return -1;
		    }
		    rtmp->rt_flags &= ~RTMPTAB_ZIPQUERY;
		}

		/* Check if this is the interface's route. */
		if ( firstnet == gate->g_iface->i_rt->rt_firstnet ) {
		    rtmp = gate->g_iface->i_rt;
		} else {
		    for ( rtmp = gate->g_rt; rtmp; rtmp = rtmp->rt_next ) {
			if ( rtmp->rt_firstnet == firstnet ) {
			    break;
			}
		    }

		    /*
		     * Update head to this rtmp entry.
		     */
		    if ( rtmp != NULL && gate->g_rt != rtmp ) {
			gate->g_rt->rt_prev->rt_next = gate->g_rt;
			gate->g_rt = rtmp;
			rtmp->rt_prev->rt_next = NULL;
		    }
		}

		zlen = *data++;
		if ( zlen > 32 || zlen <= 0 ) {
		    LOG(log_info, logtype_atalkd, "zip reply bad packet" );
		    return 1;
		}
		if ( data + zlen > end ) {
		    LOG(log_info, logtype_atalkd, "zip reply short (%d)", len );
		    return 1;
		}
		memcpy( zname, data, zlen );
		data += zlen;

		/*
		 * We won't find any rtmp entry if the gateway is no longer
		 * telling us about the entry.
		 */
		if ( rtmp == NULL ) {
		    LOG(log_info, logtype_atalkd, "zip skip reply %u from %u.%u (no rtmp)",
			    ntohs( firstnet ), ntohs( from->sat_addr.s_net ),
			    from->sat_addr.s_node );
		/*
		 * Check if the route is still in use (the iprev check is
		 * no good if rtmp is the interface's route).
		 */
		} else if ( rtmp->rt_iprev == NULL && rtmp->rt_prev != NULL ) {
		    LOG(log_info, logtype_atalkd,
			    "zip skip reply %u-%u from %u.%u (rtmp not in use)",
			    ntohs( rtmp->rt_firstnet ),
			    ntohs( rtmp->rt_lastnet ),
			    ntohs( from->sat_addr.s_net ),
			    from->sat_addr.s_node );
		/*
		 * Check if we've got an outstanding query for this route.
		 * We will often get this, since we ask every router on a
		 * net to verify our interface's zone(s).
		 */
		} else if (( rtmp->rt_flags & RTMPTAB_ZIPQUERY ) == 0 ) {
		    LOG(log_info, logtype_atalkd,
			    "zip skip reply %u-%u from %u.%u (no query)",
			    ntohs( rtmp->rt_firstnet ),
			    ntohs( rtmp->rt_lastnet ),
			    ntohs( from->sat_addr.s_net ),
			    from->sat_addr.s_node );
		} else {
		    if (addzone( rtmp, zlen, zname ) < 0) {
		        LOG(log_error, logtype_atalkd, "zip_packet: addzone");
			return -1;
		    }
		    rtmp->rt_flags |= RTMPTAB_HASZONES;
		}
	    } while ( data < end );

	    if ( rtmp && rtmp->rt_flags & RTMPTAB_HASZONES ) {
		/* XXX */
		if ( rtmp->rt_gate == NULL &&
			zonecheck( rtmp, gate->g_iface ) != 0 ) {
		    LOG(log_error, logtype_atalkd, "zip_packet seed zonelist mismatch" );
		    return -1;
		}
		rtmp->rt_flags &= ~RTMPTAB_ZIPQUERY;
	    }
	    break;

	case ZIPOP_EREPLY :
	    for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
		if (( from->sat_addr.s_net == 0 ||
			gate->g_sat.sat_addr.s_net == from->sat_addr.s_net ) &&
			gate->g_sat.sat_addr.s_node == from->sat_addr.s_node ) {
		    break;
		}
	    }
	    if ( gate == NULL ) {
		LOG(log_info, logtype_atalkd, "zip ereply from non-gateway %u.%u", 
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
		return 1;
	    }

	    /*
	     * Note that we're not advancing "data" here.  We do that
	     * at the top of the do-while loop, below.
	     */
	    if ( data + sizeof( u_short ) + 1 > end ) {	/* + strlen */
		LOG(log_info, logtype_atalkd, "zip ereply short (%d)", len );
		return 1;
	    }
	    memcpy( &firstnet, data, sizeof( u_short ));

	    /* Check if this is the interface's route. */
	    if ( firstnet == gate->g_iface->i_rt->rt_firstnet ) {
		rtmp = gate->g_iface->i_rt;
	    } else {
		for ( rtmp = gate->g_rt; rtmp; rtmp = rtmp->rt_next ) {
		    if ( rtmp->rt_firstnet == firstnet ) {
			break;
		    }
		}
		if ( rtmp == NULL ) {
		    LOG(log_info, logtype_atalkd, "zip ereply %u from %u.%u (no rtmp)",
			    ntohs( firstnet ), ntohs( from->sat_addr.s_net ),
			    from->sat_addr.s_node );
		    return 1;
		}
		if ( rtmp->rt_iprev == NULL ) {
		    LOG(log_info, logtype_atalkd,
			    "zip ereply %u-%u from %u.%u (rtmp not in use)",
			    ntohs( rtmp->rt_firstnet ),
			    ntohs( rtmp->rt_lastnet ),
			    ntohs( from->sat_addr.s_net ),
			    from->sat_addr.s_node );
		}

		/* update head to *next* rtmp entry */
		if ( rtmp->rt_next != NULL ) {
		    gate->g_rt->rt_prev->rt_next = gate->g_rt;
		    gate->g_rt = rtmp->rt_next;
		    rtmp->rt_next = NULL;
		}
	    }

	    if (( rtmp->rt_flags & RTMPTAB_ZIPQUERY ) == 0 ) {
		LOG(log_info, logtype_atalkd, "zip ereply %u-%u from %u.%u (no query)",
			ntohs( rtmp->rt_firstnet ),
			ntohs( rtmp->rt_lastnet ),
			ntohs( from->sat_addr.s_net ),
			from->sat_addr.s_node );
		return 0;
	    }

	    do {
		/*
		 * We copy out firstnet, twice (see above).  Not
		 * a big deal, and it makes the end condition cleaner.
		 */
		if ( data + sizeof( u_short ) + 1 > end ) {	/* + strlen */
		    LOG(log_info, logtype_atalkd, "zip ereply short (%d)", len );
		    return 1;
		}
		memcpy( &firstnet, data, sizeof( u_short ));
		data += sizeof( u_short );

		/* check route */
		if ( firstnet != rtmp->rt_firstnet ) {
		    LOG(log_info, logtype_atalkd, "zip ereply with multiple nets" );
		    return 1;
		}

		zlen = *data++;
		if ( zlen > 32 || zlen <= 0 ) {
		    LOG(log_info, logtype_atalkd, "zip ereply bad zone length (%d)", zlen );
		    return 1;
		}
		if ( data + zlen > end ) {
		    LOG(log_info, logtype_atalkd, "zip ereply short (%d)", len );
		    return 1;
		}
		memcpy( zname, data, zlen );
		data += zlen;
		if (addzone( rtmp, zlen, zname ) < 0) {
		    LOG(log_error, logtype_atalkd, "zip_packet: addzone");
		    return -1;
		}
	    } while ( data < end );

	    if ( rtmp ) {
		/*
		 * Count zones for rtmptab entry.
		 */
		for ( n = 0, l = rtmp->rt_zt; l; l = l->l_next, n++ )
		    ;
		if ( n == zh.zh_count ) {
		    rtmp->rt_flags |= RTMPTAB_HASZONES;
		    /* XXX */
		    if ( rtmp->rt_gate == NULL &&
			    zonecheck( rtmp, gate->g_iface ) != 0 ) {
			LOG(log_error, logtype_atalkd, "zip_packet seed zonelist mismatch" );
			return -1;
		    }
		    rtmp->rt_flags &= ~RTMPTAB_ZIPQUERY;
		}
	    }
	    break;

	case ZIPOP_GNI :
	    /*
	     * Don't answer with bogus information.
	     */
	    if (((iface->i_flags & IFACE_ISROUTER) == 0) ||
		iface->i_rt->rt_zt == NULL ||
		( iface->i_flags & IFACE_CONFIG ) == 0 ) {
		return 0;
	    }

	    if ( zh.zh_zero != 0 || data + 2 * sizeof( u_short ) > end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }

	    memcpy( &firstnet, data, sizeof( u_short ));
	    data += sizeof( u_short );
	    memcpy( &lastnet, data, sizeof( u_short ));
	    data += sizeof( u_short );
	    if ( firstnet != 0 || lastnet != 0 || data >= end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }

	    zlen = *data++;
	    if ( zlen < 0 || zlen > 32 ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }
	    memcpy( zname, data, zlen );

	    data = packet;
	    end = data + sizeof( packet );
	    zh.zh_op = ZIPOP_GNIREPLY;
	    zh.zh_flags = 0;

	    /*
	     * Skip to the nets.  Fill in header when we're done.
	     */
	    data += 1 + sizeof( struct ziphdr );
	    memcpy( data, &iface->i_rt->rt_firstnet, sizeof( u_short ));
	    data += sizeof( u_short );
	    memcpy( data, &iface->i_rt->rt_lastnet, sizeof( u_short ));
	    data += sizeof( u_short );

	    *data++ = zlen;
	    memcpy( data, zname, zlen );
	    data += zlen;

	    /*
	     * Check if the given zone is valid.  If it's valid, just fill in
	     * the multicast address.  If it's not, fill the multicast address
	     * in with the default zone and return the default zone.
	     */
	    for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
		zt = (struct ziptab *)l->l_data;
		if ( zt->zt_len == zlen &&
			strndiacasecmp( zname, zt->zt_name, zlen ) == 0 ) {
		    break;
		}
	    }
	    if ( l == NULL ) {
		zt = (struct ziptab *)iface->i_rt->rt_zt->l_data;
		zh.zh_flags |= ZIPGNI_INVALID;
	    }

	    for ( n = 0, l = iface->i_rt->rt_zt; l; l = l->l_next, n++ )
		;
	    if ( n == 1 ) {
		zh.zh_flags |= ZIPGNI_ONEZONE;
	    }

	    /* multicast */
	    *data++ = 6;	/* sizeof ??? */
	    if (zone_bcast(zt) < 0) {
	      LOG(log_error, logtype_atalkd, "zip_packet: zone_bcast");
	      return -1;
	    }
	    memcpy(data, zt->zt_bcast, 6);
	    data += 6;

	    /*
	     * Add default zone.
	     */
	    if ( zh.zh_flags & ZIPGNI_INVALID ) {
		*data++ = zt->zt_len;
		memcpy( data, zt->zt_name, zt->zt_len );
		data += zt->zt_len;
	    }

	    /* fill in header */
	    *packet = DDPTYPE_ZIP;
	    memcpy( packet + 1, &zh, sizeof( struct ziphdr ));

	    /*
	     * If the address we received this request from isn't correct
	     * for the net we received it on, send a broadcast.
	     */
	    if ( ntohs( from->sat_addr.s_net ) <
		    ntohs( iface->i_rt->rt_firstnet ) ||
		    ntohs( from->sat_addr.s_net ) >
		    ntohs( iface->i_rt->rt_lastnet )) {
		from->sat_addr.s_net = 0;
		from->sat_addr.s_node = ATADDR_BCAST;
	    }

	    if ( sendto( ap->ap_fd, packet, data - packet, 0,
		    (struct sockaddr *)from,
		    sizeof( struct sockaddr_at )) < 0 ) {
		LOG(log_error, logtype_atalkd, "zip gni sendto %u.%u: %s",
			ntohs( from->sat_addr.s_net ), from->sat_addr.s_node,
			strerror(errno) );
		return 1;
	    }
	    break;

	case ZIPOP_GNIREPLY :
	    /*
	     * Ignore ZIP GNIReplys which are either late or unsolicited.
	     */
	    LOG(log_debug, logtype_atalkd, "zip gnireply from %u.%u (%s %x)",
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node,
		    iface->i_name, iface->i_flags );

	    if (( iface->i_flags & ( IFACE_CONFIG|IFACE_PHASE1 )) ||
		    ( iface->i_flags & IFACE_ADDR ) == 0 ) {
		LOG(log_debug, logtype_atalkd, "zip ignoring gnireply" );
		return 1;
	    }

	    if ( data + 2 * sizeof( u_short ) > end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }
	    memcpy( &firstnet, data, sizeof( u_short ));
	    data += sizeof( u_short );
	    memcpy( &lastnet, data, sizeof( u_short ));
	    data += sizeof( u_short );

	    /*
	     * We never ask for a zone, so we can get back what the
	     * default zone is.
	     */
	    if ( data >= end || data + *data > end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }
	    if ( *data++ != 0 ) {
		LOG(log_info, logtype_atalkd, "zip_packet unsolicited zone" );
		return 1;
	    }

	    /* skip multicast (should really check it) */
	    if ( data >= end || data + *data > end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }
	    data += *data + 1;

	    if ( data >= end || data + *data > end ) {
		LOG(log_info, logtype_atalkd, "zip_packet malformed packet" );
		return 1;
	    }

	    /*
	     * First, if we're not seed, we always get our zone information
	     * from the net -- we don't even save what was in the file.
	     * Second, if we are seed, we keep our zone list in the
	     * interface structure, not in the zone table.  This allows us
	     * to check that the net is giving us good zones.
	     */
	    if ( (iface->i_flags & IFACE_SEED) && iface->i_czt) {
		if ( iface->i_czt->zt_len != *data ||
			strndiacasecmp( iface->i_czt->zt_name,
			data + 1, *data ) != 0 ) {
		    LOG(log_error, logtype_atalkd, "default zone mismatch on %s",
			    iface->i_name );
		    LOG(log_error, logtype_atalkd, "%.*s != %.*s",
			    iface->i_czt->zt_len, iface->i_czt->zt_name,
			    *data, data + 1 );
		    LOG(log_error, logtype_atalkd, "Seed error! Exiting!" );
		    return -1;
		}
	    }

	    if (addzone( iface->i_rt, *data, data + 1 ) < 0) {
	        LOG(log_error, logtype_atalkd, "zip_packet: addzone");
	        return -1;
	    }
	    
	    /*
	     * The netrange we received from the router doesn't match the
	     * range we have locally. This is not a problem, unless we
	     * have seed information.
	     */
	    if ( firstnet != iface->i_rt->rt_firstnet ||
		    lastnet != iface->i_rt->rt_lastnet ) {
		if ( iface->i_flags & IFACE_SEED ) {
		    LOG(log_error, logtype_atalkd, "netrange mismatch on %s",
			    iface->i_name );
		    LOG(log_error, logtype_atalkd, "%u-%u != %u-%u",
			    ntohs( firstnet ), ntohs( lastnet ),
			    ntohs( iface->i_rt->rt_firstnet ),
			    ntohs( iface->i_rt->rt_lastnet ));
		    LOG(log_error, logtype_atalkd, "Seed error! Exiting!" );
		    return -1;
		}


		/*
		 * It is possible that we will corrupt our route database
		 * by just forcing this change.  A better solution would
		 * be to search all of our current routes, looking for
		 * this new route, and delete any old versions.  Also, we
		 * would call rtmp_delete() on the old net range, in case
		 * there is some other net which actually had that range.  XXX
		 */
		iface->i_rt->rt_firstnet = firstnet;
		iface->i_rt->rt_lastnet = lastnet;

		if ( ntohs( iface->i_addr.sat_addr.s_net ) <
			ntohs( firstnet ) ||
			ntohs( iface->i_addr.sat_addr.s_net ) >
			ntohs( lastnet )) {
		    iface->i_addr.sat_addr.s_net = 0;	/* ATADDR_ANYNET? */
		}
		setaddr( iface, IFACE_PHASE2, iface->i_addr.sat_addr.s_net,
			iface->i_addr.sat_addr.s_node, firstnet, lastnet );
		stabletimer = UNSTABLE;
	    }

	    /* add addr to loopback route */
	    if ( looproute( iface, RTMP_ADD )) { /* -1 or 1 */
		LOG(log_error, logtype_atalkd,
			"zip_packet: can't route %u.%u to loopback: %s",
			ntohs( iface->i_addr.sat_addr.s_net ),
			iface->i_addr.sat_addr.s_node,
			strerror(errno) );
		return -1;
	    }

	    LOG(log_info, logtype_atalkd, "zip_packet configured %s from %u.%u",
		    iface->i_name, ntohs( from->sat_addr.s_net ),
		    from->sat_addr.s_node );
	    iface->i_flags |= IFACE_CONFIG;
	    if ( iface == ciface ) {
		ciface = ciface->i_next;
		bootaddr( ciface );
	    }
	    break;

	case ZIPOP_NOTIFY :
#ifdef DEBUG
	    printf( "zip notify from %u.%u\n", ntohs( from->sat_addr.s_net ),
		    from->sat_addr.s_node );
#endif /* DEBUG */
	    break;

	default :
	    LOG(log_info, logtype_atalkd, "zip_packet bad zip op from %u.%u",
		    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
	}
	break;

    case DDPTYPE_ATP :
	if ( data + sizeof( struct atphdr ) > end ) {
	    LOG(log_info, logtype_atalkd, "zip atp malformed packet" );
	    return 1;
	}
	memcpy( &ah, data, sizeof( struct atphdr ));
	data += sizeof( struct atphdr );
	if ( ah.atphd_ctrlinfo != ATP_TREQ ) {
	    LOG(log_info, logtype_atalkd, "zip atp bad control" );
	    return 1;
	}
	ah.atphd_ctrlinfo = ATP_TRESP | ATP_EOM;
	if ( ah.atphd_bitmap != 1 ) {
	    LOG(log_error, logtype_atalkd, "zip atp bad bitmap" );
	    return 1;
	}
	ah.atphd_bitmap = 0;

	zipop = *data++;
	data++;
	memcpy( &index, data, sizeof( u_short ));
	data += sizeof( u_short );
	index = ntohs( index );
	if ( data != end ) {
	    LOG(log_info, logtype_atalkd, "zip atp malformed packet" );
	    return 1;
	}

	data = packet;
	end = data + sizeof( packet );
	*data++ = DDPTYPE_ATP;
	memcpy( data, &ah, sizeof( struct atphdr ));
	data += sizeof( struct atphdr );
	lastflag = data++;		/* mark and space for last flag */
	*data++ = 0;
	nzones = data;			/* mark and space for zone count */
	data += sizeof( u_short );

	switch ( zipop ) {
	case ZIPOP_GETMYZONE :
	    if ( index != 0 ) {
		LOG(log_info, logtype_atalkd, "zip atp gmz bad index" );
		return 1;
	    }

	    if ( iface->i_flags & IFACE_LOOPBACK ) {
		iface = interfaces->i_next;	/* first interface */
	    } else if ( ntohs( iface->i_rt->rt_firstnet ) >
		    ntohs( from->sat_addr.s_net ) ||
		    ntohs( iface->i_rt->rt_lastnet ) <
		    ntohs( from->sat_addr.s_net )) {
		return 0;
	    }

	    if ( iface->i_rt->rt_zt == NULL ) {
		return 0;
	    }
	    zt = (struct ziptab *)iface->i_rt->rt_zt->l_data;
	    if ( data + 1 + zt->zt_len > end ) {
		LOG(log_info, logtype_atalkd, "zip atp gmz reply too long" );
		return 1;
	    }
	    *data++ = zt->zt_len;
	    memcpy( data, zt->zt_name, zt->zt_len );
	    data += zt->zt_len;

	    *lastflag = 0;
	    nz = 1;
	    break;

	case ZIPOP_GETZONELIST :
	    for ( zt = ziptab; zt && ( index > 1 ); zt = zt->zt_next, index-- )
		;
	    for ( nz = 0; zt; zt = zt->zt_next, nz++ ) {
		if ( data + 1 + zt->zt_len > end ) {
		    break;
		}
		*data++ = zt->zt_len;
		memcpy( data, zt->zt_name, zt->zt_len );
		data += zt->zt_len;
	    }

	    *lastflag = ( zt == NULL );		/* Too clever? */
	    break;

	case ZIPOP_GETLOCALZONES :
	    if ( iface->i_flags & IFACE_LOOPBACK ) {
		iface = interfaces->i_next;	/* first interface */
	    } else if ( ntohs( iface->i_rt->rt_firstnet ) >
		    ntohs( from->sat_addr.s_net ) ||
		    ntohs( iface->i_rt->rt_lastnet ) <
		    ntohs( from->sat_addr.s_net )) {
		return 0;
	    }

	    for ( l = iface->i_rt->rt_zt; l && ( index > 1 );
		    l = l->l_next, index-- )
		;
	    for ( nz = 0; l; l = l->l_next, nz++ ) {
		zt = (struct ziptab *)l->l_data;
		if ( data + 1 + zt->zt_len > end ) {
		    break;
		}
		*data++ = zt->zt_len;
		memcpy( data, zt->zt_name, zt->zt_len );
		data += zt->zt_len;
	    }

	    *lastflag = ( l == NULL );
	    break;

	default :
	    LOG(log_info, logtype_atalkd, "zip atp bad option" );
	    return 1;
	}

	/* send reply */
	if ( nz > 0 ) {
	    nz = htons( nz );
	    memcpy( nzones, &nz, sizeof( u_short ));
	    if ( sendto( ap->ap_fd, packet, data - packet, 0,
		    (struct sockaddr *)from,
		    sizeof( struct sockaddr_at )) < 0 ) {
		LOG(log_error, logtype_atalkd, "zip atp sendto %u.%u: %s",
			ntohs( from->sat_addr.s_net ), from->sat_addr.s_node,
			strerror(errno) );
		return 1;
	    }
	}
	break;

    default :
	LOG(log_info, logtype_atalkd, "zip_packet bad ddp type from %u.%u",
		ntohs( from->sat_addr.s_net ), from->sat_addr.s_node );
	return 1;
    }

    return 0;
}

int zip_getnetinfo(struct interface *iface)
{
    struct atport	*ap;
    struct ziphdr	zh;
    struct sockaddr_at	sat;
    char		*data, packet[ 40 ];
    u_short		net;

    LOG(log_info, logtype_atalkd, "zip_getnetinfo for %s", iface->i_name );

    for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	if ( ap->ap_packet == zip_packet ) {
	    break;
	}
    }
    if ( ap == NULL ) {
	LOG(log_error, logtype_atalkd, "zip_getnetinfo can't find zip socket!" );
	return -1;
    }

    data = packet;

    *data++ = DDPTYPE_ZIP;

    zh.zh_op = ZIPOP_GNI;
    zh.zh_zero = 0;
    memcpy( data, &zh, sizeof( struct ziphdr ));
    data += sizeof( struct ziphdr );
    net = 0;
    memcpy( data, &net, sizeof( u_short ));
    data += sizeof( u_short );
    memcpy( data, &net, sizeof( u_short ));
    data += sizeof( u_short );

    /*
     * Set our requesting zone to NULL, so the response will contain
     * the default zone.
     */
    *data++ = 0;

#ifdef BSD4_4
    sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = 0;
    sat.sat_addr.s_node = ATADDR_BCAST;
    sat.sat_port = ap->ap_port;

    if ( sendto( ap->ap_fd, packet, data - packet, 0, (struct sockaddr *)&sat,
	    sizeof( struct sockaddr_at )) < 0 ) {
	LOG(log_error, logtype_atalkd, "zip_getnetinfo sendto: %s", strerror(errno) );
	return -1;
    }
    return 0;
}

struct ziptab *newzt(const int len, const char *name)
{
    struct ziptab	*zt;

    if (( zt = (struct ziptab *)calloc(1, sizeof( struct ziptab ))) == NULL ) {
	return( NULL );
    }

    zt->zt_len = len;
    if (( zt->zt_name = (char *)malloc( len )) == NULL ) {
	free(zt);
	return( NULL );
    }

    memcpy( zt->zt_name, name, len );
    return( zt );
}


/*
 * Insert at the end.  Return 1 if a mapping already exists, 0 otherwise.
 * -1 on error.
 */
static int add_list(struct list **head, void *data)
{
    struct list	*l, *l2;

    for ( l = *head; l; l = l->l_next ) {
	if ( l->l_data == data ) {
	    return( 1 );
	}
    }
    if (( l = (struct list *)malloc( sizeof( struct list ))) == NULL ) {
	LOG(log_error, logtype_atalkd, "add_list malloc: %s", strerror(errno) );
	return -1;
    }

    l->l_data = data;
    l->l_next = NULL;
    if ( *head == NULL ) {
	l->l_prev = NULL;
	*head = l;
    } else {
	/* find end of list */
	for ( l2 = *head; l2->l_next; l2 = l2->l_next )
	    ;
	l->l_prev = l2;
	l2->l_next = l;
    }
    return( 0 );
}

int addzone(struct rtmptab *rt, int len, char *zone)
{
    struct ziptab	*zt;
    int			cc, exists = 0;

    for ( zt = ziptab; zt; zt = zt->zt_next ) {
	if ( zt->zt_len == len &&
		strndiacasecmp( zt->zt_name, zone, len ) == 0 ) {
	    break;
	}
    }
    if ( zt == NULL ) {
	if (( zt = newzt( len, zone )) == NULL ) {
	    LOG(log_error, logtype_atalkd, "addzone newzt: %s", strerror(errno) );
	    return -1;
	}
	if ( ziptab == NULL ) {
	    zt->zt_prev = NULL;
	    ziptab = zt;
	} else {
	    zt->zt_prev = ziplast;
	    ziplast->zt_next = zt;
	}
	ziplast = zt;
    }

    if ((cc = add_list( &zt->zt_rt, rt )) < 0) 
      return -1;

    if (cc)
      exists++;

    if ((cc = add_list( &rt->rt_zt, zt )) < 0 )
      return -1;

    if (cc) {
        if ( !exists ) {
	    LOG(log_error, logtype_atalkd, "addzone corrupted route/zone mapping" );
	    return -1;
	}
	/*
	 * We get the repeat for local nets which have zone information
	 * already: we ask anyway, just to make sure.
	 */
	
	return 0;
    }
    if ( exists ) {
	LOG(log_error, logtype_atalkd, "addzone corrupted zone/route mapping" );
	return -1;
    }
    return 0;
}
