/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Author:  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * Purpose: Avahi based Zeroconf support
 * Docs:    http://avahi.org/download/doxygen/
 *
 */

#ifndef AFPD_AVAHI_H
#define AFPD_AVAHI_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <atalk/logger.h>

#include "afp_config.h"

#define AFP_DNS_SERVICE_TYPE "_afpovertcp._tcp"
#define ADISK_SERVICE_TYPE "_adisk._tcp"
#define DEV_INFO_SERVICE_TYPE "_device-info._tcp"

#define MAXINSTANCENAMELEN 63

struct context {
	/* Avahi stuff */
  int               thread_running;
  AvahiThreadedPoll *threaded_poll;
  AvahiClient       *client;
  AvahiEntryGroup   *group;
	/* Netatalk stuff */
	const AFPConfig   *configs;
};

/* prototype definitions */
void av_zeroconf_register(const AFPConfig *configs);
int av_zeroconf_unregister(void);

#endif   /* AFPD_AVAHI_H */
