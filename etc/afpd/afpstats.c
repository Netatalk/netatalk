/*
 * Copyright (c) 2013 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2024-2025 Daniel Markstedt <daniel@mindani.net>
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
#include <unistd.h>
#include <sys/wait.h>

#include <glib.h>
#include <gio/gio.h>

#include <atalk/compat.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/server_child.h>

#include "afpstats_obj.h"

/*
 * Thread state management structure
 */
typedef struct {
    GMainLoop *loop;
    GMainContext *context;
    GError *init_error;
    gboolean initialized;
    gboolean failed;
    pthread_mutex_t lock;
    GThread *gthread;
} afpstats_thread_state_t;

static server_child_t *childs;
static GDBusNodeInfo *introspection_data = NULL;
static afpstats_thread_state_t *thread_state = NULL;

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

static gpointer afpstats_thread(gpointer _data)
{
    afpstats_thread_state_t *state = (afpstats_thread_state_t *)_data;
    GDBusConnection *bus = NULL;
    GError *error = NULL;
    GBusNameOwnerFlags request_name_result = 0;
    guint registration_id = 0;
    sigset_t sigs;
    AFPStatsObj *obj = NULL;

    if (!state) {
        LOG(log_error, logtype_afpd, "[afpstats] Invalid thread state");
        return NULL;
    }

    /* Block all signals in this thread */
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    /* Create and configure GLib main context */
    state->context = g_main_context_new();
    if (!state->context) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to create GMainContext");
        state->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                                "Failed to create GMainContext");
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        return NULL;
    }

    g_main_context_push_thread_default(state->context);

    state->loop = g_main_loop_new(state->context, FALSE);
    if (!state->loop) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to create GMainLoop");
        state->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                                "Failed to create GMainLoop");
        g_main_context_pop_thread_default(state->context);
        g_main_context_unref(state->context);
        state->context = NULL;
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        return NULL;
    }

    /* Connect to system bus */
    bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!bus) {
        LOG(log_error, logtype_afpd, "[afpstats] Couldn't connect to system bus: %s",
            error ? error->message : "Unknown error");
        state->init_error = error;
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        g_main_loop_unref(state->loop);
        state->loop = NULL;
        g_main_context_pop_thread_default(state->context);
        g_main_context_unref(state->context);
        state->context = NULL;
        return NULL;
    }

    /* Parse and register D-Bus interface */
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
        LOG(log_error, logtype_afpd, "[afpstats] Failed to parse D-Bus introspection: %s",
            error ? error->message : "Unknown error");
        state->init_error = error;
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        g_object_unref(bus);
        g_main_loop_unref(state->loop);
        state->loop = NULL;
        g_main_context_pop_thread_default(state->context);
        g_main_context_unref(state->context);
        state->context = NULL;
        return NULL;
    }

    /* Create AFPStats object */
    obj = g_object_new(AFPSTATS_TYPE_OBJECT, NULL);
    if (!obj) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to create AFPStatsObj");
        state->init_error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED,
                                                "Failed to create AFPStatsObj");
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        g_dbus_node_info_unref(introspection_data);
        introspection_data = NULL;
        g_object_unref(bus);
        g_main_loop_unref(state->loop);
        state->loop = NULL;
        g_main_context_pop_thread_default(state->context);
        g_main_context_unref(state->context);
        state->context = NULL;
        return NULL;
    }

    /* Register D-Bus object */
    error = NULL;
    registration_id = g_dbus_connection_register_object(bus,
                      "/org/netatalk/AFPStats",
                      introspection_data->interfaces[0],
                      &interface_vtable,
                      g_object_ref(obj),
                      g_object_unref,
                      &error);
    if (registration_id == 0) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to register D-Bus object: %s",
            error ? error->message : "Unknown error");
        state->init_error = error;
        pthread_mutex_lock(&state->lock);
        state->initialized = FALSE;
        state->failed = TRUE;
        pthread_mutex_unlock(&state->lock);
        g_object_unref(obj);
        g_dbus_node_info_unref(introspection_data);
        introspection_data = NULL;
        g_object_unref(bus);
        g_main_loop_unref(state->loop);
        state->loop = NULL;
        g_main_context_pop_thread_default(state->context);
        g_main_context_unref(state->context);
        state->context = NULL;
        return NULL;
    }

    /* Request D-Bus name */
    error = NULL;
    request_name_result = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                         "org.netatalk.AFPStats",
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         on_bus_acquired,
                                         on_name_acquired,
                                         on_name_lost,
                                         NULL,
                                         NULL);

    LOG(log_info, logtype_afpd, "[afpstats] Thread initialized successfully");

    /* Signal successful initialization */
    pthread_mutex_lock(&state->lock);
    state->initialized = TRUE;
    pthread_mutex_unlock(&state->lock);

    /* Run main loop */
    g_main_loop_run(state->loop);

    LOG(log_debug, logtype_afpd, "[afpstats] Main loop exited, cleaning up");

    /* Cleanup */
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

    if (state->loop) {
        g_main_loop_unref(state->loop);
        state->loop = NULL;
    }

    g_main_context_pop_thread_default(state->context);

    if (state->context) {
        g_main_context_unref(state->context);
        state->context = NULL;
    }

    LOG(log_debug, logtype_afpd, "[afpstats] Thread cleanup complete");

    return NULL;
}

server_child_t *afpstats_get_and_lock_childs(void)
{
    if (childs) {
        pthread_mutex_lock(&childs->servch_lock);
    }
    return childs;
}

void afpstats_unlock_childs(void)
{
    if (childs) {
        pthread_mutex_unlock(&childs->servch_lock);
    }
}

/**
 * Check if D-Bus is available by attempting a non-blocking connection test
 * Returns TRUE if D-Bus appears available, FALSE otherwise
 * Does not poll, just checks synchronously
 */
static gboolean is_dbus_available(void)
{
    GError *error = NULL;
    GDBusConnection *test_bus = NULL;
    
    /* Use a very short timeout to test D-Bus availability */
    GCancellable *cancellable = g_cancellable_new();
    
    /* Try to get the system bus with a timeout */
    test_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, cancellable, &error);
    
    g_object_unref(cancellable);
    
    if (test_bus) {
        g_object_unref(test_bus);
        return TRUE;
    }
    
    if (error) {
        LOG(log_debug, logtype_afpd, "[afpstats] D-Bus check failed: %s", error->message);
        g_error_free(error);
    }
    
    return FALSE;
}

int afpstats_init(server_child_t *childs_in)
{
    gboolean init_success = FALSE;

    if (!childs_in) {
        LOG(log_error, logtype_afpd, "[afpstats] Invalid server_child_t pointer");
        return -1;
    }

    /* Quick check if D-Bus is available without forking */
    if (!is_dbus_available()) {
        LOG(log_info, logtype_afpd, 
            "[afpstats] D-Bus system bus not available. Skipping AFPStats module.");
        return 0; /* Return success - D-Bus just isn't available */
    }

    /* Allocate thread state */
    thread_state = g_malloc0(sizeof(afpstats_thread_state_t));
    if (!thread_state) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to allocate thread state");
        return -1;
    }

    pthread_mutex_init(&thread_state->lock, NULL);
    thread_state->initialized = FALSE;
    thread_state->failed = FALSE;
    thread_state->init_error = NULL;
    thread_state->gthread = NULL;

    childs = childs_in;

    /* Create thread */
    thread_state->gthread = g_thread_new("afpstats", afpstats_thread, thread_state);
    if (!thread_state->gthread) {
        LOG(log_error, logtype_afpd, "[afpstats] Failed to create afpstats thread");
        pthread_mutex_destroy(&thread_state->lock);
        g_free(thread_state);
        thread_state = NULL;
        return -1;
    }

    /* Give thread time to initialize */
    g_usleep(200000); /* 200ms */

    /* Check initialization status */
    pthread_mutex_lock(&thread_state->lock);
    if (thread_state->initialized) {
        init_success = TRUE;
    } else if (thread_state->init_error) {
        LOG(log_error, logtype_afpd, "[afpstats] Thread initialization failed: %s",
            thread_state->init_error->message);
    }
    if (thread_state->failed) {
        LOG(log_info, logtype_afpd,
            "[afpstats] Thread initialization failed");
    }
    GThread *thread_to_join = thread_state->gthread;
    pthread_mutex_unlock(&thread_state->lock);

    /* If initialization failed, join the thread immediately to clean up */
    if (!init_success) {
        LOG(log_debug, logtype_afpd, "[afpstats] Waiting for failed thread to clean up");
        if (thread_to_join) {
            g_thread_join(thread_to_join);
        }

        /* Clean up thread_state after thread has finished */
        pthread_mutex_lock(&thread_state->lock);
        if (thread_state->init_error) {
            g_clear_error(&thread_state->init_error);
        }
        pthread_mutex_unlock(&thread_state->lock);
        pthread_mutex_destroy(&thread_state->lock);
        g_free(thread_state);
        thread_state = NULL;

        LOG(log_debug, logtype_afpd, "[afpstats] Failed thread cleanup complete");
        return 0; /* Return success - we tried but D-Bus initialization failed */
    }

    LOG(log_info, logtype_afpd, "[afpstats] afpstats module initialized successfully");

    return 0;
}

/**
 * Gracefully shutdown the afpstats thread
 * Call this during daemon shutdown
 */
void afpstats_shutdown(void)
{
    if (!thread_state) {
        return;
    }

    LOG(log_debug, logtype_afpd, "[afpstats] Initiating graceful shutdown");

    pthread_mutex_lock(&thread_state->lock);
    GThread *thread_to_join = thread_state->gthread;
    if (thread_state->loop) {
        g_main_loop_quit(thread_state->loop);
    }
    pthread_mutex_unlock(&thread_state->lock);

    /* Join the thread to ensure it finishes cleanup */
    if (thread_to_join) {
        LOG(log_debug, logtype_afpd, "[afpstats] Joining thread");
        g_thread_join(thread_to_join);
    }

    /* Now safe to cleanup thread_state */
    pthread_mutex_lock(&thread_state->lock);
    if (thread_state->init_error) {
        g_clear_error(&thread_state->init_error);
    }
    pthread_mutex_unlock(&thread_state->lock);

    pthread_mutex_destroy(&thread_state->lock);
    g_free(thread_state);
    thread_state = NULL;

    LOG(log_debug, logtype_afpd, "[afpstats] Shutdown complete");
}
