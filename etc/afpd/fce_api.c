/*
 * Copyright (c) 2010 Mark Williams
 * Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>
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

/*!
 * @file
 * @brief File change event API for netatalk
 *
 * for every detected filesystem change a UDP packet is sent to an arbitrary list
 * of listeners. Each packet contains unix path of modified filesystem element,
 * event reason, and a consecutive event id (32 bit). Technically we are UDP client and are sending
 * out packets synchronuosly as they are created by the afp functions. This should not affect
 * performance measurably. The only delaying calls occur during initialization, if we have to
 * resolve non-IP hostnames to IP. All numeric data inside the packet is network byte order, so use
 * ntohs / ntohl to resolve length and event id. Ideally a listener receives every packet with
 * no gaps in event ids, starting with event id 1 and mode FCE_CONN_START followed by
 * data events from id 2 up to 0xFFFFFFFF, followed by 0 to 0xFFFFFFFF and so on.
 *
 * A gap or not starting with 1 mode FCE_CONN_START or receiving mode FCE_CONN_BROKEN means that
 * the listener has lost at least one filesystem event
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

#include "desktop.h"
#include "directory.h"
#include "file.h"
#include "fork.h"
#include "volume.h"

/* ONLY USED IN THIS FILE */
#include "fce_api_internal.h"

extern int afprun_bg(char *cmd);

/* We store our connection data here */
static struct udp_entry udp_socket_list[FCE_MAX_UDP_SOCKS];
static int udp_sockets = 0;
static bool udp_initialized = false;
static unsigned long fce_ev_enabled;

/* flags of additional info to send in events */
static uint8_t fce_ev_info;

#define MAXIOBUF 4096
static unsigned char iobuf[MAXIOBUF];
static const char **skip_files;
static const char **skip_directories;
static struct fce_close_event last_close_event;

static char *fce_event_names[] = {
    [FCE_FILE_MODIFY] = "FCE_FILE_MODIFY",
    [FCE_FILE_DELETE] = "FCE_FILE_DELETE",
    [FCE_DIR_DELETE] = "FCE_DIR_DELETE",
    [FCE_FILE_CREATE] = "FCE_FILE_CREATE",
    [FCE_DIR_CREATE] = "FCE_DIR_CREATE",
    [FCE_FILE_MOVE] = "FCE_FILE_MOVE",
    [FCE_DIR_MOVE] = "FCE_DIR_MOVE",
    [FCE_LOGIN] = "FCE_LOGIN",
    [FCE_LOGOUT] = "FCE_LOGOUT",
    [FCE_CONN_START] = "FCE_CONN_START",
    [FCE_CONN_BROKEN] = "FCE_CONN_BROKEN"
};

/*!
 * @brief Initialize network structs for any listeners
 * @note We don't give return code because all errors are handled internally (I hope..)
 */
void fce_init_udp(void)
{
    int rv;
    struct addrinfo hints, *servinfo, *p;

    if (udp_initialized == true) {
        return;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    for (int i = 0; i < udp_sockets; i++) {
        struct udp_entry *udp_entry = udp_socket_list + i;

        if ((rv = getaddrinfo(udp_entry->addr, udp_entry->port, &hints,
                              &servinfo)) != 0) {
            LOG(log_error, logtype_fce, "fce_init_udp: getaddrinfo(%s:%s): %s",
                udp_entry->addr, udp_entry->port, gai_strerror(rv));
            continue;
        }

        /* loop through all the results and make a socket */
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((udp_entry->sock = socket(p->ai_family, p->ai_socktype,
                                          p->ai_protocol)) == -1) {
                LOG(log_error, logtype_fce, "fce_init_udp: socket(%s:%s): %s",
                    udp_entry->addr, udp_entry->port, strerror(errno));
                continue;
            }

            break;
        }

        if (p == NULL) {
            LOG(log_error, logtype_fce, "fce_init_udp: no socket for %s:%s",
                udp_entry->addr, udp_entry->port);
            freeaddrinfo(servinfo);
            continue;
        }

        udp_entry->addrinfo = *p;
        memcpy(&udp_entry->sockaddr, p->ai_addr, p->ai_addrlen);
        freeaddrinfo(servinfo);
    }

    udp_initialized = true;
}

/*!
 * @brief Drop every FCE listener and its socket
 *
 * configinit() calls it before it reads 'fce listener', at startup and at
 * each reload, so the table holds only the listener afp.conf names.
 */
void fce_cleanup(void)
{
    for (int i = 0; i < udp_sockets; i++) {
        struct udp_entry *udp_entry = udp_socket_list + i;

        if (udp_entry->sock != -1) {
            close(udp_entry->sock);
        }

        free(udp_entry->addr);
        free(udp_entry->port);
    }

    udp_sockets = 0;
    udp_initialized = false;
}

/*!
 * @brief Build the UDP packet of one FCE event in a buffer
 *
 * The packet starts with the 8-byte magic and the protocol version. Version 2
 * adds an info flags byte, a filler byte, and 8 reserved bytes around the
 * event code. The 4-byte event ID follows, then the pid and the user name
 * when the flags ask for them. The path and the source path of a move end the
 * packet, each with a 2-byte length. All numbers go in network order.
 *
 * @param[in]  obj       session object with the FCE protocol version
 * @param[out] iobuf     buffer of MAXIOBUF bytes
 * @param[in]  event     event code
 * @param[in]  path      path of the event, capped at MAXPATHLEN - 1 bytes
 * @param[in]  oldpath   source path of a move, or NULL
 * @param[in]  pid       session pid
 * @param[in]  user      login name
 * @param[in]  event_id  event counter
 * @returns the packet length, or -1 when the packet does not fit
 */
static ssize_t build_fce_packet(const AFPObj *obj,
                                unsigned char *iobuf,
                                fce_ev_t event,
                                const char *path,
                                const char *oldpath,
                                pid_t pid,
                                const char *user,
                                uint32_t event_id)
{
    unsigned char *p = iobuf;
    size_t pathlen;
    ssize_t datalen = 0;
    size_t remaining = MAXIOBUF;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;
    uint8_t packet_info = fce_ev_info;

    /* FCE magic */
    if (remaining < 8) {
        return -1;
    }

    memcpy(p, FCE_PACKET_MAGIC, 8);
    p += 8;
    datalen += 8;
    remaining -= 8;

    /* version */
    if (remaining < 1) {
        return -1;
    }

    *p = (unsigned char)obj->options.fce_version;
    p += 1;
    datalen += 1;
    remaining -= 1;

    /* optional: options */
    if (obj->options.fce_version > 1) {
        if (remaining < 1) {
            return -1;
        }

        if (oldpath) {
            packet_info |= FCE_EV_INFO_SRCPATH;
        }

        *p = packet_info;
        p += 1;
        datalen += 1;
        remaining -= 1;
    }

    /* event */
    if (remaining < 1) {
        return -1;
    }

    *p = event;
    p += 1;
    datalen += 1;
    remaining -= 1;

    /* optional: padding */
    if (obj->options.fce_version > 1) {
        if (remaining < 1) {
            return -1;
        }

        *p = 0;
        p += 1;
        datalen += 1;
        remaining -= 1;
    }

    /* optional: reserved */
    if (obj->options.fce_version > 1) {
        if (remaining < 8) {
            return -1;
        }

        memset(p, 0, 8);
        p += 8;
        datalen += 8;
        remaining -= 8;
    }

    /* event ID */
    if (remaining < sizeof(uint32)) {
        return -1;
    }

    uint32 = htonl(event_id);
    memcpy(p, &uint32, sizeof(uint32));
    p += sizeof(uint32);
    datalen += sizeof(uint32);
    remaining -= sizeof(uint32);

    /* optional: pid */
    if (packet_info & FCE_EV_INFO_PID) {
        if (remaining < sizeof(uint64)) {
            return -1;
        }

        uint64 = pid;
        uint64 = hton64(uint64);
        memcpy(p, &uint64, sizeof(uint64));
        p += sizeof(uint64);
        datalen += sizeof(uint64);
        remaining -= sizeof(uint64);
    }

    /* optional: username */
    if (packet_info & FCE_EV_INFO_USER) {
        size_t userlen = strnlen(user, MAXPATHLEN);

        if (userlen >= MAXPATHLEN) {
            userlen = MAXPATHLEN - 1;
        }

        if (remaining < sizeof(uint16) + userlen) {
            return -1;
        }

        uint16 = htons((uint16_t)userlen);
        memcpy(p, &uint16, sizeof(uint16));
        p += sizeof(uint16);
        datalen += sizeof(uint16);
        memcpy(p, user, userlen);
        p += userlen;
        datalen += userlen;
        remaining -= sizeof(uint16) + userlen;
    }

    /* path */
    if ((pathlen = strlen(path)) >= MAXPATHLEN) {
        pathlen = MAXPATHLEN - 1;
    }

    if (remaining < sizeof(uint16) + pathlen) {
        return -1;
    }

    uint16 = pathlen;
    uint16 = htons(uint16);
    memcpy(p, &uint16, sizeof(uint16));
    p += sizeof(uint16);
    datalen += sizeof(uint16);
    memcpy(p, path, pathlen);
    p += pathlen;
    datalen += pathlen;
    remaining -= sizeof(uint16) + pathlen;

    /* optional: source path */
    if (oldpath && packet_info & FCE_EV_INFO_SRCPATH) {
        if ((pathlen = strlen(oldpath)) >= MAXPATHLEN) {
            pathlen = MAXPATHLEN - 1;
        }

        if (remaining < sizeof(uint16) + pathlen) {
            return -1;
        }

        uint16 = pathlen;
        uint16 = htons(uint16);
        memcpy(p, &uint16, sizeof(uint16));
        p += sizeof(uint16);
        datalen += sizeof(uint16);
        memcpy(p, oldpath, pathlen);
        p += pathlen;
        datalen += pathlen;
    }

    /* return the packet len */
    return datalen;
}

/*!
 * @brief Send one FCE event to every listener and to the notify script
 *
 * The first event records the user, opens the sockets, and sends
 * FCE_CONN_START. The notify script runs in the background with the event as
 * quoted options. The fce sendwait option delays every event by its value,
 * from 1 to 999 ms. A failed socket reopens after FCE_SOCKET_RETRY_DELAY_S
 * seconds and first sends FCE_CONN_BROKEN. A short send closes the socket
 * until then. The event ID rises by 1 per event, and 0xFFFFFFFF rolls over to
 * 0.
 *
 * @param[in] obj      session object
 * @param[in] event    event code
 * @param[in] path     path of the event
 * @param[in] oldpath  source path of a move, or NULL
 */
static void send_fce_event(const AFPObj *obj, int event, const char *path,
                           const char *oldpath)
{
    static bool first_event = true;
    /* the unique packet couter to detect packet/data loss.
     * Going from 0xFFFFFFFF to 0x0 is a valid increment */
    static uint32_t event_id = 0;
    static char user[MAXUSERLEN];
    ssize_t data_len;

    /* initialized ? */
    if (first_event == true) {
        first_event = false;
        const struct passwd *pwd = getpwuid(obj->uid);
        strlcpy(user, pwd ? pwd->pw_name : obj->username, sizeof(user));

        switch (obj->options.fce_version) {
        case 1:
            /* fce_ev_info unused */
            break;

        case 2:
            fce_ev_info = FCE_EV_INFO_PID | FCE_EV_INFO_USER;
            break;

        default:
            fce_ev_info = 0;
            LOG(log_error, logtype_fce, "Unsupported FCE protocol version %d",
                obj->options.fce_version);
            break;
        }

        fce_init_udp();
        /* Notify listeners the we start from the beginning */
        send_fce_event(obj, FCE_CONN_START, "", NULL);
    }

    /* run script */
    if (obj->options.fce_notify_script) {
        static bstring quote = NULL;
        static bstring quoterep = NULL;
        static bstring slash = NULL;
        static bstring slashrep = NULL;

        if (!quote) {
            quote = bfromcstr("'");
            quoterep = bfromcstr("'\\''");
            slash = bfromcstr("\\");
            slashrep = bfromcstr("\\\\");
        }

        bstring cmd = bformat("%s -v %d -e %s -i %" PRIu32 "",
                              obj->options.fce_notify_script,
                              obj->options.fce_version,
                              fce_event_names[event],
                              event_id);

        if (path[0]) {
            bstring bpath = bfromcstr(path);
            bfindreplace(bpath, slash, slashrep, 0);
            bfindreplace(bpath, quote, quoterep, 0);
            bformata(cmd, " -P '%s'", bdata(bpath));
            bdestroy(bpath);
        }

        if (fce_ev_info & FCE_EV_INFO_PID) {
            bformata(cmd, " -p %" PRIu64 "", (uint64_t)obj->pid);
        }

        if (fce_ev_info & FCE_EV_INFO_USER) {
            bstring buser = bfromcstr(user);
            bfindreplace(buser, slash, slashrep, 0);
            bfindreplace(buser, quote, quoterep, 0);
            bformata(cmd, " -u '%s'", bdata(buser));
            bdestroy(buser);
        }

        if (oldpath) {
            bstring boldpath = bfromcstr(oldpath);
            bfindreplace(boldpath, slash, slashrep, 0);
            bfindreplace(boldpath, quote, quoterep, 0);
            bformata(cmd, " -S '%s'", bdata(boldpath));
            bdestroy(boldpath);
        }

        (void)afprun_bg(bdata(cmd));
        bdestroy(cmd);
    }

    if (obj->options.fce_sendwait > 0 && obj->options.fce_sendwait < 1000) {
        struct timespec t;
        t.tv_sec  = 0;
        t.tv_nsec = 1000 * 1000 * obj->options.fce_sendwait;

        while (nanosleep(&t, NULL));
    }

    for (int i = 0; i < udp_sockets; i++) {
        int sent_data = 0;
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* we had a problem earlier ? */
        if (udp_entry->sock == -1) {
            time_t now = time(NULL);

            /* We still have to wait ?*/
            if (now < udp_entry->next_try_on_error) {
                continue;
            }

            /* Reopen socket */
            udp_entry->sock = socket(udp_entry->addrinfo.ai_family,
                                     udp_entry->addrinfo.ai_socktype,
                                     udp_entry->addrinfo.ai_protocol);

            if (udp_entry->sock == -1) {
                /* failed again, so go to rest again */
                LOG(log_error, logtype_fce,
                    "Cannot recreate socket for fce UDP connection: errno %d", errno);
                udp_entry->next_try_on_error = now + FCE_SOCKET_RETRY_DELAY_S;
                continue;
            }

            udp_entry->next_try_on_error = 0;
            /* Okay, we have a running socket again, send server that we had a problem on our side*/
            data_len = build_fce_packet(obj, iobuf, FCE_CONN_BROKEN, "", NULL, obj->pid,
                                        user, 0);
            sendto(udp_entry->sock,
                   iobuf,
                   data_len,
                   0,
                   (struct sockaddr *)&udp_entry->sockaddr,
                   udp_entry->addrinfo.ai_addrlen);
        }

        /* build our data packet */
        data_len = build_fce_packet(obj, iobuf, event, path, oldpath, obj->pid, user,
                                    event_id);

        if (data_len < 0) {
            LOG(log_error, logtype_fce,
                "send_fce_event: event for \"%s\" does not fit a packet", path);
            continue;
        }

        sent_data = sendto(udp_entry->sock,
                           iobuf,
                           data_len,
                           0,
                           (struct sockaddr *)&udp_entry->sockaddr,
                           udp_entry->addrinfo.ai_addrlen);

        /* Problems ? */
        if (sent_data != data_len) {
            /* Argh, socket broke, we close and retry later */
            LOG(log_error, logtype_fce,
                "send_fce_event: error sending packet to %s:%s, transferred %d of %d: %s",
                udp_entry->addr, udp_entry->port, sent_data, data_len, strerror(errno));
            close(udp_entry->sock);
            udp_entry->sock = -1;
            udp_entry->next_try_on_error = time(NULL)
                                           + FCE_SOCKET_RETRY_DELAY_S;
        }
    }

    event_id++;
}

/*!
 * @brief Add an FCE listener to the table
 *
 * @param[in] target_ip    host name or address
 * @param[in] target_port  port, or NULL for FCE_DEFAULT_PORT_STRING
 * @returns AFP_OK, or AFPERR_PARAM when the table holds FCE_MAX_UDP_SOCKS
 *          listeners
 */
static int add_udp_socket(const char *target_ip, const char *target_port)
{
    if (target_port == NULL) {
        target_port = FCE_DEFAULT_PORT_STRING;
    }

    if (udp_sockets >= FCE_MAX_UDP_SOCKS) {
        LOG(log_error, logtype_fce,
            "Too many file change api UDP connections (max %d allowed)", FCE_MAX_UDP_SOCKS);
        return AFPERR_PARAM;
    }

    udp_socket_list[udp_sockets].addr = strdup(target_ip);
    udp_socket_list[udp_sockets].port = strdup(target_port);
    udp_socket_list[udp_sockets].sock = -1;
    memset(&udp_socket_list[udp_sockets].addrinfo, 0, sizeof(struct addrinfo));
    memset(&udp_socket_list[udp_sockets].sockaddr, 0,
           sizeof(struct sockaddr_storage));
    udp_socket_list[udp_sockets].next_try_on_error = 0;
    udp_sockets++;
    return AFP_OK;
}

/*!
 * @brief Hold back a file modify event for the fce fmodwait delay
 *
 * A held event for another path goes out first. A repeat for the same path
 * only restarts the delay.
 *
 * @param[in] obj   session object
 * @param[in] path  path of the modified file
 */
static void save_close_event(const AFPObj *obj, const char *path)
{
    time_t now = time(NULL);

    /* Check if it's a close for the same event as the last one */
    if (last_close_event.time   /* is there any saved event ? */
            && (strcmp(path, last_close_event.path) != 0)) {
        /* no, so send the saved event out now */
        send_fce_event(obj, FCE_FILE_MODIFY, last_close_event.path, NULL);
    }

    LOG(log_debug, logtype_fce, "save_close_event: %s", path);
    last_close_event.time = now;
    strncpy(last_close_event.path, path, MAXPATHLEN);
}

/*!
 * @brief Resolve a path for the ignore-directory compare
 *
 * A path resolves while it exists. A path that is gone, or not there yet,
 * resolves through its parent with its own name appended, so a deleted file
 * compares under its directory's resolved name. A path that resolves neither
 * way compares as it is.
 *
 * @param[in]  path  absolute path
 * @param[out] buf   MAXPATHLEN + 1 bytes for the resolved path
 * @returns buf, or path
 */
static const char *fce_resolved(const char *path, char *buf)
{
    const char *base = strrchr(path, '/');
    char parent[MAXPATHLEN + 1];
    size_t plen;

    if (realpath(path, buf)) {
        return buf;
    }

    /* "" and a bare name have no parent; "/x" keeps the root's */
    plen = base == NULL ? 0 : base == path ? 1 : (size_t)(base - path);

    if (plen == 0 || plen > MAXPATHLEN) {
        return path;
    }

    memcpy(parent, path, plen);
    parent[plen] = '\0';

    if (realpath(parent, buf) == NULL) {
        return path;
    }

    /* the root already ends in the slash */
    return strlcat(buf, buf[1] ? base : base + 1, MAXPATHLEN + 1) > MAXPATHLEN
           ? path : buf;
}

/*!
 * @brief Split a comma-separated ignore option into a NULL-terminated array
 *
 * A directory is stored resolved, through its parent when it does not exist
 * yet, and without trailing slashes unless it is "/". A failed array
 * allocation, or an option without entries, leaves it NULL, which ignores
 * nothing; a failed entry copy drops that entry.
 *
 * @param[in]  ignores       fce ignore names or fce ignore directories option
 * @param[out] dest_array    receives the allocated array, or NULL
 * @param[in]  is_directory  true for directory paths
 */
static void fce_init_ign_paths(const char *ignores,
                               const char ***dest_array,
                               bool is_directory)
{
    char *names = strdup(ignores);
    char *saveptr = NULL;
    /* entries: one more than the commas, at most */
    int count = 1;
    int i = 0;

    for (const char *p = ignores; (p = strchr(p, ',')) != NULL; p++) {
        count++;
    }

    *dest_array = names ? calloc(count + 1, sizeof(char *)) : NULL;

    if (*dest_array == NULL) {
        free(names);
        return;
    }

    for (char *p = strtok_r(names, ",", &saveptr); p;
            p = strtok_r(NULL, ",", &saveptr)) {
        char resolved[MAXPATHLEN + 1];

        /* "dir/" names dir; "/" stays */
        if (is_directory) {
            (void)stripped_slashes_basename(p);
        }

        const char *entry = strdup(is_directory ? fce_resolved(p, resolved)
                                   : p);

        if (entry) {
            (*dest_array)[i++] = entry;
        }
    }

    free(names);

    /* fce_register() resolves each event's path while a list exists */
    if (i == 0) {
        free(*dest_array);
        *dest_array = NULL;
    }
}


/*!
 * @brief Send a file change event unless the options filter it out
 *
 * An event passes when the fce events option lists it and a listener or a
 * notify script exists. A base name in fce ignore names drops out, and so
 * does a path at or below an fce ignore directories entry, compared
 * resolved, through its parent once it is gone. Events that fce coalesce
 * merges drop out too. With fce fmodwait set, an FCE_FILE_MODIFY event waits
 * fce fmodwait seconds, and repeats for the same file merge.
 *
 * @param[in] obj      session object
 * @param[in] event    event code
 * @param[in] path     path of the changed object
 * @param[in] oldpath  source path of a move, or NULL
 * @returns AFP_OK
 */
int fce_register(const AFPObj *obj, fce_ev_t event, const char *path,
                 const char *oldpath)
{
    static bool first_event = true;
    const char *bname;

    if (!(fce_ev_enabled & (1 << event))) {
        return AFP_OK;
    }

    AFP_ASSERT(event >= FCE_FIRST_EVENT && event <= FCE_LAST_EVENT);
    AFP_ASSERT(path);
    LOG(log_debug, logtype_fce, "register_fce(path: %s, event: %s)",
        path, fce_event_names[event]);

    if ((udp_sockets == 0) && (obj->options.fce_notify_script == NULL)) {
        /* No listeners configured */
        return AFP_OK;
    }

    /* do some initialization on the fly the first time */
    if (first_event) {
        fce_initialize_history();

        if (obj->options.fce_ign_names != NULL) {
            fce_init_ign_paths(obj->options.fce_ign_names, &skip_files, false);
        }

        if (obj->options.fce_ign_directories != NULL) {
            fce_init_ign_paths(obj->options.fce_ign_directories,
                               &skip_directories, true);
        }

        first_event = false;
    }

    /* handle files or dirs which should not cause events (.DS_Store etc. ) */
    bname = basename_safe(path);

    if (bname && skip_files != NULL) {
        for (int i = 0; skip_files[i] != NULL; i++) {
            if (strcmp(bname, skip_files[i]) == 0) {
                LOG(log_debug, logtype_fce, "Skip file change event for file <%s>",
                    skip_files[i]);
                return AFP_OK;
            }
        }
    }

    if (skip_directories != NULL) {
        char resolved[MAXPATHLEN + 1];
        const char *dirname = fce_resolved(path, resolved);

        for (int i = 0; skip_directories[i] != NULL; i++) {
            if (path_at_or_below(dirname, skip_directories[i])) {
                LOG(log_debug, logtype_fce, "Skip file change event for directory <%s>",
                    skip_directories[i]);
                return AFP_OK;
            }
        }
    }

    /* Can we ignore this event based on type or history? */
    if (fce_handle_coalescation(event, path)) {
        LOG(log_debug9, logtype_fce, "Coalesced fc event <%d> for <%s>", event, path);
        return AFP_OK;
    }

    switch (event) {
    case FCE_FILE_MODIFY:
        if (obj->options.fce_fmodwait != 0) {
            save_close_event(obj, path);
        } else {
            send_fce_event(obj, event, path, oldpath);
        }

        break;

    default:
        send_fce_event(obj, event, path, oldpath);
        break;
    }

    return AFP_OK;
}

/*!
 * @brief Send the held file modify event once the fce fmodwait delay passed
 *
 * @param[in] obj  session object
 */
static void check_saved_close_events(const AFPObj *obj)
{
    /* check if configured holdclose time has passed */
    if (last_close_event.time
            && last_close_event.time + obj->options.fce_fmodwait < time(NULL)) {
        LOG(log_debug, logtype_fce, "check_saved_close_events: sending event: %s",
            last_close_event.path);
        /* yes, send event */
        send_fce_event(obj, FCE_FILE_MODIFY, &last_close_event.path[0], NULL);
        last_close_event.path[0] = 0;
        last_close_event.time = 0;
    }
}

/******************** External calls start here **************************/

/*!
 * @brief Send a held file modify event whose delay passed
 *
 * The session loop calls it after each request. Without a listener or a
 * notify script it does nothing.
 *
 * @param[in] obj  session object
 */
void fce_pending_events(const AFPObj *obj)
{
    if (!udp_sockets && obj->options.fce_notify_script == NULL) {
        return;
    }

    check_saved_close_events(obj);
}

/*!
 * @brief Add an FCE listener from the fce listener option
 *
 * The target is a host with an optional port after the first colon, which
 * IPv6 addresses also contain. The port defaults to FCE_DEFAULT_PORT_STRING.
 *
 * @param[in] target  host or host:port, at most 255 bytes
 * @returns AFP_OK, or AFPERR_PARAM when the table holds FCE_MAX_UDP_SOCKS
 *          listeners
 */
int fce_add_udp_socket(const char *target)
{
    const char *port = FCE_DEFAULT_PORT_STRING;
    char target_ip[256] = {""};
    strncpy(target_ip, target, sizeof(target_ip) - 1);
    char *port_delim = strchr(target_ip, ':');

    if (port_delim) {
        *port_delim = 0;
        port = port_delim + 1;
    }

    return add_udp_socket(target_ip, port);
}

/*!
 * @brief Set the FCE event types to send from the fce events option
 *
 * The option lists fmod, fdel, ddel, fcre, dcre, fmov, dmov, login, and
 * logout, separated by commas or spaces. The list replaces the default set,
 * and unknown names drop out.
 *
 * @param[in] events  option value
 * @returns AFP_OK, AFPERR_PARAM for NULL, or AFPERR_MISC when the copy fails
 */
int fce_set_events(const char *events)
{
    char *e;
    char *p;

    if (events == NULL) {
        return AFPERR_PARAM;
    }

    if ((e = strdup(events)) == NULL) {
        return AFPERR_MISC;
    }

    fce_ev_enabled = 0;

    for (p = strtok(e, ", "); p; p = strtok(NULL, ", ")) {
        if (strcmp(p, "fmod") == 0) {
            fce_ev_enabled |= (1 << FCE_FILE_MODIFY);
        } else if (strcmp(p, "fdel") == 0) {
            fce_ev_enabled |= (1 << FCE_FILE_DELETE);
        } else if (strcmp(p, "ddel") == 0) {
            fce_ev_enabled |= (1 << FCE_DIR_DELETE);
        } else if (strcmp(p, "fcre") == 0) {
            fce_ev_enabled |= (1 << FCE_FILE_CREATE);
        } else if (strcmp(p, "dcre") == 0) {
            fce_ev_enabled |= (1 << FCE_DIR_CREATE);
        } else if (strcmp(p, "fmov") == 0) {
            fce_ev_enabled |= (1 << FCE_FILE_MOVE);
        } else if (strcmp(p, "dmov") == 0) {
            fce_ev_enabled |= (1 << FCE_DIR_MOVE);
        } else if (strcmp(p, "login") == 0) {
            fce_ev_enabled |= (1 << FCE_LOGIN);
        } else if (strcmp(p, "logout") == 0) {
            fce_ev_enabled |= (1 << FCE_LOGOUT);
        }
    }

    free(e);
    return AFP_OK;
}
