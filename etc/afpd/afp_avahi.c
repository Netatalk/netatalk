/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Author:  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 * Purpose: Avahi based Zeroconf support
 * Docs:    http://avahi.org/download/doxygen/
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_AVAHI

#include <unistd.h>
#include "afp_avahi.h"

static void publish_reply(AvahiEntryGroup *g,
                          AvahiEntryGroupState state,
                          void *userdata);

/*
 * This function tries to register the AFP DNS
 * SRV service type.
 */
static void register_stuff(struct context *ctx) {
  char r[128];
  int ret;

  assert(ctx->client);

  if (!ctx->group) {
    if (!(ctx->group = avahi_entry_group_new(ctx->client,
                                             publish_reply,
                                             ctx))) {
      LOG(log_error, logtype_afpd, "Failed to create entry group: %s",
          avahi_strerror(avahi_client_errno(ctx->client)));
      goto fail;
    }

  }

  LOG(log_info, logtype_afpd, "Adding service '%s'", ctx->name);

  if (avahi_entry_group_is_empty(ctx->group)) {
    /* Register our service */

    if (avahi_entry_group_add_service(ctx->group,
                                      AVAHI_IF_UNSPEC,
                                      AVAHI_PROTO_UNSPEC,
                                      0,
                                      ctx->name,
                                      AFP_DNS_SERVICE_TYPE,
                                      NULL,
                                      NULL,
                                      ctx->port,
                                      NULL) < 0) {
      LOG(log_error, logtype_afpd, "Failed to add service: %s",
          avahi_strerror(avahi_client_errno(ctx->client)));
      goto fail;
    }

    if (avahi_entry_group_commit(ctx->group) < 0) {
      LOG(log_error, logtype_afpd, "Failed to commit entry group: %s",
          avahi_strerror(avahi_client_errno(ctx->client)));
      goto fail;
    }
  }

  return;

fail:
	avahi_client_free (ctx->client);
	avahi_threaded_poll_quit(ctx->threaded_poll);
}

/* Called when publishing of service data completes */
static void publish_reply(AvahiEntryGroup *g,
                          AvahiEntryGroupState state,
                          AVAHI_GCC_UNUSED void *userdata)
{
  struct context *ctx = userdata;
	char *n;

  assert(g == ctx->group);

  switch (state) {

  case AVAHI_ENTRY_GROUP_ESTABLISHED :
    /* The entry group has been established successfully */
    break;

  case AVAHI_ENTRY_GROUP_COLLISION:
    /* Pick a new name for our service */
    n = avahi_alternative_service_name(ctx->name);
    assert(n);
    avahi_free(ctx->name);
    ctx->name = n;
    register_stuff(ctx);
    break;
		
  case AVAHI_ENTRY_GROUP_FAILURE:
    LOG(log_error, logtype_afpd, "Failed to register service: %s",
				avahi_strerror(avahi_client_errno(ctx->client)));
		avahi_client_free(avahi_entry_group_get_client(g));
		avahi_threaded_poll_quit(ctx->threaded_poll);
		break;

	case AVAHI_ENTRY_GROUP_UNCOMMITED:
		break;
  case AVAHI_ENTRY_GROUP_REGISTERING:
		break;
  }
}

static void client_callback(AvahiClient *client,
                            AvahiClientState state,
                            void *userdata)
{
  struct context *ctx = userdata;

  ctx->client = client;

  switch (state) {
  case AVAHI_CLIENT_S_RUNNING:
    /* The server has startup successfully and registered its host
     * name on the network, so it's time to create our services */
    if (!ctx->group)
      register_stuff(ctx);
    break;

  case AVAHI_CLIENT_S_COLLISION:
    if (ctx->group)
      avahi_entry_group_reset(ctx->group);
    break;

  case AVAHI_CLIENT_FAILURE: {
    if (avahi_client_errno(client) == AVAHI_ERR_DISCONNECTED) {
      int error;

      avahi_client_free(ctx->client);
      ctx->client = NULL;
      ctx->group = NULL;

      /* Reconnect to the server */
      if (!(ctx->client = avahi_client_new(avahi_threaded_poll_get(ctx->threaded_poll),
                                           AVAHI_CLIENT_NO_FAIL,
                                           client_callback,
                                           ctx,
                                           &error))) {

        LOG(log_error, logtype_afpd, "Failed to contact server: %s\n",
            avahi_strerror(error));

        avahi_client_free (ctx->client);
        avahi_threaded_poll_quit(ctx->threaded_poll);
      }

		} else {
			LOG(log_error, logtype_afpd, "Client failure: %s\n",
					avahi_strerror(avahi_client_errno(client)));
			avahi_client_free (ctx->client);
			avahi_threaded_poll_quit(ctx->threaded_poll);
		}
    break;
  }

  case AVAHI_CLIENT_S_REGISTERING:
		break;
  case AVAHI_CLIENT_CONNECTING:
    break;
  }
}

/*
 * Tries to setup the Zeroconf thread and any
 * neccessary config setting.
 */
void* av_zeroconf_setup(unsigned long port, const char *name) {
  struct context *ctx = NULL;

  /* default service name, if there's none in
   * the config file.
   */
  char service[256] = "AFP Server on ";
  int error, ret;

  /* initialize the struct that holds our
   * config settings.
   */
  ctx = malloc(sizeof(struct context));
  assert(ctx);
  ctx->client = NULL;
  ctx->group = NULL;
  ctx->threaded_poll = NULL;
  ctx->thread_running = 0;

  LOG(log_debug, logtype_afpd, "Setting port for Zeroconf service to: %i.", port);  
  ctx->port = port;

  /* Prepare service name */
  if (!name) {
    LOG(log_debug, logtype_afpd, "Assigning default service name.");
    gethostname(service+14, sizeof(service)-15);
    service[sizeof(service)-1] = 0;
    ctx->name = strdup(service);
  } else {
    ctx->name = strdup(name);
  }

  assert(ctx->name);

/* first of all we need to initialize our threading env */
  if (!(ctx->threaded_poll = avahi_threaded_poll_new())) {
     goto fail;
  }

/* now we need to acquire a client */
  if (!(ctx->client = avahi_client_new(avahi_threaded_poll_get(ctx->threaded_poll),
                                       AVAHI_CLIENT_NO_FAIL,
                                       client_callback,
                                       ctx,
                                       &error))) {
    LOG(log_error, logtype_afpd, "Failed to create client object: %s",
        avahi_strerror(avahi_client_errno(ctx->client)));
    goto fail;
  }

  return ctx;

fail:
  if (ctx)
    av_zeroconf_unregister(ctx);

  return NULL;
}

/*
 * This function finally runs the loop impl.
 */
int av_zeroconf_run(void *u) {
  struct context *ctx = u;
  int ret;

  /* Finally, start the event loop thread */
  if (avahi_threaded_poll_start(ctx->threaded_poll) < 0) {
    LOG(log_error,
        logtype_afpd,
        "Failed to create thread: %s",
        avahi_strerror(avahi_client_errno(ctx->client)));
    goto fail;
  } else {
    LOG(log_info, logtype_afpd, "Successfully started avahi loop.");
  }

  ctx->thread_running = 1;
  return 0;

fail:
	if (ctx)
    av_zeroconf_unregister(ctx);

  return -1;
}

/*
 * Tries to shutdown this loop impl.
 * Call this function from outside this thread.
 */
void av_zeroconf_shutdown(void *u) {
  struct context *ctx = u;

  /* Call this when the app shuts down */
  avahi_threaded_poll_stop(ctx->threaded_poll);
  avahi_free(ctx->name);
  avahi_client_free(ctx->client);
  avahi_threaded_poll_free(ctx->threaded_poll);
}

/*
 * Tries to shutdown this loop impl.
 * Call this function from inside this thread.
 */
int av_zeroconf_unregister(void *u) {
  struct context *ctx = u;

  if (ctx->thread_running) {
    /* First, block the event loop */
    avahi_threaded_poll_lock(ctx->threaded_poll);

    /* Than, do your stuff */
    avahi_threaded_poll_quit(ctx->threaded_poll);

    /* Finally, unblock the event loop */
    avahi_threaded_poll_unlock(ctx->threaded_poll);
    ctx->thread_running = 0;
  }

  avahi_free(ctx->name);

  if (ctx->client)
    avahi_client_free(ctx->client);

  if (ctx->threaded_poll)
    avahi_threaded_poll_free(ctx->threaded_poll);

  free(ctx);

  return 0;
}

#endif /* USE_AVAHI */
