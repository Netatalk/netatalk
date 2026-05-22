/*
 * Copyright (c) 2013 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2024-2026 Daniel Markstedt <daniel@mindani.net>
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
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/server_child.h>

#include "afpstats.h"

static server_child_t *childs;

int afpstats_init(server_child_t *childs_in, const char *sock_path,
                  bool set_group, gid_t gid)
{
    int fd;
    struct sockaddr_un addr;

    if (!childs_in) {
        LOG(log_error, logtype_afpd, "afpstats_init: NULL server_child_t pointer");
        return -1;
    }

    if (!sock_path) {
        LOG(log_error, logtype_afpd, "afpstats_init: NULL socket path");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    if (strlcpy(addr.sun_path, sock_path, sizeof(addr.sun_path))
            >= sizeof(addr.sun_path)) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: socket path too long: %s", sock_path);
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: socket: %s", strerror(errno));
        return -1;
    }

    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);

    /*
     * Netatalk-managed startup serializes afpd through the top-level
     * netatalk PID-file check. Treat a pre-existing afpstats socket here
     * as stale cleanup from a previous run.
     */
    if (unlink(sock_path) != 0 && errno != ENOENT) {
        LOG(log_warning, logtype_afpd,
            "afpstats_init: unlink stale %s: %s", sock_path, strerror(errno));
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: bind %s: %s", sock_path, strerror(errno));
        close(fd);
        return -1;
    }

    if (set_group && chown(sock_path, (uid_t) -1, gid) != 0) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: chown %s: %s", sock_path, strerror(errno));
        close(fd);
        unlink(sock_path);
        return -1;
    }

    if (chmod(sock_path, 0660) != 0) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: chmod %s: %s", sock_path, strerror(errno));
        close(fd);
        unlink(sock_path);
        return -1;
    }

    if (listen(fd, 8) != 0) {
        LOG(log_error, logtype_afpd,
            "afpstats_init: listen: %s", strerror(errno));
        close(fd);
        unlink(sock_path);
        return -1;
    }

    childs = childs_in;
    LOG(log_info, logtype_afpd, "afpstats listening on %s", sock_path);
    return fd;
}

static const char *state_name(int16_t state)
{
    switch (state) {
    case ASP_RUNNING:
    case DSI_RUNNING:
        return "active";

    case DSI_SLEEPING:
    case DSI_EXTSLEEP:
        return "sleeping";

    case DSI_DISCONNECTED:
        return "disconnected";

    default:
        return "unknown";
    }
}

static const char *protocol_name(int16_t state)
{
    /*
     * afp_child_t does not carry the listener protocol, so infer it from
     * state. ASP sessions can be visible after IPC_LOGINDONE but before
     * IPC_STATE sets ASP_RUNNING; report "unknown" in that window rather than
     * defaulting to TCP/IP.
     */
    if (state & ASP_RUNNING) {
        return "AppleTalk";
    }

    if (state & (DSI_RUNNING | DSI_SLEEPING | DSI_EXTSLEEP | DSI_DISCONNECTED)) {
        return "TCP/IP";
    }

    return "unknown";
}

static const char *user_name(uid_t uid, char *buf, size_t buflen)
{
    const struct passwd *pw;
    pw = getpwuid(uid);

    if (pw && pw->pw_name) {
        return pw->pw_name;
    }

    snprintf(buf, buflen, "uid=%u", (unsigned)uid);
    return buf;
}

void afpstats_handle_accept(int listen_fd)
{
    int client_fd;
    struct timeval tv;
    afp_child_t *child;

    do {
        client_fd = accept(listen_fd, NULL, NULL);
    } while (client_fd < 0 && errno == EINTR);

    if (client_fd < 0) {
        LOG(log_error, logtype_afpd,
            "afpstats: accept: %s", strerror(errno));
        return;
    }

    (void)fcntl(client_fd, F_SETFD, FD_CLOEXEC);
    /* Cap how long a stuck client can stall the parent's main event loop.
     * The payload is tiny — anything more than a second is a misbehaving peer. */
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    (void)setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    for (int j = 0; j < CHILD_HASHSIZE; j++) {
        child = childs->servch_table[j];

        while (child) {
            if (child->afpch_valid) {
                char namebuf[32];
                const char *uname;
                char timebuf[64];
                time_t login_time = child->afpch_logintime;
                uname = user_name(child->afpch_uid, namebuf, sizeof(namebuf));
                strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S",
                         localtime(&login_time));

                /* afpd ignores SIGPIPE at startup, so closed peers surface
                 * here as write errors instead of terminating the parent. */
                if (dprintf(client_fd,
                            "name: %s, pid: %d, logintime: %s, state: %s, protocol: %s, volumes: %s, hostname: %s\n",
                            uname, child->afpch_pid, timebuf,
                            state_name(child->afpch_state),
                            protocol_name(child->afpch_state),
                            child->afpch_volumes ? child->afpch_volumes : "-",
                            child->afpch_hostname ? child->afpch_hostname : "-") < 0) {
                    /* Client went away mid-write; abandon this iteration. */
                    close(client_fd);
                    return;
                }
            }

            child = child->afpch_next;
        }
    }

    close(client_fd);
}
