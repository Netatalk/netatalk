/*
 * $Id: main.c,v 1.25 2009-12-13 02:21:47 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>

/* POSIX.1 check */
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifndef WEXITSTATUS 
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif /* ! WEXITSTATUS */
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif /* ! WIFEXITED */
#ifndef WIFSTOPPED
#define WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#endif

#include <errno.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>

#include <signal.h>
#include <atalk/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/zip.h>
#include <atalk/rtmp.h>
#include <atalk/nbp.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/paths.h>
#include <atalk/util.h>

#ifdef __svr4__
#include <sys/sockio.h>
#include <termios.h>
#endif /* __svr4__ */

#include "interface.h"
#include "gate.h"
#include "list.h"
#include "rtmp.h"
#include "zip.h"
#include "nbp.h"
#include "atserv.h"
#include "main.h"

/* Forward Declarations */
int ifconfig(const char *iname, unsigned long cmd, struct sockaddr_at *sa);

/* FIXME/SOCKLEN_T: socklen_t is a unix98 feature */
#ifndef SOCKLEN_T
#define SOCKLEN_T unsigned int
#endif /* SOCKLEN_T */

#ifndef WEXITSTATUS
#define WEXITSTATUS(x)	((x).w_retcode)
#endif /* WEXITSTATUS */

/* linux has a special ioctl for appletalk device destruction.  as of
 * 2.1.57, SIOCDIFADDR works w/ linux. okay, we need to deal with the
 * fact that SIOCDIFADDR may be defined on linux despite the fact that
 * it doesn't work. */
#if !defined(SIOCDIFADDR) && defined(SIOCATALKDIFADDR)
#define SIOCDIFADDR SIOCATALKDIFADDR
#endif

#define elements(a)	(sizeof(a)/sizeof((a)[0]))

#define PKTSZ	1024

extern int aep_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len);

int		rtfd;
int 		transition = 0;
int		stabletimer, newrtmpdata = 0;

static struct atserv	atserv[] = {
    { "rtmp",		1,	rtmp_packet },		/* 0 */
    { "nbp",		2,	nbp_packet },		/* 1 */
    { "echo",		4,	aep_packet },		/* 2 */
    { "zip",		6,	zip_packet },		/* 3 */
};
static int		atservNATSERV = elements( atserv );

struct interface	*interfaces = NULL, *ciface = NULL;

static int		debug = 0, quiet = 0, chatty = 0;
static char		*configfile = NULL;
static int		ziptimeout = 0;
static int		stable = 0, noparent = 0;
static int		ninterfaces;
static int		defphase = IFACE_PHASE2;
static int		nfds = 0;
static fd_set		fds;
static char		Packet[ PKTSZ ];
static char		*version = VERSION;
static char     	*pidfile = _PATH_ATALKDLOCK;


/* from config.c */

int readconf( char * );
int getifconf( void );
int writeconf( char * );

/* this is the messiest of the bunch as atalkd can exit pretty much
 * everywhere. we delete interfaces here instead of in as_down. */
static void atalkd_exit(const int i)
{
#ifdef SIOCDIFADDR
  struct interface *iface;

  for (iface = interfaces; iface; iface = iface->i_next) {
    if (ifconfig( iface->i_name, SIOCDIFADDR, &iface->i_addr)) {
#ifdef SIOCATALKDIFADDR
#if (SIOCDIFADDR != SIOCATALKDIFADDR)
      if (!ifconfig(iface->i_name, SIOCATALKDIFADDR, &iface->i_addr)) 
	continue;
#endif /* SIOCDIFADDR != SIOCATALKDIFADDR */
#endif /* SIOCATALKIFADDR */
      LOG(log_error, logtype_atalkd, "difaddr(%u.%u): %s", 
	      ntohs(iface->i_addr.sat_addr.s_net), 
	      iface->i_addr.sat_addr.s_node, strerror(errno));
    }
#ifdef linux
    if (!(iface->i_flags & IFACE_WASALLMULTI) && (iface->i_flags & IFACE_ALLMULTI))
        ifsetallmulti(iface->i_name, 0);
#endif /* linux */
  }
#endif /* SOPCDOFADDR */

  server_unlock(pidfile);
  exit(i);
}

/* XXX need better error handling for gone interfaces, delete routes and so on 
 * moreover there's no way to put an interface back short of restarting atalkd
 * thus after the first time, silently fail
*/
static ssize_t sendto_iface(struct interface *iface, int sockfd, const void *buf, size_t len, 
                       const struct sockaddr_at	 *dest_addr)
{
    ssize_t ret = sendto( sockfd, buf, len, 0, (struct sockaddr *)dest_addr, sizeof( struct sockaddr_at ));

    if (ret < 0 ) {
        if (!(iface->i_flags & IFACE_ERROR)) {
            LOG(log_error, logtype_atalkd, "as_timer sendto %u.%u (%u): %s",
				    ntohs( dest_addr->sat_addr.s_net ),
				    dest_addr->sat_addr.s_node,
				    ntohs( iface->i_rt->rt_firstnet ),
				    strerror(errno) );
        }
        iface->i_flags |= IFACE_ERROR;
    }
    else {
        iface->i_flags &= ~IFACE_ERROR;
    }
    return ret;
}

static void as_timer(int sig _U_)
{
    struct sockaddr_at	sat;
    struct ziphdr	zh;
    struct rtmp_head	rh;
    struct rtmp_tuple	rt;
    struct atport	*ap, *zap, *rap;
    struct interface	*iface, *iface2;
    struct gate		*gate, *fgate = NULL;
    struct rtmptab	*rtmp, *frtmp;
    struct ziptab	*zt;
    char		*data, *end, packet[ ATP_BUFSIZ ];
    int			sentzipq = 0;
    int			n, cc;

    ap=zap=rap=NULL;

    memset(&sat, 0, sizeof( struct sockaddr_at ));

    for ( iface = interfaces; iface; iface = iface->i_next ) {
	if ( iface->i_flags & IFACE_LOOPBACK ) {
	    continue;
	}
	for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	    if ( ap->ap_packet == zip_packet ) {
		zap = ap;
	    }
	    if ( ap->ap_packet == rtmp_packet ) {
		rap = ap;
	    }
	}

	if (( iface->i_flags & ( IFACE_ADDR|IFACE_CONFIG|IFACE_NOROUTER )) == IFACE_ADDR ) {
	    if ( iface->i_time < 3 ) {
		if ( iface->i_flags & IFACE_PHASE1 ) {
		  if (rtmp_request( iface ) < 0) {
		      LOG(log_error, logtype_atalkd, "rtmp_request: %s", strerror(errno));
		      atalkd_exit(1);
		  }
		    newrtmpdata = 1;
		} else {
		  if (zip_getnetinfo( iface ) < 0) {
		    LOG(log_error, logtype_atalkd, "zip_getnetinfo: %s", strerror(errno));
		    atalkd_exit(1);
		  }
		  sentzipq = 1;
		}
		iface->i_time++;
	    } else {
		iface->i_flags |= IFACE_NOROUTER;
		if ((iface->i_flags & IFACE_ISROUTER)) {
		    if (( iface->i_flags & IFACE_SEED ) == 0 ) {
			/*
			 * No seed info, and we've got multiple interfaces.
			 * Wait forever.
			 */
			LOG(log_info, logtype_atalkd, "as_timer multiple interfaces, no seed" );
			LOG(log_info, logtype_atalkd, "as_timer can't configure %s", iface->i_name );
			LOG(log_info, logtype_atalkd, "as_timer waiting for router" );
			iface->i_time = 0;
			continue;
		    } else {
			/*
			 * Complete configuration for iface, and boot next
			 * interface.
			 */
			iface->i_flags |= IFACE_CONFIG;
			for ( zt = iface->i_czt; zt; zt = zt->zt_next ) {
			    if (addzone( iface->i_rt, zt->zt_len, zt->zt_name) < 0) {
			      LOG(log_error, logtype_atalkd, "addzone: %s", strerror(errno));
			      atalkd_exit(1);
			    }
			}
			if ( iface->i_rt->rt_zt ) {
			    iface->i_rt->rt_flags &= ~RTMPTAB_ZIPQUERY;
			    iface->i_rt->rt_flags |= RTMPTAB_HASZONES;
			}
			if ( iface->i_flags & IFACE_PHASE1 ) {
			    LOG(log_info, logtype_atalkd, "as_timer configured %s phase 1 from seed", iface->i_name );
			    setaddr( iface, IFACE_PHASE1,
				    iface->i_caddr.sat_addr.s_net,
				    iface->i_addr.sat_addr.s_node,
				    iface->i_caddr.sat_addr.s_net,
				    iface->i_caddr.sat_addr.s_net );
			} else {
			    LOG(log_info, logtype_atalkd, "as_timer configured %s phase 2 from seed", iface->i_name );
			}

			if ( looproute( iface, RTMP_ADD )) { /* -1 or 1 */
			    LOG(log_error, logtype_atalkd, "as_timer: can't route %u.%u to loop: %s", 
			            ntohs( iface->i_addr.sat_addr.s_net ),
				    iface->i_addr.sat_addr.s_node, strerror(errno) );
			    atalkd_exit( 1 );
			}
			if ( iface == ciface ) {
			    ciface = ciface->i_next;
			    bootaddr( ciface );
			}
		    }
		} else {
		    /*
		     * Configure for no router operation.  Wait for a route
		     * to become available in rtmp_packet().
		     */
		    LOG(log_info, logtype_atalkd, "config for no router" );
		      
		    if ( iface->i_flags & IFACE_PHASE2 ) {
			iface->i_rt->rt_firstnet = 0;
			iface->i_rt->rt_lastnet = htons( STARTUP_LASTNET );
			setaddr( iface, IFACE_PHASE2, iface->i_addr.sat_addr.s_net, iface->i_addr.sat_addr.s_node,
				0, htons( STARTUP_LASTNET ));
		    }
		    if ( looproute( iface, RTMP_ADD ) ) { /* -1 or 1 */
			LOG(log_error, logtype_atalkd, "as_timer: can't route %u.%u to loopback: %s",
				ntohs( iface->i_addr.sat_addr.s_net ), iface->i_addr.sat_addr.s_node,
				strerror(errno) );
			atalkd_exit( 1 );
		    }

		    if ( iface == ciface ) {
		      ciface = ciface->i_next;
		      bootaddr( ciface );
		    }
		}
	    }
	}

	for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
	    if ( fgate ) {
		free( (caddr_t)fgate );
		fgate = NULL;
	    }

	    n = 0;
	    data = packet + 1 + sizeof( struct ziphdr );
	    end = packet + sizeof( packet );

	    sat = gate->g_sat;
	    sat.sat_port = zap->ap_port;

	    /*
	     * Perform timeouts on routers.  If we've only got one
	     * interface, we'll use these timeouts to decide that
	     * our zone has gone away.
	     */
	    if ( ++gate->g_state >= RTMPTAB_BAD ) {
		LOG(log_info, logtype_atalkd, "as_timer gateway %u.%u down", ntohs( gate->g_sat.sat_addr.s_net ),
			gate->g_sat.sat_addr.s_node );
		rtmp = gate->g_rt;
		while ( rtmp ) {
		    frtmp = rtmp->rt_next;
		    if ( rtmp->rt_hops == RTMPHOPS_POISON || rtmp->rt_iprev == NULL ) {
			rtmp_free( rtmp );
		    } else {
			rtmp->rt_hops = RTMPHOPS_POISON;
			if ((cc = rtmp_replace( rtmp )) < 0) {
			  LOG(log_error, logtype_atalkd, "rtmp_replace: %s", strerror(errno));
			  atalkd_exit(1);
			}
			if (cc) {
			    gate->g_state = rtmp->rt_state = RTMPTAB_GOOD;
			}
		    }
		    rtmp = frtmp;
		}
		if ( gate->g_rt == NULL ) {
		    if ( gate->g_prev == NULL ) {
			gate->g_iface->i_gate = gate->g_next;
		    } else {
			gate->g_prev->g_next = gate->g_next;
		    }
		    if ( gate->g_next != NULL ) {
			gate->g_next->g_prev = gate->g_prev;
		    }
		    fgate = gate;	/* can't free here, just mark it */
		}
		/*
		 * If this is the last router on the only interface,
		 * reconfigure our netrange.  By marking the interface
		 * as having no router, we will notice when a router
		 * comes back up.
		 *
		 * XXX: actually, we always reconfigure an interface
		 * if we're not a seed router.
		 */

		if ( gate->g_iface->i_gate == NULL && ((iface->i_flags & IFACE_SEED) == 0)) {
		    gate->g_iface->i_flags |= IFACE_NOROUTER;
		    gate->g_iface->i_flags &= ~IFACE_CONFIG;

		    /* get rid of any zones associated with this iface */
		    if (gate->g_iface->i_rt->rt_zt) {
		      rtmp_delzonemap(gate->g_iface->i_rt);
		      gate->g_iface->i_rt->rt_flags &= ~RTMPTAB_HASZONES;
		    }

		    LOG(log_info, logtype_atalkd, "as_timer last gateway down" );

		    /* Set netrange to 0-fffe.  */
		    if ( gate->g_iface->i_flags & IFACE_PHASE2 ) {
			gate->g_iface->i_rt->rt_firstnet = 0;
			gate->g_iface->i_rt->rt_lastnet =
				htons( STARTUP_LASTNET );
			setaddr( iface, IFACE_PHASE2,
				iface->i_addr.sat_addr.s_net,
				iface->i_addr.sat_addr.s_node,
				0, htons( STARTUP_LASTNET ));
		    }
		}
		continue;
	    }

	    /*
	     * If we don't have a zone for our interface yet, ask for
	     * it from any router (all routers) on the interface.
	     */
	    if (( iface->i_rt->rt_flags & RTMPTAB_HASZONES ) == 0 ) {
		iface->i_rt->rt_flags |= RTMPTAB_ZIPQUERY;
		memcpy( data, &iface->i_rt->rt_firstnet, sizeof( u_short ));
		data += sizeof( u_short );
		n++;
	    }

	    rtmp = gate->g_rt;
	    while ( rtmp ) {
		/*
		 * Delete old routing tuples.
		 */
		if ( rtmp->rt_state != RTMPTAB_PERM ) {
		    rtmp->rt_state++;
		}

		/*
		 * We've not been updated for this route in a while.  If
		 * it's not in use, go ahead and remove it.  If it is in
		 * use, mark the route as down (POISON), and look for a
		 * better route.  If one is found, delete this route and use
		 * the new one.  If it's not found, mark the route as GOOD
		 * (so we'll propogate our poison) and delete it the next
		 * time it becomes BAD.
		 */
		if ( rtmp->rt_state >= RTMPTAB_BAD ) {
		    frtmp = rtmp->rt_next;
		    if ( rtmp->rt_iprev == NULL ) {	/* not in use */
			rtmp_free( rtmp );
		    } else {				/* in use */
			if ( rtmp->rt_hops == RTMPHOPS_POISON ) {
			    rtmp_free( rtmp );
			} else {
			    rtmp->rt_hops = RTMPHOPS_POISON;
			    if ((cc = rtmp_replace( rtmp )) < 0) {
			    	LOG(log_error, logtype_atalkd, "rtmp_replace: %s", strerror(errno));
			    	atalkd_exit(1);
			    }
		            if (cc)
				rtmp->rt_state = RTMPTAB_GOOD;
			}
		    }
		    rtmp = frtmp;
		    continue;
		}

		/*
		 * Do ZIP lookups.
		 */
		if ( rtmp->rt_iprev && ( rtmp->rt_flags & RTMPTAB_HASZONES ) == 0 ) {
		    if ( data + sizeof( u_short ) > end || n == 255 ) {
			/* send what we've got */
			zh.zh_op = ZIPOP_QUERY;
			zh.zh_count = n;
			cc = data - packet;
			data = packet;
			*data++ = DDPTYPE_ZIP;
			memcpy( data, &zh, sizeof( struct ziphdr ));

			sendto_iface(iface,  zap->ap_fd, packet, cc, &sat);
			sentzipq = 1;

			n = 0;
			data = packet + 1 + sizeof( struct ziphdr );
			end = packet + sizeof( packet );
		    }

		    /*
		     * rt_nzq is number of ZIP Queries we've issued for a
		     * given netrange.  If we've got ziptimeout on, we
		     * will only ask 3 times for any given netrange.
		     * Interestingly enough, since rt_nzq is a u_char,
		     * it will overflow after a while.  This means we will
		     * periodically ask for nets that we've decided not to
		     * ask about, and warn that we can't get it's zone.
		     */
		    if ( rtmp->rt_nzq++ == 3 ) {
			LOG(log_info, logtype_atalkd, "as_timer can't get zone for %u", ntohs( rtmp->rt_firstnet ));
		    }
		    if ( rtmp->rt_nzq > 3 ) {
			if ( ziptimeout ) {
			    rtmp = rtmp->rt_next;
			    continue;
			}
		    } else {
			sentzipq = 1;
		    }
		    rtmp->rt_flags |= RTMPTAB_ZIPQUERY;
		    memcpy( data, &rtmp->rt_firstnet, sizeof( u_short ));
		    data += sizeof( u_short );
		    n++;
		}
		rtmp = rtmp->rt_next;
	    }

	    /* send what we've got */
	    if ( n > 0 ) {
		zh.zh_op = ZIPOP_QUERY;
		zh.zh_count = n;
		cc = data - packet;
		data = packet;
		*data++ = DDPTYPE_ZIP;
		memcpy( data, &zh, sizeof( struct ziphdr ));

		sendto_iface( iface, zap->ap_fd, packet, cc, &sat);
	    }
	}
	if ( fgate ) {
	    free( (caddr_t)fgate );
	    fgate = NULL;
	}

	/*
	 * Send RTMP broadcasts if we have multiple interfaces or our 
	 * interface is configured as a router.  
	 */
	if ((iface->i_flags & IFACE_ISROUTER)) {
#ifdef BSD4_4
	    sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
	    sat.sat_family = AF_APPLETALK;
	    sat.sat_addr.s_net = ATADDR_ANYNET;
	    sat.sat_addr.s_node = ATADDR_BCAST;
	    sat.sat_port = rap->ap_port;

	    data = packet;
	    end = data + sizeof( packet );
	    *data++ = DDPTYPE_RTMPRD;
	    rh.rh_net = iface->i_addr.sat_addr.s_net;
	    rh.rh_nodelen = 8;
	    rh.rh_node = iface->i_addr.sat_addr.s_node;
	    memcpy( data, &rh, sizeof( struct rtmp_head ));
	    data += sizeof( struct rtmp_head );
	    n = 0;


	    if ( iface->i_flags & IFACE_PHASE1 ) {
		rt.rt_net = 0;
		rt.rt_dist = 0x82;
		memcpy( data, &rt, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;
	    } else {
		rt.rt_net = iface->i_rt->rt_firstnet;
		rt.rt_dist = 0x80;
		memcpy( data, &rt, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;

		rt.rt_net = iface->i_rt->rt_lastnet;
		rt.rt_dist = 0x82;
		memcpy( data, &rt, SZ_RTMPTUPLE );
		data += SZ_RTMPTUPLE;
	    }

	    for ( iface2 = interfaces; iface2; iface2 = iface2->i_next ) {

	      /* XXX: there used to be a bit checking against iface ==
		 iface2. also, we don't want to send an rtmp broadcast
		 to an interface that doesn't want it.  */
	        if ((( iface2->i_flags & IFACE_CONFIG ) == 0) ||
		    ((iface2->i_flags & IFACE_ISROUTER) == 0)) {
		    continue;
		}
		/*
		 * Fill in tuples.  Always send the same thing, regardless
		 * of the phase of the destination.  Routers who don't
		 * understand extended rtmp packets will toss extended
		 * tuples because their distance will have the high bit set.
		 */
		for ( rtmp = iface2->i_rt; rtmp; rtmp = rtmp->rt_inext ) {
		    /* don't broadcast routes we have no zone for */
		    if ( rtmp->rt_zt == NULL ||
			    ( rtmp->rt_flags & RTMPTAB_ZIPQUERY ) ||
			    ( rtmp->rt_flags & RTMPTAB_HASZONES ) == 0 ) {
			continue;
		    }

		    /* split horizon */
		    if (rtmp->rt_iface == iface) {
		        continue;
		    }

		    if ((( rtmp->rt_flags & RTMPTAB_EXTENDED ) &&
			    data + 2 * SZ_RTMPTUPLE > end ) ||
			    data + SZ_RTMPTUPLE > end ) {

			sendto_iface(iface,rap->ap_fd, packet, data - packet, &sat);

			if ( iface->i_flags & IFACE_PHASE2 ) {
			    data = packet + 1 + sizeof( struct rtmp_head ) + 2 * SZ_RTMPTUPLE;
			} else {
			    data = packet + 1 + sizeof( struct rtmp_head ) + SZ_RTMPTUPLE;
			}
			n = 0;
		    }

		    rt.rt_net = rtmp->rt_firstnet;
		    rt.rt_dist = rtmp->rt_hops;
		    if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
			rt.rt_dist |= 0x80;
		    }
		    memcpy( data, &rt, SZ_RTMPTUPLE );
		    data += SZ_RTMPTUPLE;

		    if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
			rt.rt_net = rtmp->rt_lastnet;
			rt.rt_dist = 0x82;
			memcpy( data, &rt, SZ_RTMPTUPLE );
			data += SZ_RTMPTUPLE;
		    }
		    n++;
		}
	    }

	    /* send rest */
	    if ( n ) {
		sendto_iface(iface, rap->ap_fd, packet, data - packet, &sat);
	    }
	}
    }

    /*
     * Check if we're stable.  Each time we configure an interface, we
     * sent stabletimer to UNSTABLE.  If stabletimer ever gets to
     * STABLEANYWAY, we give up and decide to "be" stable anyway.
     * Normally, we wait for stabletimer get <= STABLE with no new rtmp
     * data and all zip data complete.
     */
    if ( !stable ) {
	if ( stabletimer <= STABLE && !newrtmpdata && !sentzipq ) {
	    /* write out config file */
	    stable = 1;
	    writeconf( configfile );
	} else {
	    if ( stabletimer-- <= STABLEANYWAY ) {
		stable = 1;
	    }
	}
	newrtmpdata = 0;

	if ( stable && !noparent ) {
	    noparent = 1;
	    LOG(log_info, logtype_atalkd, "ready %d/%d/%d", stabletimer, newrtmpdata,
		    sentzipq );
	    if ( !debug ) {
		/*
		 * Seems like we could get here more than once...
		 */
		if ( kill( getpid(), SIGSTOP ) < 0 ) {
		    LOG(log_error, logtype_atalkd, "as_timer: kill-self failed!" );
		    atalkd_exit( 1 );
		}
	    }
	}
    }

#ifdef DEBUG
    consistency();
#endif /* DEBUG */
}

#ifdef DEBUG
/*
* Consistency check...
*/
consistency()
{
    struct rtmptab	*rtmp;
    struct list		*lr, *lz;
    struct ziptab	*zt;

    for ( zt = ziptab; zt; zt = zt->zt_next ) {
	for ( lr = zt->zt_rt; lr; lr = lr->l_next ) {
	    rtmp = (struct rtmptab *)lr->l_data;
	    if ( rtmp->rt_iprev == 0 && rtmp->rt_gate != 0 ) {
		LOG(log_error, logtype_atalkd, "%.*s has %u-%u (unused)",
			zt->zt_len, zt->zt_name, ntohs( rtmp->rt_firstnet ),
			ntohs( rtmp->rt_lastnet ));
		atalkd_exit(1);
	    }
	    for ( lz = rtmp->rt_zt; lz; lz = lz->l_next ) {
		if ( zt == (struct ziptab *)lz->l_data ) {
		    break;
		}
	    }
	    if ( lz == 0 ) {
		LOG(log_error, logtype_atalkd, "no map from %u-%u to %.*s", 
			ntohs( rtmp->rt_firstnet ),
			ntohs( rtmp->rt_lastnet ),
			zt->zt_len, zt->zt_name );
		atalkd_exit(1);
	    }
	}
    }
}
#endif /* DEBUG */

static void
as_debug(int sig _U_)
{
    struct interface	*iface;
    struct list		*l;
    struct ziptab	*zt;
    struct gate		*gate;
    struct rtmptab	*rt;
    FILE		*rtmpdebug;

    if (( rtmpdebug = fopen( _PATH_ATALKDEBUG, "w" )) == NULL ) {
	LOG(log_error, logtype_atalkd, "rtmp: %s", strerror(errno) );
    }

    for ( iface = interfaces; iface; iface = iface->i_next ) {
	fprintf( rtmpdebug, "interface %s %u.%u ", iface->i_name,
		ntohs( iface->i_addr.sat_addr.s_net ),
		iface->i_addr.sat_addr.s_node );
	if ( iface->i_flags & IFACE_PHASE1 ) {
	    putc( '1', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_PHASE2 ) {
	    putc( '2', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_RSEED ) {
	    putc( 'R', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_SEED ) {
	    putc( 'S', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_DONTROUTE ) {
	    putc( 'D', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_ADDR ) {
	    putc( 'A', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_CONFIG ) {
	    putc( 'C', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_NOROUTER ) {
	    putc( 'N', rtmpdebug );
	}
	if ( iface->i_flags & IFACE_LOOP ) {
	    putc( 'L', rtmpdebug );
	}
	putc( '\n', rtmpdebug );

	if ( iface->i_rt ) {
	    fprintf( rtmpdebug, "\t%u-%u ",
		    ntohs( iface->i_rt->rt_firstnet ),
		    ntohs( iface->i_rt->rt_lastnet ));
	    if ( iface->i_rt->rt_flags & RTMPTAB_ZIPQUERY ) {
		putc( 'q', rtmpdebug );
	    }
	    if ( iface->i_rt->rt_flags & RTMPTAB_HASZONES ) {
		putc( 'z', rtmpdebug );
	    }
	    if ( iface->i_rt->rt_flags & RTMPTAB_EXTENDED ) {
		putc( 'x', rtmpdebug );
	    }
	    putc( 'i', rtmpdebug );
	    for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
		zt = (struct ziptab *)l->l_data;
		fprintf( rtmpdebug, " '%.*s'", zt->zt_len, zt->zt_name );
	    }
	    fprintf( rtmpdebug, "\n" );
	}

	for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
	    fprintf( rtmpdebug, "gate %u.%u %X\n",
		    ntohs( gate->g_sat.sat_addr.s_net ),
		    gate->g_sat.sat_addr.s_node, gate->g_state );
	    for ( rt = gate->g_rt; rt; rt = rt->rt_next ) {
		fprintf( rtmpdebug, "\t%u-%u ", ntohs( rt->rt_firstnet ),
			ntohs( rt->rt_lastnet ));
		if ( rt->rt_flags & RTMPTAB_ZIPQUERY ) {
		    putc( 'q', rtmpdebug );
		}
		if ( rt->rt_flags & RTMPTAB_HASZONES ) {
		    putc( 'z', rtmpdebug );
		}
		if ( rt->rt_flags & RTMPTAB_EXTENDED ) {
		    putc( 'x', rtmpdebug );
		}
		if ( rt->rt_iprev ) {
		    putc( 'i', rtmpdebug );
		}
		for ( l = rt->rt_zt; l; l = l->l_next ) {
		    zt = (struct ziptab *)l->l_data;
		    fprintf( rtmpdebug, " '%.*s'", zt->zt_len, zt->zt_name );
		}
		fprintf( rtmpdebug, "\n" );
	    }
	}
    }

    fclose( rtmpdebug );
}

/*
 * Called when SIGTERM is recieved.  Remove all routes and then exit.
 */
static void
as_down(int sig _U_)
{
    struct interface	*iface;
    struct gate		*gate;
    struct rtmptab	*rt;

    for ( iface = interfaces; iface; iface = iface->i_next ) {
	for ( gate = iface->i_gate; gate; gate = gate->g_next ) {
	    for ( rt = gate->g_rt; rt; rt = rt->rt_next ) {
		if ( rt->rt_iprev ) {
		    if ( gateroute( RTMP_DEL, rt ) < 0 ) {
			LOG(log_error, logtype_atalkd, "as_down remove %u-%u failed: %s",
				ntohs( rt->rt_firstnet ),
				ntohs( rt->rt_lastnet ),
				strerror(errno) );
		    }
		}
	    }
	}
	if ( iface->i_flags & IFACE_LOOP ) {
	  if (looproute( iface, RTMP_DEL )) {
	    LOG(log_error, logtype_atalkd, "as_down remove %s %u.%u failed: %s",
		    iface->i_name, ntohs( iface->i_addr.sat_addr.s_net ),
		    iface->i_addr.sat_addr.s_node,
		    strerror(errno) );
	  }
	}
    }

    LOG(log_info, logtype_atalkd, "done" );
    atalkd_exit( 0 );
}

int main( int ac, char **av)
{
    extern char         *optarg;
    extern int          optind;

    struct sockaddr_at	sat;
    struct sigaction	sv;
    struct itimerval	it;
    sigset_t            signal_set, old_set;
    
    struct interface	*iface;
    int			status;
    struct atport	*ap;
    fd_set		readfds;
    int			i, c;
    SOCKLEN_T 		fromlen;
    char		*prog;

    while (( c = getopt( ac, av, "12qsdtf:P:v" )) != EOF ) {
	switch ( c ) {
	case '1' :
	    defphase = IFACE_PHASE1;
	    break;

	case '2' :
	    defphase = IFACE_PHASE2;
	    break;

	case 'd' :
	    debug++;
	    break;

	case 'f' :
	    configfile = optarg;
	    break;

	case 'q' :	/* don't seed */
	    quiet++;
	    break;

	case 's' :	/* seed */
	    chatty++;
	    break;

	case 't' :	/* transition */
	    transition++;
	    break;

	case 'P' :	/* pid file */
	    pidfile = optarg;
	    break;

	case 'v' :	/* version */
	    printf( "atalkd (version %s)\n", version );
	    exit ( 1 );
	    break;

	default :
	    fprintf( stderr, "Unknown option -- '%c'\n", c );
	    exit( 1 );
	}
    }
    if ( optind != ac ) {
	fprintf( stderr, "Too many arguments.\n" );
	exit( 1 );
    }

    if (( prog = strrchr( av[ 0 ], '/' )) == NULL ) {
	prog = av[ 0 ];
    } else {
	prog++;
    }

    /*
     * Configure loop back address first, so appearances of "lo0" in
     * the config file fail.  Also insures that lo0 gets configured,
     * even if there's some hangup during configuration of some
     * other interface.
     */
    if (( interfaces = newiface( LOOPIFACE )) == NULL ) {
	perror( "newiface" );
	exit( 1 );
    }
    interfaces->i_flags |= IFACE_PHASE2 | IFACE_LOOPBACK;

    /*
     * Check our initial configuration before we fork. This way we can
     * complain about syntax errors on stdout.
     *
     * Basically, if we're going to read our config file, we should read
     * it and initialize our data structures. If we're not going to read
     * our config file, use GIFCONF to initialize our data structures.
     */
    if ( readconf( configfile ) < 0 && getifconf() < 0 ) {
	fprintf( stderr, "%s: can't get interfaces, exiting.\n", prog );
	exit( 1 );
    }

    /* we need to count up our interfaces so that we can simplify things
     * later. we also need to figure out if we have more than one interface
     * that is routing. */
    for (i = 0, ninterfaces = 0, iface = interfaces; iface;
	 iface=iface->i_next) {
      if (iface->i_flags & IFACE_DONTROUTE)
	i++;
      ninterfaces++;
    }
    i = ninterfaces - i; /* number of routable interfaces */

    /*
     * At this point, we have (at least partially) initialized data
     * structures. Fill in what we can and verify that nothing is obviously
     * broken.
     */
    for (iface = interfaces; iface; iface = iface->i_next) {
	/* Apply the default phase */
	if (( iface->i_flags & IFACE_PHASE1 ) == 0 &&
		( iface->i_flags & IFACE_PHASE2 ) == 0 ) {
	    iface->i_flags |= defphase;
	}

        /* set up router flag information. if we have multiple interfaces
	 * and DONTROUTE isn't set, set up ROUTER. i is the number of 
	 * interfaces that don't have the DONTROUTE flag set. */
	if ((i > IFBASE) && ((iface->i_flags & IFACE_DONTROUTE) == 0)) {
	  iface->i_flags |= IFACE_ISROUTER;
	}

	/* Set default addresses */
	if ( iface->i_rt == NULL ) {
	    if (( iface->i_rt = newrt(iface)) == NULL ) {
		perror( "newrt" );
		exit( 1 );
	    }

	    if ( iface->i_flags & IFACE_PHASE1 ) {
		iface->i_rt->rt_firstnet = iface->i_rt->rt_lastnet =
			iface->i_caddr.sat_addr.s_net;
	    } else {
		if ( iface->i_caddr.sat_addr.s_net != ATADDR_ANYNET ||
			( iface->i_flags & IFACE_LOOPBACK )) {
		    iface->i_rt->rt_firstnet = iface->i_rt->rt_lastnet =
			    iface->i_caddr.sat_addr.s_net;
		} else {
		    iface->i_rt->rt_firstnet = htons( STARTUP_FIRSTNET );
		    iface->i_rt->rt_lastnet = htons( STARTUP_LASTNET );
		}
	    }
	}
	
	if (( iface->i_flags & IFACE_PHASE1 ) == 0 ) {
	    iface->i_rt->rt_flags |= RTMPTAB_EXTENDED;
	}

	if ( iface->i_caddr.sat_addr.s_net == ATADDR_ANYNET ) {
	    iface->i_caddr.sat_addr.s_net = iface->i_rt->rt_firstnet;
	}

	if ( debug ) {
	    dumpconfig( iface );	/* probably needs args */
	}
    }

    /*
     * A little consistency check...
     */
    if ( ninterfaces < IFBASE ) {
	fprintf( stderr, "%s: zero interfaces, exiting.\n", prog );
	exit( 1 );
    }

    /*
     * Set process name for logging
     */

    set_processname("atalkd");

    /* do this here so that we can use ifconfig */
#ifdef __svr4__
    if ( plumb() < 0 ) {
	fprintf(stderr, "can't establish STREAMS plumbing, exiting.\n" );
	atalkd_exit( 1 );
    }
#endif /* __svr4__ */

    /* delete pre-existing interface addresses. */
#ifdef SIOCDIFADDR
    for (iface = interfaces; iface; iface = iface->i_next) {
      if (ifconfig(iface->i_name, SIOCDIFADDR, &iface->i_addr)) {
#ifdef SIOCATALKDIFADDR
#if (SIOCDIFADDR != SIOCATALKDIFADDR)
	ifconfig(iface->i_name, SIOCATALKDIFADDR, &iface->i_addr);
#endif /* SIOCDIFADDR != SIOCATALKDIFADDR */
#endif /* SIOCATALKDIFADDR */
      }
    }
#endif /* SIOCDIFADDR */

    /*
     * Disassociate. The child will send itself a signal when it is
     * stable. This indicates that other processes may begin using
     * AppleTalk.
     */
    switch (i = server_lock("atalkd", pidfile, debug)) {
    case -1:
      exit(1);
    case 0: /* child */
      break;
    default: /* parent */
      /*
       * Wait for the child to send itself a SIGSTOP, after which
       * we send it a SIGCONT and exit ourself.
       */
      if ( wait3( &status, WUNTRACED, (struct rusage *)0 ) != i) {
	perror( "wait3" );	/* Child died? */
	atalkd_exit( 1 );
      }
      if ( !WIFSTOPPED( status )) {
	fprintf( stderr, "AppleTalk not up! Check your syslog for the reason." );
	if ( WIFEXITED( status )) {
	  fprintf( stderr, " Child exited with %d.\n",
		   WEXITSTATUS( status ));
	} else {
	  fprintf( stderr, " Child died.\n" );
	}
	atalkd_exit( 1 );
      }
      if ( kill(i, SIGCONT ) < 0 ) {
	perror( "kill" );
	atalkd_exit( 1 );
      }
      exit( 0 );
    }

#ifdef ultrix
    openlog( prog, LOG_PID );
#else /* ultrix */
    set_processname(prog);
    syslog_setup(log_debug, logtype_default, logoption_pid, logfacility_daemon );
#endif /* ultrix */

    LOG(log_info, logtype_atalkd, "restart (%s)", version );

    /*
     * Socket for use in routing ioctl()s. Can't add routes to our
     * interfaces until we have our routing socket.
     */
#ifdef BSD4_4
    if (( rtfd = socket( PF_ROUTE, SOCK_RAW, AF_APPLETALK )) < 0 ) {
	LOG(log_error, logtype_atalkd, "route socket: %s", strerror(errno) );
	atalkd_exit( 1 );
    }
    if ( shutdown( rtfd, 0 ) < 0 ) {
	LOG(log_error, logtype_atalkd, "route shutdown: %s", strerror(errno) );
	atalkd_exit( 1 );
    }
#else /* BSD4_4 */
    if (( rtfd = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
	LOG(log_error, logtype_atalkd, "route socket: %s", strerror(errno) );
	atalkd_exit( 1 );
    }
#endif /* BSD4_4 */

    ciface = interfaces;
    bootaddr( ciface );

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = as_down;
    sigemptyset( &sv.sa_mask );
    sigaddset( &sv.sa_mask, SIGUSR1 );
    sigaddset( &sv.sa_mask, SIGALRM );
    sigaddset( &sv.sa_mask, SIGTERM );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &sv, NULL) < 0 ) {
	LOG(log_error, logtype_atalkd, "sigterm: %s", strerror(errno) );
	atalkd_exit( 1 );
    }

    sv.sa_handler = as_debug;
    sigemptyset( &sv.sa_mask );
    sigaddset( &sv.sa_mask, SIGUSR1 );
    sigaddset( &sv.sa_mask, SIGALRM );
    sigaddset( &sv.sa_mask, SIGTERM );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &sv, NULL) < 0 ) {
	LOG(log_error, logtype_atalkd, "sigusr1: %s", strerror(errno) );
	atalkd_exit( 1 );
    }

    sv.sa_handler = as_timer;
    sigemptyset( &sv.sa_mask );
    sigaddset( &sv.sa_mask, SIGUSR1 );
    sigaddset( &sv.sa_mask, SIGALRM );
    sigaddset( &sv.sa_mask, SIGTERM );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, NULL) < 0 ) {
	LOG(log_error, logtype_atalkd, "sigalrm: %s", strerror(errno) );
	atalkd_exit( 1 );
    }

    it.it_interval.tv_sec = 10L;
    it.it_interval.tv_usec = 0L;
    it.it_value.tv_sec = 10L;
    it.it_value.tv_usec = 0L;
    if ( setitimer( ITIMER_REAL, &it, NULL) < 0 ) {
	LOG(log_error, logtype_atalkd, "setitimer: %s", strerror(errno) );
	atalkd_exit( 1 );
    }

    sigemptyset( &signal_set );
    sigaddset(&signal_set, SIGALRM);
#if 0
    /* don't block SIGTERM */
    sigaddset(&signal_set, SIGTERM);
#endif
    sigaddset(&signal_set, SIGUSR1);

    for (;;) {
	readfds = fds;
	if ( select( nfds, &readfds, NULL, NULL, NULL) < 0 ) {
	    if ( errno == EINTR ) {
		errno = 0;
		continue;
	    } else {
		LOG(log_error, logtype_atalkd, "select: %s", strerror(errno) );
		atalkd_exit( 1 );
	    }
	}

	for ( iface = interfaces; iface; iface = iface->i_next ) {
	    for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
		if ( FD_ISSET( ap->ap_fd, &readfds )) {
		    if ( ap->ap_packet ) {
			fromlen = sizeof( struct sockaddr_at );
			if (( c = recvfrom( ap->ap_fd, Packet, sizeof( Packet ),
				0, (struct sockaddr *)&sat, &fromlen )) < 0 ) {
			    LOG(log_error, logtype_atalkd, "recvfrom: %s", strerror(errno) );
			    continue;
			}
#ifdef DEBUG1
			if ( debug ) {
			    printf( "packet from %u.%u on %s (%x) %d (%d)\n",
				    ntohs( sat.sat_addr.s_net ),
				    sat.sat_addr.s_node, iface->i_name,
				    iface->i_flags, ap->ap_port, ap->ap_fd );
			    bprint( Packet, c );
			}
#endif 
			if (sigprocmask(SIG_BLOCK, &signal_set, &old_set) < 0) {
			    LOG(log_error, logtype_atalkd, "sigprocmask: %s", strerror(errno) );
			    atalkd_exit( 1 );
			}

			if (( *ap->ap_packet )( ap, &sat, Packet, c ) < 0) {
			  LOG(log_error, logtype_atalkd, "ap->ap_packet: %s", strerror(errno));
			  atalkd_exit(1);
			}

#ifdef DEBUG
			consistency();
#endif 
			if (sigprocmask(SIG_SETMASK, &old_set, NULL) < 0) {
			    LOG(log_error, logtype_atalkd, "sigprocmask old set: %s", strerror(errno) );
			    atalkd_exit( 1 );
			}

		    }
		}
	    }
	}
    }
}

/*
 * This code is called (from main(), as_timer(), zip_packet(),
 * and rtmp_packet()) to set the initial "bootstrapping" address
 * on an interface.
 */
void bootaddr(struct interface *iface)
{
    if ( iface == NULL ) {
	return;
    }

    /* consistency */
    if ( iface->i_flags & IFACE_ADDR ) {
	LOG(log_error, logtype_atalkd, "bootaddr OOPS!" );
	atalkd_exit(1);
    }

    if ( iface->i_flags & IFACE_PHASE1 ) {
	setaddr( iface, IFACE_PHASE1, 0,
		iface->i_caddr.sat_addr.s_node, 0, 0 );

	if ( iface->i_flags & IFACE_LOOPBACK ) {
	    iface->i_flags |= IFACE_CONFIG | IFACE_ADDR;
	    if ( ciface == iface ) {
		ciface = ciface->i_next;
		bootaddr( ciface );
	    }

	} else if (rtmp_request( iface ) < 0) {
	  LOG(log_error, logtype_atalkd, "bootaddr (rtmp_request): %s", strerror(errno));
	  atalkd_exit(1);
	}

    } else {
	setaddr( iface, IFACE_PHASE2, iface->i_caddr.sat_addr.s_net,
		iface->i_caddr.sat_addr.s_node,
		iface->i_rt->rt_firstnet, iface->i_rt->rt_lastnet );

	if ( iface->i_flags & IFACE_LOOPBACK ) {
	    iface->i_flags |= IFACE_CONFIG | IFACE_ADDR;
	    if ( ciface == iface ) {
		ciface = ciface->i_next;
		bootaddr( ciface );
	    }
	    
	} else if (zip_getnetinfo( iface ) < 0) {
	  LOG(log_error, logtype_atalkd, "bootaddr (zip_getnetinfo): %s", strerror(errno));
	  atalkd_exit(1);
	}
    }
    ++iface->i_time;
    iface->i_flags |= IFACE_ADDR;
    stabletimer = UNSTABLE;
}


/*
 * Change setaddr()
 * to manage the i_ports field and the fds for select().
 */
void setaddr(struct interface *iface,
             u_int8_t  phase, u_int16_t net, u_int8_t node,
	     u_int16_t first, u_int16_t last)
{
    int			i;
    struct atserv	*as;
    struct atport	*ap;
    struct servent	*se;
    struct sockaddr_at	sat;
    struct netrange	nr;

    if ( iface->i_ports == NULL ) {	/* allocate port structures */
	for ( i = 0, as = atserv; i < atservNATSERV; i++, as++ ) {
	    if (( se = getservbyname( as->as_name, "ddp" )) == NULL ) {
		LOG(log_info, logtype_atalkd, "%s: service unknown", as->as_name );
	    } else {
		as->as_port = ntohs( se->s_port );
	    }
	    if (( ap = (struct atport *)malloc( sizeof( struct atport ))) ==
		    NULL ) {
		LOG(log_error, logtype_atalkd, "malloc: %s", strerror(errno) );
		atalkd_exit( 1 );
	    }
	    ap->ap_fd = 0;
	    ap->ap_next = iface->i_ports;
	    ap->ap_iface = iface;
	    ap->ap_port = as->as_port;
	    ap->ap_packet = as->as_packet;

	    iface->i_ports = ap;
	}
    } else {				/* close ports */
	for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	    (void)close( ap->ap_fd );
	}
    }

#ifdef BSD4_4
    iface->i_addr.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    iface->i_addr.sat_family = AF_APPLETALK;
    iface->i_addr.sat_addr.s_net = net;
    iface->i_addr.sat_addr.s_node = node;

    nr.nr_phase = phase;
    nr.nr_firstnet = first;
    nr.nr_lastnet = last;
    memcpy( iface->i_addr.sat_zero, &nr, sizeof( struct netrange ));

    if ( ifconfig( iface->i_name, SIOCSIFADDR, &iface->i_addr )) {
      LOG(log_error, logtype_atalkd, "setifaddr: %s (%u-%u): %s. try specifying a \
smaller net range.", iface->i_name, ntohs(first), ntohs(last), strerror(errno));
	atalkd_exit( 1 );
    }
    if ( ifconfig( iface->i_name, SIOCGIFADDR, &iface->i_addr )) {
	LOG(log_error, logtype_atalkd, "getifaddr: %s: %s", iface->i_name, strerror(errno) );
	atalkd_exit( 1 );
    }

    /* open ports */
    i = 1; /* enable broadcasts */
#if 0
    /* useless message, no? */
    LOG(log_info, logtype_atalkd, "setsockopt incompatible w/ Solaris STREAMS module.");
#endif /* __svr4__ */
    for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	if (( ap->ap_fd = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
	    LOG(log_error, logtype_atalkd, "socket: %s", strerror(errno) );
	    atalkd_exit( 1 );
	}
#ifndef __svr4__
	setsockopt(ap->ap_fd, SOL_SOCKET, SO_BROADCAST, &i, sizeof(i));
#endif /* ! __svr4 */

	memset( &sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
	sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
	sat.sat_family = AF_APPLETALK;
	sat.sat_addr.s_net = iface->i_addr.sat_addr.s_net;
	sat.sat_addr.s_node = iface->i_addr.sat_addr.s_node;
	sat.sat_port = ap->ap_port;

	if ( bind( ap->ap_fd, (struct sockaddr *)&sat,
		sizeof( struct sockaddr_at )) < 0 ) {
	    LOG(log_error, logtype_atalkd, "bind %u.%u:%u: %s",
		    ntohs( sat.sat_addr.s_net ),
		    sat.sat_addr.s_node, sat.sat_port, strerror(errno) );
#ifdef SIOCDIFADDR
	    /* remove all interfaces if we have a problem with bind */
	    for (iface = interfaces; iface; iface = iface->i_next) {
	      if (ifconfig( iface->i_name, SIOCDIFADDR, &iface->i_addr )) {
#ifdef SIOCATALKDIFADDR
#if (SIOCDIFADDR != SIOCATALKDIFADDR)
		ifconfig( iface->i_name, SIOCATALKDIFADDR, &iface->i_addr );
#endif /* SIOCDIFADDR != SIOCATALKDIFADDR */
#endif /* SIOCATALKDIFADDR */
	      }
	    }
#endif /* SIOCDIFADDR */
	    atalkd_exit( 1 );
	}
    }

    /* recalculate nfds and fds */
    FD_ZERO( &fds );
    for ( nfds = 0, iface = interfaces; iface; iface = iface->i_next ) {
	for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
	    FD_SET( ap->ap_fd, &fds );
	    if ( ap->ap_fd > nfds ) {
		nfds = ap->ap_fd;
	    }
	}
    }
    nfds++;
}

int ifsetallmulti (const char *iname, int set)
{
    int sock;
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));

    if (( sock = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
        return( -1 );
    }

    /* get interface config */
    strlcpy(ifr.ifr_name, iname, sizeof(ifr.ifr_name));
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        close(sock);
        return (-1);
    }

    /* should we set or unset IFF_ALLMULTI */
    if (set)
	    ifr.ifr_flags |= IFF_ALLMULTI;
    else
	    ifr.ifr_flags &= ~IFF_ALLMULTI;

    /* set interface config */
    strlcpy(ifr.ifr_name, iname, sizeof(ifr.ifr_name));
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        close(sock);	
        return -1;
    }

    close(sock);
    return (0);
}

int ifconfig( const char *iname, unsigned long cmd, struct sockaddr_at *sa)
{
    struct ifreq	ifr;
    int			s;

    memset(&ifr, 0, sizeof(ifr));
    strcpy( ifr.ifr_name, iname );
    ifr.ifr_addr = *(struct sockaddr *)sa;

    if (( s = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0 ) {
	return( 1 );
    }
    if ( ioctl( s, cmd, &ifr ) < 0 ) {
	close(s);
	return( 1 );
    }
    close( s );
    if ( cmd == SIOCGIFADDR ) {
	*(struct sockaddr *)sa = ifr.ifr_addr;
    }
    return( 0 );
}

void dumpconfig( struct interface *iface)
{
    struct list		*l;

    printf( "%s", iface->i_name );
    if ( iface->i_flags & IFACE_RSEED ) {
	printf( " -router" );
    } else if ( iface->i_flags & IFACE_SEED ) {
	printf( " -seed" );
    }

    if ( iface->i_flags & IFACE_DONTROUTE) 
        printf( " -dontroute");

    printf( " -phase" );
    if ( iface->i_flags & IFACE_PHASE1 ) {
	printf( " 1" );
    } else {
	printf( " 2" );
    }
    printf( " -net %d", ntohs( iface->i_rt->rt_firstnet ));
    if ( iface->i_rt->rt_lastnet != iface->i_rt->rt_firstnet ) {
	printf( "-%d", ntohs( iface->i_rt->rt_lastnet ));
    }
    printf( " -addr %u.%u", ntohs( iface->i_addr.sat_addr.s_net ),
	    iface->i_addr.sat_addr.s_node );
    printf( " -caddr %u.%u", ntohs( iface->i_caddr.sat_addr.s_net ),
	    iface->i_caddr.sat_addr.s_node );
    for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
	printf( " -zone %.*s", ((struct ziptab *)l->l_data)->zt_len,
		((struct ziptab *)l->l_data)->zt_name );
    }
    printf( "\n" );
}

#ifdef DEBUG
void dumproutes(void)
{
    struct interface	*iface;
    struct rtmptab	*rtmp;
    struct list		*l;
    struct ziptab	*zt;

    for ( iface = interfaces; iface; iface = iface->i_next ) {
	for ( rtmp = iface->i_rt; rtmp; rtmp = rtmp->rt_inext ) {
	    if ( rtmp->rt_gate == 0 ) {
		if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
		    printf( "%u-%u", ntohs( rtmp->rt_firstnet ),
			    ntohs( rtmp->rt_lastnet ));
		} else {
		    printf( "%u", ntohs( rtmp->rt_firstnet ));
		}
	    } else {
		if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
		    printf( "%u.%u for %u-%u",
			    ntohs( rtmp->rt_gate->g_sat.sat_addr.s_net ),
			    rtmp->rt_gate->g_sat.sat_addr.s_node,
			    ntohs( rtmp->rt_firstnet ),
			    ntohs( rtmp->rt_lastnet ));
		} else {
		    printf( "%u.%u for %u",
			    ntohs( rtmp->rt_gate->g_sat.sat_addr.s_net ),
			    rtmp->rt_gate->g_sat.sat_addr.s_node,
			    ntohs( rtmp->rt_firstnet ));
		}
	    }

	    if ( rtmp->rt_iprev == 0 && rtmp != iface->i_rt ) {
		printf( " *" );
	    }

	    for ( l = rtmp->rt_zt; l; l = l->l_next ) {
		zt = (struct ziptab *)l->l_data;
		printf( " %.*s", zt->zt_len, zt->zt_name );
	    }

	    printf( "\n" );
	}
    }

    printf( "\n" );
    fflush( stdout );
}

void dumpzones(void)
{
    struct interface	*iface;
    struct rtmptab	*rtmp;
    struct list		*l;
    struct ziptab	*zt;

    for ( zt = ziptab; zt; zt = zt->zt_next ) {
	printf( "%.*s", zt->zt_len, zt->zt_name );
	for ( l = zt->zt_rt; l; l = l->l_next ) {
	    rtmp = (struct rtmptab *)l->l_data;
	    if ( rtmp->rt_flags & RTMPTAB_EXTENDED ) {
		printf( " %u-%u", ntohs( rtmp->rt_firstnet ),
			ntohs( rtmp->rt_lastnet ));
	    } else {
		printf( " %u", ntohs( rtmp->rt_firstnet ));
	    }
	    if ( rtmp->rt_iprev == 0 && rtmp->rt_gate != 0 ) {
		printf( "*" );
	    }
	}
	printf( "\n" );
    }

    printf( "\n" );
    fflush( stdout );
}
#endif /* DEBUG */
