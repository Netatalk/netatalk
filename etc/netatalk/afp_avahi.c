/*
 * Author:  Daniel S. Haischt <me@daniel.stefan.haischt.name>
 */

/*!
 * @file
 * @brief Avahi based Zeroconf support
 * @sa https://avahi.org/doxygen/html/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_AVAHI

#include <time.h>
#include <unistd.h>

#include <avahi-common/strlst.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/unicode.h>
#include <atalk/util.h>

#include "afp_avahi.h"
#include "afp_zeroconf.h"

/*****************************************************************
 * Global variables
 *****************************************************************/
struct context *ctx = NULL;

/*****************************************************************
 * Private functions
 *****************************************************************/

static void publish_reply(AvahiEntryGroup *g,
                          AvahiEntryGroupState state,
                          void *userdata);

/*!
 * @brief Register the AFP service, the Time Machine volumes, and the device
 * model with Avahi
 *
 * The call fills an empty entry group only. The AFP service goes out under
 * the server name, or the host name without one. Time Machine volumes go
 * into the TXT records of an adisk service on the discard port, each with
 * its volume UUID when it has one. The mimic model adds a device-info
 * service. A failure logs and leaves the group as it is.
 */
static void register_stuff(void)
{
    uint port;
    const struct vol *volume;
    char name[MAXINSTANCENAMELEN + 1];
    AvahiStringList *strlist = NULL;
    AvahiStringList *strlist2 = NULL;
    char tmpname[256];
    assert(ctx->client);

    if (!ctx->group) {
        if (!(ctx->group = avahi_entry_group_new(ctx->client, publish_reply, ctx))) {
            LOG(log_error, logtype_afpd, "Failed to create entry group: %s",
                avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }
    }

    if (avahi_entry_group_is_empty(ctx->group)) {
        /* Register our service */
        /* Build AFP volumes list */
        int i = 0;
        strlist = avahi_string_list_add_printf(strlist, "sys=waMa=0,adVF=0x100");

        for (volume = getvolumes(); volume; volume = volume->v_next) {
            if (convert_string(CH_UCS2, CH_UTF8_MAC, volume->v_u8mname, -1, tmpname,
                               255) <= 0) {
                LOG(log_error, logtype_afpd,
                    "Could not set Zeroconf volume name for TimeMachine");
                goto fail;
            }

            if (volume->v_flags & AFPVOL_TM) {
                if (volume->v_uuid) {
                    LOG(log_info, logtype_afpd,
                        "Registering volume '%s' with UUID: '%s' for TimeMachine",
                        volume->v_localname, volume->v_uuid);
                    strlist = avahi_string_list_add_printf(strlist,
                                                           "dk%u=adVN=%s,adVF=0xa1,adVU=%s",
                                                           i++, tmpname, volume->v_uuid);
                } else {
                    LOG(log_warning, logtype_afpd,
                        "Registering volume '%s' for TimeMachine. But UUID is invalid.",
                        volume->v_localname);
                    strlist = avahi_string_list_add_printf(strlist, "dk%u=adVN=%s,adVF=0xa1",
                                                           i++, tmpname);
                }
            }
        }

        port = atoi(ctx->obj->options.port);
        LOG(log_info, logtype_afpd, "hostname: %s", ctx->obj->options.hostname);

        if (ctx->obj->options.servername) {
            if (convert_string(ctx->obj->options.unixcharset,
                               CH_UTF8,
                               ctx->obj->options.servername,
                               -1,
                               name,
                               MAXINSTANCENAMELEN) <= 0) {
                LOG(log_error, logtype_afpd, "Could not set Zeroconf instance name: %s",
                    ctx->obj->options.servername);
                goto fail;
            }
        } else {
            if (convert_string(ctx->obj->options.unixcharset,
                               CH_UTF8,
                               ctx->obj->options.hostname,
                               -1,
                               name,
                               MAXINSTANCENAMELEN) <= 0) {
                LOG(log_error, logtype_afpd, "Could not set Zeroconf instance name: %s",
                    ctx->obj->options.hostname);
                goto fail;
            }
        }

        LOG(log_info, logtype_afpd, "Registering server '%s' with Zeroconf", name);

        if (avahi_entry_group_add_service(ctx->group,
                                          AVAHI_IF_UNSPEC,
                                          AVAHI_PROTO_UNSPEC,
                                          0,
                                          name,
                                          AFP_DNS_SERVICE_TYPE,
                                          NULL,
                                          NULL,
                                          port,
                                          NULL) < 0) {
            LOG(log_error, logtype_afpd, "Failed to add service: %s",
                avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }

        if (i && avahi_entry_group_add_service_strlst(ctx->group,
                                                      AVAHI_IF_UNSPEC,
                                                      AVAHI_PROTO_UNSPEC,
                                                      0,
                                                      name,
                                                      ADISK_SERVICE_TYPE,
                                                      NULL,
                                                      NULL,
                                                      9, /* discard */
                                                      strlist) < 0) {
            LOG(log_error, logtype_afpd, "Failed to add service: %s",
                avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }	/* if */

        if (ctx->obj->options.mimicmodel) {
            strlist2 = avahi_string_list_add_printf(strlist2, "model=%s",
                                                    ctx->obj->options.mimicmodel);

            if (avahi_entry_group_add_service_strlst(ctx->group,
                                                     AVAHI_IF_UNSPEC,
                                                     AVAHI_PROTO_UNSPEC,
                                                     0,
                                                     name,
                                                     DEV_INFO_SERVICE_TYPE,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     strlist2) < 0) {
                LOG(log_error, logtype_afpd, "Failed to add service: %s",
                    avahi_strerror(avahi_client_errno(ctx->client)));
                goto fail;
            }
        } /* if (config->obj.options.mimicmodel) */

        if (avahi_entry_group_commit(ctx->group) < 0) {
            LOG(log_error, logtype_afpd, "Failed to commit entry group: %s",
                avahi_strerror(avahi_client_errno(ctx->client)));
            goto fail;
        }
    }	/* if avahi_entry_group_is_empty*/

    return;
fail:
    time(NULL);
#if 0
    avahi_threaded_poll_quit(ctx->threaded_poll);
#endif
}

/*!
 * @brief Log the state of the entry group, and stop the Avahi thread on a name
 * collision or a failure
 *
 * @param[in] g         entry group
 * @param[in] state     new state of the group
 * @param[in] userdata  unused
 */
static void publish_reply(AvahiEntryGroup *g,
                          AvahiEntryGroupState state,
                          AVAHI_GCC_UNUSED void *userdata)
{
    assert(ctx->group == NULL || g == ctx->group);

    switch (state) {
    case AVAHI_ENTRY_GROUP_ESTABLISHED :
        /* The entry group has been established successfully */
        LOG(log_debug, logtype_afpd, "publish_reply: AVAHI_ENTRY_GROUP_ESTABLISHED");
        break;

    case AVAHI_ENTRY_GROUP_COLLISION:
        /* With multiple names there's no way to know which one collided */
        LOG(log_error, logtype_afpd, "publish_reply: AVAHI_ENTRY_GROUP_COLLISION",
            avahi_strerror(avahi_client_errno(ctx->client)));
        avahi_threaded_poll_quit(ctx->threaded_poll);
        break;

    case AVAHI_ENTRY_GROUP_FAILURE:
        LOG(log_error, logtype_afpd, "Failed to register service: %s",
            avahi_strerror(avahi_client_errno(ctx->client)));
        avahi_threaded_poll_quit(ctx->threaded_poll);
        break;

    case AVAHI_ENTRY_GROUP_UNCOMMITED:
        break;

    case AVAHI_ENTRY_GROUP_REGISTERING:
        break;
    }
}

/*!
 * @brief Track the state of the Avahi client, and register the services once
 * the daemon runs
 *
 * A host name collision resets the entry group. A disconnect from the daemon
 * frees the client and connects again. Any other failure stops the Avahi
 * thread.
 *
 * @param[in] client    Avahi client
 * @param[in] state     new state of the client
 * @param[in] userdata  unused
 */
static void client_callback(AvahiClient *client,
                            AvahiClientState state,
                            void *userdata)
{
    ctx->client = client;

    switch (state) {
    case AVAHI_CLIENT_S_RUNNING:

        /* The server has startup successfully and registered its host
         * name on the network, so it's time to create our services */
        if (!ctx->group) {
            register_stuff();
        }

        break;

    case AVAHI_CLIENT_S_COLLISION:
        if (ctx->group) {
            avahi_entry_group_reset(ctx->group);
        }

        break;

    case AVAHI_CLIENT_FAILURE: {
        if (avahi_client_errno(client) == AVAHI_ERR_DISCONNECTED) {
            int error;
            avahi_client_free(ctx->client);
            ctx->client = NULL;
            ctx->group = NULL;

            /* Reconnect to the server */
            if (!(ctx->client = avahi_client_new(avahi_threaded_poll_get(
                                                     ctx->threaded_poll),
                                                 AVAHI_CLIENT_NO_FAIL,
                                                 client_callback,
                                                 ctx,
                                                 &error))) {
                LOG(log_error, logtype_afpd, "Failed to contact server: %s",
                    avahi_strerror(error));
                avahi_threaded_poll_quit(ctx->threaded_poll);
            }
        } else {
            LOG(log_error, logtype_afpd, "Client failure: %s",
                avahi_strerror(avahi_client_errno(client)));
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

/************************************************************************
 * Public funcions
 ************************************************************************/

/*!
 * @brief Start the Avahi thread, which registers the AFP services with Zeroconf
 *
 * A second call resets the entry group and starts a new thread and client.
 * A failure tears the setup down through av_zeroconf_unregister.
 *
 * @param[in] obj  AFP object with the options
 */
void av_zeroconf_register(const AFPObj *obj)
{
    int error;

    /* initialize the struct that holds our config settings. */
    if (ctx) {
        LOG(log_debug, logtype_afpd, "Resetting zeroconf records");
        avahi_entry_group_reset(ctx->group);
    } else {
        ctx = calloc(1, sizeof(struct context));
        ctx->obj = obj;
        assert(ctx);
    }

    /* first of all we need to initialize our threading env */
    if (!(ctx->threaded_poll = avahi_threaded_poll_new())) {
        goto fail;
    }

    /* now we need to acquire a client */
    if (!(ctx->client = avahi_client_new(avahi_threaded_poll_get(
                                             ctx->threaded_poll),
                                         AVAHI_CLIENT_NO_FAIL,
                                         client_callback,
                                         NULL,
                                         &error))) {
        LOG(log_error, logtype_afpd, "Failed to create client object: %s",
            avahi_strerror(error));
        goto fail;
    }

    if (avahi_threaded_poll_start(ctx->threaded_poll) < 0) {
        LOG(log_error, logtype_afpd, "Failed to create thread: %s",
            avahi_strerror(avahi_client_errno(ctx->client)));
        goto fail;
    } else {
        LOG(log_info, logtype_afpd, "Successfully started avahi loop.");
    }

    ctx->thread_running = 1;
    return;
fail:
    av_zeroconf_unregister();
    return;
}

/*!
 * @brief Stop the Avahi thread and free the Zeroconf state
 *
 * The call stops the thread with avahi_threaded_poll_stop, so the caller has
 * to run outside the Avahi thread.
 *
 * @returns 0
 */
int av_zeroconf_unregister()
{
    LOG(log_debug, logtype_afpd, "av_zeroconf_unregister");

    if (ctx) {
        if (ctx->threaded_poll) {
            avahi_threaded_poll_stop(ctx->threaded_poll);
        }

        if (ctx->client) {
            avahi_client_free(ctx->client);
        }

        if (ctx->threaded_poll) {
            avahi_threaded_poll_free(ctx->threaded_poll);
        }

        free(ctx);
        ctx = NULL;
    }

    return 0;
}

#endif /* USE_AVAHI */

