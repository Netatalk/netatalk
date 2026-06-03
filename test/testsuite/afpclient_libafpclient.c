#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <afp.h>
#include <afp_protocol.h>
#include <libafpclient.h>
#include <uams_def.h>

#include "dsi.h"
#undef dsi_send
#include "afpclient_transport.h"

#define AFPT_AFP_ERROR_BASE (-5000)
#define AFPT_RAW_TIMEOUT 20

struct afptest_libafpclient_transport {
    struct afp_server *server;
    pthread_t loop_thread;
};

struct CONN {
    DSI dsi;
    int type;
    int afp_version;
    uint16_t login_cont_id;
    size_t login_cont_len;
    uint8_t login_cont_data[DSI_CMDSIZ];
    void *transport;
    int use_legacy_transport;
    void *pending_payload;
    size_t pending_payload_len;
    size_t pending_payload_cap;
    uint8_t pending_dsi_command;
    uint32_t pending_data_offset;
    void *reply_queue_head;
    void *reply_queue_tail;
    void *reply_payload;
    size_t reply_payload_len;
    size_t reply_payload_pos;
    uint8_t reply_dsi_command;
    uint32_t reply_dsi_code;
};

struct afptest_dsi_header {
    uint8_t flags;
    uint8_t command;
    uint16_t requestid;
    union {
        int error_code;
        unsigned int data_offset;
    } return_code;
    uint32_t length;
    uint32_t reserved;
};

struct afptest_raw_reply {
    void *reply;
    size_t reply_cap;
    size_t reply_len;
};

struct afptest_afp_rx_buffer {
    unsigned int size;
    unsigned int maxsize;
    char *data;
    int errorcode;
};

extern int (*afp_replies[])(struct afp_server *, char *, unsigned int, void *);
void dsi_setup_header(struct afp_server *server,
                      struct afptest_dsi_header *header,
                      char command);
int dsi_send(struct afp_server *server, char *msg, int size, int wait,
             unsigned char subcommand, void *other);

static pthread_mutex_t libafpclient_init_lock = PTHREAD_MUTEX_INITIALIZER;
static int libafpclient_started;

static int raw_reply(struct afp_server *server, char *buf, unsigned int size,
                     void *other)
{
    struct afptest_raw_reply *rx = other;
    size_t payload_len;
    (void)server;

    if (!rx || size < sizeof(struct afptest_dsi_header)) {
        return 0;
    }

    payload_len = size - sizeof(struct afptest_dsi_header);

    if (payload_len > rx->reply_cap) {
        payload_len = rx->reply_cap;
    }

    if (payload_len) {
        memcpy(rx->reply, buf + sizeof(struct afptest_dsi_header), payload_len);
    }

    rx->reply_len = payload_len;
    return 0;
}

static int ensure_libafpclient_started(pthread_t *loop_thread)
{
    pthread_mutex_lock(&libafpclient_init_lock);

    if (!libafpclient_started) {
        init_uams();
        afp_main_quick_startup(loop_thread);
        libafpclient_started = 1;
    }

    pthread_mutex_unlock(&libafpclient_init_lock);
    return 0;
}

static size_t raw_read_request_size(const void *payload, size_t payload_len,
                                    unsigned char subcommand)
{
    const uint8_t *p = payload;
    uint32_t count32;
    uint32_t count_hi;
    uint32_t count_lo;
    uint64_t count64;

    if (subcommand == afpRead) {
        if (payload_len < 12) {
            return 0;
        }

        memcpy(&count32, p + 8, sizeof(count32));
        return ntohl(count32);
    }

    if (subcommand == afpReadExt) {
        if (payload_len < 20) {
            return 0;
        }

        memcpy(&count_hi, p + 12, sizeof(count_hi));
        memcpy(&count_lo, p + 16, sizeof(count_lo));
        count64 = ((uint64_t)ntohl(count_hi) << 32) | ntohl(count_lo);

        if (count64 > SIZE_MAX) {
            return SIZE_MAX;
        }

        return (size_t)count64;
    }

    return 0;
}

static unsigned int afp_version_number(const char *vers)
{
    if (!vers) {
        return 34;
    }

    if (strcmp(vers, "AFPVersion 2.1") == 0) {
        return 21;
    }

    if (strcmp(vers, "AFP2.2") == 0) {
        return 22;
    }

    if (strcmp(vers, "AFPX03") == 0) {
        return 30;
    }

    if (strcmp(vers, "AFP3.1") == 0) {
        return 31;
    }

    if (strcmp(vers, "AFP3.2") == 0) {
        return 32;
    }

    if (strcmp(vers, "AFP3.3") == 0) {
        return 33;
    }

    return 34;
}

const char *afptest_libafpclient_uam_name(const char *uam)
{
    if (!uam || !*uam) {
        return NULL;
    }

    return resolve_uam_shorthand(uam);
}

int afptest_libafpclient_login(struct CONN *conn, const char *host, int port,
                               const char *vers, const char *uam,
                               const char *user, const char *password)
{
    struct afptest_libafpclient_transport *transport;
    struct afp_connection_request req;
    unsigned int uam_mask;

    if (!conn || !host || !user || !password) {
        return htonl((uint32_t)kFPParamErr);
    }

    ensure_libafpclient_started(NULL);
    afptest_libafpclient_close(conn);
    transport = calloc(1, sizeof(*transport));

    if (!transport) {
        return htonl((uint32_t)kFPMiscErr);
    }

    memset(&req, 0, sizeof(req));
    afp_default_url(&req.url);
    snprintf(req.url.servername, sizeof(req.url.servername), "%s", host);
    snprintf(req.url.username, sizeof(req.url.username), "%s", user);
    snprintf(req.url.password, sizeof(req.url.password), "%s", password);
    req.url.port = port;
    req.url.requested_version = (int)afp_version_number(vers);

    if (uam && *uam) {
        uam_mask = find_uam_by_name(uam);

        if (uam_mask == 0) {
            free(transport);
            return htonl((uint32_t)kFPBadUAM);
        }
    } else {
        uam_mask = default_uams_mask();
    }

    req.uam_mask = uam_mask;
    transport->server = afp_server_full_connect(NULL, &req);

    if (!transport->server) {
        free(transport);
        return htonl((uint32_t)kFPUserNotAuth);
    }

    conn->transport = transport;
    conn->dsi.socket = transport->server->fd;
    conn->dsi.server_quantum = transport->server->tx_quantum;
    conn->dsi.attn_quantum = transport->server->attention_quantum;
    conn->afp_version = transport->server->using_version
                        ? transport->server->using_version->av_number
                        : (int)afp_version_number(vers);
    conn->dsi.header.dsi_code = 0;
    conn->dsi.cmdlen = 0;
    conn->dsi.datalen = 0;
    return 0;
}

int afptest_libafpclient_raw_command(struct CONN *conn,
                                     const void *payload, size_t payload_len,
                                     uint8_t dsi_command,
                                     uint32_t data_offset,
                                     uint32_t *dsi_code,
                                     void *reply, size_t reply_cap,
                                     size_t *reply_len)
{
    struct afptest_libafpclient_transport *transport;
    struct afptest_dsi_header *hdr;
    struct afptest_raw_reply rx;
    struct afptest_afp_rx_buffer read_rx;
    int (*old_reply)(struct afp_server *, char *, unsigned int, void *);
    unsigned char subcommand;
    void *reply_context;
    char *read_storage = NULL;
    size_t read_cap;
    size_t read_requested;
    char *msg;
    int rc;

    if (reply_len) {
        *reply_len = 0;
    }

    if (dsi_code) {
        *dsi_code = htonl((uint32_t)kFPParamErr);
    }

    if (!conn || !conn->transport || !payload || payload_len == 0) {
        return -1;
    }

    transport = conn->transport;

    if (!transport->server) {
        return -1;
    }

    msg = calloc(1, sizeof(*hdr) + payload_len);

    if (!msg) {
        if (dsi_code) {
            *dsi_code = htonl((uint32_t)kFPMiscErr);
        }

        return -1;
    }

    hdr = (struct afptest_dsi_header *)msg;
    dsi_setup_header(transport->server, hdr, dsi_command);
    hdr->return_code.data_offset = htonl(data_offset);
    memcpy(msg + sizeof(*hdr), payload, payload_len);
    subcommand = ((const unsigned char *)payload)[0];
    memset(&rx, 0, sizeof(rx));
    rx.reply = reply;
    rx.reply_cap = reply_cap;
    reply_context = &rx;

    if (subcommand == afpRead || subcommand == afpReadExt) {
        read_requested = raw_read_request_size(payload, payload_len, subcommand);
        read_cap = read_requested;

        if (transport->server->rx_quantum && read_cap > transport->server->rx_quantum) {
            read_cap = transport->server->rx_quantum;
        }

        if (read_cap < reply_cap) {
            read_cap = reply_cap;
        }

        if (read_cap > UINT_MAX) {
            read_cap = UINT_MAX;
        }

        if (read_cap > reply_cap) {
            read_storage = malloc(read_cap);

            if (!read_storage) {
                free(msg);

                if (dsi_code) {
                    *dsi_code = htonl((uint32_t)kFPMiscErr);
                }

                return -1;
            }
        }

        memset(&read_rx, 0, sizeof(read_rx));
        read_rx.data = read_storage ? read_storage : reply;
        read_rx.maxsize = (unsigned int)read_cap;
        reply_context = &read_rx;
    }

    old_reply = afp_replies[subcommand];

    if (subcommand != afpRead && subcommand != afpReadExt) {
        afp_replies[subcommand] = raw_reply;
    }

    rc = dsi_send(transport->server, msg, (int)(sizeof(*hdr) + payload_len),
                  AFPT_RAW_TIMEOUT, subcommand, reply_context);

    if (subcommand != afpRead && subcommand != afpReadExt) {
        afp_replies[subcommand] = old_reply;
    }

    free(msg);

    if (subcommand == afpRead || subcommand == afpReadExt) {
        rx.reply_len = read_rx.size > reply_cap ? reply_cap : read_rx.size;

        if (read_storage && rx.reply_len) {
            memcpy(reply, read_storage, rx.reply_len);
        }
    }

    free(read_storage);

    if (dsi_code) {
        *dsi_code = htonl((uint32_t)rc);
    }

    if (reply_len) {
        *reply_len = rx.reply_len;
    }

    /* Negative AFP result codes still carry valid DSI reply payloads. */
    return rc == 0 || rc <= AFPT_AFP_ERROR_BASE ? 0 : -1;
}

void afptest_libafpclient_logout(struct CONN *conn)
{
    struct afptest_libafpclient_transport *transport;

    if (!conn || !conn->transport) {
        return;
    }

    transport = conn->transport;

    if (transport->server) {
        afp_logout(transport->server, 1);
    }
}

void afptest_libafpclient_close(struct CONN *conn)
{
    struct afptest_libafpclient_transport *transport;

    if (!conn || !conn->transport) {
        return;
    }

    transport = conn->transport;

    if (transport->server) {
        afp_server_remove(transport->server);
    }

    free(transport);
    conn->transport = NULL;
    conn->dsi.socket = -1;
}

#endif
