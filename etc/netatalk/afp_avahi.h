/*!
 * @file
 * @author  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * @brief   Avahi based Zeroconf support
 * @sa      https://avahi.org/doxygen/html/
 */

#ifndef AFPD_AVAHI_H
#define AFPD_AVAHI_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/thread-watch.h>

#include <atalk/logger.h>

struct context {
    /* Avahi stuff */
    int               thread_running;
    AvahiThreadedPoll *threaded_poll;
    AvahiClient       *client;
    AvahiEntryGroup   *group;
    /* Netatalk stuff */
    const AFPObj      *obj;
};

/* prototype definitions */
void av_zeroconf_register(const AFPObj *obj);
int av_zeroconf_unregister(void);

#endif   /* AFPD_AVAHI_H */
