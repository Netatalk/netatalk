/*
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static int	_flock_dummy;

# if defined( sun ) && defined( __svr4__ )

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include </usr/ucbinclude/sys/file.h>

int flock( fd, operation )
    int		fd;
    int		operation;
{
    flock_t	l;
    int		rc, op;

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
# endif sun __svr4__
