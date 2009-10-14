/*
 * $Id: pa.c,v 1.6 2009-10-14 02:24:05 didg Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

/* This is used with pa.h */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "pa.h"

pa_buf_t *pa_init(int fd)
{
	pa_buf_t *h;
	int rc;

	h = (pa_buf_t *) malloc( sizeof( pa_buf_t ));
	h->buf = (char *) malloc( PA_BUFBLK * 2 );
	h->bufsz = PA_BUFBLK * 2;

	if (( rc = read( fd, h->buf, PA_BUFBLK )) < 0 ) {
		return( NULL );
	}

	h->cur = h->buf - 1;
	h->end = h->buf + rc - 1;
	h->state = PA_NORMAL;
	h->fd = fd;

	return( h );
}

char *pa_gettok(pa_buf_t *h)
{
	h->state = PA_NORMAL;
	h->tmp = *(h->cur);
	*(h->cur) = 0;
	return( h->mark );
}

char _pa_fixbuf(pa_buf_t *h)
{
	int rc;
	char *t;

	if ( h->state == PA_NORMAL ) {
		*(h->buf) = *(h->cur);
		h->cur = h->buf;
	} else {
		bcopy( h->mark, h->buf, h->end - h->mark + 1 );
		h->cur = h->buf + ( h->cur - h->mark );
		h->end = h->buf + ( h->end - h->mark );
		h->mark = h->buf;
	}

	if ( h->bufsz - (( h->cur - h->buf ) + 1 ) < PA_BUFBLK ) {
		t = h->buf;
		h->buf = (char *) realloc( h->buf, h->bufsz + PA_BUFBLK );
		h->bufsz += PA_BUFBLK;
		h->mark = h->buf + ( h->mark - t );
		h->cur = h->buf + ( h->cur - t );
		h->end = h->buf + ( h->end - t );
	}

	if (( rc = read( h->fd, h->cur + 1, PA_BUFBLK )) <= 0 ) {
		return( (char) 0 );
	}

	h->end = h->cur + rc;
	return( *(++(h->cur)) );
}
