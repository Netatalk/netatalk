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

#ifndef HAVE_AVAHI_THREADED_POLL
#include <avahi-common/simple-watch.h>
#include <signal.h> /* SIG_BLOCK */
#else
#include <avahi-common/thread-watch.h>
#endif

#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <atalk/logger.h>

#define AFP_DNS_SERVICE_TYPE "_afpovertcp._tcp"

struct context {
  int thread_running;
  pthread_t thread_id;
  pthread_mutex_t mutex;
  char *name;
#ifndef HAVE_AVAHI_THREADED_POLL
  AvahiSimplePoll   *simple_poll;
#else
  AvahiThreadedPoll *threaded_poll;
#endif
  AvahiClient       *client;
  AvahiEntryGroup   *group;
  unsigned long     port;
};

/* prototype definitions */
void* av_zeroconf_setup(unsigned long, const char *);
int av_zeroconf_run(void*);
int av_zeroconf_unregister(void*);
void av_zeroconf_shutdown(void*);
void av_zeroconf_lock(void *);
void av_zeroconf_unlock(void *);

#endif   /* AFPD_AVAHI_H */
