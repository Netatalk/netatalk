/*
 * $Id: asp_child.h,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
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

#ifndef _ASP_CHILD_H
#define _ASP_CHILD_H 1

struct asp_child {
    int			ac_pid;
    int			ac_state;
    struct sockaddr_at	ac_sat;
};

#define ACSTATE_DEAD	0
#define ACSTATE_OK	1
#define ACSTATE_BAD	7

#endif /* _ASP_CHILD_H */
