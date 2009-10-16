/*
 * $Id: psf.c,v 1.13 2009-10-16 01:50:50 didg Exp $
 *
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * PostScript Filter, psf.
 *
 * Handles both PostScript files and text files.  Files with the
 * '%!' PostScript header are sent directly to the printer,
 * unmodified.  Text files are first converted to PostScript,
 * then sent.  Printers may be directly attached or on an AppleTalk
 * network.  Other media are possible.  Currently, psf invokes
 * pap to send files to AppleTalk-ed printers.  Replace the pap*
 * variables to use another program for communication.  psf only
 * converts plain-text.  If called as "tf" or "df", psf will invoke
 * a troff or dvi to PostScript converter.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define FUCKED

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/time.h>

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

#include <sys/file.h>
#include <syslog.h>
#include <atalk/paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

/* Forward Declarations */
int pexecv(char *path, char *argv[]);
int copyio();
int textps();

static char		psapath[] = _PATH_PSA;
static char		*psaargv[] = { "psa", NULL, NULL, NULL, NULL };

/*
 * If we're not doing accounting, we just call pap as below.
 * If we are doing accounting, we call pap twice.  The first time,
 * we call it with "-E" in arg 2, pagecount.ps in arg 3, and "-" in
 * arg 4.  The second time, we call it with "-c" in arg 2, pagecount.ps
 * in arg 3, and 0 in arg 4.
 */
static char		pappath[] = _PATH_PAP;
static char		*papargv[] = { "pap", "-sstatus", NULL, NULL, NULL, NULL, NULL, NULL };

static char		revpath[] = _PATH_PSORDER;
static char		*revargv[] = { "psorder", "-d", NULL };

static char		*filtargv[] = { NULL, NULL, NULL };

static char		inbuf[ 1024 * 8 ];
static int		inlen;

static int		literal;
static int		width = 80, length = 66, indent = 0;
static char		*prog, *name, *host;

static struct papersize {
    int		width;
    int		length;
   float	win;
    float	lin;
} papersizes[] = {
    { 80, 66, 8.5, 11.0 },			/* US Letter */
    { 80, 70, 8.27, 11.69 },			/* A4 */
};

int main( int ac, char **av)
{
    int			c, rc, children = 0;
#ifdef FUCKED
    int			psafileno = 0, multiconn = 0, waitidle = 0, waitidle2 = 0;
#endif /* FUCKED */
    int			status;
    extern char		*optarg;
    extern int		optind, opterr;

    opterr = 0;
    if (( prog = rindex( av[ 0 ], '/' )) == NULL ) {
	prog = av[ 0 ];
    } else {
	prog++;
    }
#ifdef ultrix
    openlog( prog, LOG_PID );
#else /* ultrix */
    openlog( prog, LOG_PID, LOG_LPR );
#endif /* ultrix */

    while (( c = getopt( ac, av, "P:C:D:F:L:J:x:y:n:h:w:l:i:c" )) != EOF ) {
	switch ( c ) {
	case 'n' :
	    name = optarg;
	    break;

	case 'h' :
	    host = optarg;
	    break;

	case 'w' :
	    width = atoi( optarg );
#ifdef ZEROWIDTH
	    /*
	     * Some version of lpd pass 0 for the page width.
	     */
	    if ( width == 0 ) {
		width = 80;
	    }
#endif /* ZEROWIDTH */
	    break;

	case 'l' :
	    length = atoi( optarg );
	    break;

	case 'i' :
	    indent = atoi( optarg );
	    break;

	case 'c' :	/* Print control chars */
	    literal++;
	    break;

	case 'F' :
	case 'L' :
	case 'J' :
	case 'P' :
	case 'x' :
	case 'y' :
	    break;
	
#ifdef notdef
	default :
	    syslog( LOG_ERR, "bad option: %c", c );
	    exit( 2 );
#endif /* notdef */
	}
    }
    if ( ac - optind > 1 ) {
	syslog( LOG_ERR, "Too many arguments" );
	exit( 2 );
    }
#ifdef FUCKED
    if ( index( prog, 'w' )) {
	waitidle++;
    }
    if ( index( prog, 'W' )) {
	waitidle2++;
    }
    if ( index( prog, 'm' )) {
	multiconn++;
    }
#endif /* FUCKED */

    syslog( LOG_INFO, "starting for %s", name ? name : "?" );

restart:
    if (( inlen = read( 0, inbuf, sizeof( inbuf ))) < 0 ) {
	syslog( LOG_ERR, "read: %s", strerror(errno) );
	exit( 1 );
    }
    if ( inlen == 0 ) {	/* nothing to be done */
	syslog( LOG_INFO, "done" );
	exit( 0 );
    }

    /*
     * If we've been given an accounting file, start the accounting
     * process.
     */
    if ( optind < ac ) {
	/* build arguments */
	psaargv[ 1 ] = av[ optind ];
	psaargv[ 2 ] = name;
	psaargv[ 3 ] = host;
	if (( c = pexecv( psapath, psaargv )) < 0 ) {
	    syslog( LOG_ERR, "%s: %s", psapath, strerror(errno) );
	    exit( 2 );
	}
	children++;
	syslog( LOG_INFO, "accounting with psa[%d]", c );
    }

    /*
     * Check prog's name to decide what programs to execute.
     */
    if ( strstr( prog, "pap" ) != NULL ) {
	if ( optind < ac ) {	/* accounting */
#ifdef FUCKED
	    if ( multiconn ) {
		psafileno = getdtablesize();
		psafileno--;
		dup2( 1, psafileno );

		if ( waitidle2 ) {
		    papargv[ 2 ] = "-W";
		    papargv[ 3 ] = "-c";
		    papargv[ 4 ] = "-E";
		    papargv[ 5 ] = _PATH_PAGECOUNT;
		    papargv[ 6 ] = "-";
		    papargv[ 7 ] = NULL;
		} else if ( waitidle ) {
		    papargv[ 2 ] = "-w";
		    papargv[ 3 ] = "-c";
		    papargv[ 4 ] = "-E";
		    papargv[ 5 ] = _PATH_PAGECOUNT;
		    papargv[ 6 ] = "-";
		    papargv[ 7 ] = NULL;
		} else {
		    papargv[ 2 ] = "-c";
		    papargv[ 3 ] = "-E";
		    papargv[ 4 ] = _PATH_PAGECOUNT;
		    papargv[ 5 ] = "-";
		    papargv[ 6 ] = NULL;
		}
	    } else {
		/*
		 * This is how it should be done.
		 */
		papargv[ 2 ] = "-c";
		papargv[ 3 ] = _PATH_PAGECOUNT;
		papargv[ 4 ] = "-";
		papargv[ 5 ] = _PATH_PAGECOUNT;
		papargv[ 6 ] = NULL;
	    }
#endif /* FUCKED */
	} else {
	    papargv[ 2 ] = "-c";
	    papargv[ 3 ] = "-E";
	    papargv[ 4 ] = NULL;
	}

	if (( c = pexecv( pappath, papargv )) < 0 ) {
	    syslog( LOG_ERR, "%s: %s", pappath, strerror(errno) );
	    exit( 2 );
	}
	children++;
	syslog( LOG_INFO, "sending to pap[%d]", c );
    }

    /*
     * Might be a good idea to have both a "forw" and a "rev", so that
     * reversed documents can be reordered for the printing device.
     */
    if ( strstr( prog, "rev" ) != NULL ) {
	if (( c = pexecv( revpath, revargv )) < 0 ) {
	    syslog( LOG_ERR, "%s: %s", revpath, strerror(errno) );
	    exit( 2 );
	}
	syslog( LOG_INFO, "sending to rev[%d]", c );
	children++;
    }

    /*
     * Invoke an external (script) filter to produce PostScript from
     * non-text input.
     */
    if ( *prog != 'i' && *prog != 'o' && *( prog + 1 ) == 'f' ) {
	filtargv[ 0 ] = filtargv[ 1 ] = prog;
	if (( c = pexecv( _PATH_PSFILTER, filtargv )) < 0 ) {
	    syslog( LOG_ERR, "%s: %s", _PATH_PSFILTER, strerror(errno) );
	    exit( 2 );
	}
	syslog( LOG_INFO, "external filter[%d]", c );
	children++;
	rc = copyio();		/* external filter */
    } else {
	if ( inlen >= 2 && inbuf[ 0 ] == '%' && inbuf[ 1 ] == '!' ) {
	    syslog( LOG_INFO, "PostScript" );
	    rc = copyio();	/* PostScript */
	} else if ( inlen >= 2 && inbuf[ 0 ] == '\033' && inbuf[ 1 ] == '%' ) {
	    syslog( LOG_INFO, "PostScript w/PJL" );
	    rc = copyio();      /* PostScript */
	} else {
	    syslog( LOG_INFO, "straight text" );
	    rc = textps();	/* straight text */
	}
    }

#ifdef FUCKED
    if ( strstr( prog, "pap" ) != NULL && optind < ac && multiconn ) {
	dup2( psafileno, 1 );
	close( psafileno );
	papargv[ 2 ] = "-c";
	if ( waitidle2 ) {
	    papargv[ 3 ] = "-W";
	    papargv[ 4 ] = _PATH_PAGECOUNT;
	    papargv[ 5 ] = NULL;
	} else if ( waitidle ) {
	    papargv[ 3 ] = "-w";
	    papargv[ 4 ] = _PATH_PAGECOUNT;
	    papargv[ 5 ] = NULL;
	} else {
	    papargv[ 3 ] = _PATH_PAGECOUNT;
	    papargv[ 4 ] = NULL;
	}

	if (( c = pexecv( pappath, papargv )) < 0 ) {
	    syslog( LOG_ERR, "%s: %s", pappath, strerror(errno) );
	    exit( 2 );
	}
	children++;
	syslog( LOG_INFO, "pagecount with pap[%d]", c );
    }
#endif /* FUCKED */

    if ( children ) {
	close( 1 );
    }
    while ( children ) {
	if (( c = wait3( &status, 0, NULL )) < 0 ) {
	    syslog( LOG_ERR, "wait3: %s", strerror(errno) );
	    exit( 1 );
	}
	if ( WIFEXITED( status )) {
#ifndef WEXITSTATUS
#define WEXITSTATUS(x)	((x).w_status)
#endif /* WEXITSTATUS */
	    if ( WEXITSTATUS( status ) != 0 ) {
		syslog( LOG_ERR, "%d died with %d", c, WEXITSTATUS( status ));
		exit( WEXITSTATUS( status ));
	    } else {
		syslog( LOG_INFO, "%d done", c );
		children--;
	    }
	} else {
	    syslog( LOG_ERR, "%d died badly", c );
	    exit( 1 );
	}
    }

    if ( rc == 3 ) {
	syslog( LOG_INFO, "pausing" );
	kill( getpid(), SIGSTOP );
	syslog( LOG_INFO, "restarting" );
	goto restart;
    }

    syslog( LOG_INFO, "done" );
    exit( rc );
}

int copyio(void)
{
    /* implement the FSM needed to do the suspend. Note that
     * the last characters will be \031\001 so don't worry
     * Fun things: 1. \031\001 should not be written to output device
     *  2. The \031 can be last char of one read, \001 first of next
     *      - we need to write \031 if not followed by \001
     */
    struct timeval	tv;
    fd_set		fdset;
    int			ctl = 0;

notdone:
    do {
	/*
	 * First, \031 and \001 *must* be the last things in the buffer
	 * (\001 can be the first thing in the next buffer).  There's no
	 * need to scan any of the intervening bytes.  Second, if there's
	 * more input, the escape sequence was bogus, and we should keep
	 * reading.
	 */
	if ( inlen == 1 ) {
	    if ( ctl == 1 ) {
		if ( inbuf[ 0 ] == '\001' ) {
		    ctl = 2;
		    break;
		}
		if ( write( 1, "\031", 1 ) != 1 ) {
		    syslog( LOG_ERR, "write: %s", strerror(errno) );
		    return( 1 );
		}
		ctl = 0;
	    }

	    if ( inbuf[ 0 ] == '\031' ) {
		ctl = 1;
	    }

	} else {
	    if ( ctl == 1 ) {
		if ( write( 1, "\031", 1 ) != 1 ) {
		    syslog( LOG_ERR, "write: %s", strerror(errno) );
		    return( 1 );
		}
	    }
	    ctl = 0;
	    if ( inbuf[ inlen - 2 ] == '\031' &&
		    inbuf[ inlen - 1 ] == '\001' ) {
		ctl = 2;
	    } else if ( inbuf[ inlen - 1 ] == '\031' ) {
		ctl = 1;
	    }
	}

	inlen -= ctl;
	if (( inlen > 0 ) && ( write( 1, inbuf, inlen ) != inlen )) {
	    syslog( LOG_ERR, "write: %s", strerror(errno) );
	    return( 1 );
	}
	if ( ctl == 2 ) {
	    break;
	}
    } while (( inlen = read( 0, inbuf, sizeof( inbuf ))) > 0 );

    if ( ctl == 2 ) {
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	FD_ZERO( &fdset );
	FD_SET( 0, &fdset );
	if ( select( 1, &fdset, NULL, NULL, &tv ) != 0 ) {
	    if (( inlen = read( 0, inbuf, sizeof( inbuf ))) > 0 ) {
		goto notdone;
	    }
	}
    }

    if ( inlen < 0 ) {
	syslog( LOG_ERR, "read: %s", strerror(errno) );
	return( 1 );
    }

    if ( ctl == 1 ) {
	if ( write( 1, "\031", 1 ) != 1 ) {
	    syslog( LOG_ERR, "write: %s", strerror(errno) );
	    return( 1 );
	}
    } else if ( ctl == 2 ) {
	return( 3 );
    }
    return( 0 );
}

static char		*font = "Courier";
static int		point = 11;

static char		pspro[] = "\
/GSV save def						% global VM\n\
/SP {\n\
	/SV save def					% save vmstate\n\
	dup /H exch def					% save font height\n\
	exch findfont exch scalefont setfont		% select font\n\
	( ) stringwidth pop /W exch def			% save font width\n\
	0.5 sub 72 mul /CY exch def			% save start Y\n\
	pop 0.5 add 72 mul /CX exch def			% save start X\n\
	CX CY moveto					% make current point\n\
} bind def\n\
/S /show load def\n\
/NL { CX CY H sub dup /CY exch def moveto } bind def\n\
/CR { CX CY moveto } bind def\n\
/B { W neg 0 rmoveto}bind def\n\
/T { W mul 0 rmoveto}bind def\n\
/EP { SV restore showpage } bind def\n\
%%EndProlog\n";

int textps(void)
{
    struct papersize	papersize;
    int			state = 0, line = 0, col = 0, npages = 0, rc;
    unsigned int	i;
    char		*p, *end;

#define elements(x)	(sizeof(x)/sizeof((x)[0]))
    for ( i = 0; i < elements( papersizes ); i++ ) {
	if ( width == papersizes[ 0 ].width &&
		length == papersizes[ 0 ].length ) {
	    papersize = papersizes[ i ];
	    break;
	}
    }
    if ( i >= elements( papersizes )) {
	papersize = papersizes[ 0 ];		/* default */
    }

#define ST_AVAIL		(1<<0)
#define ST_CONTROL		(1<<1)
#define ST_PAGE			(1<<2)
    /*
     * convert text lines to postscript.
     * A grungy little state machine. If I was more creative, I could
     * probably think of a better way of doing this...
     */
    do {
	p = inbuf;
	end = inbuf + inlen;
	while ( p < end ) {
	    if (( state & ST_PAGE ) == 0 && *p != '\031' && *p != '\001' ) {
		if ( npages == 0 ) {
		    printf( "%%!PS-Adobe-2.0\n%%%%Pages: (atend)\n" );
		    printf( "%%%%DocumentFonts: %s\n", font );
		    fflush( stdout );

		    /* output postscript prologue: */
		    if ( write( 1, pspro, sizeof( pspro ) - 1 ) !=
			    sizeof( pspro ) - 1 ) {
			syslog( LOG_ERR, "write prologue: %s", strerror(errno) );
			return( 1 );
		    }
		    if ( name && host ) {
			printf( "statusdict /jobname (%s@%s) put\n", name,
				host );
		    }
		}

		printf( "%%%%Page: ? %d\n", ++npages );
		printf( "%d %f %f /%s %d SP\n", indent,
			papersize.win, papersize.lin, font, point );
		state |= ST_PAGE;
	    }
	    if ( state & ST_CONTROL && *p != '\001' ) {
		/* It is a very bad thing to toss a job because it contains
		 * unprintable characters.  Instead, we will convert them to
		 * question marks.  This is adapted from a solution described
		 * by Werner Eugster <eugster@giub.unibe.ch> on his ApplePrint
		 * webpage (http://www.giub.unibe.ch/~eugster/appleprint.html).
		 *
		 * Note that this is rather ugly code.  The same change is
		 * applied identically at two different locations in this file.
		 * It would be better someday to combine the two.
                */
		if ( !literal ) {
			fprintf( stderr,
				"unprintable character (0x%x) converted to ?!\n",
				(unsigned char)*p );
			putchar( '?' ); /* Replace unprintable char with a question mark. */
		} else {
			printf( "\\%o", (unsigned char)031 );
		}
		state &= ~ST_CONTROL;
		col++;
	    }

	    switch ( *p ) {
	    case '\n' :		/* end of line */
		if ( state & ST_AVAIL ) {
		    printf( ")S\n" );
		    state &= ~ST_AVAIL;
		}
		printf( "NL\n" );
		line++;
		col = 0;
		if ( line >= length ) {
		    printf( "EP\n" );
		    state &= ~ST_PAGE;
		    line = 0;
		}
		break;

	    case '\r' :		/* carriage return (for overtyping) */
		if ( state & ST_AVAIL ) {
		    printf( ")S CR\n" );
		    state &= ~ST_AVAIL;
		}
		col = 0;
		break;

	    case '\f' :		/* form feed */
		if ( state & ST_AVAIL ) {
		    printf( ")S\n" );
		    state &= ~ST_AVAIL;
		}
		printf( "EP\n" );
		state &= ~ST_PAGE;
		line = 0;
		col = 0;
		break;

	    case '\b' :		/* backspace */
		/* show line, back up one character */
		if ( state & ST_AVAIL ) {
		    printf( ")S\n" );
		    state &= ~ST_AVAIL;
		}
		printf( "B\n" );
		col--;
		break;

	    case '\t' :		/* tab */
		if ( state & ST_AVAIL ) {
		    printf( ")S\n" );
		    state &= ~ST_AVAIL;
		}
		printf( "%d T\n", 8 - ( col % 8 ));
		col += 8 - ( col % 8 );
		break;

	    /*
	     * beginning of lpr control sequence
	     */
	    case '\031' :
		state |= ST_CONTROL;
		break;

	    case '\001' :	/* lpr control sequence */
		if ( state & ST_CONTROL ) {
		    rc = 3;
		    goto out;
		}
		/* FALLTHROUGH */

	    case '\\' :
	    case ')' :
	    case '(' :
		if (( state & ST_AVAIL ) == 0 ) {
		    printf( "(" );
		    state |= ST_AVAIL;
		}
		putchar( '\\' );
		/* FALLTHROUGH */

	    default :
		if (( state & ST_AVAIL ) == 0 ) {
		    printf( "(" );
		    state |= ST_AVAIL;
		}
		if ( !isascii( *p ) || !isprint( *p )) {
		    if ( !literal ) {
                        fprintf( stderr,
			    "unprintable character (0x%x) converted to ?!\n",
			    (unsigned char)*p );
			putchar( '?' ); /* Replace unprintable char with a question mark. */
		    } else {
		        printf( "\\%o", (unsigned char)*p );
		    }
		} else {
		    putchar( *p );
		}
		col++;
		break;
	    }
	p++;
	}
    } while (( inlen = read( 0, inbuf, sizeof( inbuf ))) > 0 );
    if ( inlen < 0 ) {
	syslog( LOG_ERR, "read: %s", strerror(errno) );
	return( 1 );
    }
    rc = 0;

out:
    if ( state & ST_AVAIL ) {
	printf( ")S\n" );
	state &= ~ST_AVAIL;
    }

    if ( state & ST_PAGE ) {
	printf( "EP\n" );
	state &= ~ST_PAGE;
    }

    if ( npages > 0 ) {
	printf( "%%%%Trailer\nGSV restore\n%%%%Pages: %d\n%%%%EOF\n", npages );
	fflush( stdout );
    }

    return( rc );
}

/*
 * Interface to pipe and exec, for starting children in pipelines.
 *
 * Manipulates file descriptors 0, 1, and 2, such that the new child
 * is reading from the parent's output.
 */
int pexecv( char *path, char *argv[])
{
    int		fd[ 2 ], c;

    if ( pipe( fd ) < 0 ) {
	return( -1 );
    }

    switch ( c = fork()) {
    case -1 :
	return( -1 );
	/* NOTREACHED */

    case 0 :
	if ( close( fd[ 1 ] ) < 0 ) {
	    return( -1 );
	}
	if ( dup2( fd[ 0 ], 0 ) < 0 ) {
	    return( -1 );
	}
	if ( close( fd[ 0 ] ) < 0 ) {
	    return( -1 );
	}
	execv( path, argv );
	return( -1 );
	/* NOTREACHED */

    default :
	if ( close( fd[ 0 ] ) < 0 ) {
	    return( -1 );
	}
	if ( dup2( fd[ 1 ], 1 ) < 0 ) {
	    return( -1 );
	}
	if ( close( fd[ 1 ] ) < 0 ) {
	    return( -1 );
	}
	return( c );
    }
}
