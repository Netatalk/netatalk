/*
 * $Id: flock.c,v 1.4 2001-06-29 14:14:46 rufustfirefly Exp $
 *
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

static int	_flock_dummy;

#ifndef HAVE_FLOCK

#include <sys/types.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <errno.h>

#define LOCK_SH         1
#define LOCK_EX         2
#define LOCK_NB         4
#define LOCK_UN         8

int flock( fd, operation )
    int		fd;
    int		operation;
{
    struct flock	l;
    int			rc, op;

    if ( operation & LOCK_NB ) {
	op = F_SETLK;
    } else {
	op = F_SETLKW;
    }

    if ( operation & LOCK_EX ) {
	l.l_type = F_WRLCK;
    }

    if ( operation & LOCK_SH ) {
	l.l_type = F_RDLCK;
    }

    if ( operation & LOCK_UN ) {
	l.l_type = F_UNLCK;
    }

    l.l_whence = 0;
    l.l_start = 0;
    l.l_len = 0;

    if (( rc = fcntl( fd, F_SETLK, &l )) < 0 ) {
	if ( errno == EAGAIN || errno == EACCES ) {
	    errno = EWOULDBLOCK;
	}
    }
    return( rc );
}
#endif /* !HAVE_FLOCK */
