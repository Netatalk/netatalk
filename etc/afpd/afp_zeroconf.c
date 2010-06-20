/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Author:  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * Purpose: Zeroconf facade, that abstracts access to a
 *          particular Zeroconf implementation
 * Doc:     http://www.dns-sd.org/
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "afp_zeroconf.h"

/*
 * Global Definitions
 */
#ifdef HAVE_AVAHI
struct context *ctx = NULL;
#endif

/*
 * Functions (actually they are just facades)
 */
void zeroconf_register(int port, const char *hostname)
{
#if defined (HAVE_AVAHI)
  LOG(log_debug, logtype_afpd, "Attempting to register with mDNS using Avahi");

	ctx = av_zeroconf_setup(port ? port : AFP_PORT, 
													(hostname && strlen(hostname) > 0) ? hostname : NULL);
  av_zeroconf_run(ctx);
#endif
}

void zeroconf_register_volume(const char *volname)
{
#if defined (HAVE_AVAHI)
  LOG(log_debug, logtype_afpd, "Attempting to register volume with mDNS using Avahi");

	ctx = av_zeroconf_setup(-1, volname);

  av_zeroconf_run(ctx);
#endif
}


void zeroconf_deregister(void)
{
#if defined (HAVE_AVAHI)
  LOG(log_debug, logtype_afpd, "Attempting to de-register mDNS using Avahi");
  if (ctx)
    av_zeroconf_shutdown(ctx);
#endif
}
