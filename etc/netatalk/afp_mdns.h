/*!
 * @file
 * @author   Lee Essen <lee.essen@nowonline.co.uk>
 * @brief    mDNS based Zeroconf support
 * @note     Based on: avahi support from Daniel S. Haischt <me@daniel.stefan.haischt.name>
 */

#ifndef AFPD_MDNS_H
#define AFPD_MDNS_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <dns_sd.h>

#include <atalk/logger.h>

/* prototype definitions */
void md_zeroconf_register(const AFPObj *obj);
int md_zeroconf_unregister(void);

#endif   /* AFPD_MDNS_H */
