/*
 * $Id: a2boot.c,v 1.2 2003-01-15 06:24:28 jmarcus Exp $
 *   Apple II boot support code.       with aid of Steven N. Hirsch
 *
 * based on timelord 1.6 so below copyrights still apply
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
#include <sys/file.h>
#include <sys/uio.h>

#include <unistd.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/atp.h>
#include <atalk/nbp.h>

#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif /* HAVE_SGTTY_H */

#include <errno.h>
#include <signal.h>
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

#define	TL_OK		'\0'
#define TL_EOF		'\1'

int	debug = 0;
char	*bad = "Bad request!";
char	buf[ 4624 ];
char	*server;
int32_t fileoff;

long a2bootreq(char *fname);

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
    int regerr;
    regerr = nbp_unrgstr( server, "Apple //gs", "*", NULL );
    regerr += nbp_unrgstr( server, "Apple //e Boot", "*", NULL );
    regerr += nbp_unrgstr( server, "ProDOS16 Image", "*", NULL );
    if ( regerr < 0 ) {
	LOG(log_error, logtype_default, "Can't unregister Apple II boot files %s", server );
	exit( 1 );
    }
    LOG(log_info, logtype_default, "going down" );
    exit( 0 );
}

int main( int ac, char **av )
{
    ATP		atp;
    struct sockaddr_at	sat;
    struct atp_block	atpb;
    struct iovec	iov;
    char	hostname[ MAXHOSTNAMELEN ];
    char	*p;
    int		c;
    int32_t	req, resp;
    int 	regerr;
    extern char	*optarg;
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

/*
	force port 3 as the semi-official ATP access port        MJ 2002
*/
    if (( atp = atp_open( (u_int8_t)3, &sat.sat_addr )) == NULL ) {
	LOG(log_error, logtype_default, "main: atp_open: %s", strerror( errno ) );
	exit( 1 );
    }

    regerr = nbp_rgstr( atp_sockaddr( atp ), server, "Apple //gs", "*" );
    regerr += nbp_rgstr( atp_sockaddr( atp ), server, "Apple //e Boot", "*" );
    regerr += nbp_rgstr( atp_sockaddr( atp ), server, "ProDOS16 Image", "*" );
    if ( regerr < 0 ) {
	LOG(log_error, logtype_default, "Can't register Apple II boot files %s", server );
	exit( 1 );
    }

    LOG(log_info, logtype_default, "%s:Apple 2 Boot started", server );

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
	bcopy( p, &req, sizeof( int32_t ));
	req = ntohl( req );
	p += sizeof( int32_t );

/*
    LOG(log_info, logtype_default, "req = %08lx",(long)req );
*/
	/* Byte-swap and multiply by 0x200. Converts block number to
	   file offset. */
	fileoff = (( req & 0x00ff0000 ) >> 7 ) | (( req & 0x0000ff00 ) << 9 );
	req &= 0xff000000;

/*
    LOG(log_info, logtype_default, "       reqblklo = %02x",(int)reqblklo );
    LOG(log_info, logtype_default, "       reqblkhi = %02x",(int)reqblkhi );
    LOG(log_info, logtype_default, "       req now = %08lx",(long)req );
*/

	switch( req ) {
	case 0x01000000 :	/* Apple IIgs both ROM 1 and ROM 3 */
/*    LOG(log_info, logtype_default, "          Req ProDOS16 Boot Blocks" ); */
		resp = a2bootreq(_PATH_A_GS_BLOCKS);
	    break;

	case 0x02000000 :	/* Apple 2 Workstation card  */
/*    LOG(log_info, logtype_default, "          Req Apple //e Boot" );  */
	    	resp = a2bootreq(_PATH_A_2E_BLOCKS);
	    break;

	case 0x03000000 :	/* Apple IIgs both ROM 1 and ROM 3 */
/*    LOG(log_info, logtype_default, "          Req ProDOS16 Image" );    */
		resp = a2bootreq(_PATH_P16_IMAGE);
	    break;

	default :
	    LOG(log_error, logtype_default, bad );

	    resp = TL_EOF;
	    *( buf + sizeof( int32_t ) ) = (unsigned char)strlen( bad );
	    strcpy( buf + 1 + sizeof( int32_t ), bad );

	    break;
	}

	bcopy( &resp, buf, sizeof( int32_t ));

	iov.iov_len = sizeof( int32_t ) + 512;
	iov.iov_base = buf;
	atpb.atp_sresiov = &iov;
	atpb.atp_sresiovcnt = 1;

	if ( atp_sresp( atp, &atpb ) < 0 ) {
	    LOG(log_error, logtype_default, "main: atp_sresp: %s", strerror( errno ) );
	    exit( 1 );
	}
    }
}


/* below MJ 2002 (initially borrowed from aep_packet */
long a2bootreq(fname)
	char	*fname;
{
	int f,m;
	int32_t readlen;
/*
    LOG(log_info, logtype_default, "          a2bootreq( %s )",fname );
*/
	f=open(fname,O_RDONLY );
	if(f==EOF) {
	LOG(log_error, logtype_default, "a2boot open error on %s",fname);
		return close(f);
	}

/*
    LOG(log_info, logtype_default, "would lseek to %08lx",fileoff);
*/
	lseek(f,fileoff,0);
	readlen=read(f, buf + sizeof( int32_t ), 512 );

/*
    LOG(log_info, logtype_default, "length is %08lx", readlen);
*/

	if(readlen < 0x200) {
/*    LOG(log_info, logtype_default, "Read to EOF");  */
		close(f);
		return TL_EOF;
	}
	close(f);
	return  TL_OK;
}

