/*
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#if defined( sun ) && defined( __svr4__ )
#include </usr/ucbinclude/sys/file.h>
#else sun __svr4__
#include <sys/file.h>
#endif sun __svr4__
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/wait.h>
#include <errno.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/pap.h>
#include <atalk/paths.h>
#include <atalk/util.h>

#include "printer.h"

#define _PATH_PAPDPPDFILE	".ppd"

#define PIPED_STATUS	"status: print spooler processing job"

struct printer	defprinter;
struct printer	*printers = NULL;

int		debug = 0;
char		*conffile = _PATH_PAPDCONF;
char		*printcap = _PATH_PAPDPRINTCAP;
unsigned char	connid, quantum, sock, oquantum = PAP_MAXQUANTUM;
char		*cannedstatus = PIPED_STATUS;
struct printer	*printer = NULL;
char		*version = VERSION;
static char      *pidfile = _PATH_PAPDLOCK;

char		*uamlist;
char		*uampath = _PATH_PAPDUAMPATH;

/* this only needs to be used by the server process */
static void papd_exit(const int i)
{
  server_unlock(pidfile);
  auth_unload();
  exit(i);
}

#if !defined( ibm032 ) && !defined( _IBMR2 )
    void
#endif ibm032 _IBMR2
die( n )
    int			n;
{
    struct printer	*pr;

    for ( pr = printers; pr; pr = pr->p_next ) {
	if ( pr->p_flags & P_REGISTERED ) {
	    if ( nbp_unrgstr( pr->p_name, pr->p_type, pr->p_zone ) < 0 ) {
		syslog( LOG_ERR, "can't unregister %s:%s@%s\n", pr->p_name,
			pr->p_type, pr->p_zone );
		papd_exit( n + 1 );
	    }
	    syslog( LOG_ERR, "unregister %s:%s@%s\n", pr->p_name, pr->p_type,
		    pr->p_zone );
	}
    }
    papd_exit( n );
}

#if !defined( ibm032 ) && !defined( _IBMR2 )
    void
#endif ibm032 _IBMR2
reap()
{
    int		status;
    int		pid;

    while (( pid = wait3( &status, WNOHANG, 0 )) > 0 ) {
	if ( WIFEXITED( status )) {
	    if ( WEXITSTATUS( status )) {
		syslog( LOG_ERR, "child %d exited with %d", pid,
			WEXITSTATUS( status ));
	    } else {
		syslog( LOG_INFO, "child %d done", pid );
	    }
	} else {
	    if ( WIFSIGNALED( status )) {
		syslog( LOG_ERR, "child %d killed with %d", pid,
			WTERMSIG( status ));
	    } else {
		syslog( LOG_ERR, "child %d died", pid );
	    }
	}
    }
    return;
}

char		rbuf[ 255 + 1 + 8 ];

main( ac, av )
    int		ac;
    char	**av;
{
    extern char         *optarg;
    extern int          optind;

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

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	perror( "gethostname" );
	exit( 1 );
    }
    if (( p = strchr( hostname, '.' )) != 0 ) {
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
    defprinter.p_ppdfile = _PATH_PAPDPPDFILE;
#ifdef __svr4__
    defprinter.p_flags = P_PIPED;
    defprinter.p_printer = "/usr/bin/lp -T PS";
#else
    defprinter.p_flags = P_SPOOLED;
    defprinter.p_printer = "lp";
#endif
    defprinter.p_operator = "operator";
    defprinter.p_spool = _PATH_PAPDSPOOLDIR;
#ifdef ABS_PRINT
    defprinter.p_role = NULL;
    defprinter.p_srvid = 0;
#endif ABS_PRINT
    defprinter.p_pagecost = 200;		/* default cost */
    defprinter.p_pagecost_msg = NULL;
    defprinter.p_lock = "lock";

    while (( c = getopt( ac, av, "adf:p:P:" )) != EOF ) {
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

	default :
	    fprintf( stderr,
		    "Usage:\t%s [ -d ] [ -f conffile ] [ -p printcap ]\n",
		    *av );
	    exit( 1 );
	}
    }

    getprinters( conffile );

    switch (server_lock("papd", pidfile, debug)) {
    case 0: /* open a couple things again in the child */
      if ((c = open("/", O_RDONLY)) >= 0) {
	dup2(c, 1);
	dup2(c, 2);
      }
      break;
    case -1:
      exit(1);
    default:
      exit(0);
    }      

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
#else ultrix
    openlog( p, LOG_NDELAY|LOG_PID, LOG_LPR );
#endif ultrix

    syslog( LOG_INFO, "restart (%s)", version );

    for ( pr = printers; pr; pr = pr->p_next ) {
	if (( pr->p_flags & P_SPOOLED ) && rprintcap( pr ) < 0 ) {
	    syslog( LOG_ERR, "printcap problem: %s", pr->p_printer );
	}
	if (( pr->p_atp = atp_open( ATADDR_ANYPORT, &pr->p_addr )) == NULL ) {
	    syslog( LOG_ERR, "atp_open: %m" );
	    papd_exit( 1 );
	}
	if ( nbp_rgstr( atp_sockaddr( pr->p_atp ), pr->p_name, pr->p_type,
		pr->p_zone ) < 0 ) {
	    syslog( LOG_ERR, "can't register %s:%s@%s", pr->p_name, pr->p_type,
		    pr->p_zone );
	    die( 1 );
	}
	if ( pr->p_flags & P_AUTH ) {
		syslog( LOG_INFO, "Authentication enabled: %s", pr->p_name );
	}
	else {
		syslog( LOG_INFO, "Authentication disabled: %s", pr->p_name );
	}
	syslog( LOG_INFO, "register %s:%s@%s", pr->p_name, pr->p_type,
		pr->p_zone );
	pr->p_flags |= P_REGISTERED;
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = die;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "sigaction: %m" );
	papd_exit( 1 );
    }

    sv.sa_handler = reap;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "sigaction: %m" );
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
	if (( c = select( FD_SETSIZE, &fdset, 0, 0, 0 )) < 0 ) {
	    if ( errno == EINTR ) {
		continue;
	    }
	    syslog( LOG_ERR, "select: %m" );
	    papd_exit( 1 );
	}

	for ( pr = printers; pr; pr = pr->p_next ) {
	    if ( FD_ISSET( atp_fileno( pr->p_atp ), &fdset )) {
		int		err = 0;

		bzero( &sat, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
		sat.sat_len = sizeof( struct sockaddr_at );
#endif BSD4_4
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
		    syslog( LOG_ERR, "atp_rreq: %m" );
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
			syslog( LOG_ERR, "printcap problem: %s",
				pr->p_printer );
			rbuf[ 2 ] = rbuf[ 3 ] = 0xff;
			err = 1;
		    }

		    /*
		     * If this fails, we've run out of sockets. Rather than
		     * just die(), let's try to continue. Maybe some sockets
		     * will close, and we can continue;
		     */
		    if (( atp = atp_open( ATADDR_ANYPORT, 
					  &pr->p_addr)) == NULL ) {
			syslog( LOG_ERR, "atp_open: %m" );
			rbuf[ 2 ] = rbuf[ 3 ] = 0xff;
			err = 1;
		    }
		    rbuf[ 4 ] = atp_sockaddr( atp )->sat_port;
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
			syslog( LOG_ERR, "atp_sresp: %m" );
			continue;
		    }

		    if ( err ) {
			continue;
		    }

		    switch ( c = fork()) {
		    case -1 :
			syslog( LOG_ERR, "fork: %m" );
			continue;

		    case 0 : /* child */
			printer = pr;

			if (( printer->p_flags & P_SPOOLED ) &&
				chdir( printer->p_spool ) < 0 ) {
			    syslog( LOG_ERR, "chdir %s: %m", printer->p_spool );
			    exit( 1 );
			}

			sv.sa_handler = SIG_DFL;
			sigemptyset( &sv.sa_mask );
			sv.sa_flags = SA_RESTART;
			if ( sigaction( SIGTERM, &sv, 0 ) < 0 ) {
			    syslog( LOG_ERR, "sigaction: %m" );
			    exit( 1 );
			}

			for ( pr = printers; pr; pr = pr->p_next ) {
			    atp_close( pr->p_atp );
			}
			sat.sat_port = sock;
			if ( session( atp, &sat ) < 0 ) {
			    syslog( LOG_ERR, "bad session" );
			    exit( 1 );
			}
			exit( 0 );
			break;

		    default : /* parent */
			syslog( LOG_INFO, "child %d for \"%s\" from %u.%u",
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
			syslog( LOG_ERR, "atp_sresp: %m" );
		    }
		    break;

		default :
		    syslog( LOG_ERR, "Bad request from %u.%u!",
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
		    syslog( LOG_ERR, "atp_sresp: %m" );
		}
#endif notdef
	    }
	}
    }
}

/*
 * We assume buf is big enough for 255 bytes of data and a length byte.
 */
    int
getstatus( pr, buf )
    struct printer	*pr;
    char		*buf;
{
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
}

char	*pgetstr();
char	*getpname();

getprinters( cf )
    char	*cf;
{
    char		buf[ 1024 ], area[ 1024 ], *a, *p, *name, *type, *zone;
    struct printer	*pr;
    int			c;

    while (( c = getprent( cf, buf )) > 0 ) {
	a = area;
	/*
	 * Get the printer's nbp name.
	 */
	if (( p = getpname( &a )) == NULL ) {
	    fprintf( stderr, "No printer name\n" );
	    exit( 1 );
	}

	if (( pr = (struct printer *)malloc( sizeof( struct printer )))
		== NULL ) {
	    perror( "malloc" );
	    exit( 1 );
	}
	bzero( pr, sizeof( struct printer ));

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
	if (( p = pgetstr( "pd", &a )) == NULL ) {
	    pr->p_ppdfile = defprinter.p_ppdfile;
	} else {
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
	if ( pgetflag( "ca", &a ) == 1 ) {
	    pr->p_flags |= P_AUTH;
	    pr->p_flags |= P_AUTH_CAP;
	}
	if ( pgetflag( "sp", &a ) == 1 ) {
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

	pr->p_next = printers;
	printers = pr;
    }
    if ( c == 0 ) {
	endprent();
    } else {			/* No capability file, do default */
	printers = &defprinter;
    }
}

rprintcap( pr )
    struct printer	*pr;
{
    char		buf[ 1024 ], area[ 1024 ], *a, *p;
    int			c;

    /*
     * Spool directory from printcap file.
     */
    if ( pr->p_flags & P_SPOOLED ) {
	if ( pgetent( printcap, buf, pr->p_printer ) != 1 ) {
	    syslog( LOG_ERR, "No such printer: %s", pr->p_printer );
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
		syslog( LOG_ERR, "malloc: %m" );
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
		    syslog( LOG_ERR, "malloc: %m" );
		    exit( 1 );
		}
		strcpy( pr->p_role, p );
	    }

	    if (( c = pgetnum( "si" )) < 0 ) {
		pr->p_srvid = defprinter.p_srvid;
	    } else {
		pr->p_srvid = c;
	    }
#endif ABS_PRINT
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
		syslog( LOG_ERR, "malloc: %m" );
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
		syslog( LOG_ERR, "malloc: %m" );
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
#endif

	endprent();
    }

    return( 0 );
}
