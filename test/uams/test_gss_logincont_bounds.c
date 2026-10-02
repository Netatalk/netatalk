/*
 * Regression tests for GSS LoginCont username and reply bounds.
 *
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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atalk/afp.h>
#include <atalk/dsi.h>
#include <atalk/uam.h>

#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

/* Include the implementation so the test can exercise the static
 * gss_logincont() handler and reply builder directly. GSS itself is never
 * reached: the malformed username must be rejected while parsing the request. */
#include "../../etc/uams/uams_gss.c"

int uam_register(const int type _U_, const char *path _U_, const char *name _U_,
                 ...)
{
    return 0;
}

void uam_unregister(const int type _U_, const char *name _U_)
{
    /* Test stub: no UAM registration state is maintained. */
}

struct passwd *uam_getname(void *private _U_, char *name _U_, const int len _U_)
{
    return NULL;
}

int uam_checkuser(void *private _U_, const struct passwd *pwd _U_)
{
    return -1;
}

int uam_afpserver_option(void *private _U_, const int what,
                         void *option, size_t *len)
{
    static char username[256];
    static unsigned char session_key[SESSIONKEY_LEN];
    static struct session_info session = {
        .sessionkey = session_key,
        .sessionkey_len = sizeof(session_key),
    };

    switch (what) {
    case UAM_OPTION_USERNAME:
        *(char **)option = username;

        if (len != NULL) {
            *len = sizeof(username) - 1;
        }

        return 0;

    case UAM_OPTION_SESSIONINFO:
        *(struct session_info **)option = &session;
        return 0;

    default:
        return -1;
    }
}

static int test_reply_bounds(void)
{
    static const struct {
        size_t capacity;
        size_t authenticator_len;
        int succeeds;
    } cases[] = {
        {0, 0, 0},
        {1, 0, 0},
        {2, 0, 1},
        {2, 1, 0},
        {3, 1, 1},
        {DSI_DATASIZ, DSI_DATASIZ - 2, 1},
        {DSI_DATASIZ, DSI_DATASIZ - 1, 0},
        {UINT16_MAX + 2UL, UINT16_MAX, 1},
        {UINT16_MAX + 3UL, UINT16_MAX + 1UL, 0},
        {SIZE_MAX, SIZE_MAX, 0},
    };
    static unsigned char token[UINT16_MAX];
    static unsigned char reply[UINT16_MAX + 4UL];
    memset(token, 0x5a, sizeof(token));

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        gss_buffer_desc authenticator = {
            .length = cases[i].authenticator_len,
            .value = cases[i].authenticator_len == 0 ? NULL : token,
        };
        size_t reply_len = cases[i].capacity;
        size_t written = 0;
        uint16_t auth_len;
        int result;
        memset(reply, 0xa5, sizeof(reply));
        result = build_gss_reply((char *)reply, cases[i].capacity,
                                 &reply_len, &authenticator);

        if (cases[i].succeeds) {
            written = sizeof(auth_len) + cases[i].authenticator_len;
            memcpy(&auth_len, reply, sizeof(auth_len));

            if (result != 0 || reply_len != written ||
                    ntohs(auth_len) != cases[i].authenticator_len ||
                    memcmp(reply + sizeof(auth_len), token,
                           cases[i].authenticator_len) != 0) {
                fprintf(stderr, "reply boundary case %zu encoded incorrectly\n", i);
                return EXIT_FAILURE;
            }
        } else if (result == 0 || reply_len != 0) {
            fprintf(stderr, "reply boundary case %zu was not rejected\n", i);
            return EXIT_FAILURE;
        }

        /* Rejected replies must leave the whole buffer untouched; successful
         * replies must leave every byte after the encoded response untouched. */
        for (size_t j = written; j < sizeof(reply); j++) {
            if (reply[j] != 0xa5) {
                fprintf(stderr, "reply boundary case %zu overwrote byte %zu\n", i, j);
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

static int test_username_bounds(void)
{
    static const struct {
        const char *name;
        unsigned char request[6];
        size_t request_len;
    } cases[] = {
        /* The username length includes the NUL; odd lengths require padding. */
        {"missing username padding", {0, 0, 1, 'A', 'B', 0}, 6},
        {"missing ticket length", {0, 0, 1, 'A', 0}, 5},
        {"one-byte ticket length", {0, 0, 1, 'A', 0, 0}, 6},
    };
    const long page_size = sysconf(_SC_PAGESIZE);
    const size_t command_len = DSI_SERVQUANT_DEF;
    const size_t mapping_len = command_len + (size_t)page_size;
    struct passwd *authenticated_user = NULL;
    char reply[DSI_DATASIZ];
    size_t reply_len = sizeof(reply);
    unsigned char *commands;
    uint16_t login_id;
    int result;

    if (page_size <= 0 || command_len % (size_t)page_size != 0) {
        fprintf(stderr, "server quantum is not page aligned\n");
        return EXIT_FAILURE;
    }

    commands = mmap(NULL, mapping_len, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (commands == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    if (mprotect(commands + command_len, (size_t)page_size, PROT_NONE) != 0) {
        perror("mprotect");
        munmap(commands, mapping_len);
        return EXIT_FAILURE;
    }

    /* Reproduce a maximum-sized DSI command allocation. afp_logincont()
     * consumes the AFP function byte and pad byte before calling the UAM.
     * The username begins at commands[5] and has no NUL before the allocation
     * ends, so a parser that tests *ibuf before ibuflen touches the guard page. */
    memset(commands, 'A', command_len);
    commands[0] = AFP_LOGINCONT;
    commands[1] = 0;
    commands[2] = 0; /* GSS request's observed extra byte */
    login_id = htons(1);
    memcpy(commands + 3, &login_id, sizeof(login_id));
    result = gss_logincont(NULL, &authenticated_user,
                           (char *)commands + 2, command_len - 2,
                           reply, &reply_len);
    munmap(commands, mapping_len);

    if (result != AFPERR_PARAM || authenticated_user != NULL || reply_len != 0) {
        fprintf(stderr,
                "unterminated username returned %d with user %p and reply length %zu\n",
                result, (void *)authenticated_user, reply_len);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        unsigned char request[sizeof(cases[i].request)];
        memcpy(request, cases[i].request, sizeof(request));
        reply_len = sizeof(reply);
        result = gss_logincont(NULL, &authenticated_user,
                               (char *)request, cases[i].request_len,
                               reply, &reply_len);

        if (result != AFPERR_PARAM || authenticated_user != NULL || reply_len != 0) {
            fprintf(stderr,
                    "%s returned %d with user %p and reply length %zu\n",
                    cases[i].name, result, (void *)authenticated_user, reply_len);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main(void)
{
    if (test_username_bounds() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return test_reply_bounds();
}
