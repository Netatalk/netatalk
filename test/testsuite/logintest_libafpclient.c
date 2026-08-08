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

#include "logintest_libafpclient.h"

#include <stdlib.h>

#include "testsuite_libafpclient.h"

struct logintest_libafpclient_session {
    struct testsuite_libafpclient_session *session;
};

int logintest_libafpclient_init(void)
{
    return testsuite_libafpclient_init();
}

unsigned int logintest_libafpclient_uam_mask(const char *uam)
{
    return testsuite_libafpclient_uam_mask(uam);
}

const char *logintest_libafpclient_resolve_uam(const char *uam)
{
    return testsuite_libafpclient_resolve_uam(uam);
}

unsigned int logintest_libafpclient_discover_uams(const char *host, int port,
        int afp_version,
        const char *user,
        const char *password,
        bool *known)
{
    return testsuite_libafpclient_discover_uams(host, port, afp_version, user,
            password, known);
}

int logintest_libafpclient_connect(
    struct logintest_libafpclient_session **session,
    const char *host, int port, int afp_version, const char *uam,
    const char *user, const char *password)
{
    struct logintest_libafpclient_session *wrapper;

    if (!session) {
        return -1;
    }

    *session = NULL;
    wrapper = calloc(1, sizeof(*wrapper));

    if (!wrapper) {
        return -1;
    }

    if (testsuite_libafpclient_connect(&wrapper->session, host, port,
                                       afp_version, uam, user, password) != 0) {
        free(wrapper);
        return -1;
    }

    *session = wrapper;
    return 0;
}

unsigned int logintest_libafpclient_supported_uams(
    const struct logintest_libafpclient_session *session)
{
    if (!session) {
        return 0;
    }

    return testsuite_libafpclient_supported_uams(session->session);
}

unsigned int logintest_libafpclient_using_uam(
    const struct logintest_libafpclient_session *session)
{
    if (!session) {
        return 0;
    }

    return testsuite_libafpclient_using_uam(session->session);
}

int logintest_libafpclient_using_version(
    const struct logintest_libafpclient_session *session)
{
    if (!session) {
        return 0;
    }

    return testsuite_libafpclient_using_version(session->session);
}

int logintest_libafpclient_smoke(
    struct logintest_libafpclient_session *session)
{
    if (!session) {
        return -1;
    }

    return testsuite_libafpclient_smoke(session->session);
}

void logintest_libafpclient_close(
    struct logintest_libafpclient_session **session)
{
    if (!session || !*session) {
        return;
    }

    testsuite_libafpclient_close(&(*session)->session);
    free(*session);
    *session = NULL;
}
