/*
 * $Id: main.c,v 1.31 2009-10-29 13:38:15 didg Exp $
 *
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <atalk/logger.h>

/* POSIX.1 sys/wait.h check */
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

#include <errno.h>

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/pap.h>
#include <atalk/paths.h>
#include <atalk/util.h>
#include <atalk/nbp.h>
#include <atalk/unicode.h>

#include "printer.h"
#include "printcap.h"
#include "session.h"
#include "uam_auth.h"
#include "print_cups.h"


#define PIPED_STATUS	"status: print spooler processing job"

struct printer	defprinter;
struct printer	*printers = NULL;

int		debug = 0;
static char	*conffile = _PATH_PAPDCONF;
char		*printcap = _PATH_PAPDPRINTCAP;
unsigned char	connid, quantum, sock, oquantum = PAP_MAXQUANTUM;
char		*cannedstatus = PIPED_STATUS;
struct printer	*printer = NULL;
char		*version = VERSION;
static char	*pidfile = _PATH_PAPDLOCK;

char		*uamlist;
char		*uampath = _PATH_PAPDUAMPATH;

/* Prototypes for locally used functions */
int getstatus( struct printer *pr, char *buf );
int rprintcap( struct printer *pr );
static void getprinters( char *cf );


/* this only needs to be used by the server process */
static void papd_exit(const int i)
{
  server_unlock(pidfile);
  auth_unload();
  exit(i);
}

static void
die(int n)
{
    struct printer	*pr;
    struct at_addr	addr;

    memset(&addr, 0, sizeof(addr));

    for ( pr = printers; pr; pr = pr->p_next ) {
	if ( pr->p_flags & P_REGISTERED ) {
	    if ( nbp_unrgstr( pr->p_name, pr->p_type, pr->p_zone, &addr ) < 0 ) {
		LOG(log_error, logtype_papd, "can't unregister %s:%s@%s", pr->p_name,
			pr->p_type, pr->p_zone );
		papd_exit( n + 1 );
	    }
	    LOG(log_info, logtype_papd, "unregister %s:%s@%s", pr->p_name, pr->p_type,
		    pr->p_zone );
	}
#ifdef HAVE_CUPS
	if ( pr->p_flags & P_SPOOLED && pr->p_flags & P_CUPS_PPD ) {
		LOG(log_info, logtype_papd, "Deleting CUPS temp PPD file for %s (%s)", pr->p_name, pr->p_ppdfile);
		unlink (pr->p_ppdfile);
	}
#endif /* HAVE_CUPS */

    }
    papd_exit( n );
}

static void
reap(int sig _U_)
{
    int		status;
    int		pid;

    while (( pid = wait3( &status, WNOHANG, NULL )) > 0 ) {
	if ( WIFEXITED( status )) {
	    if ( WEXITSTATUS( status )) {
		LOG(log_error, logtype_papd, "child %d exited with %d", pid,
			WEXITSTATUS( status ));
	    } else {
		LOG(log_info, logtype_papd, "child %d done", pid );
	    }
	} else {
	    if ( WIFSIGNALED( status )) {
		LOG(log_error, logtype_papd, "child %d killed with %d", pid,
			WTERMSIG( status ));
	    } else {
		LOG(log_error, logtype_papd, "child %d died", pid );
	    }
	}
    }
    return;
}

static char rbuf[ 255 + 1 + 8 ];

int main(int ac, char **av)
{
    extern char         *optarg;

    ATP			atp;
    struct atp_block	atpb;
    struct sockaddr_at	sat;
    struct sigaction	sv;
    struct iovec	iov;
    fd_set		fdset;
    struct printer	*pr;
    char		*p, hostname[ MAXHOSTNAMELEN ];
    char		cbuf[ 8 ];
    int			c;
    char 		*atname;

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	perror( "gethostname" );
	exit( 1 );
    }
    if (( p = strchr( hostname, '.' )) != NULL ) {
	*p = '\0';
    }
    if (( defprinter.p_name = (char *)malloc( strlen( hostname ) + 1 ))
	    == NULL ) {
	perror( "malloc" );
	exit( 1 );
    }
    strcpy( defprinter.p_name, hostname );
    defprinter.p_type = "LaserWriter";
    defprinter.p_zone = "*";
    memset(&defprinter.p_addr, 0, sizeof(defprinter.p_addr));
#ifdef __svr4__
    defprinter.p_flags = P_PIPED;
    defprinter.p_printer = "/usr/bin/lp -T PS";
#else /* __svr4__ */
    defprinter.p_flags = P_SPOOLED;
    defprinter.p_printer = "lp";
#endif /* __svr4__ */
    defprinter.p_operator = "operator";
    defprinter.p_spool = _PATH_PAPDSPOOLDIR;
#ifdef ABS_PRINT
    defprinter.p_role = NULL;
    defprinter.p_srvid = 0;
#endif /* ABS_PRINT */
    defprinter.p_pagecost = 200;		/* default cost */
    defprinter.p_pagecost_msg = NULL;
    defprinter.p_lock = "lock";

    while (( c = getopt( ac, av, "adf:p:P:v" )) != EOF ) {
	switch ( c ) {
	case 'a' :		/* for compatibility with old papd */
	    break;

	case 'd' :		/* debug */
	    debug++;
	    break;

	case 'f' :		/* conffile */
	    conffile = optarg;
	    break;

	case 'p' :		/* printcap */
	    printcap = optarg;
	    break;

	case 'P' :
	    pidfile = optarg;
	    break;

	case 'v' :		/* version */
	    printf( "papd (version %s)\n", VERSION );
	    exit ( 1 );
	    break;

	default :
	    fprintf( stderr,
		    "Usage:\t%s [ -d ] [ -f conffile ] [ -p printcap ]\n",
		    *av );
	    exit( 1 );
	}
    }


    switch (server_lock("papd", pidfile, debug)) {
    case 0: /* open a couple things again in the child */
      if (!debug && (c = open("/", O_RDONLY)) >= 0) {
	dup2(c, 1);
	dup2(c, 2);
      }
      break;
    case -1:
      exit(1);
    default:
      exit(0);
    }      

#ifdef DEBUG1
    fault_setup(NULL);
#endif

    /*
     * Start logging.
     */
    if (( p = strrchr( av[ 0 ], '/' )) == NULL ) {
	p = av[ 0 ];
    } else {
	p++;
    }
#ifdef ultrix
    openlog( p, LOG_PID );
#else /* ultrix */
    set_processname(p);
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid |
               debug ? logoption_perror : 0, logfacility_lpr );
#endif /* ultrix */

    LOG(log_info, logtype_papd, "restart (%s)", version );
#ifdef HAVE_CUPS
    LOG(log_info, logtype_papd, "CUPS support enabled (%s)", CUPS_API_VERSION );
#endif

    getprinters( conffile );

    for ( pr = printers; pr; pr = pr->p_next ) {
	if (( pr->p_flags & P_SPOOLED ) && rprintcap( pr ) < 0 ) {
	    LOG(log_error, logtype_papd, "printcap problem: %s", pr->p_printer );
	}

	if (!(pr->p_flags & P_CUPS)) {
		if ((size_t)-1 != convert_string_allocate(CH_UNIX, CH_MAC, pr->p_name, -1, &atname)) {
			pr->p_u_name = pr->p_name;
			pr->p_name = atname;
		}
	}
			
	if (( pr->p_atp = atp_open( ATADDR_ANYPORT, &pr->p_addr )) == NULL ) {
	    LOG(log_error, logtype_papd, "atp_open: %s", strerror(errno) );
	    papd_exit( 1 );
	}
	if ( nbp_rgstr( atp_sockaddr( pr->p_atp ), pr->p_name, pr->p_type,
		pr->p_zone ) < 0 ) {
	    LOG(log_error, logtype_papd, "can't register %s:%s@%s", pr->p_u_name, pr->p_type,
		    pr->p_zone );
	    die( 1 );
	}
	if ( pr->p_flags & P_AUTH ) {
		LOG(log_info, logtype_papd, "Authentication enabled: %s", pr->p_u_name );
	}
	else {
		LOG(log_info, logtype_papd, "Authentication disabled: %s", pr->p_u_name );
	}
	LOG(log_info, logtype_papd, "register %s:%s@%s", pr->p_u_name, pr->p_type,
		pr->p_zone );
	pr->p_flags |= P_REGISTERED;
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = die;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &sv, NULL ) < 0 ) {
	LOG(log_error, logtype_papd, "sigaction: %s", strerror(errno) );
	papd_exit( 1 );
    }

    sv.sa_handler = reap;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sv, NULL ) < 0 ) {
	LOG(log_error, logtype_papd, "sigaction: %s", strerror(errno) );
	papd_exit( 1 );
    }

    /*
     * Load UAMS
     */
    auth_load(uampath, uamlist);

    /*
     * Begin accepting connections.
     */
    FD_ZERO( &fdset );
    for (;;) {
	for ( pr = printers; pr; pr = pr->p_next ) {
	    FD_SET( atp_fileno( pr->p_atp ), &fdset );
	}
	if (( c = select( FD_SETSIZE, &fdset, NULL, NULL, NULL )) < 0 ) {
	    if ( errno == EINTR ) {
		continue;
	    }
	    LOG(log_error, logtype_papd, "select: %s", strerror(errno) );
	    papd_exit( 1 );
	}

	for ( pr = printers; pr; pr = pr->p_next ) {
	    if ( FD_ISSET( atp_fileno( pr->p_atp ), &fdset )) {
		int		err = 0;

		memset( &sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
		sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
		sat.sat_family = AF_APPLETALK;
		sat.sat_addr.s_net = ATADDR_ANYNET;
		sat.sat_addr.s_node = ATADDR_ANYNODE;
		sat.sat_port = ATADDR_ANYPORT;
		/* do an atp_rsel(), to prevent hangs */
		if (( c = atp_rsel( pr->p_atp, &sat, ATP_TREQ )) != ATP_TREQ ) {
		    continue;
		}
		atpb.atp_saddr = &sat;
		atpb.atp_rreqdata = cbuf;
		atpb.atp_rreqdlen = sizeof( cbuf );
		if ( atp_rreq( pr->p_atp, &atpb ) < 0 ) {
		    LOG(log_error, logtype_papd, "atp_rreq: %s", strerror(errno) );
		    continue;
		}

		/* should check length of req buf */

		switch( cbuf[ 1 ] ) {
		case PAP_OPEN :
		    connid = (unsigned char)cbuf[ 0 ];
		    sock = (unsigned char)cbuf[ 4 ];
		    quantum = (unsigned char)cbuf[ 5 ];
		    rbuf[ 0 ] = cbuf[ 0 ];
		    rbuf[ 1 ] = PAP_OPENREPLY;
		    rbuf[ 2 ] = rbuf[ 3 ] = 0;

		    if (( pr->p_flags & P_SPOOLED ) && rprintcap( pr ) != 0 ) {
			LOG(log_error, logtype_papd, "printcap problem: %s",
				pr->p_printer );
			rbuf[ 2 ] = rbuf[ 3 ] = 0xff;
			err = 1;
		    }

#ifdef HAVE_CUPS
		   /*
		    * If cups is not accepting jobs, we return
		    * 0xffff to indicate we're busy
		    */
#ifdef DEBUG
                    LOG(log_debug9, logtype_papd, "CUPS: PAP_OPEN");
#endif
		    if ( (pr->p_flags & P_SPOOLED) && (cups_get_printer_status ( pr ) == 0)) {
                        LOG(log_error, logtype_papd, "CUPS_PAP_OPEN: %s is not accepting jobs",
                                pr->p_printer );
                        rbuf[ 2 ] = rbuf[ 3 ] = 0xff;
                        err = 1;
                    }
#endif /* HAVE_CUPS */

		    /*
		     * If this fails, we've run out of sockets. Rather than
		     * just die(), let's try to continue. Maybe some sockets
		     * will close, and we can continue;
		     */
		    if (( atp = atp_open( ATADDR_ANYPORT, 
					  &pr->p_addr)) == NULL ) {
			LOG(log_error, logtype_papd, "atp_open: %s", strerror(errno) );
			rbuf[ 2 ] = rbuf[ 3 ] = 0xff;  /* printer busy */
			rbuf[ 4 ] = 0; /* FIXME is it right? */
			err = 1;
		    }
		    else {
		       rbuf[ 4 ] = atp_sockaddr( atp )->sat_port;
                    }
		    rbuf[ 5 ] = oquantum;
		    rbuf[ 6 ] = rbuf[ 7 ] = 0;

		    iov.iov_base = rbuf;
		    iov.iov_len = 8 + getstatus( pr, &rbuf[ 8 ] );
		    atpb.atp_sresiov = &iov;
		    atpb.atp_sresiovcnt = 1;
		    /*
		     * This may error out if we lose a route, so we won't die().
		     */
		    if ( atp_sresp( pr->p_atp, &atpb ) < 0 ) {
			LOG(log_error, logtype_papd, "atp_sresp: %s", strerror(errno) );
			err = 1;
		    }

		    if ( err ) {
			if (atp) {
			   atp_close(atp);
                        }
			continue;
		    }

		    switch ( c = fork()) {
		    case -1 :
			LOG(log_error, logtype_papd, "fork: %s", strerror(errno) );
                        atp_close(atp);
			continue;

		    case 0 : /* child */
			printer = pr;

			#ifndef HAVE_CUPS
			if (( printer->p_flags & P_SPOOLED ) &&
				chdir( printer->p_spool ) < 0 ) {
			    LOG(log_error, logtype_papd, "chdir %s: %s", printer->p_spool, strerror(errno) );
			    exit( 1 );
			}
			#else
			if (( printer->p_flags & P_SPOOLED ) &&
				chdir( SPOOLDIR ) < 0 ) {
			    LOG(log_error, logtype_papd, "chdir %s: %s", SPOOLDIR, strerror(errno) );
			    exit( 1 );
			}

			#endif

			sv.sa_handler = SIG_DFL;
			sigemptyset( &sv.sa_mask );
			sv.sa_flags = SA_RESTART;
			if ( sigaction( SIGTERM, &sv, NULL ) < 0 ) {
			    LOG(log_error, logtype_papd, "sigaction: %s", strerror(errno) );
			    exit( 1 );
			}
			
			if ( sigaction( SIGCHLD, &sv, NULL ) < 0 ) {
			    LOG(log_error, logtype_papd, "sigaction: %s", strerror(errno) );
			    exit( 1 );
                        }

			for ( pr = printers; pr; pr = pr->p_next ) {
			    atp_close( pr->p_atp );
			}
			sat.sat_port = sock;
			if ( session( atp, &sat ) < 0 ) {
			    LOG(log_error, logtype_papd, "bad session" );
			    exit( 1 );
			}
			exit( 0 );
			break;

		    default : /* parent */
			LOG(log_info, logtype_papd, "child %d for \"%s\" from %u.%u",
				c, pr->p_name, ntohs( sat.sat_addr.s_net ),
				sat.sat_addr.s_node);
			atp_close( atp );
		    }
		    break;

		case PAP_SENDSTATUS :
		    rbuf[ 0 ] = 0;
		    rbuf[ 1 ] = PAP_STATUS;
		    rbuf[ 2 ] = rbuf[ 3 ] = 0;
		    rbuf[ 4 ] = rbuf[ 5 ] = 0;
		    rbuf[ 6 ] = rbuf[ 7 ] = 0;

		    iov.iov_base = rbuf;
		    iov.iov_len = 8 + getstatus( pr, &rbuf[ 8 ] );
		    atpb.atp_sresiov = &iov;
		    atpb.atp_sresiovcnt = 1;
		    /*
		     * This may error out if we lose a route, so we won't die().
		     */
		    if ( atp_sresp( pr->p_atp, &atpb ) < 0 ) {
			LOG(log_error, logtype_papd, "atp_sresp: %s", strerror(errno) );
		    }
		    break;

		default :
		    LOG(log_error, logtype_papd, "Bad request from %u.%u!",
			    ntohs( sat.sat_addr.s_net ), sat.sat_addr.s_node );
		    continue;
		    break;
		}

#ifdef notdef
		/*
		 * Sometimes the child process will send its first READ
		 * before the parent has sent the OPEN REPLY.  Moving this
		 * code into the OPEN/STATUS switch fixes this problem.
		 */
		iov.iov_base = rbuf;
		iov.iov_len = 8 + getstatus( pr, &rbuf[ 8 ] );
		atpb.atp_sresiov = &iov;
		atpb.atp_sresiovcnt = 1;
		/*
		 * This may error out if we lose a route, so we won't die().
		 */
		if ( atp_sresp( pr->p_atp, &atpb ) < 0 ) {
		    LOG(log_error, logtype_papd, "atp_sresp: %s", strerror(errno) );
		}
#endif /* notdef */
	    }
	}
    }
    return 0;
}

/*
 * We assume buf is big enough for 255 bytes of data and a length byte.
 */

int getstatus(struct printer *pr, char *buf)
{

#ifdef HAVE_CUPS
    if ( pr->p_flags & P_PIPED ) {
	*buf = strlen( cannedstatus );
	strncpy( &buf[ 1 ], cannedstatus, *buf );
	return( *buf + 1 );
    } else {
	cups_get_printer_status( pr );
	*buf = strlen ( pr->p_status );
	strncpy ( &buf[1], pr->p_status, *buf);
	return ( *buf + 1);
    }
#else

    char		path[ MAXPATHLEN ];
    int			fd = -1, rc;

    if ( pr->p_flags & P_SPOOLED && ( pr->p_spool != NULL )) {
	strcpy( path, pr->p_spool );
	strcat( path, "/status" );
	fd = open( path, O_RDONLY);
    }

    if (( pr->p_flags & P_PIPED ) || ( fd < 0 )) {
	*buf = strlen( cannedstatus );
	strncpy( &buf[ 1 ], cannedstatus, *buf );
	return( *buf + 1 );
    } else {
	if (( rc = read( fd, &buf[ 1 ], 255 )) < 0 ) {
	    rc = 0;
	}
	close( fd );
	if ( rc && buf[ rc ] == '\n' ) {	/* remove trailing newline */
	    rc--;
	}
	*buf = rc;
	return( rc + 1 );
    }
#endif /* HAVE_CUPS */
}

char	*pgetstr(char *id, char **area);
char	*getpname(char **area, int bufsize);

#define PF_CONFBUFFER	1024

static void getprinters( char *cf)
{
    char		buf[ PF_CONFBUFFER ], area[ PF_CONFBUFFER ], *a, *p, *name, *type, *zone;
    struct printer	*pr;
    int			c;

    while (( c = getprent( cf, buf, PF_CONFBUFFER )) > 0 ) {
	a = area;
	/*
	 * Get the printer's nbp name.
	 */
	if (( p = getpname( &a, PF_CONFBUFFER )) == NULL ) {
	    fprintf( stderr, "No printer name\n" );
	    exit( 1 );
	}

	if (( pr = (struct printer *)malloc( sizeof( struct printer )))
		== NULL ) {
	    perror( "malloc" );
	    exit( 1 );
	}
	memset( pr, 0, sizeof( struct printer ));

	name = defprinter.p_name;
	type = defprinter.p_type;
	zone = defprinter.p_zone;
	if ( nbp_name( p, &name, &type, &zone )) {
	    fprintf( stderr, "Can't parse \"%s\"\n", name );
	    exit( 1 );
	}
	if ( name != defprinter.p_name ) {
	    if (( pr->p_name = (char *)malloc( strlen( name ) + 1 )) == NULL ) {
		perror( "malloc" );
		exit( 1 );
	    }
	    strcpy( pr->p_name, name );
	} else {
	    pr->p_name = name;
	}
	if ( type != defprinter.p_type ) {
	    if (( pr->p_type = (char *)malloc( strlen( type ) + 1 )) == NULL ) {
		perror( "malloc" );
		exit( 1 );
	    }
	    strcpy( pr->p_type, type );
	} else {
	    pr->p_type = type;
	}
	if ( zone != defprinter.p_zone ) {
	    if (( pr->p_zone = (char *)malloc( strlen( zone ) + 1 )) == NULL ) {
		perror( "malloc" );
		exit( 1 );
	    }
	    strcpy( pr->p_zone, zone );
	} else {
	    pr->p_zone = zone;
	}

	if ( pnchktc( cf ) != 1 ) {
	    fprintf( stderr, "Bad papcap entry\n" );
	    exit( 1 );
	}

	/*
	 * Get PPD file.
	 */
	if (( p = pgetstr( "pd", &a ) )) {
	    if (( pr->p_ppdfile = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
		perror( "malloc" );
		exit( 1 );
	    }
	    strcpy( pr->p_ppdfile, p );
	}

	/*
	 * Get lpd printer name.
	 */
	if (( p = pgetstr( "pr", &a )) == NULL ) {
	    pr->p_printer = defprinter.p_printer;
	    pr->p_flags = defprinter.p_flags;
	} else {
	    if ( *p == '|' ) {
		p++;
		pr->p_flags = P_PIPED;
	    } else {
		pr->p_flags = P_SPOOLED;
	    }
	    if (( pr->p_printer = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
		perror( "malloc" );
		exit( 1 );
	    }
	    strcpy( pr->p_printer, p );
	}

	/*
	 * Do we want authenticated printing?
	 */
	if ((p = pgetstr( "ca", &a )) != NULL ) {
	    if ((pr->p_authprintdir = (char *)malloc(strlen(p)+1)) == NULL) {
		perror( "malloc" );
		exit(1);
	    }
	    strcpy( pr->p_authprintdir, p );
	    pr->p_flags |= P_AUTH;
	    pr->p_flags |= P_AUTH_CAP;
	} else { pr->p_authprintdir = NULL; }

	if ( pgetflag( "sp" ) == 1 ) {
	    pr->p_flags |= P_AUTH;
	    pr->p_flags |= P_AUTH_PSSP;
	}

	if ((p = pgetstr("am", &a)) != NULL ) {
		if ((uamlist = (char *)malloc(strlen(p)+1)) == NULL ) {
			perror("malloc");
			exit(1);
		}
		strcpy(uamlist, p);
	}

	if ( pr->p_flags & P_SPOOLED ) {
	    /*
	     * Get operator name.
	     */
	    if (( p = pgetstr( "op", &a )) == NULL ) {
		pr->p_operator = defprinter.p_operator;
	    } else {
		if (( pr->p_operator = (char *)malloc( strlen( p ) + 1 ))
			== NULL ) {
		    perror( "malloc" );
		    exit( 1 );
		}
		strcpy( pr->p_operator, p );
	    }
	}

	/* get printer's appletalk address. */
	if (( p = pgetstr( "pa", &a )) == NULL ) 
	    memcpy(&pr->p_addr, &defprinter.p_addr, sizeof(pr->p_addr));
	else 
	    atalk_aton(p, &pr->p_addr);

#ifdef HAVE_CUPS
	if ((p = pgetstr("co", &a)) != NULL ) {
            pr->p_cupsoptions = strdup(p);
            LOG (log_error, logtype_papd, "enabling cups-options for %s: %s", pr->p_name, pr->p_cupsoptions);
	}
#endif

	/* convert line endings for setup sections.
           real ugly work around for foomatic deficiencies,
	   need to get rid of this */
	if ( pgetflag("fo") == 1 ) {
            pr->p_flags |= P_FOOMATIC_HACK;
            LOG (log_error, logtype_papd, "enabling foomatic hack for %s", pr->p_name);
	}

	if (strncasecmp (pr->p_name, "cupsautoadd", 11) == 0)
	{
#ifdef HAVE_CUPS
		pr = cups_autoadd_printers (pr, printers);
		printers = pr;
#else
		LOG (log_error, logtype_papd, "cupsautoadd: Cups support not compiled in");
#endif /* HAVE_CUPS */
	}
	else {
#ifdef HAVE_CUPS
		if ( cups_check_printer ( pr, printers, 1) == 0)
		{
			pr->p_next = printers;
			printers = pr;
		}
#else
		pr->p_next = printers;
		printers = pr;
#endif /* HAVE_CUPS */
	}
    }
    if ( c == 0 ) {
	endprent();
    } else {			/* No capability file, do default */
	printers = &defprinter;
    }
}

int rprintcap( struct printer *pr)
{

#ifdef HAVE_CUPS

    char		*p;

    if ( pr->p_flags & P_SPOOLED && !(pr->p_flags & P_CUPS_AUTOADDED) ) { /* Skip check if autoadded */
	if ( cups_printername_ok ( pr->p_printer ) != 1) {
	    LOG(log_error, logtype_papd, "No such CUPS printer: '%s'", pr->p_printer );
	    return( -1 );
	}
    }

    /*
     * Check for ppd file, moved here because of cups_autoadd we cannot check at the usual location
     */

    if ( pr->p_ppdfile == NULL ) {
	if ( (p = (char *) cups_get_printer_ppd ( pr->p_printer )) != NULL ) {
	    if (( pr->p_ppdfile = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
	    	LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		exit( 1 );
	    }
	    strcpy( pr->p_ppdfile, p );
	    pr->p_flags |= P_CUPS_PPD;
	    LOG(log_info, logtype_papd, "PPD File for %s set to %s", pr->p_printer, pr->p_ppdfile );
	}
    }


#else

    char		buf[ 1024 ], area[ 1024 ], *a, *p;
    int			c;

    /*
     * Spool directory from printcap file.
     */
    if ( pr->p_flags & P_SPOOLED ) {
	if ( pgetent( printcap, buf, pr->p_printer ) != 1 ) {
	    LOG(log_error, logtype_papd, "No such printer: %s", pr->p_printer );
	    return( -1 );
	}

	/*
	 * Spool directory.
	 */
	if ( pr->p_spool != NULL && pr->p_spool != defprinter.p_spool ) {
	    free( pr->p_spool );
	}
	a = area;
	if (( p = pgetstr( "sd", &a )) == NULL ) {
	    pr->p_spool = defprinter.p_spool;
	} else {
	    if (( pr->p_spool = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
		LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		exit( 1 );
	    }
	    strcpy( pr->p_spool, p );
	}

	/*
	 * Is accounting on?
	 */
	a = area;
	if ( pgetstr( "af", &a ) == NULL ) {
	    pr->p_flags &= ~P_ACCOUNT;
	} else {
	    pr->p_flags |= P_ACCOUNT;
#ifdef ABS_PRINT
	    if ( pr->p_role != NULL && pr->p_role != defprinter.p_role ) {
		free( pr->p_role );
	    }
	    a = area;
	    if (( p = pgetstr( "ro", &a )) == NULL ) {
		pr->p_role = defprinter.p_role;
	    } else {
		if (( pr->p_role =
			(char *)malloc( strlen( p ) + 1 )) == NULL ) {
		    LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		    exit( 1 );
		}
		strcpy( pr->p_role, p );
	    }

	    if (( c = pgetnum( "si" )) < 0 ) {
		pr->p_srvid = defprinter.p_srvid;
	    } else {
		pr->p_srvid = c;
	    }
#endif /* ABS_PRINT */
	}


	/*
	 * Cost of printer.
	 */
	if ( pr->p_pagecost_msg != NULL &&
		pr->p_pagecost_msg != defprinter.p_pagecost_msg ) {
	    free( pr->p_pagecost_msg );
	}
	a = area;
	if (( p = pgetstr( "pc", &a )) != NULL ) {
	    if (( pr->p_pagecost_msg =
		    (char *)malloc( strlen( p ) + 1 )) == NULL ) {
		LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		exit( 1 );
	    }
	    strcpy( pr->p_pagecost_msg, p );
	    pr->p_pagecost = 0;
	} else if ( pr->p_flags & P_ACCOUNT ) {
	    if (( c = pgetnum( "pc" )) < 0 ) {
		pr->p_pagecost = defprinter.p_pagecost;
	    } else {
		pr->p_pagecost = c;
	    }
	    pr->p_pagecost_msg = NULL;
	}

	/*
	 * Get lpd lock file.
	 */
	if ( pr->p_lock != NULL && pr->p_lock != defprinter.p_lock ) {
	    free( pr->p_lock );
	}
	a = area;
	if (( p = pgetstr( "lo", &a )) == NULL ) {
	    pr->p_lock = defprinter.p_lock;
	} else {
	    if (( pr->p_lock = (char *)malloc( strlen( p ) + 1 )) == NULL ) {
		LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
		exit( 1 );
	    }
	    strcpy( pr->p_lock, p );
	}

#ifdef KRB
	/*
	 * Must Kerberos authenticate?
	 */
	if ( pgetflag( "ka" ) == 1 ) {
	    pr->p_flags |= P_KRB;
	} else {
	    pr->p_flags &= ~P_KRB;
	}
#endif /* KRB */

	endprent();
    }
#endif /* HAVE_CUPS */

    return( 0 );
}
