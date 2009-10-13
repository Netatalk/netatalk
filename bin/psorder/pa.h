/*
 * $Id: pa.h,v 1.3 2009-10-13 22:55:36 didg Exp $
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

/*
 * Functions to aid in parsing:
 *
 *	pa_init( fd )		Allocate and return a handle. Reads
 *				from fd. Call this first, always.
 *	pa_getchar( h )		Return a single character or nul, 0,
 *				to indicate end-of-file.
 *	pa_match( h )		Mark the character last read as the start
 *				of a match.
 *	pa_gettok( h )		The match up to but excluding the last
 *				character is returned.
 *	pa_cont( h )		Continue match with previous match. Call
 *				immediately after pa_gettok() to get
 *				correct behavior.
 *	pa_cancel( h )		Cancel previous match start.
 */

#ifndef _PA_H
#define _PA_H 1

#ifndef FILE_H
#include <stdio.h>
#endif /* FILE_H */

#define PA_BUFBLK	1024

#define PA_NORMAL	0
#define PA_MATCHING	1

typedef struct pa_buf_t {
	char *buf;
	char *cur;
	char *mark;
	char *end;
	int fd;
	int state;
	unsigned bufsz;
	char tmp;
} pa_buf_t;

extern pa_buf_t *pa_init(int fd);
extern char _pa_fixbuf(pa_buf_t *h);
extern char *pa_gettok(pa_buf_t *h);

#define pa_getchar(h)	(((h)->cur==(h)->end)?(_pa_fixbuf(h)):\
			(*(++((h)->cur))))
#define pa_match(h)	((h)->state=PA_MATCHING,(h)->mark=(h)->cur)
#define pa_cont(h)	(*((h)->cur)=(h)->tmp,(h)->state=PA_MATCHING)
#define pa_cancel(h)	((h)->state=PA_NORMAL)
#define pa_back(h)	(--((h)->cur))

#endif /* _PA_H */
