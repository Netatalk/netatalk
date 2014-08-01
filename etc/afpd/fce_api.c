/*
 * Copyright (c) 2010 Mark Williams
 * Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>
 *
 * File change event API for netatalk
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
 * 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/unix.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>

#include "fork.h"
#include "file.h"
#include "directory.h"
#include "desktop.h"
#include "volume.h"

// ONLY USED IN THIS FILE
#include "fce_api_internal.h"

extern int afprun_bg(int root, char *cmd);

/* We store our connection data here */
static struct udp_entry udp_socket_list[FCE_MAX_UDP_SOCKS];
static int udp_sockets = 0;
static bool udp_initialized = false;
static unsigned long fce_ev_enabled =
    (1 << FCE_FILE_MODIFY) |
    (1 << FCE_FILE_DELETE) |
    (1 << FCE_DIR_DELETE) |
    (1 << FCE_FILE_CREATE) |
    (1 << FCE_DIR_CREATE) |
    (1 << FCE_FILE_MOVE) |
    (1 << FCE_DIR_MOVE) |
    (1 << FCE_LOGIN) |
    (1 << FCE_LOGOUT);

static uint8_t fce_ev_info;    /* flags of additional info to send in events */

#define MAXIOBUF 4096
static unsigned char iobuf[MAXIOBUF];
static const char **skip_files;
static struct fce_close_event last_close_event;

/*
 * This only initializes consecutive events beginning at 1, high
 * numbered events must be initialized in the code
 */
static char *fce_event_names[FCE_LAST_EVENT + 1] = {
    "",
    "FCE_FILE_MODIFY",
    "FCE_FILE_DELETE",
    "FCE_DIR_DELETE",
    "FCE_FILE_CREATE",
    "FCE_DIR_CREATE",
    "FCE_FILE_MOVE",
    "FCE_DIR_MOVE",
    "FCE_LOGIN",
    "FCE_LOGOUT"
};

/*
 *
 * Initialize network structs for any listeners
 * We dont give return code because all errors are handled internally (I hope..)
 *
 * */
void fce_init_udp()
{
    int rv;
    struct addrinfo hints, *servinfo, *p;

    if (udp_initialized == true)
        return;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    for (int i = 0; i < udp_sockets; i++) {
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* Close any pending sockets */
        if (udp_entry->sock != -1)
            close(udp_entry->sock);

        if ((rv = getaddrinfo(udp_entry->addr, udp_entry->port, &hints, &servinfo)) != 0) {
            LOG(log_error, logtype_fce, "fce_init_udp: getaddrinfo(%s:%s): %s",
                udp_entry->addr, udp_entry->port, gai_strerror(rv));
            continue;
        }

        /* loop through all the results and make a socket */
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((udp_entry->sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                LOG(log_error, logtype_fce, "fce_init_udp: socket(%s:%s): %s",
                    udp_entry->addr, udp_entry->port, strerror(errno));
                continue;
            }
            break;
        }

        if (p == NULL) {
            LOG(log_error, logtype_fce, "fce_init_udp: no socket for %s:%s",
                udp_entry->addr, udp_entry->port);
        }
        udp_entry->addrinfo = *p;
        memcpy(&udp_entry->addrinfo, p, sizeof(struct addrinfo));
        memcpy(&udp_entry->sockaddr, p->ai_addr, sizeof(struct sockaddr_storage));
        freeaddrinfo(servinfo);
    }

    udp_initialized = true;
}

void fce_cleanup()
{
    if (udp_initialized == false )
        return;

    for (int i = 0; i < udp_sockets; i++)
    {
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* Close any pending sockets */
        if (udp_entry->sock != -1)
        {
            close( udp_entry->sock );
            udp_entry->sock = -1;
        }
    }
    udp_initialized = false;
}

/*
 * Construct a UDP packet for our listeners and return packet size
 * */
static ssize_t build_fce_packet(const AFPObj *obj,
                                char *iobuf,
                                fce_ev_t event,
                                const char *path,
                                const char *oldpath,
                                pid_t pid,
                                const char *user,
                                uint32_t event_id)
{
    char *p = iobuf;
    size_t pathlen;
    ssize_t datalen = 0;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;
    uint8_t packet_info = fce_ev_info;

    /* FCE magic */
    memcpy(p, FCE_PACKET_MAGIC, 8);
    p += 8;
    datalen += 8;

    /* version */
    *p = FCE_PACKET_VERSION;
    p += 1;
    datalen += 1;

    /* optional: options */
    if (FCE_PACKET_VERSION > 1) {
        if (oldpath)
            packet_info |= FCE_EV_INFO_SRCPATH;
        *p = packet_info;
        p += 1;
        datalen += 1;
    }

    /* event */
    *p = event;
    p += 1;
    datalen += 1;

    /* optional: padding */
    if (FCE_PACKET_VERSION > 1) {
        p += 1;
        datalen += 1;
    }

    /* optional: reserved */
    if (FCE_PACKET_VERSION > 1) {
        p += 8;
        datalen += 8;
    }

    /* event ID */
    uint32 = htonl(event_id);
    memcpy(p, &uint32, sizeof(uint32));
    p += sizeof(uint32);
    datalen += sizeof(uint32);

    /* optional: pid */
    if (packet_info & FCE_EV_INFO_PID) {
        uint64 = pid;
        uint64 = hton64(uint64);
        memcpy(p, &uint64, sizeof(uint64));
        p += sizeof(uint64);
        datalen += sizeof(uint64);
    }

    /* optional: username */
    if (packet_info & FCE_EV_INFO_USER) {
        uint16 = strlen(user);
        uint16 = htons(uint16);
        memcpy(p, &uint16, sizeof(uint16));
        p += sizeof(uint16);
        datalen += sizeof(uint16);
        memcpy(p, user, strlen(user));
        p += strlen(user);
        datalen += strlen(user);
    }

    /* path */
    if ((pathlen = strlen(path)) >= MAXPATHLEN)
        pathlen = MAXPATHLEN - 1;
    uint16 = pathlen;
    uint16 = htons(uint16);
    memcpy(p, &uint16, sizeof(uint16));
    p += sizeof(uint16);
    datalen += sizeof(uint16);
    memcpy(p, path, pathlen);
    p += pathlen;
    datalen += pathlen;

    /* optional: source path */
    if (packet_info & FCE_EV_INFO_SRCPATH) {
        if ((pathlen = strlen(oldpath)) >= MAXPATHLEN)
            pathlen = MAXPATHLEN - 1;
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

/*
 * Send the fce information to all (connected) listeners
 * We dont give return code because all errors are handled internally (I hope..)
 * */
static void send_fce_event(const AFPObj *obj, int event, const char *path, const char *oldpath)
{    
    static bool first_event = true;
    static uint32_t event_id = 0; /* the unique packet couter to detect packet/data loss. Going from 0xFFFFFFFF to 0x0 is a valid increment */
    static char *user;
    time_t now = time(NULL);
    ssize_t data_len;

    /* initialized ? */
    if (first_event == true) {
        first_event = false;

        fce_event_names[FCE_CONN_START] = "FCE_CONN_START";
        fce_event_names[FCE_CONN_BROKEN] = "FCE_CONN_BROKEN";

        struct passwd *pwd = getpwuid(obj->uid);
        user = strdup(pwd->pw_name);

        switch (obj->fce_version) {
        case 1:
            /* fce_ev_info unused */
            break;
        case 2:
            fce_ev_info = FCE_EV_INFO_PID | FCE_EV_INFO_USER;
            break;
        default:
            fce_ev_info = 0;
            LOG(log_error, logtype_fce, "Unsupported FCE protocol version %d", obj->fce_version);
            break;
        }

        fce_init_udp();
        /* Notify listeners the we start from the beginning */
        send_fce_event(obj, FCE_CONN_START, "", NULL);
    }

    /* run script */
    if (obj->fce_notify_script) {
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
                              obj->fce_notify_script,
                              FCE_PACKET_VERSION,
                              fce_event_names[event],
                              event_id);

        if (path[0]) {
            bstring bpath = bfromcstr(path);
            bfindreplace(bpath, slash, slashrep, 0);
            bfindreplace(bpath, quote, quoterep, 0);
            bformata(cmd, " -P '%s'", bdata(bpath));
            bdestroy(bpath);
        }
        if (fce_ev_info | FCE_EV_INFO_PID)
            bformata(cmd, " -p %" PRIu64 "", (uint64_t)getpid());
        if (fce_ev_info | FCE_EV_INFO_USER)
            bformata(cmd, " -u %s", user);
        if (oldpath) {
            bstring boldpath = bfromcstr(oldpath);
            bfindreplace(boldpath, slash, slashrep, 0);
            bfindreplace(boldpath, quote, quoterep, 0);
            bformata(cmd, " -S '%s'", bdata(boldpath));
            bdestroy(boldpath);
        }
        (void)afprun_bg(1, bdata(cmd));
        bdestroy(cmd);
    }

    for (int i = 0; i < udp_sockets; i++) {
        int sent_data = 0;
        struct udp_entry *udp_entry = udp_socket_list + i;

        /* we had a problem earlier ? */
        if (udp_entry->sock == -1) {
            /* We still have to wait ?*/
            if (now < udp_entry->next_try_on_error)
                continue;

            /* Reopen socket */
            udp_entry->sock = socket(udp_entry->addrinfo.ai_family,
                                     udp_entry->addrinfo.ai_socktype,
                                     udp_entry->addrinfo.ai_protocol);
            
            if (udp_entry->sock == -1) {
                /* failed again, so go to rest again */
                LOG(log_error, logtype_fce, "Cannot recreate socket for fce UDP connection: errno %d", errno  );

                udp_entry->next_try_on_error = now + FCE_SOCKET_RETRY_DELAY_S;
                continue;
            }

            udp_entry->next_try_on_error = 0;

            /* Okay, we have a running socket again, send server that we had a problem on our side*/
            data_len = build_fce_packet(obj, iobuf, FCE_CONN_BROKEN, "", NULL, getpid(), user, 0);

            sendto(udp_entry->sock,
                   iobuf,
                   data_len,
                   0,
                   (struct sockaddr *)&udp_entry->sockaddr,
                   udp_entry->addrinfo.ai_addrlen);
        }

        /* build our data packet */
        data_len = build_fce_packet(obj, iobuf, event, path, oldpath, getpid(), user, event_id);

        sent_data = sendto(udp_entry->sock,
                           iobuf,
                           data_len,
                           0,
                           (struct sockaddr *)&udp_entry->sockaddr,
                           udp_entry->addrinfo.ai_addrlen);

        /* Problems ? */
        if (sent_data != data_len) {
            /* Argh, socket broke, we close and retry later */
            LOG(log_error, logtype_fce, "send_fce_event: error sending packet to %s:%s, transfered %d of %d: %s",
                udp_entry->addr, udp_entry->port, sent_data, data_len, strerror(errno));

            close( udp_entry->sock );
            udp_entry->sock = -1;
            udp_entry->next_try_on_error = now + FCE_SOCKET_RETRY_DELAY_S;
        }
    }

    event_id++;
}

static int add_udp_socket(const char *target_ip, const char *target_port )
{
    if (target_port == NULL)
        target_port = FCE_DEFAULT_PORT_STRING;

    if (udp_sockets >= FCE_MAX_UDP_SOCKS) {
        LOG(log_error, logtype_fce, "Too many file change api UDP connections (max %d allowed)", FCE_MAX_UDP_SOCKS );
        return AFPERR_PARAM;
    }

    udp_socket_list[udp_sockets].addr = strdup(target_ip);
    udp_socket_list[udp_sockets].port = strdup(target_port);
    udp_socket_list[udp_sockets].sock = -1;
    memset(&udp_socket_list[udp_sockets].addrinfo, 0, sizeof(struct addrinfo));
    memset(&udp_socket_list[udp_sockets].sockaddr, 0, sizeof(struct sockaddr_storage));
    udp_socket_list[udp_sockets].next_try_on_error = 0;

    udp_sockets++;

    return AFP_OK;
}

static void save_close_event(const AFPObj *obj, const char *path)
{
    time_t now = time(NULL);

    /* Check if it's a close for the same event as the last one */
    if (last_close_event.time   /* is there any saved event ? */
        && (strcmp(path, last_close_event.path) != 0)) {
        /* no, so send the saved event out now */
        send_fce_event(obj, FCE_FILE_MODIFY,last_close_event.path, NULL);
    }

    LOG(log_debug, logtype_fce, "save_close_event: %s", path);

    last_close_event.time = now;
    strncpy(last_close_event.path, path, MAXPATHLEN);
}

static void fce_init_ign_names(const char *ignores)
{
    int count = 0;
    char *names = strdup(ignores);
    char *p;
    int i = 0;

    while (names[i]) {
        count++;
        for (; names[i] && names[i] != '/'; i++)
            ;
        if (!names[i])
            break;
        i++;
    }

    skip_files = calloc(count + 1, sizeof(char *));

    for (i = 0, p = strtok(names, "/"); p ; p = strtok(NULL, "/"))
        skip_files[i++] = strdup(p);

    free(names);
}

/*
 *
 * Dispatcher for all incoming file change events
 *
 * */
int fce_register(const AFPObj *obj, fce_ev_t event, const char *path, const char *oldpath)
{
    static bool first_event = true;
    const char *bname;

    if (!(fce_ev_enabled & (1 << event)))
        return AFP_OK;

    AFP_ASSERT(event >= FCE_FIRST_EVENT && event <= FCE_LAST_EVENT);
    AFP_ASSERT(path);

    LOG(log_debug, logtype_fce, "register_fce(path: %s, event: %s)",
        path, fce_event_names[event]);

    bname = basename_safe(path);

    if ((udp_sockets == 0) && (obj->fce_notify_script == NULL)) {
        /* No listeners configured */
        return AFP_OK;
    }

	/* do some initialization on the fly the first time */
	if (first_event) {
		fce_initialize_history();
        fce_init_ign_names(obj->fce_ign_names);
        first_event = false;
	}

	/* handle files which should not cause events (.DS_Store atc. ) */
    for (int i = 0; skip_files[i] != NULL; i++) {
        if (strcmp(bname, skip_files[i]) == 0)
			return AFP_OK;
	}

	/* Can we ignore this event based on type or history? */
	if (fce_handle_coalescation(event, path)) {
		LOG(log_debug9, logtype_fce, "Coalesced fc event <%d> for <%s>", event, path);
		return AFP_OK;
	}

    switch (event) {
    case FCE_FILE_MODIFY:
        save_close_event(obj, path);
        break;
    default:
        send_fce_event(obj, event, path, oldpath);
        break;
    }

    return AFP_OK;
}

static void check_saved_close_events(const AFPObj *obj)
{
    time_t now = time(NULL);

    /* check if configured holdclose time has passed */
    if (last_close_event.time && ((last_close_event.time + obj->options.fce_fmodwait) < now)) {
        LOG(log_debug, logtype_fce, "check_saved_close_events: sending event: %s", last_close_event.path);
        /* yes, send event */
        send_fce_event(obj, FCE_FILE_MODIFY, &last_close_event.path[0], NULL);
        last_close_event.path[0] = 0;
        last_close_event.time = 0;
    }
}

/******************** External calls start here **************************/

/*
 * API-Calls for file change api, called form outside (file.c directory.c ofork.c filedir.c)
 * */
void fce_pending_events(const AFPObj *obj)
{
    if (!udp_sockets)
        return;
    check_saved_close_events(obj);
}

/*
 *
 * Extern connect to afpd parameter, can be called multiple times for multiple listeners (up to MAX_UDP_SOCKS times)
 *
 * */
int fce_add_udp_socket(const char *target)
{
	const char *port = FCE_DEFAULT_PORT_STRING;
	char target_ip[256] = {""};

	strncpy(target_ip, target, sizeof(target_ip) -1);

	char *port_delim = strchr( target_ip, ':' );
	if (port_delim) {
		*port_delim = 0;
		port = port_delim + 1;
	}
	return add_udp_socket(target_ip, port);
}

int fce_set_events(const char *events)
{
    char *e;
    char *p;
    
    if (events == NULL)
        return AFPERR_PARAM;

    e = strdup(events);

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
