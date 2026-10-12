/*!
 * @file
 * @author  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * @brief   Zeroconf facade, that abstracts access to a
 *          particular Zeroconf implementation
 * @sa      http://www.dns-sd.org/
 */

#ifndef AFPD_ZEROCONF_H
#define AFPD_ZEROCONF_H

#include <atalk/globals.h>

#define AFP_DNS_SERVICE_TYPE "_afpovertcp._tcp"
#define ADISK_SERVICE_TYPE "_adisk._tcp"
#define DEV_INFO_SERVICE_TYPE "_device-info._tcp"

#define MAXINSTANCENAMELEN 63

/*
 * Prototype Definitions
 */

/* Register the AFP services with the Zeroconf backend */
void zeroconf_register(const AFPObj *configs);

/* Withdraw the AFP services from the Zeroconf backend */
void zeroconf_deregister(void);

#endif /* AFPD_ZEROCONF_H */
