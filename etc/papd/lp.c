/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * Portions:
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Interface to lpr system.
 */

#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#if defined( sun ) && defined( __svr4__ )
#include </usr/ucbinclude/sys/file.h>
#else sun __svr4__
#include <sys/file.h>
#endif sun __svr4__
#include <sys/un.h>
#include <netinet/in.h>
#undef s_net
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/paths.h>

#ifdef ABS_PRINT
#include <math.h>
#endif ABS_PRINT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>

#include "printer.h"
#include "file.h"

char	hostname[ MAXHOSTNAMELEN ];

/* initialize printing interface */
int	lp_init();
/* cancel current job */
int	lp_cancel();
/* print current job */
int	lp_print();

/* open a file for spooling */
int	lp_open();
/* open a buffer to the current open file */
int	lp_write();
/* close current spooling file */
int	lp_close();

struct lp {
    int			lp_flags;
    FILE		*lp_stream;
    int			lp_seq;
    char		lp_letter;
    char		*lp_person;
    char		*lp_host;
    char		*lp_job;
} lp;
#define LP_INIT		(1<<0)
#define LP_OPEN		(1<<1)
#define LP_PIPE		(1<<2)
#define LP_CONNECT	(1<<3)
#define LP_QUEUE	(1<<4)

lp_person( person )
    char	*person;
{
    if ( lp.lp_person != NULL ) {
	free( lp.lp_person );
    }
    if (( lp.lp_person = (char *)malloc( strlen( person ) + 1 )) == NULL ) {
	syslog( LOG_ERR, "malloc: %m" );
	exit( 1 );
    }
    strcpy( lp.lp_person, person );
}

#ifdef ABS_PRINT
lp_pagecost()
{
    char	cost[ 22 ];
    char	balance[ 22 ];
    int		err;

    if ( lp.lp_person == NULL ) {
	return( -1 );
    }
    err = ABS_canprint( lp.lp_person, printer->p_role, printer->p_srvid,
	    cost, balance );
    printer->p_pagecost = floor( atof( cost ) * 10000.0 );
    printer->p_balance = atof( balance ) + atof( cost );
    return( err < 0 ? -1 : 0 );
}
#endif ABS_PRINT

lp_host( host )
    char	*host;
{
    if ( lp.lp_host != NULL ) {
	free( lp.lp_host );
    }
    if (( lp.lp_host = (char *)malloc( strlen( host ) + 1 )) == NULL ) {
	syslog( LOG_ERR, "malloc: %m" );
	exit( 1 );
    }
    strcpy( lp.lp_host, host );
}

lp_job( job )
    char	*job;
{
    char	*p, *q;

    if ( lp.lp_job != NULL ) {
	free( lp.lp_job );
    }
    if (( lp.lp_job = (char *)malloc( strlen( job ) + 1 )) == NULL ) {
	syslog( LOG_ERR, "malloc: %m" );
	exit( 1 );
    }
    for ( p = job, q = lp.lp_job; *p != '\0'; p++, q++ ) {
	if ( !isascii( *p ) || !isprint( *p ) || *p == '\\' ) {
	    *q = '.';
	} else {
	    *q = *p;
	}
    }
    *q = '\0';
}

lp_init( out )
    struct papfile	*out;
{
    int		fd, n, len;
    char	*cp, buf[ BUFSIZ ];
    struct stat	st;
#ifdef ABS_PRINT
    char	cost[ 22 ];
    char	balance[ 22 ];
#endif ABS_PRINT

    if ( printer->p_flags & P_AUTH ) {
	if ( lp.lp_person == NULL ) {
	    syslog( LOG_ERR, "lp_init: must authenticate" );
	    spoolerror( out, "Authentication required." );
	    return( -1 );
	}
#ifdef ABS_PRINT
	if (( printer->p_flags & P_ACCOUNT ) && printer->p_pagecost > 0 &&
		! ABS_canprint( lp.lp_person, printer->p_role,
		printer->p_srvid, cost, balance )) {
	    syslog( LOG_ERR, "lp_init: no ABS funds" );
	    spoolerror( out, "No ABS funds available." );
	    return( -1 );
	}
#endif ABS_PRINT
    }

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	syslog( LOG_ERR, "gethostname: %m" );
	exit( 1 );
    }

    if ( lp.lp_flags & LP_INIT ) {
	syslog( LOG_ERR, "lp_init: already inited, die!" );
	abort();
    }

    lp.lp_flags = 0;
    lp.lp_stream = NULL;
    lp.lp_letter = 'A';

    if ( printer->p_flags & P_SPOOLED ) {
	/* check if queuing is enabled: mode & 010 on lock file */
	if ( stat( printer->p_lock, &st ) < 0 ) {
	    syslog( LOG_ERR, "lp_init: %s: %m", printer->p_lock );
	    spoolerror( out, NULL );
	    return( -1 );
	}
	if ( st.st_mode & 010 ) {
	    syslog( LOG_INFO, "lp_init: queuing is disabled" );
	    spoolerror( out, "Queuing is disabled." );
	    return( -1 );
	}

	if (( fd = open( ".seq", O_RDWR|O_CREAT, 0661 )) < 0 ) {
	    syslog( LOG_ERR, "lp_init: can't create .seq" );
	    spoolerror( out, NULL );
	    return( -1 );
	}

	if ( flock( fd, LOCK_EX ) < 0 ) {
	    syslog( LOG_ERR, "lp_init: can't lock .seq" );
	    spoolerror( out, NULL );
	    return( -1 );
	}

	n = 0;
	if (( len = read( fd, buf, sizeof( buf ))) < 0 ) {
	    syslog( LOG_ERR, "lp_init read: %m" );
	    spoolerror( out, NULL );
	    return( -1 );
	}
	if ( len > 0 ) {
	    for ( cp = buf; len; len--, cp++ ) {
		if ( *cp < '0' || *cp > '9' ) {
		    break;
		}
		n = n * 10 + ( *cp - '0' );
	    }
	}
	lp.lp_seq = n;

	n = ( n + 1 ) % 1000;
	sprintf( buf, "%03d\n", n );
	lseek( fd, 0L, 0 );
	write( fd, buf, strlen( buf ));
	close( fd );
    } else {
	lp.lp_flags |= LP_PIPE;
	lp.lp_seq = getpid();
    }

    lp.lp_flags |= LP_INIT;
    return( 0 );
}

lp_open( out )
    struct papfile	*out;
{
    char	name[ MAXPATHLEN ];
    int		fd;

    if (( lp.lp_flags & LP_INIT ) == 0 && lp_init( out ) != 0 ) {
	return( -1 );
    }
    if ( lp.lp_flags & LP_OPEN ) {
	syslog( LOG_ERR, "lp_open already open" );
	abort();
    }

    if ( lp.lp_flags & LP_PIPE ) {
	/* go right to program */
	if (( lp.lp_stream = popen( printer->p_printer, "w" )) == NULL ) {
	    syslog( LOG_ERR, "lp_open popen %s: %m", printer->p_printer );
	    spoolerror( out, NULL );
	    return( -1 );
	}
    } else {
	sprintf( name, "df%c%03d%s", lp.lp_letter++, lp.lp_seq, hostname );

	if (( fd = open( name, O_WRONLY|O_CREAT|O_EXCL, 0660 )) < 0 ) {
	    syslog( LOG_ERR, "lp_open %s: %m", name );
	    spoolerror( out, NULL );
	    return( -1 );
	}
	if (( lp.lp_stream = fdopen( fd, "w" )) == NULL ) {
	    syslog( LOG_ERR, "lp_open fdopen: %m" );
	    spoolerror( out, NULL );
	    return( -1 );
	}
    }
    lp.lp_flags |= LP_OPEN;

    return( 0 );
}

lp_close()
{
    if (( lp.lp_flags & LP_INIT ) == 0 || ( lp.lp_flags & LP_OPEN ) == 0 ) {
	return;
    }
    fclose( lp.lp_stream );
    lp.lp_stream = NULL;
    lp.lp_flags &= ~LP_OPEN;
    return;
}

lp_write( buf, len )
    char	*buf;
    int		len;
{
    if (( lp.lp_flags & LP_OPEN ) == 0 ) {
	return( -1 );
    }

    if ( fwrite( buf, 1, len, lp.lp_stream ) != len ) {
	syslog( LOG_ERR, "lp_write: %m" );
	abort();
    }
    return( 0 );
}

lp_cancel()
{
    char	name[ MAXPATHLEN ];
    char	letter;

    if (( lp.lp_flags & LP_INIT ) == 0 || lp.lp_letter == 'A' ) {
	return;
    }

    if ( lp.lp_flags & LP_OPEN ) {
	lp_close();
    }

    for ( letter = 'A'; letter < lp.lp_letter; letter++ ) {
	sprintf( name, "df%c%03d%s", letter, lp.lp_seq, hostname );
	if ( unlink( name ) < 0 ) {
	    syslog( LOG_ERR, "lp_cancel unlink %s: %m", name );
	}
    }

    return;
}

/*
 * Create printcap control file, signal printer.  Errors here should
 * remove queue files.
 *
 * XXX piped?
 */
lp_print()
{
    char		buf[ MAXPATHLEN ];
    char		tfname[ MAXPATHLEN ];
    char		cfname[ MAXPATHLEN ];
    char		letter;
    int			fd, n, s;
    FILE		*cfile;

    if (( lp.lp_flags & LP_INIT ) == 0 || lp.lp_letter == 'A' ) {
	return;
    }
    lp_close();

    if ( printer->p_flags & P_SPOOLED ) {
	sprintf( tfname, "tfA%03d%s", lp.lp_seq, hostname );
	if (( fd = open( tfname, O_WRONLY|O_EXCL|O_CREAT, 0660 )) < 0 ) {
	    syslog( LOG_ERR, "lp_print %s: %m", tfname );
	    return;
	}
	if (( cfile = fdopen( fd, "w" )) == NULL ) {
	    syslog( LOG_ERR, "lp_print %s: %m", tfname );
	    return;
	}
	fprintf( cfile, "H%s\n", hostname );	/* XXX lp_host? */

	if ( lp.lp_person ) {
	    fprintf( cfile, "P%s\n", lp.lp_person );
	} else {
	    fprintf( cfile, "P%s\n", printer->p_operator );
	}

	if ( lp.lp_job && *lp.lp_job ) {
	    fprintf( cfile, "J%s\n", lp.lp_job );
	    fprintf( cfile, "T%s\n", lp.lp_job );
	} else {
	    fprintf( cfile, "JMac Job\n" );
	    fprintf( cfile, "TMac Job\n" );
	}

	fprintf( cfile, "C%s\n", hostname );	/* XXX lp_host? */

	if ( lp.lp_person ) {
	    fprintf( cfile, "L%s\n", lp.lp_person );
	} else {
	    fprintf( cfile, "L%s\n", printer->p_operator );
	}

	for ( letter = 'A'; letter < lp.lp_letter; letter++ ) {
	    fprintf( cfile, "fdf%c%03d%s\n", letter, lp.lp_seq, hostname );
	    fprintf( cfile, "Udf%c%03d%s\n", letter, lp.lp_seq, hostname );
	}

	if ( lp.lp_job && *lp.lp_job ) {
	    fprintf( cfile, "N%s\n", lp.lp_job );
	} else {
	    fprintf( cfile, "NMac Job\n" );
	}
	fclose( cfile );

	sprintf( cfname, "cfA%03d%s", lp.lp_seq, hostname );
	if ( link( tfname, cfname ) < 0 ) {
	    syslog( LOG_ERR, "lp_print can't link %s to %s: %m", cfname,
		    tfname );
	    return;
	}
	unlink( tfname );

	if (( s = lp_conn_unix()) < 0 ) {
	    syslog( LOG_ERR, "lp_print: lp_conn_unix: %m" );
	    return;
	}

	sprintf( buf, "\1%s\n", printer->p_printer );
	n = strlen( buf );
	if ( write( s, buf, n ) != n ) {
	    syslog( LOG_ERR, "lp_print write: %m" );
	    return;
	}
	if ( read( s, buf, 1 ) != 1 ) {
	    syslog( LOG_ERR, "lp_print read: %m" );
	    return;
	}

	lp_disconn_unix( s );

	if ( buf[ 0 ] != '\0' ) {
	    syslog( LOG_ERR, "lp_print lpd said %c: %m", buf[ 0 ] );
	    return;
	}
    }
    syslog( LOG_INFO, "lp_print queued" );
    return;
}

lp_disconn_unix( fd )
{
    return( close( fd ));
}

lp_conn_unix()
{
    int			s;
    struct sockaddr_un	saun;

    if (( s = socket( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) {
	syslog( LOG_ERR, "lp_conn_unix socket: %m" );
	return( -1 );
    }
    bzero( &saun, sizeof( struct sockaddr_un ));
    saun.sun_family = AF_UNIX;
    strcpy( saun.sun_path, _PATH_DEVPRINTER );
    if ( connect( s, (struct sockaddr *)&saun,
	    strlen( saun.sun_path ) + 2 ) < 0 ) {
	syslog( LOG_ERR, "lp_conn_unix connect %s: %m", saun.sun_path );
	close( s );
	return( -1 );
    }

    return( s );
}

lp_disconn_inet( fd )
{
    return( close( fd ));
}

lp_conn_inet()
{
    int			privfd, port = IPPORT_RESERVED - 1;
    struct sockaddr_in	sin;
    struct servent	*sp;
    struct hostent	*hp;

    if (( sp = getservbyname( "printer", "tcp" )) == NULL ) {
	syslog( LOG_ERR, "printer/tcp: unknown service\n" );
	return( -1 );
    }

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	syslog( LOG_ERR, "gethostname: %m" );
	exit( 1 );
    }

    if (( hp = gethostbyname( hostname )) == NULL ) {
	syslog( LOG_ERR, "%s: unknown host\n", hostname );
	return( -1 );
    }

    if (( privfd = rresvport( &port )) < 0 ) {
	syslog( LOG_ERR, "lp_connect: socket: %m" );
	close( privfd );
	return( -1 );
    }

    bzero( &sin, sizeof( struct sockaddr_in ));
    sin.sin_family = AF_INET;
/*    sin.sin_addr.s_addr = htonl( INADDR_LOOPBACK ); */
    bcopy( hp->h_addr, &sin.sin_addr, hp->h_length );
    sin.sin_port = sp->s_port;

    if ( connect( privfd, (struct sockaddr *)&sin,
	    sizeof( struct sockaddr_in )) < 0 ) {
	syslog( LOG_ERR, "lp_connect: %m" );
	close( privfd );
	return( -1 );
    }

    return( privfd );
}

lp_rmjob( job )
    int		job;
{
    char	buf[ 1024 ];
    int		n, s;

    if (( s = lp_conn_inet()) < 0 ) {
	syslog( LOG_ERR, "lp_rmjob: %m" );
	return( -1 );
    }

    if ( lp.lp_person == NULL ) {
	return( -1 );
    }

    sprintf( buf, "\5%s %s %d\n", printer->p_printer, lp.lp_person, job );
    n = strlen( buf );
    if ( write( s, buf, n ) != n ) {
	syslog( LOG_ERR, "lp_rmjob write: %m" );
	lp_disconn_inet( s );
	return( -1 );
    }
    while (( n = read( s, buf, sizeof( buf ))) > 0 ) {
	syslog( LOG_DEBUG, "read %.*s", n, buf );
    }

    lp_disconn_inet( s );
    return( 0 );
}

char	*kw_rank = "Rank";
char	*kw_active = "active";

char	*tag_rank = "rank: ";
char	*tag_owner = "owner: ";
char	*tag_job = "job: ";
char	*tag_files = "files: ";
char	*tag_size = "size: ";
char	*tag_status = "status: ";

lp_queue( out )
    struct papfile	*out;
{
    char			buf[ 1024 ], *start, *stop, *p, *q;
    int				linelength, crlflength;
    static struct papfile	pf;
    int				n, len, s;
	
    if (( s = lp_conn_unix()) < 0 ) {
	syslog( LOG_ERR, "lp_queue: %m" );
	return( -1 );
    }

    sprintf( buf, "\3%s\n", printer->p_printer );
    n = strlen( buf );
    if ( write( s, buf, n ) != n ) {
	syslog( LOG_ERR, "lp_queue write: %m" );
	lp_disconn_unix( s );
	return( -1 );
    }
    pf.pf_state = PF_BOT;

    while (( n = read( s, buf, sizeof( buf ))) > 0 ) {
	append( &pf, buf, n );
    }

    for (;;) {
	if ( markline( &pf, &start, &linelength, &crlflength ) > 0 ) {
	    /* parse */
	    stop = start + linelength;
	    for ( p = start; p < stop; p++ ) {
		if ( *p == ' ' || *p == '\t' ) {
		    break;
		}
	    }
	    if ( p >= stop ) {
		CONSUME( &pf , linelength + crlflength);
		continue;
	    }

	    /*
	     * Keys: "Rank", a number, "active"
	     * Anything else is status.
	     */
	    len = p - start;
	    if ( len == strlen( kw_rank ) &&
		    strncmp( kw_rank, start, len ) == 0 ) {
		CONSUME( &pf, linelength + crlflength );
		continue;
	    }
	    if (( len == strlen( kw_active ) &&
		    strncmp( kw_active, start, len ) == 0 ) ||
		    isdigit( *start )) {		/* a job line */
		append( out, tag_rank, strlen( tag_rank ));
		append( out, start, p - start );
		append( out, "\n", 1 );

		for ( ; p < stop; p++ ) {
		    if ( *p != ' ' && *p != '\t' ) {
			break;
		    }
		}
		for ( q = p; p < stop; p++ ) {
		    if ( *p == ' ' || *p == '\t' ) {
			break;
		    }
		}
		if ( p >= stop ) {
		    append( out, ".\n", 2 );
		    CONSUME( &pf, linelength + crlflength );
		    continue;
		}
		append( out, tag_owner, strlen( tag_owner ));
		append( out, q, p - q );
		append( out, "\n", 1 );

		for ( ; p < stop; p++ ) {
		    if ( *p != ' ' && *p != '\t' ) {
			break;
		    }
		}
		for ( q = p; p < stop; p++ ) {
		    if ( *p == ' ' || *p == '\t' ) {
			break;
		    }
		}
		if ( p >= stop ) {
		    append( out, ".\n", 2 );
		    CONSUME( &pf , linelength + crlflength );
		    continue;
		}
		append( out, tag_job, strlen( tag_job ));
		append( out, q, p - q );
		append( out, "\n", 1 );

		for ( ; p < stop; p++ ) {
		    if ( *p != ' ' && *p != '\t' ) {
			break;
		    }
		}
		for ( q = p, p = stop; p > q; p-- ) {
		    if ( *p == ' ' || *p == '\t' ) {
			break;
		    }
		}
		for ( ; p > q; p-- ) {
		    if ( *p != ' ' && *p != '\t' ) {
			break;
		    }
		}
		for ( ; p > q; p-- ) {
		    if ( *p == ' ' || *p == '\t' ) {
			break;
		    }
		}
		if ( p <= q ) {
		    append( out, ".\n", 2 );
		    CONSUME( &pf, linelength + crlflength );
		    continue;
		}
		append( out, tag_files, strlen( tag_files ));
		append( out, q, p - q );
		append( out, "\n", 1 );

		for ( ; p < stop; p++ ) {
		    if ( *p != ' ' && *p != '\t' ) {
			break;
		    }
		}
		append( out, tag_size, strlen( tag_size ));
		append( out, p, stop - p );
		append( out, "\n.\n", 3 );

		CONSUME( &pf, linelength + crlflength );
		continue;
	    }

	    /* status */
	    append( out, tag_status, strlen( tag_status ));
	    append( out, start, linelength );
	    append( out, "\n.\n", 3 );

	    CONSUME( &pf, linelength + crlflength );
	} else {
	    append( out, "*\n", 2 );
	    lp_disconn_unix( s );
	    return( 0 );
	}
    }
}
