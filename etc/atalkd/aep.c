/*
 * $Id: aep.c,v 1.6 2002-01-03 17:49:39 sibaz Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <errno.h>

#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>

#include "atserv.h"

int aep_packet( ap, from, data, len )
    struct atport	*ap;
    struct sockaddr_at	*from;
    char		*data;
    int			len;
{
    char		*end;

    end = data + len;
    if ( data + 2 > end || *data != DDPTYPE_AEP ||
	    *( data + 1 ) != AEPOP_REQUEST ) {
	syslog( LOG_INFO, "aep_packet malformed packet" );
	return 1;
    }

    *( data + 1 ) = AEPOP_REPLY;
    if ( sendto( ap->ap_fd, data, len, 0, (struct sockaddr *)from,
	    sizeof( struct sockaddr_at )) < 0 ) {
	syslog( LOG_ERR, "aep sendto: %s", strerror(errno) );
	return 1;
    }

    return 0;
}
