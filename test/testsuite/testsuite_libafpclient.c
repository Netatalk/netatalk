/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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
#endif

#include "testsuite_libafpclient.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <afp.h>
#include <libafpclient.h>
#include <uams_def.h>

#include "testhelper.h"

struct testsuite_libafpclient_session {
    struct afp_server *server;
};

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static int initialized;
static pthread_t loop_thread;

static void testsuite_libafpclient_log(void *priv, enum logtypes logtype,
                                       int loglevel, const char *message)
{
    (void)priv;
    (void)logtype;

    if (Quiet) {
        return;
    }

    if (!Verbose && loglevel > LOG_INFO) {
        return;
    }

    fprintf(stdout, "%s\n", message ? message : "(null)");
}

static struct libafpclient testsuite_libafpclient = {
    .unmount_volume = NULL,
    .log_for_client = testsuite_libafpclient_log,
    .forced_ending_hook = NULL,
    .scan_extra_fds = NULL,
    .loop_started = NULL,
};

static void fill_request(struct afp_connection_request *req,
                         const char *host, int port, int afp_version,
                         const char *user, const char *password,
                         unsigned int uam_mask)
{
    memset(req, 0, sizeof(*req));
    afp_default_url(&req->url);
    snprintf(req->url.servername, sizeof(req->url.servername), "%s", host);
    snprintf(req->url.username, sizeof(req->url.username), "%s", user ? user : "");
    snprintf(req->url.password, sizeof(req->url.password), "%s",
             password ? password : "");
    req->url.port = port;
    req->url.requested_version = afp_version;
    req->uam_mask = uam_mask;
}

int testsuite_libafpclient_init(void)
{
    pthread_mutex_lock(&init_lock);

    if (!initialized) {
        libafpclient_register(&testsuite_libafpclient);
        init_uams();
        afp_main_quick_startup(&loop_thread);
        initialized = 1;
    }

    pthread_mutex_unlock(&init_lock);
    return 0;
}

unsigned int testsuite_libafpclient_uam_mask(const char *uam)
{
    if (!uam || !*uam) {
        return 0;
    }

    return find_uam_by_name(uam);
}

const char *testsuite_libafpclient_resolve_uam(const char *uam)
{
    if (!uam || !*uam) {
        return NULL;
    }

    return resolve_uam_shorthand(uam);
}

unsigned int testsuite_libafpclient_discover_uams(const char *host, int port,
        int afp_version,
        const char *user,
        const char *password,
        bool *known)
{
    struct afp_connection_request req;
    struct afp_server *server;
    unsigned int supported = 0;

    if (known) {
        *known = false;
    }

    (void)user;
    (void)password;

    if (testsuite_libafpclient_init() != 0) {
        return 0;
    }

    /* Discovery only needs the server's advertised UAM bitmap.  Use guest auth
     * so this probe does not silently exercise an encrypted UAM before the
     * per-UAM tests can report which selector is running. */
    fill_request(&req, host, port, afp_version, "", "", UAM_NOUSERAUTHENT);
    server = afp_server_full_connect(NULL, &req);

    if (!server) {
        return 0;
    }

    supported = server->supported_uams;

    if (known) {
        *known = true;
    }

    afp_logout(server, 1);
    afp_server_remove(server);
    return supported;
}

int testsuite_libafpclient_connect(
    struct testsuite_libafpclient_session **session,
    const char *host, int port, int afp_version, const char *uam,
    const char *user, const char *password)
{
    struct afp_connection_request req;
    struct afp_server *server;
    unsigned int uam_mask;

    if (!session || !host || !uam) {
        return -1;
    }

    *session = NULL;

    if (testsuite_libafpclient_init() != 0) {
        return -1;
    }

    uam_mask = find_uam_by_name(uam);

    if (uam_mask == 0) {
        return -1;
    }

    fill_request(&req, host, port, afp_version, user, password, uam_mask);
    server = afp_server_full_connect(NULL, &req);

    if (!server) {
        return -1;
    }

    *session = calloc(1, sizeof(**session));

    if (!*session) {
        afp_logout(server, 1);
        afp_server_remove(server);
        return -1;
    }

    (*session)->server = server;
    return 0;
}

unsigned int testsuite_libafpclient_supported_uams(
    const struct testsuite_libafpclient_session *session)
{
    if (!session || !session->server) {
        return 0;
    }

    return session->server->supported_uams;
}

unsigned int testsuite_libafpclient_using_uam(
    const struct testsuite_libafpclient_session *session)
{
    if (!session || !session->server) {
        return 0;
    }

    return session->server->using_uam;
}

int testsuite_libafpclient_using_version(
    const struct testsuite_libafpclient_session *session)
{
    if (!session || !session->server || !session->server->using_version) {
        return 0;
    }

    return session->server->using_version->av_number;
}

int testsuite_libafpclient_smoke(struct testsuite_libafpclient_session *session)
{
    if (!session || !session->server) {
        return -1;
    }

    return afp_getsrvrparms(session->server);
}

void testsuite_libafpclient_close(
    struct testsuite_libafpclient_session **session)
{
    if (!session || !*session) {
        return;
    }

    if ((*session)->server) {
        afp_logout((*session)->server, 1);
        afp_server_remove((*session)->server);
    }

    free(*session);
    *session = NULL;
}
