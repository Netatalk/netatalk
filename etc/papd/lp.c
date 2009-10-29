/*
 * $Id: lp.c,v 1.33 2009-10-29 13:38:15 didg Exp $
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <sys/file.h>
#include <sys/un.h>
#include <netinet/in.h>
#undef s_net

#ifdef ABS_PRINT
#include <math.h>
#endif /* ABS_PRINT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <pwd.h>

#include <atalk/logger.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/paths.h>
#include <atalk/unicode.h>

#include "printer.h"
#include "file.h"
#include "lp.h"

#ifdef HAVE_CUPS
#include  "print_cups.h"
#endif


/* These functions aren't used outside of lp.c */
int lp_conn_inet();
int lp_disconn_inet( int );
int lp_conn_unix();
int lp_disconn_unix( int );

static char hostname[ MAXHOSTNAMELEN ];

extern struct sockaddr_at *sat;

static struct lp {
    int			lp_flags;
    FILE		*lp_stream;
    int			lp_seq;
    int 		lp_origin;
    char		lp_letter;
    char		*lp_person;
    char		*lp_created_for; /* Holds the content of the Postscript %%For Comment if available */
    char		*lp_host;
    char		*lp_job;
    char		*lp_spoolfile;
} lp;
#define LP_INIT		(1<<0)
#define LP_OPEN		(1<<1)
#define LP_PIPE		(1<<2)
#define LP_CONNECT	(1<<3)
#define LP_QUEUE	(1<<4)
#define LP_JOBPENDING	(1<<5)

void lp_origin (int origin)
{
    lp.lp_origin = origin;
}

/* the converted string should always be shorter, but ... FIXME! */
static void convert_octal (char *string, charset_t dest)
{
    unsigned char *p, *q;
    char temp[4];
    long int ch;

    q=p=(unsigned char *)string;
    while ( *p != '\0' ) {
        ch = 0;
        if ( *p == '\\' ) {
            p++;
            if (dest && isdigit(*p) && isdigit(*(p+1)) && isdigit(*(p+2)) ) {
                temp[0] = *p;
                temp[1] = *(p+1);
                temp[2] = *(p+2);
                temp[3] = 0;
                ch = strtol( temp, NULL, 8);
                if ( ch && ch < 0xff)
                    *q = ch;
              	else
                    *q = '.';
                p += 2;
            }
       	    else 
                *q = '.';
       }
       else {
           *q = *p;
       }
           p++;
           q++;
    }
    *q = 0;
}


static void translate(charset_t from, charset_t dest, char **option)
{
    char *translated;

    if (*option != NULL) {
        convert_octal(*option, from);
        if (from) {
             if ((size_t) -1 != (convert_string_allocate(from, dest, *option, -1, &translated)) ) {
                 free (*option);
                 *option = translated;
             }
        }
    }
}


static void lp_setup_comments (charset_t dest)
{
    charset_t from=0;

    switch (lp.lp_origin) {
	case 1:
		from=CH_MAC;
		break;
	case 2:
		from=CH_UTF8_MAC;
		break;
    }

    if (lp.lp_job) {
#ifdef DEBUG1
        LOG(log_debug9, logtype_papd, "job: %s", lp.lp_job );
#endif
        translate(from, dest, &lp.lp_job);
    }
    if (lp.lp_created_for) {
#ifdef DEBUG1
        LOG(log_debug9, logtype_papd, "for: %s", lp.lp_created_for );
#endif
        translate(from, dest, &lp.lp_created_for);
    }
    if (lp.lp_person) {
#ifdef DEBUG1
       LOG(log_debug9, logtype_papd, "person: %s", lp.lp_person );
#endif
       translate(from, dest, &lp.lp_person);
    }
}

#define is_var(a, b) (strncmp((a), (b), 2) == 0)

#if 0
/* removed, it's not used and a pain to get it right from a security POV */
static size_t quote(char *dest, char *src, const size_t bsize, size_t len)
{
size_t used = 0;

    while (len && used < bsize ) {
        switch (*src) {
          case '$':
          case '\\':
          case '"':
          case '`':
            if (used + 2 > bsize )
              return used;
            *dest = '\\';
            dest++;
            used++;
            break;
        }
        *dest = *src;
        src++;
        dest++;
        len--;
        used++;
    }
    return used;
}

static char* pipexlate(char *src)
{
    char *p, *q, *dest; 
    static char destbuf[MAXPATHLEN +1];
    size_t destlen = MAXPATHLEN;
    int len = 0;
   
    dest = destbuf; 

    if (!src)
	return NULL;

    memset(dest, 0, MAXPATHLEN +1);
    if ((p = strchr(src, '%')) == NULL) { /* nothing to do */
        strncpy(dest, src, MAXPATHLEN);
        return destbuf;
    }
    /* first part of the path. copy and forward to the next variable. */
    len = MIN((size_t)(p - src), destlen);
    if (len > 0) {
        strncpy(dest, src, len);
        destlen -= len;
        dest += len;
    }

    while (p && destlen > 0) {
        /* now figure out what the variable is */
        q = NULL;
        if (is_var(p, "%U")) {
	    q = lp.lp_person;
        } else if (is_var(p, "%C") || is_var(p, "%J") ) {
            q = lp.lp_job;
        } else if (is_var(p, "%F")) {
            q =  lp.lp_created_for;
        } else if (is_var(p, "%%")) {
            q = "%";
        } 

        /* copy the stuff over. if we don't understand something that we
         * should, just skip it over. */
        if (q) {
            len = MIN(strlen(q), destlen);
            len = quote(dest, q, destlen, len);
        }
        else {
            len = MIN(2, destlen);
            strncpy(dest, q, len);
        }
        dest += len;
        destlen -= len;

        /* stuff up to next % */
        src = p + 2;
        p = strchr(src, '%');
        len = p ? MIN((size_t)(p - src), destlen) : destlen;
        if (len > 0) {
            strncpy(dest, src, len);
            dest += len;
            destlen -= len;
        }
    }
    if (!destlen) {
        /* reach end of buffer, maybe prematurely, give up */
        return NULL;
    }
    return destbuf;
}
#endif

void lp_person(char *person)
{
    if ( lp.lp_person != NULL ) {
	free( lp.lp_person );
    }
    if (( lp.lp_person = (char *)malloc( strlen( person ) + 1 )) == NULL ) {
	LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
	exit( 1 );
    }
    strcpy( lp.lp_person, person );
}

#ifdef ABS_PRINT
int lp_pagecost(void)
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
#endif /* ABS_PRINT */

void lp_host( char *host)
{
    if ( lp.lp_host != NULL ) {
	free( lp.lp_host );
    }
    if (( lp.lp_host = (char *)malloc( strlen( host ) + 1 )) == NULL ) {
	LOG(log_error, logtype_papd, "malloc: %s", strerror(errno) );
	exit( 1 );
    }
    strcpy( lp.lp_host, host );
    LOG(log_debug, logtype_papd, "host: %s", lp.lp_host );
}

/* Currently lp_job and lp_for will not handle the
 * conversion of macroman chars > 0x7f correctly
 * This should be added.
 */

void lp_job(char *job)
{
    if ( lp.lp_job != NULL ) {
	free( lp.lp_job );
    }

    lp.lp_job = strdup(job);
#ifdef DEBUG
    LOG(log_debug9, logtype_papd, "job: %s", lp.lp_job );
#endif
    
}

void lp_for (char *lpfor)
{
    if ( lp.lp_created_for != NULL ) {
	free( lp.lp_created_for );
    }

    lp.lp_created_for = strdup(lpfor);
}


static int lp_init(struct papfile *out, struct sockaddr_at *sat)
{
    int		authenticated = 0;
#ifndef HAVE_CUPS
    int		fd, n, len;
    char	*cp, buf[ BUFSIZ ];
    struct stat	st;
#endif /* HAVE_CUPS */
#ifdef ABS_PRINT
    char	cost[ 22 ];
    char	balance[ 22 ];
#endif /* ABS_PRINT */

    if ( printer->p_flags & P_AUTH ) {
	authenticated = 0;

	/* cap style "log on to afp server before printing" authentication */

	if ( printer->p_authprintdir && (printer->p_flags & P_AUTH_CAP) ) {
	    int addr_net = ntohs( sat->sat_addr.s_net );
	    int addr_node  = sat->sat_addr.s_node;
	    char addr_filename[256];
	    char auth_string[256];
	    char *username, *afpdpid;
	    struct stat cap_st;
	    FILE *cap_file;

	    memset( auth_string, 0, 256 );
	    sprintf(addr_filename, "%s/net%d.%dnode%d", 
		printer->p_authprintdir, addr_net/256, addr_net%256, 
		addr_node);
	    if (stat(addr_filename, &cap_st) == 0) {
		if ((cap_file = fopen(addr_filename, "r")) != NULL) {
		    if (fgets(auth_string, 256, cap_file) != NULL) {
			username = auth_string;
			if ((afpdpid = strrchr( auth_string, ':' )) != NULL) {
			    *afpdpid = '\0';
			    afpdpid++;
			}
			if (getpwnam(username) != NULL ) {
			    LOG(log_info, logtype_papd, "CAP authenticated %s", username);
			    lp_person(username);
			    authenticated = 1;
			} else {
			    LOG(log_info, logtype_papd, "CAP error: invalid username: '%s'", username);
			}
		    } else {
			LOG(log_info, logtype_papd, "CAP error: could not read username");
		    }
		} else {
		    LOG(log_info, logtype_papd, "CAP error: %s", strerror(errno));
		}
	    } else {
		LOG(log_info, logtype_papd, "CAP error: %s", strerror(errno));
	    }
	}

	if ( printer->p_flags & P_AUTH_PSSP ) {
	    if ( lp.lp_person != NULL ) {
		authenticated = 1;
	    }
	}

	if ( authenticated == 0 ) {
	    LOG(log_error, logtype_papd, "lp_init: must authenticate" );
	    spoolerror( out, "Authentication required." );
	    return( -1 );
	}

#ifdef ABS_PRINT
	if (( printer->p_flags & P_ACCOUNT ) && printer->p_pagecost > 0 &&
		! ABS_canprint( lp.lp_person, printer->p_role,
		printer->p_srvid, cost, balance )) {
	    LOG(log_error, logtype_papd, "lp_init: no ABS funds" );
	    spoolerror( out, "No ABS funds available." );
	    return( -1 );
	}
#endif /* ABS_PRINT */
    }

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	LOG(log_error, logtype_papd, "gethostname: %s", strerror(errno) );
	exit( 1 );
    }

    if ( lp.lp_flags & LP_INIT ) {
	LOG(log_error, logtype_papd, "lp_init: already inited, die!" );
	abort();
    }

    lp.lp_flags = 0;
    lp.lp_stream = NULL;
    lp.lp_letter = 'A';

    if ( printer->p_flags & P_SPOOLED ) {

#ifndef HAVE_CUPS
	/* check if queuing is enabled: mode & 010 on lock file */
	if ( stat( printer->p_lock, &st ) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_init: %s: %s", printer->p_lock, strerror(errno) );
	    spoolerror( out, NULL );
	    return( -1 );
	}
	if ( st.st_mode & 010 ) {
	    LOG(log_info, logtype_papd, "lp_init: queuing is disabled" );
	    spoolerror( out, "Queuing is disabled." );
	    return( -1 );
	}

	if (( fd = open( ".seq", O_RDWR|O_CREAT, 0661 )) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_init: can't create .seq" );
	    spoolerror( out, NULL );
	    return( -1 );
	}

#ifndef SOLARIS /* flock is unsupported, I doubt this stuff works anyway with newer solaris so ignore for now */
	if ( flock( fd, LOCK_EX ) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_init: can't lock .seq" );
	    spoolerror( out, NULL );
	    return( -1 );
	}
#endif

	n = 0;
	if (( len = read( fd, buf, sizeof( buf ))) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_init read: %s", strerror(errno) );
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
#else

	if (cups_get_printer_status ( printer ) == 0)
	{
	    spoolerror( out, "Queuing is disabled." );
	    return( -1 );
	}

	lp.lp_seq = getpid();
#endif /* HAVE CUPS */
    } else {
	lp.lp_flags |= LP_PIPE;
	lp.lp_seq = getpid();
    }

    lp.lp_flags |= LP_INIT;
    return( 0 );
}

int lp_open(struct papfile *out, struct sockaddr_at *sat)
{
    char	name[ MAXPATHLEN ];
    int		fd;
    struct passwd	*pwent;

#ifdef DEBUG
    LOG (log_debug9, logtype_papd, "lp_open");
#endif

    if ( lp.lp_flags & LP_JOBPENDING ) {
	lp_print();
    }

    if (( lp.lp_flags & LP_INIT ) == 0 && lp_init( out, sat ) != 0 ) {
	return( -1 );
    }
    if ( lp.lp_flags & LP_OPEN ) {
	/* LOG(log_error, logtype_papd, "lp_open already open" ); */
	/* abort(); */
	return (-1);
    }

    if ( lp.lp_flags & LP_PIPE ) {
        char *pipe_cmd;

	/* go right to program */
	if (lp.lp_person != NULL) {
	    if((pwent = getpwnam(lp.lp_person)) != NULL) {
		if(setreuid(pwent->pw_uid, pwent->pw_uid) != 0) {
		    LOG(log_error, logtype_papd, "setreuid error: %s", strerror(errno));
		    exit(1);
		}
	    } else {
		LOG(log_error, logtype_papd, "Error getting username (%s)", lp.lp_person);
                exit(1);
	    }
	}

	lp_setup_comments(CH_UNIX);
	pipe_cmd = printer->p_printer;
	if (!pipe_cmd) {
	    LOG(log_error, logtype_papd, "lp_open: no pipe cmd" );
	    spoolerror( out, NULL );
	    return( -1 );
	}
	if (( lp.lp_stream = popen(pipe_cmd, "w" )) == NULL ) {
	    LOG(log_error, logtype_papd, "lp_open popen %s: %s", printer->p_printer, strerror(errno) );
	    spoolerror( out, NULL );
	    return( -1 );
	}
        LOG(log_debug, logtype_papd, "lp_open: opened %s",  pipe_cmd );
    } else {
	sprintf( name, "df%c%03d%s", lp.lp_letter++, lp.lp_seq, hostname );

	if (( fd = open( name, O_WRONLY|O_CREAT|O_EXCL, 0660 )) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_open %s: %s", name, strerror(errno) );
	    spoolerror( out, NULL );
	    return( -1 );
	}

	if ( NULL == (lp.lp_spoolfile = (char *) malloc (strlen (name) +1)) ) {
	    LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
	    exit(1);
	}
	strcpy ( lp.lp_spoolfile, name);	

	if (lp.lp_person != NULL) {
	    if ((pwent = getpwnam(lp.lp_person)) == NULL) {
		LOG(log_error, logtype_papd, "getpwnam %s: no such user", lp.lp_person);
		spoolerror( out, NULL );
		return( -1 );
	    }
	} else {
	    if ((pwent = getpwnam(printer->p_operator)) == NULL) {
		LOG(log_error, logtype_papd, "getpwnam %s: no such user", printer->p_operator);
		spoolerror( out, NULL );
		return( -1 );
	    }
	}

	if (fchown(fd, pwent->pw_uid, -1) < 0) {
	    LOG(log_error, logtype_papd, "chown %s %s: %s", pwent->pw_name, name, strerror(errno));
	    spoolerror( out, NULL );
	    return( -1 );
	}

	if (( lp.lp_stream = fdopen( fd, "w" )) == NULL ) {
	    LOG(log_error, logtype_papd, "lp_open fdopen: %s", strerror(errno) );
	    spoolerror( out, NULL );
	    return( -1 );
	}
#ifdef DEBUG        
        LOG(log_debug9, logtype_papd, "lp_open: opened %s", name );
#endif	
    }
    lp.lp_flags |= LP_OPEN;
    return( 0 );
}

int lp_close(void)
{
    if (( lp.lp_flags & LP_INIT ) == 0 || ( lp.lp_flags & LP_OPEN ) == 0 ) {
	return 0;
    }
    fclose( lp.lp_stream );
    lp.lp_stream = NULL;
    lp.lp_flags &= ~LP_OPEN;
    lp.lp_flags |= LP_JOBPENDING;
    return 0;
}



int lp_write(struct papfile *in, char *buf, size_t len)
{
#define BUFSIZE 32768
    static char tempbuf[BUFSIZE];
    static char tempbuf2[BUFSIZE];
    static size_t bufpos = 0;
    static int last_line_translated = 1; /* if 0, append a \n a the start */
    char *tbuf = buf;

    /* Before we write out anything check for a pending job, e.g. cover page */
    if (lp.lp_flags & LP_JOBPENDING)
	lp_print();

    /* foomatic doesn't handle mac line endings, so we convert them for 
     * the Postscript headers
     * REALLY ugly hack, remove ASAP again */
    if ((printer->p_flags & P_FOOMATIC_HACK) && (in->pf_state & PF_TRANSLATE) && 
        (buf[len-1] != '\n') ) {
        if (len <= BUFSIZE) {
	    if (!last_line_translated) {
	    	tempbuf2[0] = '\n';
            	memcpy(tempbuf2+1, buf, len++);
	    }
	    else
            	memcpy(tempbuf2, buf, len);
		
            if (tempbuf2[len-1] == '\r')
	        tempbuf2[len-1] = '\n';
            tempbuf2[len] = 0;
            tbuf = tempbuf2;
            last_line_translated = 1;
#ifdef DEBUG
            LOG(log_debug9, logtype_papd, "lp_write: %s", tbuf );
#endif
        }
        else {
            LOG(log_error, logtype_papd, "lp_write: conversion buffer too small" );
            abort();
        }
    }
    else {
        if (printer->p_flags & P_FOOMATIC_HACK && buf[len-1] == '\n') {
            last_line_translated = 1;
	}
        else
	    last_line_translated = 0;
    }

    /* To be able to do commandline substitutions on piped printers
     * we store the start of the print job in a buffer.
     * %%EndComment triggers writing to file */
    if (( lp.lp_flags & LP_OPEN ) == 0 ) {
#ifdef DEBUG
        LOG(log_debug9, logtype_papd, "lp_write: writing to temporary buffer" );
#endif
    	if ((bufpos+len) > BUFSIZE) {
            LOG(log_error, logtype_papd, "lp_write: temporary buffer too small" );
            /* FIXME: call lp_open here? abort isn't nice... */
            abort();
        }
	else {
            memcpy(tempbuf + bufpos, tbuf, len);
            bufpos += len;
            if (bufpos > BUFSIZE/2)
                in->pf_state |= PF_STW; /* we used half of the buffer, start writing */
            return(0);
	}
    }
    else if ( bufpos) {
        if ( fwrite( tempbuf, 1, bufpos, lp.lp_stream ) != bufpos ) {
            LOG(log_error, logtype_papd, "lp_write: %s", strerror(errno) );
            abort();
        }
        bufpos=0;
    }

    if ( fwrite( tbuf, 1, len, lp.lp_stream ) != len ) {
	LOG(log_error, logtype_papd, "lp_write: %s", strerror(errno) );
	abort();
    }
    return( 0 );
}

int lp_cancel(void)
{
    char	name[ MAXPATHLEN ];
    char	letter;

    if (( lp.lp_flags & LP_INIT ) == 0 || lp.lp_letter == 'A' ) {
	return 0;
    }

    if ( lp.lp_flags & LP_OPEN ) {
	lp_close();
    }

    for ( letter = 'A'; letter < lp.lp_letter; letter++ ) {
	sprintf( name, "df%c%03d%s", letter, lp.lp_seq, hostname );
	if ( unlink( name ) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_cancel unlink %s: %s", name, strerror(errno) );
	}
    }

    return 0;
}

/*
 * Create printcap control file, signal printer.  Errors here should
 * remove queue files.
 *
 * XXX piped?
 */
int lp_print(void)
{
#ifndef HAVE_CUPS
    char		buf[ MAXPATHLEN ];
    char		tfname[ MAXPATHLEN ];
    char		cfname[ MAXPATHLEN ];
    char		letter;
    int			fd, n, s;
    FILE		*cfile;
#endif /* HAVE_CUPS */

    if (( lp.lp_flags & LP_INIT ) == 0 || lp.lp_letter == 'A' ) {
	return 0;
    }
    lp_close();
    lp.lp_flags &= ~LP_JOBPENDING;

    if ( printer->p_flags & P_SPOOLED ) {
#ifndef HAVE_CUPS
	sprintf( tfname, "tfA%03d%s", lp.lp_seq, hostname );
	if (( fd = open( tfname, O_WRONLY|O_EXCL|O_CREAT, 0660 )) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_print %s: %s", tfname, strerror(errno) );
	    return 0;
	}
	if (( cfile = fdopen( fd, "w" )) == NULL ) {
	    LOG(log_error, logtype_papd, "lp_print %s: %s", tfname, strerror(errno) );
	    return 0;
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
	    LOG(log_error, logtype_papd, "lp_print can't link %s to %s: %s", cfname,
		    tfname, strerror(errno) );
	    return 0;
	}
	unlink( tfname );

	if (( s = lp_conn_unix()) < 0 ) {
	    LOG(log_error, logtype_papd, "lp_print: lp_conn_unix: %s", strerror(errno) );
	    return 0;
	}

	sprintf( buf, "\1%s\n", printer->p_printer );
	n = strlen( buf );
	if ( write( s, buf, n ) != n ) {
	    LOG(log_error, logtype_papd, "lp_print write: %s" , strerror(errno));
	    return 0;
	}
	if ( read( s, buf, 1 ) != 1 ) {
	    LOG(log_error, logtype_papd, "lp_print read: %s" , strerror(errno));
	    return 0;
	}

	lp_disconn_unix( s );

	if ( buf[ 0 ] != '\0' ) {
	    LOG(log_error, logtype_papd, "lp_print lpd said %c: %s", buf[ 0 ], strerror(errno) );
	    return 0;
	}
#else
        if ( ! (lp.lp_job && *lp.lp_job) ) {
            lp.lp_job = strdup("Mac Job");
        }

        lp_setup_comments(add_charset(cups_get_language ()));

        if (lp.lp_person != NULL) {
	    cups_print_job ( printer->p_printer, lp.lp_spoolfile, lp.lp_job, lp.lp_person, printer->p_cupsoptions);
        } else if (lp.lp_created_for != NULL) {
            cups_print_job ( printer->p_printer, lp.lp_spoolfile, lp.lp_job, lp.lp_created_for, printer->p_cupsoptions);
        } else {
            cups_print_job ( printer->p_printer, lp.lp_spoolfile, lp.lp_job, printer->p_operator, printer->p_cupsoptions);
        }

	/*LOG(log_info, logtype_papd, "lp_print unlink %s", lp.lp_spoolfile );*/
        unlink ( lp.lp_spoolfile );
	return 0;
#endif /* HAVE_CUPS*/
    }
    LOG(log_info, logtype_papd, "lp_print queued" );
    return 0;
}

#ifndef HAVE_CUPS
int lp_disconn_unix( int fd )
{
    return( close( fd ));
}

int lp_conn_unix(void)
{
    int			s;
    struct sockaddr_un	saun;

    if (( s = socket( AF_UNIX, SOCK_STREAM, 0 )) < 0 ) {
	LOG(log_error, logtype_papd, "lp_conn_unix socket: %s", strerror(errno) );
	return( -1 );
    }
    memset( &saun, 0, sizeof( struct sockaddr_un ));
    saun.sun_family = AF_UNIX;
    strcpy( saun.sun_path, _PATH_DEVPRINTER );
    if ( connect( s, (struct sockaddr *)&saun,
	    strlen( saun.sun_path ) + 2 ) < 0 ) {
	LOG(log_error, logtype_papd, "lp_conn_unix connect %s: %s", saun.sun_path, strerror(errno) );
	close( s );
	return( -1 );
    }

    return( s );
}

int lp_disconn_inet( int fd )
{
    return( close( fd ));
}

int lp_conn_inet(void)
{
    int			privfd, port = IPPORT_RESERVED - 1;
    struct sockaddr_in	sin;
    struct servent	*sp;
    struct hostent	*hp;

    if (( sp = getservbyname( "printer", "tcp" )) == NULL ) {
	LOG(log_error, logtype_papd, "printer/tcp: unknown service" );
	return( -1 );
    }

    if ( gethostname( hostname, sizeof( hostname )) < 0 ) {
	LOG(log_error, logtype_papd, "gethostname: %s", strerror(errno) );
	exit( 1 );
    }

    if (( hp = gethostbyname( hostname )) == NULL ) {
	LOG(log_error, logtype_papd, "%s: unknown host", hostname );
	return( -1 );
    }

    if (( privfd = rresvport( &port )) < 0 ) {
	LOG(log_error, logtype_papd, "lp_connect: socket: %s", strerror(errno) );
	close( privfd );
	return( -1 );
    }

    memset( &sin, 0, sizeof( struct sockaddr_in ));
    sin.sin_family = AF_INET;
/*    sin.sin_addr.s_addr = htonl( INADDR_LOOPBACK ); */
    memcpy( &sin.sin_addr, hp->h_addr, hp->h_length );
    sin.sin_port = sp->s_port;

    if ( connect( privfd, (struct sockaddr *)&sin,
	    sizeof( struct sockaddr_in )) < 0 ) {
	LOG(log_error, logtype_papd, "lp_connect: %s", strerror(errno) );
	close( privfd );
	return( -1 );
    }

    return( privfd );
}

int lp_rmjob( int job)
{
    char	buf[ 1024 ];
    int		n, s;

    if (( s = lp_conn_inet()) < 0 ) {
	LOG(log_error, logtype_papd, "lp_rmjob: %s", strerror(errno) );
	return( -1 );
    }

    if ( lp.lp_person == NULL ) {
	return( -1 );
    }

    sprintf( buf, "\5%s %s %d\n", printer->p_printer, lp.lp_person, job );
    n = strlen( buf );
    if ( write( s, buf, n ) != n ) {
	LOG(log_error, logtype_papd, "lp_rmjob write: %s", strerror(errno) );
	lp_disconn_inet( s );
	return( -1 );
    }
    while (( n = read( s, buf, sizeof( buf ))) > 0 ) {
	LOG(log_debug, logtype_papd, "read %.*s", n, buf );
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

int lp_queue( struct papfile *out)
{
    char			buf[ 1024 ], *start, *stop, *p, *q;
    int				linelength, crlflength;
    static struct papfile	pf;
    int				s;
    size_t			len;
    ssize_t			n;
	
    if (( s = lp_conn_unix()) < 0 ) {
	LOG(log_error, logtype_papd, "lp_queue: %s", strerror(errno) );
	return( -1 );
    }

    sprintf( buf, "\3%s\n", printer->p_printer );
    n = strlen( buf );
    if ( write( s, buf, n ) != n ) {
	LOG(log_error, logtype_papd, "lp_queue write: %s", strerror(errno) );
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
#endif /* HAVE_CUPS */
