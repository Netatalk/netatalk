/*
 * $Id: aecho.c,v 1.9 2009-10-14 01:38:28 didg Exp $
 *
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

/*
 * AppleTalk Echo Protocol Client
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/aep.h>
#include <atalk/nbp.h>
#include <atalk/netddp.h>
#include <atalk/ddp.h>
#include <atalk/util.h>

/* FIXME/SOCKLEN_T: socklen_t is a unix98 feature */
#ifndef SOCKLEN_T
#define SOCKLEN_T unsigned int
#endif /* ! SOCKLEN_T */

static struct sockaddr_at	target;
static int			sock;
static unsigned int		nsent = 0, nrecv = 0;
static time_t			totalms = 0, minms = -1, maxms = -1;
static unsigned int     	pings = 0;

static void usage(char *);

static void done(int sig _U_)
{
    if ( nsent) {
	printf( "\n----%d.%d AEP Statistics----\n",
		ntohs( target.sat_addr.s_net ), target.sat_addr.s_node );
	printf( "%d packets sent, %d packets received, %d%% packet loss\n",
		nsent, nrecv, (( nsent - nrecv ) * 100 ) / nsent );
	if ( nrecv ) {
	    printf( "round-trip (ms)  min/avg/max = %ld/%ld/%ld\n",
		    minms, totalms / nrecv, maxms );
	}	
    }
    exit( 0 );
}
  
static void aep_send(int sig _U_)
{
    struct timeval	tv;
    char		*p, buf[ 1024 ];
    static unsigned int	seq = 0;

    p = buf;
    *p++ = DDPTYPE_AEP;
    *p++ = AEPOP_REQUEST;
    memcpy( p, &seq, sizeof( unsigned int ));
    p += sizeof( unsigned int );
    seq++;

    if ( gettimeofday( &tv, (struct timezone *)0 ) < 0 ) {
	perror( "gettimeofday" );
	exit( 1 );
    }
    memcpy( p, &tv, sizeof( struct timeval ));
    p += sizeof( struct timeval );

    if ( netddp_sendto( sock, buf, p - buf, 0, (struct sockaddr *) &target,
	    sizeof( struct sockaddr_at )) < 0 ) {
	perror( "sendto" );
	exit( 1 );
    }
    nsent++;
    if (pings && nsent > pings) done(0);
}

int main(int ac, char **av)
{
    struct servent	*se;
    struct sigaction	sv;
    struct itimerval	it;
    struct sockaddr_at	sat, saddr;
    struct timeval	tv, atv;
    struct nbpnve	nn;
    char		*obj = NULL, *type = "Workstation", *zone = "*";
    int			cc;
    SOCKLEN_T		satlen;
    unsigned int	seq;
    time_t		ms;
    char		buf[ 1024 ], *p;
    unsigned char	port;

    extern char		*optarg;
    extern int		optind;
  
    memset(&saddr, 0, sizeof(saddr));
    while (( cc = getopt( ac, av, "c:A:" )) != EOF ) {
	switch ( cc ) {
	case 'A':
	    if (!atalk_aton(optarg, &saddr.sat_addr)) {
	        fprintf(stderr, "Bad address.\n");
		exit(1);
	    }
	    break;

	  case 'c' :
	    pings = atoi( optarg );
	    break;

	default :
	    usage( av[ 0 ] );
	    exit( 1 );
	}
    }
    if ( ac - optind != 1 ) {
	usage( av[ 0 ] );
	exit( 1 );
    }
    
    /*
     * Save the port, since nbp_lookup calls getservbyname() to get the
     * nbp port.
     */
    if (( se = getservbyname( "echo", "ddp" )) == NULL ) 
       port = 4;
    else
       port = ntohs( se->s_port );

    memset( &target, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    target.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    target.sat_family = AF_APPLETALK;
    if ( !atalk_aton( av[ optind ], &target.sat_addr )) {
	if ( nbp_name( av[ optind ], &obj, &type, &zone ) || !obj ) {
	    fprintf( stderr, "Bad name: %s\n", av[ optind ] );
	    exit( 1 );
	}
	if ( nbp_lookup( obj, type, zone, &nn, 1, &saddr.sat_addr) <= 0 ) {
	    fprintf( stderr, "Can't find: %s\n", av[ optind ] );
	    exit( 1 );
	}
	memcpy( &target, &nn.nn_sat, sizeof( struct sockaddr_at ));
    }
    target.sat_port = port;

    if ((sock = netddp_open(saddr.sat_addr.s_net || saddr.sat_addr.s_node ? 
			 &saddr : NULL, NULL)) < 0) {
       perror("ddp_open");
       exit(1);
    }        

    sv.sa_handler = aep_send;
    sigemptyset( &sv.sa_mask );
    sigaddset( &sv.sa_mask, SIGINT );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, (struct sigaction *)0 ) < 0 ) {
	perror( "sigaction" );
	exit( 1 );
    }

    sv.sa_handler = done;
    sigemptyset( &sv.sa_mask );
    sigaddset( &sv.sa_mask, SIGALRM );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGINT, &sv, (struct sigaction *)0 ) < 0 ) {
	perror( "sigaction" );
	exit( 1 );
    }

    it.it_interval.tv_sec = 1L;
    it.it_interval.tv_usec = 0L;
    it.it_value.tv_sec = 1L;
    it.it_value.tv_usec = 0L;

    if ( setitimer( ITIMER_REAL, &it, (struct itimerval *)0 ) < 0 ) {
	perror( "setitimer" );
	exit( 1 );
    }

    for (;;) {
	satlen = sizeof( struct sockaddr_at );
	if (( cc = netddp_recvfrom( sock, buf, sizeof( buf ), 0, 
				    (struct sockaddr *) &sat,
				    &satlen )) < 0 ) {
	    if ( errno == EINTR ) {
		errno = 0;
		continue;
	    } else {
		perror( "recvfrom" );
		exit( 1 );
	    }
	}
	p = buf;
	if ( *p++ != DDPTYPE_AEP || *p++ != AEPOP_REPLY ) {
	    fprintf( stderr, "%s: bad packet!\n", av[ 0 ] );
	    continue;
	}
	if ( gettimeofday( &tv, (struct timezone *)0 ) < 0 ) {
	    perror( "gettimeofday" );
	    exit( 1 );
	}
	memcpy( &seq, p, sizeof( unsigned int ));
	p += sizeof( unsigned int );
	memcpy( &atv, p, sizeof( struct timeval ));
	nrecv++;
	ms = ( tv.tv_sec - atv.tv_sec ) * 1000 +
		( tv.tv_usec - atv.tv_usec ) / 1000;
	totalms += ms;
	if ( ms > maxms ) {
	    maxms = ms;
	}
	if ( ms < minms || minms == -1 ) {
	    minms = ms;
	}
	printf( "%d bytes from %u.%u: aep_seq=%d. time=%ld. ms\n",
		cc, ntohs( sat.sat_addr.s_net ), sat.sat_addr.s_node,
		seq, ms );
        if (pings && seq + 1 >= pings) done(0);
    }
}

static void usage( char * av0 )
{
    fprintf( stderr, "usage:\t%s [-A source address ] [-c count] ( addr | nbpname )\n", av0 );
    exit( 1 );
}
