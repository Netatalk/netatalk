#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

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
#define AFPT_SMOKE_SIZE 4096
#define AFPT_OPENACC_RD (1 << 0)
#define AFPT_OPENACC_WR (1 << 1)

struct afptest_libafpclient_transport {
    struct afp_server *server;
};

struct CONN {
    DSI dsi;
    int type;
    int afp_version;
    uint16_t login_cont_id;
    size_t login_cont_len;
    uint8_t login_cont_data[DSI_CMDSIZ];
    void *transport;
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
void rm_fd_and_signal(int fd);
extern int Quiet;
extern int Verbose;

static pthread_mutex_t libafpclient_init_lock = PTHREAD_MUTEX_INITIALIZER;
static int libafpclient_started;
static pthread_t loop_thread;

static void afptest_libafpclient_log(void *priv, enum logtypes logtype,
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

    if (message) {
        fprintf(stdout, "%s\n", message);
    }
}

static struct libafpclient afptest_libafpclient = {
    .unmount_volume = NULL,
    .log_for_client = afptest_libafpclient_log,
    .forced_ending_hook = NULL,
    .scan_extra_fds = NULL,
    .loop_started = NULL,
};

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
        memcpy(rx->reply, buf + sizeof(struct afptest_dsi_header),
               payload_len);
    }

    rx->reply_len = payload_len;
    return 0;
}

static int ensure_libafpclient_started(void)
{
    pthread_mutex_lock(&libafpclient_init_lock);

    if (!libafpclient_started) {
        libafpclient_register(&afptest_libafpclient);
        init_uams();
        afp_main_quick_startup(&loop_thread);
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

static void afptest_libafpclient_smoke_step(const char *step)
{
    if (!Quiet && Verbose) {
        fprintf(stdout, "libafpclient smoke: %s\n", step);
        fflush(stdout);
    }
}

static void afptest_libafpclient_smoke_error(const char *step, int rc)
{
    if (!Quiet) {
        fprintf(stdout, "libafpclient smoke: %s failed (%d)\n", step, rc);
        fflush(stdout);
    }
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

    ensure_libafpclient_started();
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

        if (transport->server->rx_quantum
                && read_cap > transport->server->rx_quantum) {
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

    return rc == 0 || rc <= AFPT_AFP_ERROR_BASE ? 0 : -1;
}

int afptest_libafpclient_smoke(struct CONN *conn, const char *volume_name)
{
    struct afptest_libafpclient_transport *transport;
    struct afp_server *server;
    struct afp_volume *volume;
    struct afp_file_info file_info;
    struct afp_rx_buffer rx;
    char dir_name[AFP_MAX_PATH];
    char file_name[AFP_MAX_PATH];
    char write_buf[AFPT_SMOKE_SIZE];
    char read_buf[AFPT_SMOKE_SIZE];
    unsigned int dirid = 0;
    unsigned short forkid = 0;
    uint32_t written = 0;
    int cleanup_rc = 0;
    int dir_created = 0;
    int file_created = 0;
    int rc;

    if (!conn || !conn->transport || !volume_name || !*volume_name) {
        return -1;
    }

    transport = conn->transport;
    server = transport->server;

    if (!server) {
        return -1;
    }

    snprintf(dir_name, sizeof(dir_name), "libafpclient smoke %ld",
             (long)getpid());
    snprintf(file_name, sizeof(file_name), "libafpclient smoke %ld.bin",
             (long)getpid());
    afptest_libafpclient_smoke_step("getting server parameters");
    rc = afp_getsrvrparms(server);

    if (rc != 0) {
        afptest_libafpclient_smoke_error("get server parameters", rc);
        return rc;
    }

    afptest_libafpclient_smoke_step("finding volume");
    volume = find_volume_by_name(server, (char *)volume_name);

    if (!volume) {
        afptest_libafpclient_smoke_error("find volume", -1);
        return -1;
    }

    if (volume->volid == 0) {
        afptest_libafpclient_smoke_step("opening volume");
        rc = afp_volopen(volume, kFPVolIDBit, NULL);

        if (rc != 0) {
            afptest_libafpclient_smoke_error("open volume", rc);
            return rc;
        }
    }

    afptest_libafpclient_smoke_step("creating directory");
    rc = afp_createdir(volume, AFP_ROOT_DID, dir_name, &dirid);

    if (rc != 0) {
        afptest_libafpclient_smoke_error("create directory", rc);
        return rc;
    }

    dir_created = 1;

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "libafpclient smoke: created directory '%s', reply did=%u "
                "(ntohl=%u)\n",
                dir_name, dirid, ntohl(dirid));
        fflush(stdout);
    }

    afptest_libafpclient_smoke_step("creating file");
    rc = afp_createfile(volume, 0, AFP_ROOT_DID, file_name);

    if (rc != 0) {
        afptest_libafpclient_smoke_error("create file", rc);
        goto cleanup;
    }

    file_created = 1;
    memset(&file_info, 0, sizeof(file_info));
    afptest_libafpclient_smoke_step("opening data fork");
    rc = afp_openfork(volume, 0, AFP_ROOT_DID,
                      AFPT_OPENACC_RD | AFPT_OPENACC_WR,
                      file_name, &file_info);

    if (rc != 0) {
        afptest_libafpclient_smoke_error("open fork", rc);
        goto cleanup;
    }

    forkid = file_info.forkid;
    memset(write_buf, 'S', sizeof(write_buf));
    afptest_libafpclient_smoke_step("writing data fork");
    rc = afp_write(volume, forkid, 0, sizeof(write_buf), write_buf, &written);

    if (rc != 0 || written != sizeof(write_buf)) {
        afptest_libafpclient_smoke_error("write data fork",
                                         rc != 0 ? rc : -1);
        rc = rc != 0 ? rc : -1;
        goto cleanup;
    }

    memset(read_buf, 0, sizeof(read_buf));
    memset(&rx, 0, sizeof(rx));
    rx.data = read_buf;
    rx.maxsize = sizeof(read_buf);
    afptest_libafpclient_smoke_step("reading data fork");
    rc = afp_read(volume, forkid, 0, sizeof(read_buf), &rx);

    if (rc != 0 || rx.size != sizeof(read_buf)
            || read_buf[0] != 'S'
            || read_buf[sizeof(read_buf) - 1] != 'S') {
        afptest_libafpclient_smoke_error("read data fork",
                                         rc != 0 ? rc : -1);
        rc = rc != 0 ? rc : -1;
        goto cleanup;
    }

cleanup:

    if (forkid) {
        cleanup_rc = afp_closefork(volume, forkid);

        if (cleanup_rc != 0 && rc == 0) {
            afptest_libafpclient_smoke_error("close fork", cleanup_rc);
            rc = cleanup_rc;
        }
    }

    if (file_created) {
        cleanup_rc = afp_delete(volume, AFP_ROOT_DID, file_name);

        if (cleanup_rc != 0 && rc == 0) {
            afptest_libafpclient_smoke_error("delete file", cleanup_rc);
            rc = cleanup_rc;
        }
    }

    if (dir_created) {
        cleanup_rc = afp_delete(volume, AFP_ROOT_DID, dir_name);

        if (cleanup_rc != 0 && rc == 0) {
            afptest_libafpclient_smoke_error("delete directory", cleanup_rc);
            rc = cleanup_rc;
        }
    }

    return rc;
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
    int fd;

    if (!conn) {
        return;
    }

    afptest_libafpclient_clear_io_state(conn);

    if (!conn->transport) {
        return;
    }

    transport = conn->transport;

    if (transport->server) {
        fd = transport->server->fd >= 0 ? transport->server->fd
             : conn->dsi.socket;

        if (fd >= 0) {
            rm_fd_and_signal(fd);
        }

        afp_server_remove(transport->server);
    }

    free(transport);
    conn->transport = NULL;
    conn->dsi.socket = -1;
}

#endif
