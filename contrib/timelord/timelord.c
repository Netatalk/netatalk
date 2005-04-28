/*
 * $Id: timelord.c,v 1.8 2005-04-28 20:49:36 bfernhomberg Exp $
 *
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * The "timelord protocol" was reverse engineered from Timelord,
 * distributed with CAP, Copyright (c) 1990, The University of
 * Melbourne.  The following copyright, supplied by The University
 * of Melbourne, may apply to this code:
 *
 *	This version of timelord.c is based on code distributed
 *	by the University of Melbourne as part of the CAP package.
 *
 *	The tardis/Timelord package for Macintosh/CAP is
 *	Copyright (c) 1990, The University of Melbourne.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <stdlib.h>

#include <unistd.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/atp.h>
#include <atalk/nbp.h>

#include <time.h>
#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif /* HAVE_SGTTY_H */
#include <errno.h>
#include <signal.h>
#pragma warn "testing 123"
#include <atalk/logger.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif /* HAVE_TERMIOS_H */
#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#endif /* HAVE_SYS_TERMIOS_H */

#define	TL_GETTIME	0
#define	TL_OK		12
#define TL_BAD		10
#define EPOCH		0x7C25B080	/* 00:00:00 GMT Jan 1, 1970 for Mac */

int	debug = 0;
char	*bad = "Bad request!";
char	buf[ 4624 ];
char	*server;

void usage( char *p )
{
    char	*s;

    if (( s = rindex( p, '/' )) == NULL ) {
	s = p;
    } else {
	s++;
    }
    fprintf( stderr, "Usage:\t%s -d -n nbpname\n", s );
    exit( 1 );
}

/*
 * Unregister ourself on signal.
 */
void
goaway(int signal)
{
    if ( nbp_unrgstr( server, "TimeLord", "*", NULL ) < 0 ) {
	LOG(log_error, logtype_default, "Can't unregister %s", server );
	exit( 1 );
    }
    LOG(log_info, logtype_default, "going down" );
    exit( 0 );
}

int main( int ac, char **av )
{
    ATP			atp;
    struct sockaddr_at	sat;
    struct atp_block	atpb;
    struct timeval	tv;
    struct timezone	tz;
    struct iovec	iov;
    struct tm		*tm;
    char		hostname[ MAXHOSTNAMELEN ];
    char		*p;
    int			c;
    long		req, mtime, resp;
    extern char		*optarg;
    extern int		optind;

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	perror( "gethostname" );
	exit( 1 );
    }
    if (( server = index( hostname, '.' )) != 0 ) {
	*server = '\0';
    }
    server = hostname;

    while (( c = getopt( ac, av, "dn:" )) != EOF ) {
	switch ( c ) {
	case 'd' :
	    debug++;
	    break;
	case 'n' :
	    server = optarg;
	    break;
	default :
	    fprintf( stderr, "Unknown option -- '%c'\n", c );
	    usage( *av );
	}
    }

    /*
     * Disassociate from controlling tty.
     */
    if ( !debug ) {
	int		i, dt;

	switch ( fork()) {
	case 0 :
	    dt = getdtablesize();
	    for ( i = 0; i < dt; i++ ) {
		(void)close( i );
	    }
	    if (( i = open( "/dev/tty", O_RDWR )) >= 0 ) {
		(void)ioctl( i, TIOCNOTTY, 0 );
		setpgid( 0, getpid());
		(void)close( i );
	    }
	    break;
	case -1 :
	    perror( "fork" );
	    exit( 1 );
	default :
	    exit( 0 );
	}
    }

    if (( p = rindex( *av, '/' )) == NULL ) {
	p = *av;
    } else {
	p++;
    }

#ifdef ultrix
    openlog( p, LOG_PID );
#else /* ultrix */
    set_processname(p);
    syslog_setup(log_debug, logtype_default, logoption_ndelay|logoption_pid, logfacility_daemon );
#endif /* ultrix */

    /* allocate memory */
    memset (&sat.sat_addr, 0, sizeof (sat.sat_addr));

    if (( atp = atp_open( ATADDR_ANYPORT, &sat.sat_addr )) == NULL ) {
	LOG(log_error, logtype_default, "main: atp_open: %s", strerror( errno ) );
	exit( 1 );
    }

    if ( nbp_rgstr( atp_sockaddr( atp ), server, "TimeLord", "*" ) < 0 ) {
	LOG(log_error, logtype_default, "Can't register %s", server );
	exit( 1 );
    }
    LOG(log_info, logtype_default, "%s:TimeLord started", server );

	signal(SIGHUP, goaway);
	signal(SIGTERM, goaway);

    for (;;) {
	/*
	 * Something seriously wrong with atp, since these assigns must
	 * be in the loop...
	 */
	atpb.atp_saddr = &sat;
	atpb.atp_rreqdata = buf;
	bzero( &sat, sizeof( struct sockaddr_at ));
	atpb.atp_rreqdlen = sizeof( buf );
	if ( atp_rreq( atp, &atpb ) < 0 ) {
	    LOG(log_error, logtype_default, "main: atp_rreq: %s", strerror( errno ) );
	    exit( 1 );
	}

	p = buf;
	bcopy( p, &req, sizeof( long ));
	req = ntohl( req );
	p += sizeof( long );

	switch( req ) {
	case TL_GETTIME :
	    if ( atpb.atp_rreqdlen > 5 ) {
		    bcopy( p + 1, &mtime, sizeof( long ));
		    mtime = ntohl( mtime );
		    LOG(log_info, logtype_default, "gettime from %s %s was %lu",
			    (*( p + 5 ) == '\0' ) ? "<unknown>" : p + 5,
			    ( *p == 0 ) ? "at boot" : "in chooser",
			    mtime );
	    } else {
		    LOG(log_info, logtype_default, "gettime" );
	    }

	    if ( gettimeofday( &tv, &tz ) < 0 ) {
		LOG(log_error, logtype_default, "main: gettimeofday: %s", strerror( errno ) );
		exit( 1 );
	    }
	    if (( tm = gmtime( &tv.tv_sec )) == 0 ) {
		perror( "localtime" );
		exit( 1 );
	    }

	    mtime = tv.tv_sec + EPOCH;
	    mtime = htonl( mtime );

	    resp = TL_OK;
	    bcopy( &resp, buf, sizeof( long ));
	    bcopy( &mtime, buf + sizeof( long ), sizeof( long ));
	    iov.iov_len = sizeof( long ) + sizeof( long );
	    break;

	default :
	    LOG(log_error, logtype_default, bad );

	    resp = TL_BAD;
	    bcopy( &resp, buf, sizeof( long ));
	    *( buf + 4 ) = (unsigned char)strlen( bad );
	    strcpy( buf + 5, bad );
	    iov.iov_len = sizeof( long ) + 2 + strlen( bad );
	    break;
	}

	iov.iov_base = buf;
	atpb.atp_sresiov = &iov;
	atpb.atp_sresiovcnt = 1;
	if ( atp_sresp( atp, &atpb ) < 0 ) {
	    LOG(log_error, logtype_default, "main: atp_sresp: %s", strerror( errno ) );
	    exit( 1 );
	}
    }
}
