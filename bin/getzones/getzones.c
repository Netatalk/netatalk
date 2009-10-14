/*
 * $Id: getzones.c,v 1.9 2009-10-14 01:38:28 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/util.h>
#include <atalk/unicode.h>
#include <atalk/zip.h>

static void print_zones(short n, char *buf);

static void usage( char *s)
{
    fprintf( stderr, "usage:\t%s [-m | -l] [address]\n", s );
    exit( 1 );
}

int main( int argc, char *argv[])
{
    struct atp_handle	*ah;
    struct atp_block	atpb;
    struct sockaddr_at	saddr;
    struct servent	*se;
    char		reqdata[4], buf[ ATP_MAXDATA ];
    struct iovec	iov;
    short		temp, index = 0;
    int			c, myzoneflg = 0, localzonesflg = 0, errflg = 0;
    extern int		optind;

    reqdata[ 0 ] = ZIPOP_GETZONELIST;

    while (( c = getopt( argc, argv, "ml" )) != EOF ) {
	switch (c) {
	case 'm':
	    if ( localzonesflg ) {
		++errflg;
	    }
	    ++myzoneflg;
	    reqdata[ 0 ] = ZIPOP_GETMYZONE;
	    break;
	case 'l':
	    if ( myzoneflg ) {
		++errflg;
	    }
	    ++localzonesflg;
	    reqdata[ 0 ] = ZIPOP_GETLOCALZONES;
	    break;
	default:
	    ++errflg;
	}
    }

    if ( errflg || argc - optind > 1 ) {
	usage( argv[ 0 ] );
    }

    memset( &saddr, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    saddr.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    saddr.sat_family = AF_APPLETALK;
    if (( se = getservbyname( "zip", "ddp" )) == NULL )
	saddr.sat_port = 6;
    else 
        saddr.sat_port = ntohs( se->s_port );

    if ( argc == optind ) {
	saddr.sat_addr.s_net = ATADDR_ANYNET;
	saddr.sat_addr.s_node = ATADDR_ANYNODE;
    } else {
	if ( !atalk_aton( argv[ optind ], &saddr.sat_addr )) {
	    fprintf( stderr, "Bad address.\n" );
	    exit( 1 );
	}
    }

    if (( ah = atp_open( ATADDR_ANYPORT, &saddr.sat_addr )) == NULL ) {
	perror( "atp_open" );
	exit( 1 );
    }

    index = ( myzoneflg ? 0 : 1 );
    reqdata[1] = 0;

    do {
	atpb.atp_saddr = &saddr;
	temp = htons( index );
	memcpy( reqdata + 2, &temp, 2 );
	atpb.atp_sreqdata = reqdata;
	atpb.atp_sreqdlen = 4;
	atpb.atp_sreqto = 2;
	atpb.atp_sreqtries = 5;

	/* send getzone request zones (or get my zone)
	*/
	if ( atp_sreq( ah, &atpb, 1, 0 ) < 0 ) {
	    perror( "atp_sreq" );
	    exit( 1 );
	}

	iov.iov_base = buf;
	iov.iov_len = ATP_MAXDATA;
	atpb.atp_rresiov = &iov;
	atpb.atp_rresiovcnt = 1;

	if ( atp_rresp( ah, &atpb ) < 0 ) {
	    perror( "atp_rresp" );
	    exit( 1 );
	}

	memcpy( &temp, (char *) iov.iov_base + 2, 2 );
	temp = ntohs( temp );
	print_zones( temp, (char *) iov.iov_base+4 );
	index += temp;
    } while ( !myzoneflg && !((char *)iov.iov_base)[ 0 ] );

    if ( atp_close( ah ) != 0 ) {
	perror( "atp_close" );
	exit( 1 );
    }

    exit( 0 );
}


/*
 * n:   number of zones in this packet
 * buf: zone length/name pairs
 */
static void print_zones( short n, char *buf )
{
    size_t zone_len;
    char *zone;

    for ( ; n--; buf += (*buf) + 1 ) {

        if ((size_t)(-1) == (zone_len = convert_string_allocate( CH_MAC,
                       CH_UNIX, buf+1, *buf, &zone)) ) {
            zone_len = *buf;
            if (( zone = strdup(buf+1)) == NULL ) {
	        perror( "strdup" );
	        exit( 1 );
            }
        }

	printf( "%.*s\n", (int)zone_len, zone );

	free(zone);
    }
}
