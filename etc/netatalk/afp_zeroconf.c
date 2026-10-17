/*
 * Copyright (c) 2006 Daniel S. Haischt <me@daniel.stefan.haischt.name>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*!
 * @file
 * @brief Register and withdraw AFP services through the Zeroconf
 * implementation
 * @sa http://www.dns-sd.org/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "afp_zeroconf.h"

#ifdef HAVE_MDNS
#include "afp_mdns.h"
#elif defined (HAVE_AVAHI)
#include "afp_avahi.h"
#endif


/*!
 * @brief Register the AFP services with mDNSResponder or Avahi, whichever the
 * build has
 *
 * The facade prefers mDNSResponder when the build has both. Without either
 * the call does nothing.
 *
 * @param[in] configs  AFP object with the options
 */
void zeroconf_register(const AFPObj *configs _U_)
{
#if defined (HAVE_MDNS)
    LOG(log_debug, logtype_afpd,
        "Attempting to register with mDNS using mDNSResponder");
    md_zeroconf_register(configs);
#elif defined (HAVE_AVAHI)
    LOG(log_debug, logtype_afpd, "Attempting to register with mDNS using Avahi");
    av_zeroconf_register(configs);
#endif
}

/*!
 * @brief Withdraw the AFP services from mDNSResponder or Avahi, whichever the
 * build has
 */
void zeroconf_deregister(void)
{
#if defined (HAVE_MDNS)
    LOG(log_debug, logtype_afpd,
        "Attempting to de-register mDNS using mDNSResponder");
    md_zeroconf_unregister();
#elif defined (HAVE_AVAHI)
    LOG(log_debug, logtype_afpd, "Attempting to de-register mDNS using Avahi");
    av_zeroconf_unregister();
#endif
}
