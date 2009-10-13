/*
 * $Id: aep.c,v 1.9 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>

#include "atserv.h"

int aep_packet(
    struct atport	*ap,
    struct sockaddr_at	*from,
    char		*data,
    int			len)
{
    char		*end;

    end = data + len;
    if ( data + 2 > end || *data != DDPTYPE_AEP ||
	    *( data + 1 ) != AEPOP_REQUEST ) {
	LOG(log_info, logtype_atalkd, "aep_packet malformed packet" );
	return 1;
    }

    *( data + 1 ) = AEPOP_REPLY;
    if ( sendto( ap->ap_fd, data, len, 0, (struct sockaddr *)from,
	    sizeof( struct sockaddr_at )) < 0 ) {
	LOG(log_error, logtype_atalkd, "aep sendto: %s", strerror(errno) );
	return 1;
    }

    return 0;
}
