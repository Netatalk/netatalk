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
static server_child_t * volatile childs = NULL;
static GDBusNodeInfo * volatile introspection_data = NULL;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

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
            g_clear_error(&local_error);
        }

        g_strfreev(users);
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

static gpointer afpstats_thread(gpointer _data _U_)
{
    GDBusConnection *bus;
    GError *error = NULL;
    GMainContext *ctxt;
    GMainLoop *thread_loop;
    GBusNameOwnerFlags request_name_result;
    guint registration_id;
    sigset_t sigs;
    /* Block all signals in this thread */
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
    ctxt = g_main_context_new();
    g_main_context_push_thread_default(ctxt);
    thread_loop = g_main_loop_new(ctxt, FALSE);

    if (!(bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM,
                               NULL,
                               &error))) {
        LOG(log_error, logtype_afpd, "Couldn't connect to system bus: %s",
            error->message);
        g_error_free(error);
        g_main_loop_unref(thread_loop);
        g_main_context_pop_thread_default(ctxt);
        g_main_context_unref(ctxt);
        return NULL;
    }

    static const gchar introspection_xml[] =
        "<node>"
        "  <interface name='org.netatalk.AFPStats'>"
        "    <method name='GetUsers'>"
        "      <arg name='ret' type='as' direction='out'/>"
        "    </method>"
        "  </interface>"
        "</node>";
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (!introspection_data) {
        LOG(log_error, logtype_afpd, "Failed to parse introspection data: %s",
            error ? error->message : "Unknown error");
        g_clear_error(&error);
        g_object_unref(bus);
        g_main_loop_unref(thread_loop);
        g_main_context_pop_thread_default(ctxt);
        g_main_context_unref(ctxt);
        return NULL;
    }

    AFPStatsObj *obj = g_object_new(AFPSTATS_TYPE_OBJECT, NULL);
    if (!obj) {
        LOG(log_error, logtype_afpd, "Failed to create AFPStatsObj");
        g_dbus_node_info_unref(introspection_data);
        g_object_unref(bus);
        g_main_loop_unref(thread_loop);
        g_main_context_pop_thread_default(ctxt);
        g_main_context_unref(ctxt);
        return NULL;
    }

    registration_id = g_dbus_connection_register_object(bus,
                      "/org/netatalk/AFPStats",
                      introspection_data->interfaces[0],
                      &interface_vtable,
                      g_object_ref(obj),
                      g_object_unref,
                      &error);
    if (registration_id == 0) {
        LOG(log_error, logtype_afpd, "Failed to register object: %s",
            error ? error->message : "Unknown error");
        g_clear_error(&error);
        g_object_unref(obj);
        g_dbus_node_info_unref(introspection_data);
        g_object_unref(bus);
        g_main_loop_unref(thread_loop);
        g_main_context_pop_thread_default(ctxt);
        g_main_context_unref(ctxt);
        return NULL;
    }
    request_name_result = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                         "org.netatalk.AFPStats",
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         on_bus_acquired,
                                         on_name_acquired,
                                         on_name_lost,
                                         NULL,
                                         NULL);
    g_main_loop_run(thread_loop);
    g_bus_unown_name(request_name_result);
    g_dbus_connection_unregister_object(bus, registration_id);
    g_dbus_node_info_unref(introspection_data);
    g_object_unref(obj);
    g_object_unref(bus);
    g_main_context_pop_thread_default(ctxt);
    g_main_context_unref(ctxt);
    g_main_loop_unref(thread_loop);
    return NULL;
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
    pthread_mutex_lock(&childs->servch_lock);
    return childs;
}

void afpstats_unlock_childs(void)
{
    pthread_mutex_unlock(&childs->servch_lock);
}

int afpstats_init(server_child_t *childs_in)
{
    GThread *thread;
    GError *error = NULL;

    if (childs_in == NULL) {
        LOG(log_error, logtype_afpd, "afpstats_init: childs_in is NULL");
        return -1;
    }

    pthread_mutex_lock(&init_mutex);
    if (childs != NULL) {
        /* Already initialized */
        pthread_mutex_unlock(&init_mutex);
        LOG(log_warning, logtype_afpd, "afpstats_init: Already initialized");
        return 0;
    }

    childs = childs_in;
    pthread_mutex_unlock(&init_mutex);

    (void)g_log_set_default_handler(my_glib_log, NULL);

    thread = g_thread_try_new("afpstats", afpstats_thread, NULL, &error);
        LOG(log_error, logtype_afpd, "afpstats_init: ?? is NULL");
    if (thread == NULL) {
        LOG(log_error, logtype_afpd, "afpstats_init: failed to create thread: %s",
            error->message);
        g_error_free(error);
        return -1;
    }

    /* Thread will be automatically joined when the process exits */
    g_thread_unref(thread);
    return 0;
}
