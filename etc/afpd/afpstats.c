/*
 * Copyright (c) 2013 Frank Lahm <franklahm@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <atalk/compat.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/server_child.h>

#include "afpstats_obj.h"

/*
 * Beware: this struct is accessed and modified from the main thread
 * and from this thread, thus be careful to lock and unlock the mutex.
 */
static server_child_t *childs;
static GDBusNodeInfo *introspection_data = NULL;
static GMainLoop *thread_loop = NULL;
static GThread *afpstats_thread_handle = NULL;
static GDBusConnection *bus = NULL;  /* Thread-safe: only accessed from afpstats_thread */

static void handle_method_call(GDBusConnection *connection _U_,
                               const gchar           *sender _U_,
                               const gchar           *object_path _U_,
                               const gchar           *interface_name _U_,
                               const gchar           *method_name,
                               GVariant              *parameters _U_,
                               GDBusMethodInvocation *invocation,
                               gpointer               user_data)
{
    AFPStatsObj *obj = AFPSTATS_OBJECT(user_data);

    if (g_strcmp0(method_name, "GetUsers") == 0) {
        GError *local_error = NULL;
        gchar **users = NULL;

        if (afpstats_obj_get_users(obj, &users, &local_error)) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(^as)",
                                                  users));
        } else {
            g_dbus_method_invocation_return_gerror(invocation, local_error);
        }

        g_strfreev(users);
        g_clear_error(&local_error);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    &handle_method_call,
    NULL,
    NULL,
    { 0 }
};

static void on_bus_acquired(GDBusConnection *connection _U_,
                            const gchar     *name,
                            gpointer         user_data _U_)
{
    LOG(log_debug, logtype_afpd, "[afpstats] on_bus_acquired(): %s", name);
}

static void on_name_acquired(GDBusConnection *connection _U_,
                             const gchar     *name,
                             gpointer         user_data _U_)
{
    LOG(log_debug, logtype_afpd, "[afpstats] on_name_acquired(): %s", name);
}

static void on_name_lost(GDBusConnection *connection _U_,
                         const gchar     *name,
                         gpointer         user_data _U_)
{
    LOG(log_debug, logtype_afpd, "[afpstats] on_name_lost(): %s", name);
}

static void on_bus_got(GObject *source _U_, GAsyncResult *res, gpointer user_data _U_)
{
    GError *error = NULL;
    bus = g_bus_get_finish(res, &error);
    if (error) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to get bus: %s", error->message);
        g_error_free(error);
        g_main_loop_quit(thread_loop);
        return;
    }
    LOG(log_debug, logtype_afpd, "[afpstats] D-Bus connection acquired");
}

static gpointer afpstats_thread(gpointer _data _U_)
{
    EC_INIT;
    GError *error = NULL;
    GMainContext *ctxt = NULL;
    GBusNameOwnerFlags request_name_result;
    guint registration_id = 0;
    sigset_t sigs;
    AFPStatsObj *obj = NULL;

    /* Block all signals in this thread */
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    ctxt = g_main_context_new();
    EC_NULL_LOG(ctxt);
    g_main_context_push_thread_default(ctxt);

    thread_loop = g_main_loop_new(ctxt, FALSE);
    EC_NULL_LOG(thread_loop);

    g_bus_get(G_BUS_TYPE_SYSTEM, NULL, on_bus_got, NULL);

    static const gchar introspection_xml[] =
        "<node>"
        "  <interface name='org.netatalk.AFPStats'>"
        "    <method name='GetUsers'>"
        "      <arg name='ret' type='as' direction='out'/>"
        "    </method>"
        "  </interface>"
        "</node>";

    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    EC_NULL_LOG(introspection_data);

    obj = g_object_new(AFPSTATS_TYPE_OBJECT, NULL);
    EC_NULL_LOG(obj);

    error = NULL;
    registration_id = g_dbus_connection_register_object(bus,
                                                        "/org/netatalk/AFPStats",
                                                        introspection_data->interfaces[0],
                                                        &interface_vtable,
                                                        g_object_ref(obj),
                                                        g_object_unref,
                                                        &error);

    if (registration_id == 0) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to register object: %s",
            error ? error->message : "unknown error");
        if (error) {
            g_error_free(error);
        }
        ret = -1;
        goto EC_CLEANUP;
    }

    request_name_result = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                         "org.netatalk.AFPStats",
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         on_bus_acquired,
                                         on_name_acquired,
                                         on_name_lost,
                                         NULL,
                                         NULL);

    LOG(log_info, logtype_afpd, "[afpstats] D-Bus thread started successfully");
    g_main_loop_run(thread_loop);

EC_CLEANUP:
    if (request_name_result > 0) {
        g_bus_unown_name(request_name_result);
    }

    if (registration_id > 0) {
        g_dbus_connection_unregister_object(bus, registration_id);
    }

    if (introspection_data) {
        g_dbus_node_info_unref(introspection_data);
        introspection_data = NULL;
    }

    if (obj) {
        g_object_unref(obj);
    }

    if (bus) {
        g_object_unref(bus);
    }

    if (ctxt) {
        g_main_context_pop_thread_default(ctxt);
        g_main_context_unref(ctxt);
    }

    LOG(log_info, logtype_afpd, "[afpstats] D-Bus thread shutting down (ret=%d)", ret);
    return GINT_TO_POINTER(ret);
}

static void my_glib_log(const gchar *log_domain,
                        GLogLevelFlags log_level _U_,
                        const gchar *message,
                        gpointer user_data _U_)
{
    LOG(log_debug, logtype_afpd, "[afpstats] %s: %s", log_domain, message);
}

server_child_t *afpstats_get_and_lock_childs(void)
{
    if (childs == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&childs->servch_lock);
    return childs;
}

void afpstats_unlock_childs(void)
{
    if (childs == NULL) {
        return;
    }
    pthread_mutex_unlock(&childs->servch_lock);
}

int afpstats_init(server_child_t *childs_in)
{
    EC_INIT;

    if (childs_in == NULL) {
        LOG(log_error, logtype_afpd, "[afpstats] init: childs_in is NULL");
        return -1;
    }

    childs = childs_in;
    (void)g_log_set_default_handler(my_glib_log, NULL);

    afpstats_thread_handle = g_thread_new("afpstats", afpstats_thread, NULL);
    EC_NULL_LOG(afpstats_thread_handle);

    LOG(log_info, logtype_afpd, "[afpstats] initialized");
    return 0;

EC_CLEANUP:
    return ret;
}

void afpstats_shutdown(void)
{
    gpointer thread_ret;

    if (thread_loop == NULL) {
        LOG(log_debug, logtype_afpd, "[afpstats] shutdown: not initialized");
        return;
    }

    LOG(log_info, logtype_afpd, "[afpstats] shutting down D-Bus thread");

    /* Signal the GLib event loop to exit */
    g_main_loop_quit(thread_loop);

    /* Wait for thread to finish and collect return code */
    if (afpstats_thread_handle) {
        thread_ret = g_thread_join(afpstats_thread_handle);
        LOG(log_info, logtype_afpd, "[afpstats] D-Bus thread joined (ret=%d)",
            GPOINTER_TO_INT(thread_ret));
        afpstats_thread_handle = NULL;
    }

    /* Release references */
    if (thread_loop) {
        g_main_loop_unref(thread_loop);
        thread_loop = NULL;
    }

    childs = NULL;
    LOG(log_info, logtype_afpd, "[afpstats] shutdown complete");
}
