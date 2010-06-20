/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Author:  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * Purpose: Zeroconf facade, that abstracts access to a
 *          particular Zeroconf implementation
 * Doc:     http://www.dns-sd.org/
 *
 */

#ifndef AFPD_ZEROCONF_H
#define AFPD_ZEROCONF_H

#include <netinet/in.h> /* htons() */
#include <atalk/logger.h>

#ifdef HAVE_AVAHI
#include "afp_avahi.h"
#endif

#define AFP_PORT 548

/*
 * Prototype Definitions
 */

/*
 * registers service with a particular Zerconf implemenation.
 */
void zeroconf_register(int port, const char *hostname);

/*
 * registers volume name with a particular Zerconf implemenation.
 */
void zeroconf_register_volume(const char *volname);

/*
 * de-registers the ntpd service with a particular Zerconf implemenation.
 */
void zeroconf_deregister(void);

#endif /* AFPD_ZEROCONF_H */
