/*
 * $Id: session.c,v 1.20 2009-10-16 01:10:59 didg Exp $
 *
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif /* HAVE_SYS_ERRNO_H */
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <atalk/logger.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/pap.h>

#include "file.h"
#include "lp.h"
#include "session.h"

int ps(struct papfile *infile, struct papfile *outfile, struct sockaddr_at *sat);

extern unsigned char	connid, quantum, oquantum;

static char		buf[ PAP_MAXQUANTUM ][ 4 + PAP_MAXDATA ];
static struct iovec	niov[ PAP_MAXQUANTUM ] = {
    { buf[ 0 ],	0 },
    { buf[ 1 ],	0 },
    { buf[ 2 ],	0 },
    { buf[ 3 ],	0 },
    { buf[ 4 ],	0 },
    { buf[ 5 ],	0 },
    { buf[ 6 ],	0 },
    { buf[ 7 ],	0 },
};

/*
 * Accept files until the client closes the connection.
 * Read lines of a file, until the client sends eof, after
 * which we'll send eof also.
 */
int session(ATP atp, struct sockaddr_at *sat)
{
    struct timeval	tv;
    struct atp_block	atpb;
    struct sockaddr_at	ssat;
    struct papfile	infile, outfile;
    fd_set		fds;
    char		cbuf[ 578 ];
    int			i, cc, timeout = 0, readpending = 0;
    u_int16_t		seq = 0, rseq = 1, netseq;
    u_char		readport; /* uninitialized, OK 310105 */

    infile.pf_state = PF_BOT;
    infile.pf_bufsize = 0;
    infile.pf_datalen = 0;
    infile.pf_buf = NULL;
    infile.pf_data = NULL;

    outfile.pf_state = PF_BOT;
    outfile.pf_bufsize = 0;
    outfile.pf_datalen = 0;
    outfile.pf_buf = NULL;
    outfile.pf_data = NULL;

    /*
     * Ask for data.
     */
    cbuf[ 0 ] = connid;
    cbuf[ 1 ] = PAP_READ;
    if (++seq == 0) seq = 1;
    netseq = htons( seq );
    memcpy( &cbuf[ 2 ], &netseq, sizeof( netseq ));
    atpb.atp_saddr = sat;
    atpb.atp_sreqdata = cbuf;
    atpb.atp_sreqdlen = 4;		/* bytes in SendData request */
    atpb.atp_sreqto = 5;		/* retry timer */
    atpb.atp_sreqtries = -1;		/* infinite retries */
    if ( atp_sreq( atp, &atpb, oquantum, ATP_XO )) {
	LOG(log_error, logtype_papd, "atp_sreq: %s", strerror(errno) );
	return( -1 );
    }

    for (;;) {
	/*
	 * Time between tickles.
	 */
	tv.tv_sec = 60;
	tv.tv_usec = 0;

	/*
	 * If we don't get anything for a while, time out.
	 */
	FD_ZERO( &fds );
	FD_SET( atp_fileno( atp ), &fds );

	do { /* do list until success or an unrecoverable error occurs */
	  if (( cc = select( FD_SETSIZE, &fds, NULL, NULL, &tv )) < 0 )
	      LOG(log_error, logtype_papd, "select: %s", strerror(errno) ); /* log all errors */
	} while (( cc < 0 ) && (errno == 4));

	if ( cc < 0 ) {
	  LOG(log_error, logtype_papd, "select: Error is unrecoverable" );
	  return( -1 );
	}
	if ( cc == 0 ) {
	    if ( timeout++ > 2 ) {
		LOG(log_error, logtype_papd, "connection timed out" );
		lp_cancel();
		return( -1 );
	    }

	    /*
	     * Send a tickle.
	     */
	    cbuf[ 0 ] = connid;
	    cbuf[ 1 ] = PAP_TICKLE;
	    cbuf[ 2 ] = cbuf[ 3 ] = 0;
	    atpb.atp_saddr = sat;
	    atpb.atp_sreqdata = cbuf;
	    atpb.atp_sreqdlen = 4;		/* bytes in Tickle request */
	    atpb.atp_sreqto = 0;		/* best effort */
	    atpb.atp_sreqtries = 1;		/* try once */
	    if ( atp_sreq( atp, &atpb, 0, 0 )) {
		LOG(log_error, logtype_papd, "atp_sreq: %s", strerror(errno) );
		return( -1 );
	    }
	    continue;
	} else {
	    timeout = 0;
	}

	memset( &ssat, 0, sizeof( struct sockaddr_at ));
	switch( atp_rsel( atp, &ssat, ATP_TRESP | ATP_TREQ )) {
	case ATP_TREQ :
	    atpb.atp_saddr = &ssat;
	    atpb.atp_rreqdata = cbuf;
	    atpb.atp_rreqdlen = sizeof( cbuf );
	    if ( atp_rreq( atp, &atpb ) < 0 ) {
		LOG(log_error, logtype_papd, "atp_rreq: %s", strerror(errno) );
		return( -1 );
	    }
	    /* sanity */
	    if ( (unsigned char)cbuf[ 0 ] != connid ) {
		LOG(log_error, logtype_papd, "Bad ATP request!" );
		continue;
	    }

	    switch( cbuf[ 1 ] ) {
	    case PAP_READ :
		/*
		 * Other side is ready for some data.
		 */
		memcpy( &netseq, &cbuf[ 2 ], sizeof( netseq ));
		if ( netseq != 0 ) {
		    if ( rseq != ntohs( netseq )) {
			break;
		    }
		    if ( rseq++ == 0xffff ) rseq = 1;
		}
		readpending = 1;
		readport = ssat.sat_port;
		break;

	    case PAP_CLOSE :
		/*
		 * Respond to the close request.
		 * If we're in the middle of a file, clean up.
		 */
		if (( infile.pf_state & PF_BOT ) ||
			( infile.pf_datalen == 0 &&
			( infile.pf_state & PF_EOF ))) {
		    lp_print();
		} else {
		    lp_cancel();
		}

		niov[ 0 ].iov_len = 4;
		((char *)niov[ 0 ].iov_base)[ 0 ] = connid;
		((char *)niov[ 0 ].iov_base)[ 1 ] = PAP_CLOSEREPLY;
		((char *)niov[ 0 ].iov_base)[ 2 ] =
			((char *)niov[ 0 ].iov_base)[ 3 ] = 0;
		atpb.atp_sresiov = niov;
		atpb.atp_sresiovcnt = 1;
		if ( atp_sresp( atp, &atpb ) < 0 ) {
		    LOG(log_error, logtype_papd, "atp_sresp: %s", strerror(errno) );
		    exit( 1 );
		}
		return( 0 );
		break;

	    case PAP_TICKLE :
		break;
	    default :
		LOG(log_error, logtype_papd, "Bad PAP request!" );
	    }

	    break;

	case ATP_TRESP :
	    atpb.atp_saddr = &ssat;
	    for ( i = 0; i < oquantum; i++ ) {
		niov[ i ].iov_len = PAP_MAXDATA + 4;
	    }
	    atpb.atp_rresiov = niov;
	    atpb.atp_rresiovcnt = oquantum;
	    if ( atp_rresp( atp, &atpb ) < 0 ) {
		LOG(log_error, logtype_papd, "atp_rresp: %s", strerror(errno) );
		return( -1 );
	    }

	    /* sanity */
	    if ( ((unsigned char *)niov[ 0 ].iov_base)[ 0 ] != connid ||
		    ((char *)niov[ 0 ].iov_base)[ 1 ] != PAP_DATA ) {
		LOG(log_error, logtype_papd, "Bad data response!" );
		continue;
	    }

	    for ( i = 0; i < atpb.atp_rresiovcnt; i++ ) {
		append( &infile,
			(char *)niov[ i ].iov_base + 4, niov[ i ].iov_len - 4 );
		if (( infile.pf_state & PF_EOF ) == 0 &&
			((char *)niov[ 0 ].iov_base)[ 2 ] ) {
		    infile.pf_state |= PF_EOF;
		}
	    }

	    /* move data */
	    if ( ps( &infile, &outfile, sat ) < 0 ) {
		LOG(log_error, logtype_papd, "parse: bad return" );
		return( -1 );	/* really?  close? */
	    }

	    /*
	     * Ask for more data.
	     */
	    cbuf[ 0 ] = connid;
	    cbuf[ 1 ] = PAP_READ;
	    if ( ++seq == 0 ) seq = 1;
	    netseq = htons( seq );
	    memcpy( &cbuf[ 2 ], &netseq, sizeof( netseq ));
	    atpb.atp_saddr = sat;
	    atpb.atp_sreqdata = cbuf;
	    atpb.atp_sreqdlen = 4;		/* bytes in SendData request */
	    atpb.atp_sreqto = 5;		/* retry timer */
	    atpb.atp_sreqtries = -1;		/* infinite retries */
	    if ( atp_sreq( atp, &atpb, oquantum, ATP_XO )) {
		LOG(log_error, logtype_papd, "atp_sreq: %s", strerror(errno) );
		return( -1 );
	    }
	    break;

	case 0:
	    break;

	default :
	    LOG(log_error, logtype_papd, "atp_rsel: %s", strerror(errno) );
	    return( -1 );
	}

	/* send any data that we have */
	if ( readpending &&
		( outfile.pf_datalen || ( outfile.pf_state & PF_EOF ))) {
	    for ( i = 0; i < quantum; i++ ) {
		((char *)niov[ i ].iov_base)[ 0 ] = connid;
		((char *)niov[ i ].iov_base)[ 1 ] = PAP_DATA;
		((char *)niov[ i ].iov_base)[ 2 ] =
			((char *)niov[ i ].iov_base)[ 3 ] = 0;

		if ( outfile.pf_datalen > PAP_MAXDATA  ) {
		    cc = PAP_MAXDATA;
		} else {
		    cc = outfile.pf_datalen;
		    if ( outfile.pf_state & PF_EOF ) {
			((char *)niov[ 0 ].iov_base)[ 2 ] = 1;	/* eof */
			outfile.pf_state = PF_BOT;
			infile.pf_state = PF_BOT;
		    }
		}

		niov[ i ].iov_len = 4 + cc;
		memcpy( (char *)niov[ i ].iov_base + 4, outfile.pf_data, cc );
		CONSUME( &outfile, cc );
		if ( outfile.pf_datalen == 0 ) {
		    i++;
		    break;
		}
	    }
	    ssat.sat_port = readport;
	    atpb.atp_saddr = &ssat;
	    atpb.atp_sresiov = niov;
	    atpb.atp_sresiovcnt = i;	/* reported by stevebn@pc1.eos.co.uk */
	    if ( atp_sresp( atp, &atpb ) < 0 ) {
		LOG(log_error, logtype_papd, "atp_sresp: %s", strerror(errno) );
		return( -1 );
	    }
	    readpending = 0;
	}
    }
}
