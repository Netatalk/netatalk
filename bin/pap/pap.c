/*
 * $Id: pap.c,v 1.14 2009-10-16 01:10:59 didg Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <errno.h>
#include <atalk/atp.h>
#include <atalk/pap.h>
#include <atalk/nbp.h>
#include <atalk/util.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>

#define FUCKED

#define _PATH_PAPRC	".paprc"

/* Forward Declarations */
static void updatestatus(char *s, int len);
static int send_file(int fd, ATP atp, int lastfile);

static void usage(char *path)
{
    char	*p;

    if (( p = strrchr( path, '/' )) == NULL ) {
	p = path;
    } else {
	p++;
    }
    fprintf( stderr,
	"Usage:\t%s [ -A address ] [ -c ] [ -d ] [ -e ] [ -E ] [ -p printer ]\n"
	"    [ -s statusfile ] [ -w ] [ -W ] [ FILES ]\n"
	"  -A address    - printer Appletalk address\n"
	"  -c            - take cuts (lie about wait time)\n"
	"  -d            - enable debug\n"
	"  -e            - send stdout to stderr\n"
	"  -E            - don't wait for EOF from printer\n"
	"  -p printer    - printer name\n"
	"  -s statusfile - put current printer status in statusfile\n"
	"  -w            - wait for printer status = 'waiting'\n"
	"  -W            - wait for printer status = 'idle'\n"
	"  FILES         - send FILES to printer\n"
	, p );
    exit( 2 );
}

static char *
paprc(void)
{
    static char	s[ 32 + 1 + 32 + 1 + 32 ];
    char	*name = NULL;
    FILE	*f;

    if (( f = fopen( _PATH_PAPRC, "r" )) == NULL ) {
	if ( errno == ENOENT ) {
	    return( NULL );
	} else {
	    perror( _PATH_PAPRC );
	    exit( 2 );
	}
    }
    while ( fgets( s, sizeof( s ), f ) != NULL ) {
	s[ strlen( s ) - 1 ] = '\0';	/* remove trailing newline */
	if ( *s == '#' ) {
	    continue;
	}
	name = s;
	break;
    }
    fclose( f );
    return( name );
}

static char			*printer = NULL;
static char			*status = NULL;
static int			noeof = 0;
static int			waitforprinter = 0;

static unsigned char		connid, quantum, oquantum = PAP_MAXQUANTUM;
static struct sockaddr_at	sat;

static char			cbuf[ 8 ];
static struct nbpnve		nn;
static ATP			satp;

static char		fbuf[ PAP_MAXQUANTUM ][ 4 + PAP_MAXDATA ];
static struct iovec	rfiov[ PAP_MAXQUANTUM ] = {
    { fbuf[ 0 ] + 4,	0 },
    { fbuf[ 1 ] + 4,	0 },
    { fbuf[ 2 ] + 4,	0 },
    { fbuf[ 3 ] + 4,	0 },
    { fbuf[ 4 ] + 4,	0 },
    { fbuf[ 5 ] + 4,	0 },
    { fbuf[ 6 ] + 4,	0 },
    { fbuf[ 7 ] + 4,	0 },
};

static struct iovec	sniov[ PAP_MAXQUANTUM ] = {
    { fbuf[ 0 ],	0 },
    { fbuf[ 1 ],	0 },
    { fbuf[ 2 ],	0 },
    { fbuf[ 3 ],	0 },
    { fbuf[ 4 ],	0 },
    { fbuf[ 5 ],	0 },
    { fbuf[ 6 ],	0 },
    { fbuf[ 7 ],	0 },
};

static char		nbuf[ PAP_MAXQUANTUM ][ 4 + PAP_MAXDATA ];
static struct iovec	rniov[ PAP_MAXQUANTUM ] = {
    { nbuf[ 0 ],	0 },
    { nbuf[ 1 ],	0 },
    { nbuf[ 2 ],	0 },
    { nbuf[ 3 ],	0 },
    { nbuf[ 4 ],	0 },
    { nbuf[ 5 ],	0 },
    { nbuf[ 6 ],	0 },
    { nbuf[ 7 ],	0 },
};

static struct iovec	sfiov[ PAP_MAXQUANTUM ] = {
    { nbuf[ 0 ] + 4,	0 },
    { nbuf[ 1 ] + 4,	0 },
    { nbuf[ 2 ] + 4,	0 },
    { nbuf[ 3 ] + 4,	0 },
    { nbuf[ 4 ] + 4,	0 },
    { nbuf[ 5 ] + 4,	0 },
    { nbuf[ 6 ] + 4,	0 },
    { nbuf[ 7 ] + 4,	0 },
};

static int debug;

int main( int ac, char	**av)
{
    ATP			atp;
    struct atp_block	atpb;
    int			c, err = 0, fd, cuts = 0;
    char		*obj = NULL, *type = "LaserWriter", *zone = "*";
    struct timeval	stv, tv;
    char		rbuf[ ATP_MAXDATA ];
    struct iovec	iov;
    unsigned short	waiting, result;
    int			connattempts = 10;
    int			waitforidle = 0;
    struct at_addr      addr;

    extern char		*optarg;
    extern int		optind;

    memset(&addr, 0, sizeof(addr));
    while (( c = getopt( ac, av, "dWwcep:s:EA:" )) != EOF ) {
	switch ( c ) {
#ifdef FUCKED
	case 'w' :
	    waitforprinter = 1;
	    break;

	case 'W' :
	    waitforidle = 1;
	    break;
#endif /* FUCKED */

	/* enable debugging */
	case 'd' :
	    debug++;
	    break;

	case 'c' :
	    cuts++;
	    break;

	case 'e' :	/* send stdout to stderr */
	    dup2( 2, 1 );
	    break;

	case 'p' :
	    printer = optarg;
	    break;

	case 's' :
	    status = optarg;
	    break;

	case 'E' :
	    noeof = 1;
	    break;
	   
	case 'A':
	    if (!atalk_aton(optarg, &addr)) {
	      fprintf(stderr, "Bad address.\n");
	      exit(1);
	    }
	    break;

	default :
	    err++;
	}
    }
    if ( err ) {
	usage( *av );
    }
    if ( printer == NULL && (( printer = paprc()) == NULL )) {
	fprintf( stderr, "No printer specified and ./.paprc not found.\n" );
	exit( 2 );
    }

    /*
     * Open connection.
     */
    if ( nbp_name( printer, &obj, &type, &zone ) < 0 ) {
	fprintf( stderr, "%s: Bad name\n", printer );
	exit( 2 );
    }
    if ( obj == NULL ) {
	fprintf( stderr, "%s: Bad name\n", printer );
	exit( 2 );
    }

    if ( nbp_lookup( obj, type, zone, &nn, 1, &addr ) <= 0 ) {
	if ( errno != 0 ) {
	    perror( "nbp_lookup" );
	    exit( 2 );
	}
	fprintf( stderr, "%s:%s@%s: NBP Lookup failed\n", obj, type, zone );
	exit( 1 );
    }

    if ( isatty( 1 )) {
	printf( "Trying %u.%d:%d ...\n", ntohs( nn.nn_sat.sat_addr.s_net ),
		nn.nn_sat.sat_addr.s_node, nn.nn_sat.sat_port );
    }

    if (( atp = atp_open( ATADDR_ANYPORT, &addr )) == NULL ) {
	perror( "atp_open" );
	exit( 2 );
    }
    if (( satp = atp_open( ATADDR_ANYPORT, &addr )) == NULL ) {
	perror( "atp_open" );
	exit( 2 );
    }

    while ( waitforidle ) {
	char	st_buf[ 1024 ];	/* XXX too big */

	cbuf[ 0 ] = 0;
	cbuf[ 1 ] = PAP_SENDSTATUS;
	cbuf[ 2 ] = cbuf[ 3 ] = 0;
	atpb.atp_saddr = &nn.nn_sat;
	atpb.atp_sreqdata = cbuf;
	atpb.atp_sreqdlen = 4;	/* bytes in SendStatus request */
	atpb.atp_sreqto = 2;		/* retry timer */
	atpb.atp_sreqtries = 5;		/* retry count */
	if ( atp_sreq( satp, &atpb, 1, 0 ) < 0 ) {
	    perror( "atp_sreq" );
	    exit( 1 );
	}

	if(debug){ printf( "SENDSTATUS >\n" ), fflush( stdout );}

	atpb.atp_saddr = &nn.nn_sat;
	rniov[ 0 ].iov_len = PAP_MAXDATA + 4;
	atpb.atp_rresiov = rniov;
	atpb.atp_rresiovcnt = 1;
	if ( atp_rresp( satp, &atpb ) < 0 ) {
	    perror( "atp_rresp" );
	    continue;
	}

#ifndef NONZEROSTATUS
	/*
	 * The stinking LaserWriter IINTX puts crap in this
	 * field.
	 */
	if ( ((char *)rniov[ 0 ].iov_base)[ 0 ] != 0 ) {
	    fprintf( stderr, "Bad status response!\n" );
	    exit( 1 );
	}
#endif /* NONZEROSTATUS */

	if ( ((char *)rniov[ 0 ].iov_base)[ 1 ] != PAP_STATUS ||
		atpb.atp_rresiovcnt != 1 ) {
	    fprintf( stderr, "Bad status response!\n" );
	    exit( 1 );
	}

	if(debug){ printf( "< STATUS\n" ), fflush( stdout );}

	memcpy( st_buf, (char *) rniov[ 0 ].iov_base + 9, 
		((char *)rniov[ 0 ].iov_base)[ 8 ] );
	st_buf[ (int) ((char *)rniov[ 0 ].iov_base)[ 8 ]] = '\0';
	if ( strstr( st_buf, "idle" ) != NULL ) {
	    waitforidle = 0;
	} else {
	    updatestatus( (char *) rniov[ 0 ].iov_base + 9,
		    ((char *)rniov[ 0 ].iov_base)[ 8 ] );
	    sleep( 5 );
	}
    }

    cbuf[ 0 ] = connid = getpid() & 0xff;
    cbuf[ 1 ] = PAP_OPEN;
    cbuf[ 2 ] = cbuf[ 3 ] = 0;
    cbuf[ 4 ] = atp_sockaddr( atp )->sat_port;
    cbuf[ 5 ] = oquantum;	/* flow quantum */
    if ( gettimeofday( &stv, NULL ) < 0 ) {
	perror( "gettimeofday" );
	exit( 2 );
    }
    for (;;) {
	if ( cuts ) {
	    waiting = 0xffff;
	} else {
	    if ( gettimeofday( &tv, NULL ) < 0 ) {
		perror( "gettimeofday" );
		exit( 2 );
	    }
	    waiting = htons( tv.tv_sec - stv.tv_sec );
	}
	memcpy(cbuf +  6, &waiting, sizeof( waiting ));

	atpb.atp_saddr = &nn.nn_sat;
	atpb.atp_sreqdata = cbuf;
	atpb.atp_sreqdlen = 8;		/* bytes in OpenConn request */
	atpb.atp_sreqto = 2;		/* retry timer */
	atpb.atp_sreqtries = 5;		/* retry count */
	if ( atp_sreq( atp, &atpb, 1, ATP_XO ) < 0 ) {
	    perror( "atp_sreq" );
	    exit( 1 );
	}

	if(debug){ printf( "OPEN >\n" ), fflush( stdout );}

	iov.iov_base = rbuf;
	iov.iov_len = sizeof( rbuf );
	atpb.atp_rresiov = &iov;
	atpb.atp_rresiovcnt = 1;
	if ( atp_rresp( atp, &atpb ) < 0 ) {
	    perror( "atp_rresp" );
	    if ( connattempts-- <= 0 ) {
		fprintf( stderr, "Can't connect!\n" );
		exit( 1 );
	    }
	    continue;
	}

	/* sanity */
	if ( iov.iov_len < 8 || (unsigned char)rbuf[ 0 ] != connid ||
		rbuf[ 1 ] != PAP_OPENREPLY ) {
	    fprintf( stderr, "Bad response!\n" );
	    continue;	/* This is weird, since TIDs must match... */
	}

	if(debug){ printf( "< OPENREPLY\n" ), fflush( stdout );}

	if ( isatty( 1 )) {
	    printf( "%.*s\n", (int)iov.iov_len - 9, (char *) iov.iov_base + 9 );
	}
	updatestatus( (char *) iov.iov_base + 9, iov.iov_len - 9 );

	memcpy( &result, rbuf +  6,  sizeof( result ));
	if ( result != 0 ) {
	    sleep( 2 );
	} else {
	    memcpy( &sat, &nn.nn_sat, sizeof( struct sockaddr_at ));
	    sat.sat_port = rbuf[ 4 ];
	    quantum = rbuf[ 5 ];
	    break;
	}
    }

    if ( isatty( 1 )) {
	printf( "Connected to %.*s:%.*s@%.*s.\n",
		nn.nn_objlen, nn.nn_obj,
		nn.nn_typelen, nn.nn_type,
		nn.nn_zonelen, nn.nn_zone );
    }

    if ( optind == ac ) {
	send_file( 0, atp, 1 );
    } else {
	for (; optind < ac; optind++ ) {
	    if ( strcmp( av[ optind ], "-" ) == 0 ) {
		fd = 0;
	    } else if (( fd = open( av[ optind ], O_RDONLY )) < 0 ) {
		perror( av[ optind ] );
		continue;
	    }
	    send_file( fd, atp, ( optind == ac - 1 ) ? 1 : 0 );
	    if ( fd != 0 ) {
		close( fd );
	    }
	}
    }

    /*
     * Close connection.
     */
    cbuf[ 0 ] = connid;
    cbuf[ 1 ] = PAP_CLOSE;
    cbuf[ 2 ] = cbuf[ 3 ] = 0;

    atpb.atp_saddr = &sat;
    atpb.atp_sreqdata = cbuf;
    atpb.atp_sreqdlen = 4;		/* bytes in CloseConn request */
    atpb.atp_sreqto = 2;		/* retry timer */
    atpb.atp_sreqtries = 5;		/* retry count */
    if ( atp_sreq( atp, &atpb, 1, ATP_XO ) < 0 ) {
	perror( "atp_sreq" );
	exit( 1 );
    }

	if(debug){ printf( "CLOSE >\n" ), fflush( stdout );}

    iov.iov_base = rbuf;
    iov.iov_len = sizeof( rbuf );
    atpb.atp_rresiov = &iov;
    atpb.atp_rresiovcnt = 1;
    if ( atp_rresp( atp, &atpb ) < 0 ) {
	perror( "atp_rresp" );
	exit( 1 );
    }

    /* sanity */
    if ( iov.iov_len != 4 || rbuf[ 1 ] != PAP_CLOSEREPLY ) {
	fprintf( stderr, "Bad response!\n" );
	exit( 1 );
    }

#ifndef ZEROCONNID
    /*
     * The AGFA Viper Rip doesn't have the connection id in the close request.
     */
    if ((unsigned char)rbuf[ 0 ] != connid ) {
	fprintf( stderr, "Bad connid in close!\n" );
	exit( 1 );
    }
#endif /* ZEROCONNID */

	if(debug){ printf( "< CLOSEREPLY\n" ), fflush( stdout );}

    if ( isatty( 1 )) {
	printf( "Connection closed.\n" );
    }
    exit( 0 );
}

static int		data = 0;
static unsigned char	port;
static u_int16_t	seq = 0;

static int send_file( int fd, ATP atp, int lastfile)
{
    struct timeval	stv, tv;
    struct sockaddr_at	ssat;
    struct atp_block	atpb;
    fd_set		fds;
    int			fiovcnt = 0, eof = 0, senteof = 0, to = 0;
    int			cc, i;
    unsigned short	netseq;

    if ( gettimeofday( &stv, NULL ) < 0 ) {
	perror( "gettimeofday" );
	exit( 2 );
    }

    /*
     * Ask for more data.
     */
    cbuf[ 0 ] = connid;
    cbuf[ 1 ] = PAP_READ;
    if ( ++seq == 0 ) seq = 1;
    netseq = htons( seq );
    memcpy( cbuf +  2, &netseq, sizeof( netseq ));
    atpb.atp_saddr = &sat;
    atpb.atp_sreqdata = cbuf;
    atpb.atp_sreqdlen = 4;		/* bytes in SendData request */
    atpb.atp_sreqto = 15;		/* retry timer */
    atpb.atp_sreqtries = -1;		/* retry count */
    if ( atp_sreq( atp, &atpb, oquantum, ATP_XO ) < 0 ) {
	perror( "atp_sreq" );
	exit( 1 );
    }

	if(debug){ printf( "READ %d >\n", seq ), fflush( stdout );}

    for (;;) {
	if ( gettimeofday( &tv, NULL ) < 0 ) {
	    perror( "gettimeofday" );
	    exit( 2 );
	}

	if (( tv.tv_sec - stv.tv_sec ) >= 60 ) {
	    stv = tv;

	    /*
	     * Send a tickle.
	     */
	    cbuf[ 0 ] = connid;
	    cbuf[ 1 ] = PAP_TICKLE;
	    cbuf[ 2 ] = cbuf[ 3 ] = 0;
	    atpb.atp_saddr = &sat;
	    atpb.atp_sreqdata = cbuf;
	    atpb.atp_sreqdlen = 4;		/* bytes in Tickle request */
	    atpb.atp_sreqto = 0;		/* retry timer */
	    atpb.atp_sreqtries = 1;		/* retry count */
	    if ( atp_sreq( satp, &atpb, 0, 0 ) < 0 ) {
		perror( "atp_sreq" );
		exit( 1 );
	    }

	if(debug){ printf( "TICKLE >\n" ), fflush( stdout );}
	}

	tv.tv_sec = stv.tv_sec + 60 - tv.tv_sec;
	tv.tv_usec = 0;

	FD_ZERO( &fds );
	if ( !waitforprinter && !eof && fiovcnt == 0 ) {
	    FD_SET( fd, &fds );
	}
	FD_SET( atp_fileno( atp ), &fds );

	if (( cc = select( FD_SETSIZE, &fds, NULL, NULL, &tv )) < 0 ) {
	    perror( "select" );
	    exit( 2 );
	}

	/*
	 * A timeout has occured. Keep track of it.
	 */
	if ( cc == 0 ) {
	    if ( to++ > 2 ) {
		fprintf( stderr, "Connection timed out.\n" );
		exit( 1 );
	    }
	    continue;
	}

	/*
	 * Read data.
	 */
	if ( !fiovcnt && FD_ISSET( fd, &fds )) {
	    for ( i = 0; i < quantum; i++ ) {
		rfiov[ i ].iov_len = PAP_MAXDATA;
	    }
	    if (( cc = readv( fd, rfiov, quantum )) < 0 ) {
		perror( "readv" );
		exit( 2 );
	    }
	    if ( cc == 0 ) {
		eof = 1;
	    }
	    fiovcnt = cc / PAP_MAXDATA + ( cc % PAP_MAXDATA > 0 );
	    for ( i = 0; cc > 0; i++ ) {
		rfiov[ i ].iov_len = ( cc > PAP_MAXDATA ) ? PAP_MAXDATA : cc;
		cc -= ( cc > PAP_MAXDATA ) ? PAP_MAXDATA : cc;
	    }
	}

	if ( FD_ISSET( atp_fileno( atp ), &fds )) {
	    ssat = sat;
	    ssat.sat_port = ATADDR_ANYPORT;
	    switch( atp_rsel( atp, &ssat, ATP_TRESP | ATP_TREQ )) {
	    case ATP_TREQ :
		atpb.atp_saddr = &ssat;
		atpb.atp_rreqdata = cbuf;
		atpb.atp_rreqdlen = sizeof( cbuf );
		if ( atp_rreq( atp, &atpb ) < 0 ) {
		    perror( "atp_rreq" );
		    exit( 1 );
		}

		if ( (unsigned char)cbuf[ 0 ] != connid ) {
		    break;
		}

		/* reset timeout counter for all valid requests */
		to = 0;

		switch ( cbuf[ 1 ] ) {
		case PAP_READ :
		    memcpy( cbuf +  2, &netseq, sizeof( netseq ));
	if(debug){ printf( "< READ %d\n", ntohs( netseq )), fflush( stdout );}
#ifdef notdef
		    if ( netseq != 0 ) {
			if ( rseq != ntohs( netseq )) {
	if(debug){ printf( "| DUP %d\n", rseq ), fflush( stdout );}
			    break;
			}
			if ( rseq++ == 0xffff ) rseq = 1;
		    }
#endif /* notdef */

		    data = 1;
		    port = ssat.sat_port;
		    break;

		case PAP_CLOSE :

	if(debug){ printf( "< CLOSE\n" ), fflush( stdout );}

		    /*
		     * Respond to the close request, and fail.
		     */
		    sniov[ 0 ].iov_len = 4;
		    ((char *)sniov[ 0 ].iov_base)[ 0 ] = connid;
		    ((char *)sniov[ 0 ].iov_base)[ 1 ] = PAP_CLOSEREPLY;
		    ((char *)sniov[ 0 ].iov_base)[ 2 ] =
			    ((char *)sniov[ 0 ].iov_base)[ 3 ] = 0;
		    atpb.atp_sresiov = sniov;
		    atpb.atp_sresiovcnt = 1;
		    if ( atp_sresp( atp, &atpb ) < 0 ) {
			perror( "atp_sresp" );
			exit( 1 );
		    }

	if(debug){ printf( "CLOSEREPLY >\n" ), fflush( stdout );}

		    fprintf( stderr, "Connection closed by foreign host.\n" );
		    exit( 1 );

		case PAP_TICKLE :

	if(debug){ printf( "< TICKLE\n" ), fflush( stdout );}

		    break;
		default :
		    fprintf( stderr, "Bad PAP request!\n" );
		    exit( 1 );
		}
		break;

	    case ATP_TRESP :
		/* reset timeout counter for all valid requests */
		to = 0;

		atpb.atp_saddr = &ssat;
		for ( i = 0; i < oquantum; i++ ) {
		    rniov[ i ].iov_len = PAP_MAXDATA + 4;
		}
		atpb.atp_rresiov = rniov;
		atpb.atp_rresiovcnt = oquantum;
		if ( atp_rresp( atp, &atpb ) < 0 ) {
		    perror( "atp_rresp" );
		    exit( 1 );
		}

#ifndef ZEROCONNID
		/*
		 * The HP LJIIISI w/ BridgePort LocalTalk card sends
		 * zero instead of the connid.
		 */
		if ( ((unsigned char *)rniov[ 0 ].iov_base)[ 0 ] != connid ) {
		    fprintf( stderr, "Bad data response!\n" );
		    exit( 1 );
		}
#endif /* ZEROCONNID */
		if ( ((char *)rniov[ 0 ].iov_base)[ 1 ] != PAP_DATA ) {
		    fprintf( stderr, "Bad data response!\n" );
		    exit( 1 );
		}

		for ( cc = 0, i = 0; i < atpb.atp_rresiovcnt; i++ ) {
		    sfiov[ i ].iov_len = rniov[ i ].iov_len - 4;
		    cc += sfiov[ i ].iov_len;
		}
		if ( cc && writev( 1, sfiov, atpb.atp_rresiovcnt ) < cc ) {
		    perror( "writev" );
		    exit( 2 );
		}

		/* eof */
		if ( ((char *)rniov[ 0 ].iov_base)[ 2 ] ) {

	if(debug){ printf( "< DATA (eof)\n" ), fflush( stdout );}

		    return( 0 );
		}

	if(debug){ printf( "< DATA\n" ), fflush( stdout );}


		/*
		 * Ask for more data.
		 */
		cbuf[ 0 ] = connid;
		cbuf[ 1 ] = PAP_READ;
		if ( ++seq == 0 ) seq = 1;
		netseq = htons( seq );
		memcpy( cbuf +  2, &netseq, sizeof( netseq ));
		atpb.atp_saddr = &sat;
		atpb.atp_sreqdata = cbuf;
		atpb.atp_sreqdlen = 4;		/* bytes in SendData request */
		atpb.atp_sreqto = 15;		/* retry timer */
		atpb.atp_sreqtries = -1;	/* retry count */
		if ( atp_sreq( atp, &atpb, oquantum, ATP_XO ) < 0 ) {
		    perror( "atp_sreq" );
		    exit( 1 );
		}

	if(debug){ printf( "READ %d >\n", seq ), fflush( stdout );}

		break;

	    case 0:

	if(debug){ printf( "| RETRANS\n" ), fflush( stdout );}

		break;

	    default:
		perror( "atp_rsel" );
		exit( 1 );
	    }
	}

	/*
	 * Send whatever is pending.
	 */
	if ( !waitforprinter && !senteof && data && ( fiovcnt || eof )) {
	    ssat.sat_port = port;
	    atpb.atp_saddr = &ssat;
	    if ( fiovcnt ) {
		for ( i = 0; i < fiovcnt; i++ ) {
		    sniov[ i ].iov_len = rfiov[ i ].iov_len + 4;
		    ((char *)sniov[ i ].iov_base)[ 0 ] = connid;
		    ((char *)sniov[ i ].iov_base)[ 1 ] = PAP_DATA;
		    senteof = ((char *)sniov[ i ].iov_base)[ 2 ] = eof;
		    ((char *)sniov[ i ].iov_base)[ 3 ] = 0;
		}
	    } else {
		sniov[ 0 ].iov_len = 4;
		((char *)sniov[ 0 ].iov_base)[ 0 ] = connid;
		((char *)sniov[ 0 ].iov_base)[ 1 ] = PAP_DATA;
		senteof = ((char *)sniov[ 0 ].iov_base)[ 2 ] = eof;
		((char *)sniov[ 0 ].iov_base)[ 3 ] = 0;
	    }
	    atpb.atp_sresiov = sniov;
	    atpb.atp_sresiovcnt = fiovcnt ? fiovcnt : 1;
	    if ( atp_sresp( atp, &atpb ) < 0 ) {
		perror( "atp_sresp" );
		exit( 1 );
	    }
	    data = fiovcnt = 0;

	if(debug){ printf( "DATA %s\n", eof ? "(eof) >" : ">" ), fflush( stdout );}

	    /*
	     * The Apple LaserWriter IIf, the HP LWIIISi, and IV, don't
	     * seem to send us an EOF on large jobs.  To work around
	     * this heinous protocol violation, we won't wait for their
	     * EOF before closing.
	     */
	    if ( eof && noeof && lastfile ) {
		return( 0 );
	    }
	} else {
	    /*
	     * If we can't send data right now, go ahead and get the
	     * status. This is cool, because we get here reliably
	     * if there is a problem.
	     */
	    cbuf[ 0 ] = 0;
	    cbuf[ 1 ] = PAP_SENDSTATUS;
	    cbuf[ 2 ] = cbuf[ 3 ] = 0;
	    atpb.atp_saddr = &nn.nn_sat;
	    atpb.atp_sreqdata = cbuf;
	    atpb.atp_sreqdlen = 4;	/* bytes in SendStatus request */
	    atpb.atp_sreqto = 2;		/* retry timer */
	    atpb.atp_sreqtries = 5;		/* retry count */
	    if ( atp_sreq( satp, &atpb, 1, 0 ) < 0 ) {
		perror( "atp_sreq" );
		exit( 1 );
	    }

	if(debug){ printf( "SENDSTATUS >\n" ), fflush( stdout );}

	    atpb.atp_saddr = &nn.nn_sat;
	    rniov[ 0 ].iov_len = PAP_MAXDATA + 4;
	    atpb.atp_rresiov = rniov;
	    atpb.atp_rresiovcnt = 1;
	    if ( atp_rresp( satp, &atpb ) < 0 ) {
		perror( "atp_rresp" );
		continue;
	    }

#ifndef NONZEROSTATUS
	    /*
	     * The stinking LaserWriter IINTX puts crap in this
	     * field.
	     */
	    if ( ((char *)rniov[ 0 ].iov_base)[ 0 ] != 0 ) {
		fprintf( stderr, "Bad status response!\n" );
		exit( 1 );
	    }
#endif /* NONZEROSTATUS */

	    if ( ((char *)rniov[ 0 ].iov_base)[ 1 ] != PAP_STATUS ||
		    atpb.atp_rresiovcnt != 1 ) {
		fprintf( stderr, "Bad status response!\n" );
		exit( 1 );
	    }

	if(debug){ printf( "< STATUS\n" ), fflush( stdout );}

#ifdef FUCKED
	    if ( waitforprinter ) {
		char	st_buf[ 1024 ];	/* XXX too big */

		memcpy( st_buf, (char *) rniov[ 0 ].iov_base + 9, 
			((char *)rniov[ 0 ].iov_base)[ 8 ] );
		st_buf[ (int) ((char *)rniov[ 0 ].iov_base)[ 8 ]] = '\0';
		if ( strstr( st_buf, "waiting" ) != NULL ) {
		    waitforprinter = 0;
		}
	    }
#endif /* FUCKED */

	    updatestatus( (char *) rniov[ 0 ].iov_base + 9,
		    ((char *)rniov[ 0 ].iov_base)[ 8 ] );
	}
    }
}

static void updatestatus(char *s, int len)
{
    int			fd = -1;
    struct iovec	iov[ 3 ];

    if ( status ) {
	if (( fd = open( status, O_WRONLY|O_TRUNC )) < 0 ) {
	    perror( status );
	    status = NULL;
	}
    }

    if ( fd < 0 ) {
	fd = 2;
    }

    iov[ 0 ].iov_base = "%%[ ";
    iov[ 0 ].iov_len = 4;
    iov[ 1 ].iov_base = s;
    iov[ 1 ].iov_len = len;
    iov[ 2 ].iov_base = " ]%%\n";
    iov[ 2 ].iov_len = 5;

    writev( fd, iov, 3 );
    if ( status ) {
	close( fd );
    }
}
