#include <assert.h>
#include <atalk/afp.h>

#include "afpclient.h"
#include "afpclient_transport.h"
#include "testhelper.h"

static char LastHost[NI_MAXHOST] = "localhost";
static int LastPort = DSI_AFPOVERTCP_PORT;

/* Define the global test settings */
int Throttle = 0;
int Convert = 1;
int	Interactive = 0;
int	Quiet = 1;
int	Verbose = 0;
int	Color = 1;

#define UNICODE(a) (a->afp_version >= 30)

#define kTextEncodingUTF8 0x08000103

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
struct afptest_libafpclient_reply {
    uint8_t dsi_command;
    uint32_t dsi_code;
    uint8_t *payload;
    size_t payload_len;
    struct afptest_libafpclient_reply *next;
};

static int afptest_libafpclient_pop_reply(CONN *conn);
#endif

/* -------------------------------------------- */
int OpenClientSocket(char *host, int port)
{
    int sock = -1;
    assert(host);
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *ressave;
    char portstr[6];
    int attr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    snprintf(LastHost, sizeof(LastHost), "%s", host);
    LastPort = port;

    if (getaddrinfo(host, portstr, &hints, &res) != 0) {
        fprintf(stdout, "Unknown host '%s' for server.\n", host);
        return -1;
    }

    ressave = res;

    while (res) {
        sock = socket(res->ai_family, res->ai_socktype, 0);

        if (sock >= 0) {
            attr = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &attr, sizeof(attr));

            if (connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
                /* Success */
                break;
            }

            close(sock);
            sock = -1;
        }

        res = res->ai_next;
    }

    freeaddrinfo(ressave);

    if (sock < 0) {
        fprintf(stdout, "Failed to connect socket.\n");
    }

    return sock;
}

/* -------------------------------------------- */
int CloseClientSocket(int fd)
{
    if (fd < 0) {
        return 0;
    }

    return close(fd);
}

void AFPUseLegacyTransport(CONN *conn)
{
    assert(conn);
    conn->use_legacy_transport = 1;
}

/*! read raw data. return actual bytes read. this will wait until
 * it gets length bytes */
size_t dsi_stream_read(DSI *dsi, void *data, const size_t length)
{
    size_t stored;
    ssize_t len;

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    CONN *conn = (CONN *)dsi;

    if (conn->transport && !conn->use_legacy_transport) {
        size_t available;

        if (!conn->reply_payload
                || conn->reply_payload_pos >= conn->reply_payload_len) {
            return 0;
        }

        available = conn->reply_payload_len - conn->reply_payload_pos;
        stored = min(length, available);

        if (stored) {
            memcpy(data, (uint8_t *)conn->reply_payload + conn->reply_payload_pos,
                   stored);
            conn->reply_payload_pos += stored;
            dsi->read_count += stored;
        }

        return stored;
    }

#endif

    stored = 0;

    while (stored < length) {
        if ((len = read(dsi->socket, (uint8_t *) data + stored,
                        length - stored)) == -1 && errno == EINTR) {
            continue;
        }

        if (len > 0) {
            stored += len;
        } else { /* eof or error */
            if (!Quiet) {
                fprintf(stdout, "dsi_stream_read(%ld): %s\n", len,
                        (len < 0) ? strerror(errno) : "EOF");
            }

            /* Mark the header as invalid so callers see a non-zero dsi_code
             * rather than mistaking the zeroed header for a success response. */
            dsi->header.dsi_code = 0xffffffff;
            break;
        }
    }

    dsi->read_count += stored;
    return stored;
}

/* -------------------------------------------------------*/
int dsi_read_header(DSI *dsi)
{
    char block[DSI_BLOCKSIZ];

    /* read in the header */
    if (dsi_stream_read(dsi, block, sizeof(block)) != sizeof(block)) {
        return -1;
    }

    dsi->header.dsi_flags = block[0];
    dsi->header.dsi_command = block[1];
    memcpy(&dsi->header.dsi_requestID, block + 2,
           sizeof(dsi->header.dsi_requestID));
    memcpy(&dsi->header.dsi_code, block + 4, sizeof(dsi->header.dsi_code));
    memcpy(&dsi->header.dsi_len, block + 8, sizeof(dsi->header.dsi_len));
    memcpy(&dsi->header.dsi_reserved, block + 12, sizeof(dsi->header.dsi_reserved));
    dsi->serverID = ntohs(dsi->header.dsi_requestID);
    return block[1];
}

/*! read data. function on success. 0 on failure. data length gets
 * stored in length variable. this should really use size_t's, but
 * that would require changes elsewhere. */
int dsi_stream_receive(DSI *dsi, void *buf, const size_t ilength,
                       size_t *rlength)
{
    int ret;
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    CONN *conn = (CONN *)dsi;

    if (conn->transport && !conn->use_legacy_transport) {
        if (!conn->reply_payload
                || conn->reply_payload_pos >= conn->reply_payload_len) {
            if (!afptest_libafpclient_pop_reply(conn)) {
                *rlength = 0;
                return 0;
            }
        }

        size_t available = conn->reply_payload_len - conn->reply_payload_pos;
        *rlength = min(available, ilength);

        if (*rlength) {
            memmove(buf, (uint8_t *)conn->reply_payload + conn->reply_payload_pos,
                    *rlength);
            conn->reply_payload_pos += *rlength;
        }

        return dsi->header.dsi_command;
    }

#endif

    if ((ret = dsi_read_header(dsi)) < 0) {
        return 0;
    }

    /* make sure we don't over-write our buffers. */
    *rlength = min(ntohl(dsi->header.dsi_len), ilength);

    if (dsi_stream_read(dsi, buf, *rlength) != *rlength) {
        return 0;
    }

    return ret;
}

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
static void afptest_libafpclient_clear_pending(CONN *conn)
{
    free(conn->pending_payload);
    conn->pending_payload = NULL;
    conn->pending_payload_len = 0;
    conn->pending_payload_cap = 0;
    conn->pending_dsi_command = 0;
    conn->pending_data_offset = 0;
}

static void afptest_libafpclient_clear_active_reply(CONN *conn)
{
    free(conn->reply_payload);
    conn->reply_payload = NULL;
    conn->reply_payload_len = 0;
    conn->reply_payload_pos = 0;
    conn->reply_dsi_command = 0;
    conn->reply_dsi_code = 0;
}

static void afptest_libafpclient_clear_replies(CONN *conn)
{
    struct afptest_libafpclient_reply *reply = conn->reply_queue_head;

    afptest_libafpclient_clear_active_reply(conn);

    while (reply) {
        struct afptest_libafpclient_reply *next = reply->next;
        free(reply->payload);
        free(reply);
        reply = next;
    }

    conn->reply_queue_head = NULL;
    conn->reply_queue_tail = NULL;
}

static int afptest_libafpclient_pop_reply(CONN *conn)
{
    DSI *dsi = &conn->dsi;
    struct afptest_libafpclient_reply *reply = conn->reply_queue_head;

    if (!reply) {
        return 0;
    }

    afptest_libafpclient_clear_active_reply(conn);
    conn->reply_queue_head = reply->next;

    if (!conn->reply_queue_head) {
        conn->reply_queue_tail = NULL;
    }

    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_command = reply->dsi_command;
    dsi->header.dsi_code = reply->dsi_code;
    dsi->header.dsi_len = htonl((uint32_t)reply->payload_len);
    dsi->cmdlen = reply->payload_len;
    dsi->datalen = reply->payload_len;
    if (reply->payload_len) {
        conn->reply_payload = reply->payload;
        conn->reply_payload_len = reply->payload_len;
        conn->reply_payload_pos = 0;
    } else {
        free(reply->payload);
        conn->reply_payload = NULL;
        conn->reply_payload_len = 0;
        conn->reply_payload_pos = 0;
    }

    conn->reply_dsi_command = reply->dsi_command;
    conn->reply_dsi_code = reply->dsi_code;

    if (reply->payload_len > sizeof(dsi->data)) {
        memcpy(dsi->data, reply->payload, sizeof(dsi->data));
    } else if (reply->payload_len) {
        memcpy(dsi->data, reply->payload, reply->payload_len);
    }

    if (reply->payload_len > sizeof(dsi->commands)) {
        memcpy(dsi->commands, reply->payload, sizeof(dsi->commands));
    } else if (reply->payload_len) {
        memcpy(dsi->commands, reply->payload, reply->payload_len);
    }

    reply->payload = NULL;
    free(reply);
    return 1;
}

static int afptest_libafpclient_enqueue_reply(CONN *conn, uint8_t dsi_command,
        uint32_t dsi_code, uint8_t *payload, size_t payload_len)
{
    struct afptest_libafpclient_reply *reply;

    reply = calloc(1, sizeof(*reply));

    if (!reply) {
        free(payload);
        return 0;
    }

    reply->dsi_command = dsi_command;
    reply->dsi_code = dsi_code;
    reply->payload = payload;
    reply->payload_len = payload_len;

    if (conn->reply_queue_tail) {
        ((struct afptest_libafpclient_reply *)conn->reply_queue_tail)->next = reply;
    } else {
        conn->reply_queue_head = reply;
    }

    conn->reply_queue_tail = reply;
    return 1;
}

static size_t afptest_libafpclient_read_reply_size(const void *payload,
        size_t payload_len)
{
    const uint8_t *p = payload;
    uint32_t count32;
    uint32_t count_hi;
    uint32_t count_lo;

    if (!payload || payload_len == 0) {
        return DSI_DATASIZ;
    }

    if (p[0] == AFP_READ) {
        if (payload_len < 12) {
            return DSI_DATASIZ;
        }

        memcpy(&count32, p + 8, sizeof(count32));
        return ntohl(count32);
    }

    if (p[0] == AFP_READ_EXT) {
        if (payload_len < 20) {
            return DSI_DATASIZ;
        }

        memcpy(&count_hi, p + 12, sizeof(count_hi));
        memcpy(&count_lo, p + 16, sizeof(count_lo));

        if (ntohl(count_hi) != 0) {
            return DSI_DATASIZ;
        }

        return ntohl(count_lo);
    }

    return DSI_DATASIZ;
}

static int afptest_libafpclient_dispatch_raw(CONN *conn, const void *payload,
        size_t payload_len, uint8_t dsi_command, uint32_t data_offset)
{
    DSI *dsi = &conn->dsi;
    uint32_t dsi_code = 0;
    uint8_t *reply = NULL;
    size_t reply_cap = DSI_DATASIZ;
    size_t reply_len = 0;

    if (payload_len && (((const uint8_t *)payload)[0] == AFP_READ
                        || ((const uint8_t *)payload)[0] == AFP_READ_EXT)) {
        reply_cap = afptest_libafpclient_read_reply_size(payload, payload_len);
    }

    if (reply_cap == 0) {
        reply_cap = 1;
    }

    reply = malloc(reply_cap);

    if (!reply) {
        dsi->header.dsi_code = htonl((uint32_t)-1);
        dsi->header.dsi_len = 0;
        dsi->cmdlen = 0;
        dsi->datalen = 0;
        return 0;
    }

    if (afptest_libafpclient_raw_command(conn, payload, payload_len,
                                         dsi_command, data_offset, &dsi_code,
                                         reply, reply_cap,
                                         &reply_len) < 0) {
        free(reply);
        dsi->header.dsi_code = dsi_code ? dsi_code : 0xffffffff;
        dsi->header.dsi_len = 0;
        dsi->cmdlen = 0;
        dsi->datalen = 0;
        conn->reply_payload_len = 0;
        conn->reply_payload_pos = 0;
        return 0;
    }

    if (!afptest_libafpclient_enqueue_reply(conn, dsi_command, dsi_code, reply,
                                            reply_len)) {
        dsi->header.dsi_code = htonl((uint32_t)-1);
        dsi->header.dsi_len = 0;
        dsi->cmdlen = 0;
        dsi->datalen = 0;
        return 0;
    }

    return 1;
}

static int afptest_libafpclient_queue_raw(CONN *conn, const void *payload,
        size_t payload_len, size_t total_len, uint8_t dsi_command,
        uint32_t data_offset)
{
    void *pending;

    if (total_len < payload_len) {
        return 0;
    }

    pending = malloc(total_len);

    if (!pending) {
        return 0;
    }

    memcpy(pending, payload, payload_len);
    afptest_libafpclient_clear_pending(conn);
    conn->pending_payload = pending;
    conn->pending_payload_len = payload_len;
    conn->pending_payload_cap = total_len;
    conn->pending_dsi_command = dsi_command;
    conn->pending_data_offset = data_offset;
    return 1;
}

static size_t afptest_libafpclient_append_raw(CONN *conn, const void *data,
        size_t length)
{
    size_t available;
    size_t pending_len;
    void *pending;
    uint8_t dsi_command;
    uint32_t data_offset;
    int ok;

    if (!conn->pending_payload) {
        return 0;
    }

    available = conn->pending_payload_cap - conn->pending_payload_len;

    if (length > available) {
        afptest_libafpclient_clear_pending(conn);
        return 0;
    }

    memcpy((uint8_t *)conn->pending_payload + conn->pending_payload_len,
           data, length);
    conn->pending_payload_len += length;
    conn->dsi.write_count += length;

    if (conn->pending_payload_len < conn->pending_payload_cap) {
        return length;
    }

    pending = conn->pending_payload;
    pending_len = conn->pending_payload_len;
    dsi_command = conn->pending_dsi_command;
    data_offset = conn->pending_data_offset;
    conn->pending_payload = NULL;
    conn->pending_payload_len = 0;
    conn->pending_payload_cap = 0;
    conn->pending_dsi_command = 0;
    conn->pending_data_offset = 0;
    ok = afptest_libafpclient_dispatch_raw(conn, pending, pending_len,
                                           dsi_command, data_offset);
    free(pending);
    return ok ? length : 0;
}
#endif

/* ======================================================= */
size_t dsi_stream_write(DSI *dsi, void *data, const size_t length)
{
    size_t written;
    ssize_t len;
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    CONN *conn = (CONN *)dsi;

    if (conn->transport && !conn->use_legacy_transport
            && conn->pending_payload) {
        return afptest_libafpclient_append_raw(conn, data, length);
    }

#endif
    written = 0;

    while (written < length) {
        if (((len = write(dsi->socket, (uint8_t *) data + written,
                          length - written)) == -1 && errno == EINTR) ||
                !len) {
            continue;
        }

        if (len < 0) {
            if (!Quiet) {
                fprintf(stdout, "dsi_stream_write: %s\n", strerror(errno));
            }

            break;
        }

        written += len;
    }

    dsi->write_count += written;
    return written;
}

/*! write data. 0 on failure. this assumes that dsi_len will never
 * cause an overflow in the data buffer. */
static int use_writev = 1;
int dsi_stream_send(DSI *dsi, void *buf, size_t length)
{
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    CONN *conn = (CONN *)dsi;

    if (conn->transport && !conn->use_legacy_transport) {
        uint32_t total_len = ntohl(dsi->header.dsi_len);
        uint32_t data_offset = ntohl(dsi->header.dsi_code);

        if (conn->pending_payload) {
            afptest_libafpclient_clear_pending(conn);
        }

        if (conn->reply_payload
                && conn->reply_payload_pos >= conn->reply_payload_len) {
            afptest_libafpclient_clear_active_reply(conn);
        }

        if (total_len > length) {
            return afptest_libafpclient_queue_raw(conn, buf, length,
                                                  total_len,
                                                  dsi->header.dsi_command,
                                                  data_offset);
        }

        return afptest_libafpclient_dispatch_raw(conn, buf, length,
                dsi->header.dsi_command, data_offset);
    }

#endif
    char block[DSI_BLOCKSIZ];
    struct iovec iov[2];
    size_t towrite;
    ssize_t len;
    block[0] = dsi->header.dsi_flags;
    block[1] = dsi->header.dsi_command;
    memcpy(block + 2, &dsi->header.dsi_requestID,
           sizeof(dsi->header.dsi_requestID));
    memcpy(block + 4, &dsi->header.dsi_code, sizeof(dsi->header.dsi_code));
    memcpy(block + 8, &dsi->header.dsi_len, sizeof(dsi->header.dsi_len));
    memcpy(block + 12, &dsi->header.dsi_reserved, sizeof(dsi->header.dsi_reserved));

    if (Throttle) {
        sleep(1);
    }

    if (!length) { /* just write the header */
        length = (dsi_stream_write(dsi, block, sizeof(block)) == sizeof(block));
        /* really 0 on failure, 1 on success */
        return (int) length;
    }

    if (use_writev) {
        iov[0].iov_base = block;
        iov[0].iov_len = sizeof(block);
        iov[1].iov_base = buf;
        iov[1].iov_len = length;
        towrite = sizeof(block) + length;
        dsi->write_count += towrite;

        while (towrite > 0) {
            if (((len = writev(dsi->socket, iov, 2)) == -1 && errno == EINTR) || !len) {
                continue;
            }

            if (len == towrite) { /* wrote everything out */
                break;
            } else if (len < 0) { /* error */
                if (!Quiet) {
                    fprintf(stdout, "dsi_stream_send: %s", strerror(errno));
                }

                return 0;
            }

            towrite -= len;

            if (towrite > length) { /* skip part of header */
                iov[0].iov_base = (char *) iov[0].iov_base + len;
                iov[0].iov_len -= len;
            } else { /* skip to data */
                if (iov[0].iov_len) {
                    len -= iov[0].iov_len;
                    iov[0].iov_len = 0;
                }

                iov[1].iov_base = (char *) iov[1].iov_base + len;
                iov[1].iov_len -= len;
            }
        }
    } else {
        /* write the header then data */
        if ((dsi_stream_write(dsi, block, sizeof(block)) != sizeof(block)) ||
                (dsi_stream_write(dsi, buf, length) != length)) {
            return 0;
        }
    }

    return 1;
}

/* ------------------------------------- */
void dsi_tickle(DSI *dsi)
{
    char block[DSI_BLOCKSIZ];
    uint16_t id;
    id = htons(dsi_clientID(dsi));
    memset(block, 0, sizeof(block));
    block[0] = DSIFL_REQUEST;
    block[1] = DSIFUNC_TICKLE;
    memcpy(block + 2, &id, sizeof(id));
    /* code = len = reserved = 0 */
    dsi_stream_write(dsi, block, DSI_BLOCKSIZ);
}

/* ------------------------------------- */
int Attention_received;

static int dsi_receive(DSI *x, unsigned char *buf, size_t length)
{
    int ret;
    Attention_received = 0;

    while (1) {
        ret = dsi_stream_receive(x, buf, length, &x->cmdlen);

        if (ret == DSIFUNC_ATTN) {
            Attention_received = 1;
            continue;
        } else if (ret == DSIFUNC_CLOSE) {
            continue;
        } else if (ret == DSIFUNC_TICKLE) {
            dsi_tickle(x);
        } else {
            break;
        }
    }

    return ret;
}

/* ------------------------------------- */
static int dsi_full_receive(DSI *x, unsigned char *buf, int length)
{
    int ret;
    Attention_received = 0;

    while (1) {
        ret = dsi_stream_receive(x, buf, length, &x->cmdlen);

        if (ret == DSIFUNC_ATTN) {
            Attention_received = 1;
            continue;
        } else if (ret == DSIFUNC_TICKLE) {
            dsi_tickle(x);
        } else {
            break;
        }
    }

    return ret;
}

/* ------------------------------------- */
int dsi_cmd_receive(DSI *x)
{
    return dsi_receive(x, x->commands, DSI_CMDSIZ);
}

/* ------------------------------------- */
int dsi_data_receive(DSI *x)
{
    return dsi_receive(x, x->data, DSI_DATASIZ);
}

/* ------------------------------- */
void SendInit(DSI *dsi)
{
    memset(dsi->commands, 0, DSI_CMDSIZ);
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
}

/* ------------------------------- */
void SetLen(DSI *dsi, int ofs)
{
    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl((uint32_t) dsi->datalen);
    dsi->header.dsi_code = 0;
}

/* ------------------------------- */
static int  SendCmd(DSI *dsi, uint8_t cmd)
{
    int ofs;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = cmd;
    SetLen(dsi, ofs);
    return dsi_stream_send(dsi, dsi->commands, dsi->datalen);
}

/* ------------------------------- */
static int  SendCmdWithU16(DSI *dsi, uint8_t cmd, uint16_t param)
{
    int ofs;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = cmd;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &param, sizeof(param));
    ofs += sizeof(param);
    SetLen(dsi, ofs);
    return dsi_stream_send(dsi, dsi->commands, dsi->datalen);
}

/* ------------------------- */
static unsigned int SendCmdVolDidCname(CONN *conn, uint8_t cmd, uint16_t vol,
                                       int did, char *name)
{
    int ofs;
    DSI *dsi;
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = cmd;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/*!
   Open a new session
*/
unsigned int DSIOpenSession(CONN *conn)
{
    DSI *dsi;
    uint32_t i = 0;
    assert(conn);
    dsi = &conn->dsi;
    /* DSIOpenSession */
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_OPEN;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->cmdlen = 2 + sizeof(i);
    dsi->commands[0] = DSIOPT_ATTNQUANT;
    dsi->commands[1] = sizeof(i);
    i = htonl(DSI_DEFQUANT);
    memcpy(dsi->commands + 2, &i, sizeof(i));
    dsi_send(dsi);
    dsi_cmd_receive(dsi);
#if 0
    dump_header(dsi);

    if (dsi->header.dsi_command == DSIFUNC_OPEN) {
        dump_open(dsi);
    }

#endif

    if (!dsi->header.dsi_code) {
        memcpy(&i, dsi->commands + 2, sizeof(i));
        i = ntohl(i);
        dsi->server_quantum = i;
    } else {
        dsi->server_quantum = 0;
    }

    return dsi->header.dsi_code;
}

/*!
   GetStatus
*/
unsigned int DSIGetStatus(CONN *conn)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_STAT;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->cmdlen = 0;
    dsi_send(dsi);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/*!
   Close Session
   no reply
*/
unsigned int DSICloseSession(CONN *conn)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CLOSE;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->cmdlen = 0;
    dsi_send(dsi);
    return 0;
}

/*!
	@bug spec violation in netatalk
	FPlogout ==> dsiclose
*/
unsigned int AFPopenLogin(CONN *conn, const char *vers, const char *uam,
                          const char *usr, const char *pwd)
{
    uint8_t len;
    int ofs;
    DSI *dsi = &conn->dsi;
    assert(conn && vers && uam && usr && pwd);
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (!conn->use_legacy_transport) {
        int old_socket = dsi->socket;
        afptest_libafpclient_clear_pending(conn);
        afptest_libafpclient_clear_replies(conn);
        unsigned int ret = afptest_libafpclient_login(conn, LastHost, LastPort, vers,
                           uam, usr, pwd);

        if (old_socket > 0 && old_socket != dsi->socket) {
            CloseClientSocket(old_socket);
        }

        return ret;
    }

#endif

    if (DSIOpenSession(conn)) {
        return dsi->header.dsi_code;
    }

    /* -------------- */
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_LOGIN;
    len = (uint8_t) strlen(vers);
    dsi->commands[ofs++] = len;
    memcpy(&dsi->commands[ofs], vers, len);
    ofs += len;
    len = (uint8_t) strlen(uam);
    dsi->commands[ofs++] = len;
    memcpy(&dsi->commands[ofs], uam, len);
    ofs += len;
    len = (uint8_t) strlen(usr);

    if (len) {
        dsi->commands[ofs++] = len;
        memcpy(&dsi->commands[ofs], usr, len);
        ofs += len;
        len = (uint8_t) strlen(pwd);

        if (ofs & 1) {
            dsi->commands[ofs++] = 0;
        }

        memcpy(&dsi->commands[ofs], pwd, len);
        ofs += PASSWDLEN;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* Capture FPLoginExt/FPLoginCont reply block on kFPAuthContinue.
 * Reply payload layout (AFP Reference table 51): int16_t ID, then UAM-specific
 * UserAuthInfo. */
static void capture_login_cont(CONN *conn)
{
    const DSI *dsi = &conn->dsi;
    uint16_t id_be;
    conn->login_cont_id = 0;
    conn->login_cont_len = 0;

    if (dsi->cmdlen < sizeof(id_be)) {
        return;
    }

    memcpy(&id_be, dsi->commands, sizeof(id_be));
    conn->login_cont_id = ntohs(id_be);
    conn->login_cont_len = dsi->cmdlen - sizeof(id_be);

    if (conn->login_cont_len > sizeof(dsi->commands) - sizeof(id_be)) {
        conn->login_cont_len = sizeof(dsi->commands) - sizeof(id_be);
    }

    memcpy(conn->login_cont_data, dsi->commands + sizeof(id_be),
           conn->login_cont_len);
}

/* ---------------------------- */
/* FPLoginExt wire layout per AFP spec §FPLoginExt and netatalk server
 * parser in etc/afpd/auth.c:778-903. UserAuthInfo is opaque to this
 * function; callers pass the UAM-specific blob. */
unsigned int AFPopenLoginExt(CONN *conn,
                             const char *vers, const char *uam,
                             const char *usr,
                             const void *auth_info, size_t auth_info_len)
{
    uint8_t len;
    uint16_t temp16;
    size_t usr_len;
    int ofs;
    DSI *dsi = &conn->dsi;
    assert(conn && vers && uam && usr);
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (!conn->use_legacy_transport) {
        int old_socket = dsi->socket;
        afptest_libafpclient_clear_pending(conn);
        afptest_libafpclient_clear_replies(conn);
        unsigned int ret = afptest_libafpclient_login(conn, LastHost, LastPort, vers,
                           uam, usr, auth_info ? (const char *)auth_info : "");

        if (old_socket > 0 && old_socket != dsi->socket) {
            CloseClientSocket(old_socket);
        }

        return ret;
    }

#endif

    if (DSIOpenSession(conn)) {
        return dsi->header.dsi_code;
    }

    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_LOGIN_EXT;
    dsi->commands[ofs++] = 0;
    temp16 = 0;
    memcpy(&dsi->commands[ofs], &temp16, sizeof(temp16));
    ofs += sizeof(temp16);
    len = (uint8_t) strnlen(vers, UINT8_MAX);
    dsi->commands[ofs++] = len;
    assert((size_t)ofs + len <= DSI_CMDSIZ);
    memcpy(&dsi->commands[ofs], vers, len);
    ofs += len;
    len = (uint8_t) strnlen(uam, UINT8_MAX);
    dsi->commands[ofs++] = len;
    assert((size_t)ofs + len <= DSI_CMDSIZ);
    memcpy(&dsi->commands[ofs], uam, len);
    ofs += len;
    /* UserNameType = 3 (UTF-8), UserName as AFPName (uint16_t length). */
    usr_len = strnlen(usr, UINT16_MAX);
    assert((size_t)ofs + 3 + usr_len <= DSI_CMDSIZ);
    dsi->commands[ofs++] = 3;
    temp16 = htons((uint16_t) usr_len);
    memcpy(&dsi->commands[ofs], &temp16, sizeof(temp16));
    ofs += sizeof(temp16);
    memcpy(&dsi->commands[ofs], usr, usr_len);
    ofs += usr_len;
    /* PathType = 3, empty directory service pathname. */
    dsi->commands[ofs++] = 3;
    temp16 = 0;
    memcpy(&dsi->commands[ofs], &temp16, sizeof(temp16));
    ofs += sizeof(temp16);

    /* Pad to even offset before UserAuthInfo. The server checks pointer
     * alignment (auth.c:900); this matches as long as dsi->commands starts
     * at an even address, which is true for the heap-allocated CONN. */
    if (ofs & 1) {
        dsi->commands[ofs++] = 0;
    }

    if (auth_info && auth_info_len) {
        assert((size_t)ofs + auth_info_len <= DSI_CMDSIZ);
        memcpy(&dsi->commands[ofs], auth_info, auth_info_len);
        ofs += auth_info_len;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_cmd_receive(dsi);

    if (dsi->header.dsi_code == htonl((uint32_t) AFPERR_AUTHCONT)) {
        capture_login_cont(conn);
    }

    return dsi->header.dsi_code;
}

/* ---------------------------- */
unsigned int AFPopenLoginExt_pwd(CONN *conn,
                                 const char *vers, const char *uam,
                                 const char *usr, const char *pwd)
{
    uint8_t pwbuf[PASSWDLEN];
    assert(conn && vers);
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (!conn->use_legacy_transport) {
        DSI *dsi = &conn->dsi;
        int old_socket = dsi->socket;
        afptest_libafpclient_clear_pending(conn);
        afptest_libafpclient_clear_replies(conn);
        unsigned int ret = afptest_libafpclient_login(conn, LastHost, LastPort, vers,
                           uam, usr ? usr : "", pwd ? pwd : "");

        if (old_socket > 0 && old_socket != dsi->socket) {
            CloseClientSocket(old_socket);
        }

        return ret;
    }

#endif

    /* The Cleartxt Passwrd UAM over FPLoginExt has been broken in netatalk
     * for years; afpfs-ng works around it by always using FPLogin for
     * cleartext. Mirror that here so the wrapper still works for callers
     * that don't care which AFP command is used under the hood. */
    if (uam && strcmp(uam, "Cleartxt Passwrd") == 0) {
        return AFPopenLogin(conn, vers, uam, usr ? usr : "", pwd ? pwd : "");
    }

    memset(pwbuf, 0, sizeof(pwbuf));

    if (pwd) {
        size_t pwlen = strnlen(pwd, PASSWDLEN);
        memcpy(pwbuf, pwd, pwlen);
    }

    /* No User Authent / empty user: server expects no UserAuthInfo at all,
     * matching the historical "if (len16)" gate this wrapper replaces. */
    if (!usr || !*usr) {
        return AFPopenLoginExt(conn, vers, uam, "", NULL, 0);
    }

    return AFPopenLoginExt(conn, vers, uam, usr, pwbuf, PASSWDLEN);
}

/* ---------------------------- */
/* FPLoginCont wire layout per AFP spec §FPLoginCont and server parser at
 * etc/afpd/auth.c:922-944. Caller must have populated conn->login_cont_id
 * from a previous FPLoginExt/FPLoginCont that returned kFPAuthContinue. */
unsigned int AFPLoginCont(CONN *conn,
                          const void *auth_info, size_t auth_info_len)
{
    uint16_t id_be;
    int ofs;
    DSI *dsi = &conn->dsi;
    assert(conn);
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_LOGINCONT;
    dsi->commands[ofs++] = 0;
    id_be = htons(conn->login_cont_id);
    memcpy(&dsi->commands[ofs], &id_be, sizeof(id_be));
    ofs += sizeof(id_be);

    if (auth_info && auth_info_len) {
        assert((size_t)ofs + auth_info_len <= DSI_CMDSIZ);
        memcpy(&dsi->commands[ofs], auth_info, auth_info_len);
        ofs += auth_info_len;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_cmd_receive(dsi);

    if (dsi->header.dsi_code == htonl((uint32_t) AFPERR_AUTHCONT)) {
        capture_login_cont(conn);
    }

    return dsi->header.dsi_code;
}

/* --------------------------- */
unsigned int AFPChangePW(CONN *conn, char *uam, char *usr, char *opwd,
                         char *pwd)
{
    uint8_t len;
    int ofs;
    DSI *dsi;
    assert(conn && uam && usr);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CHANGEPW;
    dsi->commands[ofs++] = 0;
    len = (uint8_t) strlen(uam);
    dsi->commands[ofs++] = len;
    memcpy(&dsi->commands[ofs], uam, len);
    ofs += len;

    if (ofs & 1) {
        dsi->commands[ofs++] = 0;
    }

    len = (uint8_t) strlen(usr);

    if (len) {
        dsi->commands[ofs++] = len;
        memcpy(&dsi->commands[ofs], usr, len);
        ofs += len;
        len = (uint8_t) strlen(opwd);

        if (ofs & 1) {
            dsi->commands[ofs++] = 0;
        }

        memcpy(&dsi->commands[ofs], opwd, len);
        ofs += PASSWDLEN;
        len = (uint8_t) strlen(pwd);
        memcpy(&dsi->commands[ofs], pwd, len);
        ofs += PASSWDLEN;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}
/* ------------------------------- */
unsigned int AFPLogOut(CONN *conn)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (conn->transport && !conn->use_legacy_transport) {
        afptest_libafpclient_clear_pending(conn);
        afptest_libafpclient_clear_replies(conn);
        afptest_libafpclient_logout(conn);
        afptest_libafpclient_close(conn);
        dsi->header.dsi_code = 0;
        return 0;
    }

#endif
    SendCmd(dsi, AFP_LOGOUT);
    dsi_full_receive(dsi, dsi->commands, DSI_CMDSIZ);
    DSICloseSession(conn);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPzzz(CONN *conn, int flag)
{
    int 		ofs = 0;
    DSI			*dsi = &conn->dsi;
    uint32_t   temp;
    assert(conn);
    SendInit(dsi);
    dsi->commands[ofs++] = AFP_ZZZ;
    dsi->commands[ofs++] = 0;
    temp = flag;
    temp = htonl(temp);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPGetSrvrInfo(CONN *conn)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmd(dsi, AFP_GETSRVINFO);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPGetSrvrParms(CONN *conn)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmd(dsi, AFP_GETSRVPARAM);
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPGetSrvrMsg(CONN *conn, uint16_t type, uint16_t bitmap)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETSRVRMSG;
    dsi->commands[ofs++] = 0;
    type = htons(type);
    memcpy(dsi->commands + ofs, &type, sizeof(type));
    ofs += sizeof(type);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------  */
unsigned int AFPCloseVol(CONN *conn, uint16_t vol)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmdWithU16(dsi, AFP_CLOSEVOL, vol);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------  */
unsigned int AFPCloseDT(CONN *conn, uint16_t vol)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmdWithU16(dsi, AFP_CLOSEDT, vol);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPCloseFork(CONN *conn, uint16_t fork)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmdWithU16(dsi, AFP_CLOSEFORK, fork);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPByteLock(CONN *conn, uint16_t fork, int end, int mode,
                         int offset, int size)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_BYTELOCK;
    dsi->commands[ofs++] = (end ? 0x80 : 0) | (mode ? 1 : 0);
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));
    ofs += sizeof(fork);
    offset = htonl(offset);
    memcpy(dsi->commands + ofs, &offset, sizeof(offset));
    ofs += sizeof(offset);
    size = htonl(size);
    memcpy(dsi->commands + ofs, &size, sizeof(size));
    ofs += sizeof(size);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ---------------------------- */
static off_t get_off_t(unsigned char **ibuf, int is64)
{
    uint32_t             temp;
    off_t                 ret;
    memcpy(&temp, *ibuf, sizeof(temp));
    ret = ntohl(temp); /* ntohl is unsigned */
    *ibuf += sizeof(temp);

    if (is64) {
        memcpy(&temp, *ibuf, sizeof(temp));
        *ibuf += sizeof(temp);
        ret = ntohl(temp) | (ret << 32);
    } else {
        ret = (int)ret; /* sign extend */
    }

    return ret;
}

/* ------------------------------- */
static int set_off_t(off_t offset, uint8_t *rbuf, int is64)
{
    uint32_t  temp;
    int        ret;
    ret = 0;

    if (is64) {
        temp = htonl(offset >> 32);
        memcpy(rbuf, &temp, sizeof(temp));
        rbuf += sizeof(temp);
        ret = sizeof(temp);
        offset &= 0xffffffff;
    }

    temp = htonl((uint32_t) offset);
    memcpy(rbuf, &temp, sizeof(temp));
    ret += sizeof(temp);
    return ret;
}

/* ------------------------------- */
unsigned int AFPByteLock_ext(CONN *conn, uint16_t fork, int end, int mode,
                             off_t offset, off_t size)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_BYTELOCK_EXT;
    dsi->commands[ofs++] = (end ? 0x80 : 0) | (mode ? 1 : 0);
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));
    ofs += sizeof(fork);
    ofs += set_off_t(offset, dsi->commands + ofs, 1);
    ofs += set_off_t(size, dsi->commands + ofs, 1);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPSetForkParam(CONN *conn, uint16_t fork,  uint16_t bitmap,
                             off_t size)
{
    int ofs;
    DSI *dsi;
    int is64 = bitmap & ((1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN));
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_SETFORKPARAM;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap)); /* bitmap */
    ofs += sizeof(bitmap);
    ofs += set_off_t(size, dsi->commands + ofs, is64);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPFlush(CONN *conn, uint16_t vol)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmdWithU16(dsi, AFP_FLUSH, vol);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------  */
unsigned int AFPFlushFork(CONN *conn, uint16_t vol)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendCmdWithU16(dsi, AFP_FLUSHFORK, vol);
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------  */
uint16_t AFPOpenVol(CONN *conn, char *vol, uint16_t bitmap)
{
    int ofs;
    uint16_t result, volID = 0;
    uint8_t len;
    DSI *dsi;
    assert(conn && vol);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_OPENVOL;
    dsi->commands[ofs++] = 0;
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += 2;
    len = (uint8_t) strlen(vol);
    dsi->commands[ofs++] = len;
    memcpy(&dsi->commands[ofs], vol, len);
    ofs += len;

    /* pad to an even boundary */
    if ((len + 1) & 1) {
        ofs++;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);

    if (!dsi->header.dsi_code) {
        ofs = 0;
        /* copy of bitmap */
        memcpy(&result, dsi->commands, sizeof(result));
        ofs += sizeof(result);
        result = ntohs(result);

        /* XXXX need to fully parse this stuff */
        if (result & (1 << VOLPBIT_ATTR)) {
            /* volume attribute  */
            memcpy(&result, dsi->commands, sizeof(result));
            ofs += sizeof(result);
        }

        /*  volume ID */
        memcpy(&result, dsi->commands + ofs, sizeof(result));
        volID = result;
    }

    return volID;
}

/*! Converts Pascal string to C (null-terminated) string. */
int strp2c(char *cstr, const unsigned char *pstr)
{
    int i;
    assert(cstr && pstr);

    for (i = 0; i < pstr[0]; i++) {
        cstr[i] = pstr[i + 1];
    }

    cstr[i] = '\0';
    return i;
}

/*! Our malloc wrapper. It zeroes allocated memory. */
void *fp_malloc(size_t size)
{
    void *ret = malloc(size);

    if (ret == NULL) {
        fprintf(stdout, "fp_malloc: out of memory !\n");
        abort();
    }

    memset(ret, 0, size);
    return ret;
}

/* -------------------------------  */
char *strp2cdup(unsigned char *src)
{
    assert(src);
    char *r = fp_malloc(*src + 1);
    strp2c(r, src);
    return r;
}

/* -------------------------------  */
void *fp_realloc(void *ptr, size_t size)
{
    void *ret = realloc(ptr, size);

    if (ret == NULL) {
        fprintf(stdout, "fp_realloc: out of memory !\n");
        abort();
    }

    return ret;
}

/*! Our free wrapper. It does nothing special at the moment. */
void fp_free(void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
    }
}

/* -------------------------------  */
void afp_volume_unpack(struct afp_volume_parms *parms, unsigned char *b,
                       uint16_t rbitmap)
{
    uint16_t i;
    uint32_t l;
    assert(parms && b);

    if (rbitmap & (1 << VOLPBIT_ATTR)) {
        memcpy(&i, b, sizeof(i));
        b += sizeof(i);
        parms->attr = ntohs(i);
    }

    if (rbitmap & (1 << VOLPBIT_SIG)) {
        memcpy(&i, b, sizeof(i));
        b += sizeof(i);
        parms->sig = ntohs(i);
    }

    if (rbitmap & (1 << VOLPBIT_CDATE)) {
        memcpy(&l, b, sizeof(l));
        b += sizeof(l);
        parms->cdate = ntohl(l);
    }

    if (rbitmap & (1 << VOLPBIT_MDATE)) {
        memcpy(&l, b, sizeof(l));
        b += sizeof(l);
        parms->mdate = ntohl(l);
    }

    if (rbitmap & (1 << VOLPBIT_BDATE)) {
        memcpy(&l, b, sizeof(l));
        b += sizeof(l);
        parms->bdate = ntohl(l);
    }

    if (rbitmap & (1 << VOLPBIT_VID)) {
        memcpy(&i, b, sizeof(i));
        b += sizeof(i);
        parms->vid = i;
    }

    if (rbitmap & (1 << VOLPBIT_BFREE)) {
        memcpy(&l, b, sizeof(l));
        b += sizeof(l);
        parms->bfree = ntohl(l);
    }

    if (rbitmap & (1 << VOLPBIT_BTOTAL)) {
        memcpy(&l, b, sizeof(l));
        b += sizeof(l);
        parms->btotal = ntohl(l);
    }

    if (rbitmap & (1 << VOLPBIT_NAME)) {
        parms->name = (char *)malloc(b[0] + 1);
        l = strp2c(parms->name, b) + 1;
        b += l;
    }

    if (rbitmap & (1 << VOLPBIT_XBFREE)) {
        /* FIXME */
    }

    if (rbitmap & (1 << VOLPBIT_XBTOTAL)) {
        /* FIXME */
    }

    if (rbitmap & (1 << VOLPBIT_BSIZE)) {
        memcpy(&l, b, sizeof(l));
        parms->bsize = ntohl(l);
    }
}

/*!
 * Only backup date is valid.
*/
int afp_volume_pack(unsigned char *b, struct afp_volume_parms *parms,
                    uint16_t bitmap)
{
    uint16_t i;
    uint32_t l;
    int bit = 0;
    const unsigned char *beg = b;
    assert(b && parms);

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {   /* Common parameters */
        case VOLPBIT_ATTR:
            memcpy(&i, b, sizeof(i));
            b += sizeof(i);
            parms->attr = ntohs(i);
            break;

        case VOLPBIT_SIG:
            /* FIXME */
            break;

        case VOLPBIT_CDATE:
            l = htonl(parms->cdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case VOLPBIT_MDATE:
            l = htonl(parms->mdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case VOLPBIT_BDATE:
            l = htonl(parms->bdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case VOLPBIT_VID:
            break;

        case VOLPBIT_BFREE:
            break;

        case VOLPBIT_BTOTAL:
            break;

        case VOLPBIT_NAME:
            break;

        case VOLPBIT_XBFREE:
            break;

        case VOLPBIT_XBTOTAL:
            break;

        case VOLPBIT_BSIZE:
            break;

        default:
            break;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

    return b - beg;
}

/* FIXME: redundant bitmap parameters !
 * FIXME: some of those parameters are not tested. */
void afp_filedir_unpack(CONN *conn, struct afp_filedir_parms *filedir,
                        const unsigned char *b,
                        uint16_t rfbitmap, uint16_t rdbitmap)
{
    int isdir;
    uint16_t i, j;
    uint32_t l;
    int r;
    int bit = 0;
    uint16_t bitmap;
    const unsigned char *beg = b;
    assert(conn && filedir && b);
    isdir = filedir->isdir;
    bitmap = isdir ? rdbitmap : rfbitmap;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {   /* Common parameters */
        case FILPBIT_ATTR: /* DIRPBIT_ATTR */
            memcpy(&i, b, sizeof(i));
            b += sizeof(i);
            filedir->attr = ntohs(i);
            break;

        case FILPBIT_PDID: /* DIRPBIT_PDID */
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->pdid = ntohl(l);
            break;

        case FILPBIT_CDATE: /* DIRPBIT_CDATE */
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->cdate = ntohl(l);
            break;

        case FILPBIT_MDATE: /* DIRPBIT_MDATE */
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->mdate = ntohl(l);
            break;

        case FILPBIT_BDATE: /* DIRPBIT_BDATE */
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->bdate = ntohl(l);
            break;

        case FILPBIT_FINFO: /* DIRPBIT_FINFO */
            memcpy(filedir->finder_info, b, 32);
            b += 32;
            break;

        case FILPBIT_LNAME: /* DIRPBIT_LNAME */
            memcpy(&i, b, sizeof(i));
            b += sizeof(i);
            i = ntohs(i);

            if (i != 0) {
                filedir->lname = fp_malloc((beg + i)[0] + 1);
                /* FIXME: what to do with this after Pascal to C conversion? */
                r = strp2c(filedir->lname, beg + i) + 1;
            }

            break;

        case FILPBIT_SNAME:	/* DIRPBIT_SNAME */
            memcpy(&i, b, sizeof(i));
            b += sizeof(i);
            i = ntohs(i);

            if (i != 0) {
                filedir->sname = fp_malloc((beg + i)[0] + 1);
                /* FIXME: what to do with this after Pascal to C conversion? */
                r = strp2c(filedir->sname, beg + i) + 1;
            }

            break;

        case FILPBIT_FNUM: /* DIRPBIT_DID */
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->did = l;
            break;

        case FILPBIT_PDINFO: /* utf8 name */
            memcpy(filedir->pdinfo, b, sizeof(filedir->pdinfo));

            /* blindly try utf8 name */
            if (UNICODE(conn)) {
                memcpy(&i, b, sizeof(i));
                i = ntohs(i);

                if (i != 0) {
                    memcpy(&j, beg + i + 4, sizeof(j));
                    j = ntohs(j);

                    /* hack */
                    if (j && j < 512 && (filedir->utf8_name = fp_malloc(j + 1))) {
                        memcpy(filedir->utf8_name, beg + i + 6, j);
                        filedir->utf8_name[j] = 0;
                    }
                }
            }

            b += sizeof(filedir->pdinfo);
            break;

        case FILPBIT_UNIXPR:
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->uid = ntohl(l);
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->gid = ntohl(l);
            memcpy(&l, b, sizeof(l));
            b += sizeof(l);
            filedir->unix_priv = ntohl(l);
            memcpy(filedir->access, b, sizeof(filedir->access));
            b += sizeof(filedir->access);
            break;

        default: /* File specific parameters */
            if (!isdir) switch (bit) {
                case FILPBIT_DFLEN:
                    memcpy(&l, b, sizeof(l));
                    b += sizeof(l);
                    filedir->dflen = ntohl(l);
                    break;

                case FILPBIT_RFLEN:
                    memcpy(&l, b, sizeof(l));
                    b += sizeof(l);
                    filedir->rflen = ntohl(l);
                    break;

                default:
                    break;
                } else switch (bit) { /* Directory specific parametrs */
                case DIRPBIT_OFFCNT:
                    memcpy(&i, b, sizeof(i));
                    b += sizeof(i);
                    filedir->offcnt = ntohs(i);
                    break;

                case DIRPBIT_UID:
                    memcpy(&l, b, sizeof(l));
                    b += sizeof(l);
                    filedir->uid = ntohl(l);
                    break;

                case DIRPBIT_GID:
                    memcpy(&l, b, sizeof(l));
                    b += sizeof(l);
                    filedir->gid = ntohl(l);
                    break;

                case DIRPBIT_ACCESS:
                    memcpy(filedir->access, b, sizeof(filedir->access));
                    b += sizeof(filedir->access);
                    break;

                default:
                    break;
                }

            break;
        }

        bitmap = bitmap >> 1;
        bit++;
    }
}

/* ---------------------- */
int afp_filedir_pack(CONN *conn, unsigned char *b,
                     struct afp_filedir_parms *filedir,
                     uint16_t rfbitmap, uint16_t rdbitmap)
{
    int isdir;
    uint16_t i, tp;
    uint32_t l;
    int bit = 0;
    uint16_t bitmap;
    const unsigned char *beg = b;
    unsigned char *l_ofs = NULL;
    unsigned char *u_ofs = NULL;
    assert(conn && b && filedir);
    isdir = filedir->isdir;
    bitmap = isdir ? rdbitmap : rfbitmap;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {   /* Common parameters */
        case FILPBIT_ATTR: /* DIRPBIT_ATTR */
            i = htons(filedir->attr);
            memcpy(b, &i, sizeof(i));
            b += sizeof(i);
            break;

        case FILPBIT_PDID: /* DIRPBIT_PDID */
            l = htonl(filedir->pdid);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case FILPBIT_CDATE: /* DIRPBIT_CDATE */
            l = htonl(filedir->cdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case FILPBIT_MDATE: /* DIRPBIT_MDATE */
            l = htonl(filedir->mdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case FILPBIT_BDATE: /* DIRPBIT_BDATE */
            l = htonl(filedir->bdate);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            break;

        case FILPBIT_FINFO: /* DIRPBIT_FINFO */
            memcpy(b, filedir->finder_info, 32);
            b += 32;
            break;

        case FILPBIT_LNAME: /* DIRPBIT_LNAME */
            l_ofs = b;
            b += 2;
            break;

        case FILPBIT_SNAME:	/* DIRPBIT_SNAME */
        case FILPBIT_FNUM: /* DIRPBIT_DID */
            break;

        case FILPBIT_PDINFO: /* utf8 name */
            if (UNICODE(conn)) {
                u_ofs = b;
            } else {
                memcpy(b, filedir->pdinfo, sizeof(filedir->pdinfo));
            }

            b += sizeof(filedir->pdinfo);
            break;

        case FILPBIT_UNIXPR:
            l = htonl(filedir->uid);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            l = htonl(filedir->gid);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            l = htonl(filedir->unix_priv);
            memcpy(b, &l, sizeof(l));
            b += sizeof(l);
            memcpy(b, filedir->access, sizeof(filedir->access));
            b += sizeof(filedir->access);
            break;

        default: /* File specific parameters */
            if (!isdir) switch (bit) {
                case FILPBIT_DFLEN:
                case FILPBIT_RFLEN:
                    /* error */
                    break;

                default:
                    break;
                } else switch (bit) { /* Directory specific parametrs */
                case DIRPBIT_OFFCNT:
                    break;

                case DIRPBIT_UID:
                    l = htonl(filedir->uid);
                    memcpy(b, &l, sizeof(l));
                    b += sizeof(l);
                    break;

                case DIRPBIT_GID:
                    l = htonl(filedir->gid);
                    memcpy(b, &l, sizeof(l));
                    b += sizeof(l);
                    break;

                case DIRPBIT_ACCESS:
                    memcpy(b, filedir->access, sizeof(filedir->access));
                    b += sizeof(filedir->access);
                    break;

                default:
                    break;
                }

            break;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

    if (l_ofs) {
        i = htons(b - beg);
        memcpy(l_ofs, &i, sizeof(i));

        if (filedir->lname) {
            i = (uint16_t) strlen(filedir->lname);
            *b = (unsigned char) i;
            b++;
            memcpy(b, filedir->lname, i);
            b += i;
        } else {
            *b = 0;
            b++;
        }
    }

    if (u_ofs) {
        i = htons(b - beg);
        memcpy(u_ofs, &i, sizeof(i));
        l = htonl(0x8000103);
        memcpy(b, &l, sizeof(l));
        b += sizeof(l);

        if (filedir->utf8_name) {
            i = (uint16_t) strlen(filedir->utf8_name);
            tp = htons(i);
            memcpy(b, &tp, sizeof(tp));
            b += sizeof(tp);
            memcpy(b, filedir->utf8_name, i);
            b += i;
        } else {
            tp = 0;
            memcpy(b, &tp, sizeof(tp));
            b += sizeof(tp);
        }
    }

    return b - beg;
}

/* -------------------------------  */
unsigned int AFPGetVolParam(CONN *conn, uint16_t vol, uint16_t bitmap)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETVOLPARAM;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));	/* volume */
    ofs += sizeof(vol);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += 2;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------  */
unsigned int AFPSetVolParam(CONN *conn, uint16_t vol, uint16_t bitmap,
                            struct afp_volume_parms *parms)
{
    int ofs;
    DSI *dsi;
    uint16_t tp;
    assert(conn && parms);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_SETVOLPARAM;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));	/* volume */
    ofs += sizeof(vol);
    tp = htons(bitmap);
    memcpy(dsi->commands + ofs, &tp, sizeof(tp));
    ofs += sizeof(tp);
    ofs += afp_volume_pack(dsi->commands + ofs, parms, bitmap);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ---------------------
	FIXME for now, replace only / with 0. Should deal with ..
*/
void u2mac(uint8_t *dst, char *name, int len)
{
    assert(dst && name);

    while (len) {
        if (Convert && *name == '/') {
            *dst = 0;
        } else if (Convert && *name == '!') {
            *dst = '/';
        } else {
            *dst = *name;
        }

        dst++;
        name++;
        len--;
    }
}

/* ------------------------- */
int Force_type2;

int FPset_name(CONN *conn, int ofs, char *name)
{
    int len;
    uint16_t len16;
    uint32_t hint = htonl(kTextEncodingUTF8);
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;

    if (UNICODE(conn) && !Force_type2) {
        dsi->commands[ofs++] = 3;		/* long name */
        memcpy(&dsi->commands[ofs], &hint, sizeof(hint));
        ofs += sizeof(hint);
        len = (int) strlen(name);
        len16 = htons((uint16_t) len);
        memcpy(&dsi->commands[ofs], &len16, sizeof(len16));
        ofs += sizeof(len16);
        u2mac(&dsi->commands[ofs], name, len);
        ofs += len;
    } else {
        dsi->commands[ofs++] = 2;		/* long name */
        len = (int) strlen(name);
        dsi->commands[ofs++] = (uint8_t) len;
        u2mac(&dsi->commands[ofs], name, len);
        ofs += len;
    }

    return ofs;
}

/* ------------------------- */
unsigned int  AFPCreateFile(CONN *conn, uint16_t vol, char type, int did,
                            char *name)
{
    int ofs;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CREATEFILE;
    dsi->commands[ofs++] = type ? 0x80 : 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));	/* volume */
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did)); /* directory did */
    ofs += sizeof(did);
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPWriteHeader(DSI *dsi, uint16_t fork, int offset, int size,
                            char *data, char whence)
{
    int ofs;
    int rsize;
    uint32_t temp;
    assert(dsi);
    memset(dsi->commands, 0, DSI_CMDSIZ);
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_WRITE;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    ofs = 0;
    dsi->commands[ofs++] = AFP_WRITE;
    dsi->commands[ofs++] = whence;			/* 0 SEEK_SET, 0x80 SEEK_END */
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    temp = htonl(offset);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    rsize = htonl(size);
    memcpy(dsi->commands + ofs, &rsize, sizeof(rsize));
    ofs += sizeof(rsize);
    dsi->datalen = ofs;		/* 12*/
    dsi->header.dsi_len = htonl((uint32_t) dsi->datalen + size);
    dsi->header.dsi_code = htonl(ofs);
    dsi_stream_send(dsi, dsi->commands, ofs);
    dsi_stream_write(dsi, data, size);
    return 0;
}

/* ------------------------------- */
unsigned int AFPWriteFooter(DSI *dsi, uint16_t fork _U_, int offset, int size,
                            char *data _U_, char whence)
{
    uint32_t last;
    assert(dsi);
    dsi_cmd_receive(dsi);

    if (!dsi->header.dsi_code) {
        if (dsi->cmdlen != 4) {
            return -1;
        }

        memcpy(&last, dsi->commands, sizeof(last));
        last = ntohl(last);

        if (whence) {
            /* we don't know the size! */
            return (last >= size + offset) ? 0 : -1;
        }

        return (last == size + offset) ? 0 : -1;
    }

    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPWrite(CONN *conn, uint16_t fork, int offset, int size,
                      char *data, char whence)
{
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    AFPWriteHeader(dsi, fork, offset, size, data, whence);
    return AFPWriteFooter(dsi, fork, offset, size, data, whence);
}

/* ------------------------------- */
unsigned int AFPWrite_ext(CONN *conn, uint16_t fork, off_t offset, off_t size,
                          char *data, char whence)
{
    int ofs;
    DSI *dsi;
    off_t last;
    unsigned char *ptr;
    assert(conn);
    dsi = &conn->dsi;
    memset(dsi->commands, 0, DSI_CMDSIZ);
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_WRITE;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    ofs = 0;
    dsi->commands[ofs++] = AFP_WRITE_EXT;
    dsi->commands[ofs++] = whence;			/* 0 SEEK_SET, 0x80 SEEK_END */
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    ofs += set_off_t(offset, dsi->commands + ofs, 1);
    ofs += set_off_t(size, dsi->commands + ofs, 1);
    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl(dsi->datalen + size);
    dsi->header.dsi_code = htonl(ofs);
    dsi_stream_send(dsi, dsi->commands, ofs);
    dsi_stream_write(dsi, data, size);
    dsi_cmd_receive(dsi);

    if (!dsi->header.dsi_code) {
        if (dsi->cmdlen != 8) {
            return -1;
        }

        ptr = dsi->commands;
        last = get_off_t(&ptr, 1);

        if (whence) {
            /* we don't know the size! */
            return (last >= size + offset) ? 0 : -1;
        }

        return (last == size + offset) ? 0 : -1;
    }

    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPWrite_ext_async(CONN *conn, uint16_t fork, off_t offset,
                                off_t size, char *data, char whence)
{
    int ofs;
    DSI *dsi = &conn->dsi;
    assert(conn);
    memset(dsi->commands, 0, DSI_CMDSIZ);
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_WRITE;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    ofs = 0;
    dsi->commands[ofs++] = AFP_WRITE_EXT;
    dsi->commands[ofs++] = whence;			/* 0 SEEK_SET, 0x80 SEEK_END */
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    ofs += set_off_t(offset, dsi->commands + ofs, 1);
    ofs += set_off_t(size, dsi->commands + ofs, 1);
    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl(dsi->datalen + size);
    dsi->header.dsi_code = htonl(ofs);
    dsi_stream_send(dsi, dsi->commands, ofs);
    dsi_stream_write(dsi, data, size);
    return 0;
}

/* -------------------------------
	type : resource or data
*/
uint16_t  AFPOpenFork(CONN *conn, uint16_t vol, char type, uint16_t bitmap,
                      int did, char *name, uint16_t access)
{
    int ofs;
    uint16_t ofork = 0, result;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_OPENFORK;
    dsi->commands[ofs++] = type;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));	/* volume */
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did)); /* directory did */
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap)); /* bitmap */
    ofs += sizeof(bitmap);
    access =  htons(access);
    memcpy(dsi->commands + ofs, &access, sizeof(access)); /* access right */
    ofs += sizeof(access);
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);

    if (!dsi->header.dsi_code) {
        ofs = 0;
        memcpy(&result, dsi->commands, sizeof(result));			/* bitmap */
        ofs += sizeof(result);
        memcpy(&result, dsi->commands + ofs, sizeof(result));
        ofork = result;
    }

    return ofork;
}

/* ------------------------------- */
unsigned int AFPDelete(CONN *conn, uint16_t vol, int did, char *name)
{
    assert(conn && name);
    return  SendCmdVolDidCname(conn, AFP_DELETE, vol, did, name);
}

/* ------------------------------- */
unsigned int AFPGetComment(CONN *conn, uint16_t vol, int did, char *name)
{
    assert(conn && name);
    return  SendCmdVolDidCname(conn, AFP_GETCMT, vol, did, name);
}

/* ------------------------------- */
unsigned int AFPRemoveComment(CONN *conn, uint16_t vol, int did, char *name)
{
    assert(conn && name);
    return  SendCmdVolDidCname(conn, AFP_RMVCMT, vol, did, name);
}

/* ------------------------------- */
unsigned int AFPAddComment(CONN *conn, uint16_t vol, int did, char *name,
                           char *cmt)
{
    int ofs;
    uint8_t len;
    DSI *dsi;
    assert(conn && name && cmt);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_ADDCMT;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    ofs = FPset_name(conn, ofs, name);

    if (ofs & 1) {
        ofs++;
    }

    len = (uint8_t) strlen(cmt);
    dsi->commands[ofs++] = len;
    memcpy(dsi->commands + ofs, cmt, len);
    ofs += len;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPGetSessionToken(CONN *conn, int type, uint32_t time, int len,
                                char *token)
{
    int ofs;
    uint16_t tp = htons((uint16_t) type);
    uint32_t temp;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETSESSTOKEN;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &tp, sizeof(tp));
    ofs += sizeof(tp);

    if (type) {
        temp = htonl(len);
        memcpy(dsi->commands + ofs, &temp, sizeof(temp));
        ofs += sizeof(temp);

        if ((type == 3) || (type == 4)) {
            time = htonl(time);
            memcpy(dsi->commands + ofs, &time, sizeof(time));
            ofs += sizeof(time);
        }

        memcpy(dsi->commands + ofs, token, len);
        ofs += len;
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPDisconnectOldSession(CONN *conn, uint16_t type, int len,
                                     char *token)
{
    int ofs;
    uint16_t tp = htons(type);
    uint32_t temp;
    DSI *dsi;
    assert(conn && token);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_DISCTOLDSESS;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &tp, sizeof(tp));
    ofs += sizeof(tp);
    temp = htonl(len);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    memcpy(dsi->commands + ofs, token, len);
    ofs += len;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPGetUserInfo(CONN *conn, char flag, int id, uint16_t bitmap)
{
    int ofs;
    uint16_t type = htons(bitmap);
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETUSERINFO;
    memcpy(dsi->commands + ofs, &flag, sizeof(flag));
    ofs++;
    id = htonl(id);
    memcpy(dsi->commands + ofs, &id, sizeof(id));
    ofs += sizeof(id);
    memcpy(dsi->commands + ofs, &type, sizeof(type));
    ofs += sizeof(type);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPMapID(CONN *conn, char fn, int id)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_MAPID;
    memcpy(dsi->commands + ofs, &fn, sizeof(fn));
    ofs++;
    id = htonl(id);
    memcpy(dsi->commands + ofs, &id, sizeof(id));
    ofs += sizeof(id);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPMapName(CONN *conn, char fn, char *name)
{
    int ofs;
    uint16_t len, l;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_MAPNAME;
    memcpy(dsi->commands + ofs, &fn, sizeof(fn));
    ofs++;
    len = (uint16_t) strlen(name);

    switch (fn) {
    case 1:
    case 2:
        l = htons(len);
        memcpy(dsi->commands + ofs, &l, sizeof(l));
        ofs += sizeof(l);
        break;

    case 3:
    case 4:
    default:
        dsi->commands[ofs++] = (uint8_t) len;
        break;
    }

    memcpy(dsi->commands + ofs, name, len);
    ofs += len;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPBadPacket(CONN *conn, char fn, char *name)
{
    int ofs;
    uint16_t l;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_MAPNAME;
    memcpy(dsi->commands + ofs, &fn, sizeof(fn));
    ofs++;
    memcpy(dsi->commands + ofs, &l, sizeof(l));
    ofs += sizeof(l);
    memcpy(dsi->commands + ofs, name, sizeof(&name));
    ofs += sizeof(&name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* ------------------------------- */
unsigned int AFPReadHeader(DSI *dsi, uint16_t fork, int offset, int size,
                           char *data _U_)
{
    int ofs;
    uint32_t  temp;
    assert(dsi);
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_READ;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    offset = htonl(offset);
    memcpy(dsi->commands + ofs, &offset, sizeof(offset));
    ofs += sizeof(offset);
    temp = htonl(size);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    dsi->commands[ofs++] = 0;	/* NewLineMask */
    dsi->commands[ofs++] = 0;	/* NewLineChar */
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    return 0;
}

/* ------------------------------- */
unsigned int AFPReadFooter(DSI *dsi, uint16_t fork _U_, int offset _U_,
                           int size, char *data)
{
    int rsize;
    assert(dsi && data);
    dsi_cmd_receive(dsi);
    memcpy(data, dsi->commands, dsi->cmdlen);
    rsize =  ntohl(dsi->header.dsi_len);
    rsize -= dsi->cmdlen;
    dsi_stream_read(dsi, data + dsi->cmdlen, rsize);

    if (dsi->header.dsi_code) {
        return dsi->header.dsi_code;
    } else if (rsize + dsi->cmdlen == size) {
        return 0;
    } else {
        return -1;
    }
}

/* ------------------------------- */
unsigned int AFPRead(CONN *conn, uint16_t fork, int offset, int size,
                     char *data)
{
    DSI *dsi;
    assert(conn && data);
    dsi = &conn->dsi;
    AFPReadHeader(dsi, fork, offset, size, data);
    return AFPReadFooter(dsi, fork, offset, size, data);
}

/*!
 * @note Assume size < 2GB
*/
unsigned int AFPRead_ext(CONN *conn, uint16_t fork, off_t offset, off_t size,
                         char *data)
{
    int ofs;
    int rsize;
    DSI *dsi;
    assert(conn && data);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_READ_EXT;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    ofs += set_off_t(offset, dsi->commands + ofs, 1);
    ofs += set_off_t(size, dsi->commands + ofs, 1);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
#if 0
    dsi_stream_receive(dsi, dsi->data, DSI_DATASIZ, &dsi->datalen);
    dump_header(dsi);
    rsize =  ntohl(dsi->header.dsi_len);
    rsize -= dsi->datalen;

    while (rsize > 0) {
        int len = min(rsize, DSI_DATASIZ);

        if (dsi_stream_read(dsi, dsi->data, len) != len) {
            break;
        }

        rsize -= len;
    }

#endif
    dsi_cmd_receive(dsi);
    memcpy(data, dsi->commands, dsi->cmdlen);
    rsize =  ntohl(dsi->header.dsi_len);
    rsize -= dsi->cmdlen;
    dsi_stream_read(dsi, data + dsi->cmdlen, rsize);

    if (dsi->header.dsi_code) {
        return dsi->header.dsi_code;
    } else if (rsize + dsi->cmdlen == size) {
        return 0;
    } else {
        return -1;
    }
}

unsigned int AFPRead_ext_async(CONN *conn, uint16_t fork, off_t offset,
                               off_t size, char *data _U_)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_READ_EXT;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    ofs += set_off_t(offset, dsi->commands + ofs, 1);
    ofs += set_off_t(size, dsi->commands + ofs, 1);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    return 0;
}

/* -------------------------------- */
unsigned int  AFPCreateDir(CONN *conn, uint16_t vol, int did, char *name)
{
    int ofs;
    unsigned int dir = 0;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CREATEDIR;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);

    if (!dsi->header.dsi_code) {
        memcpy(&dir, dsi->commands, sizeof(dir));			/* did */

        if (!Quiet) {
            fprintf(stdout, "directory ID 0x%x\n", ntohl(dir));
        }
    }

    return dir;
}

/* ------------------------------- */
unsigned int AFPGetForkParam(CONN *conn, uint16_t fork, uint16_t bitmap)
{
    int ofs;
    DSI *dsi;
    assert(conn);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETFORKPARAM;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &fork, sizeof(fork));	/* fork num */
    ofs += sizeof(fork);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap)); /* bitmap */
    ofs += sizeof(bitmap);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPGetAPPL(CONN *conn, uint16_t dt, char *name, uint16_t index,
                        uint16_t f_bitmap)
{
    int ofs;
    uint16_t bitmap;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETAPPL;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &dt, sizeof(dt));
    ofs += sizeof(dt);
    memcpy(dsi->commands + ofs, name, 4);
    ofs += 4;
    bitmap = htons(index);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    bitmap = htons(f_bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPAddAPPL(CONN *conn, uint16_t dt, int did, char *creator,
                        uint32_t tag, char *name)
{
    int ofs;
    DSI *dsi;
    assert(conn && creator && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_ADDAPPL;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &dt, sizeof(dt));
    ofs += sizeof(dt);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    memcpy(dsi->commands + ofs, creator, 4);
    ofs += 4;
    memcpy(dsi->commands + ofs, &tag, sizeof(tag));
    ofs += sizeof(tag);
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPRemoveAPPL(CONN *conn, uint16_t dt, int did, char *creator,
                           char *name)
{
    int ofs;
    DSI *dsi;
    assert(conn && creator && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_RMVAPPL;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &dt, sizeof(dt));
    ofs += sizeof(dt);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    memcpy(dsi->commands + ofs, creator, 4);
    ofs += 4;
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPCatSearch(CONN *conn, uint16_t vol, uint32_t nbe, char *pos,
                          uint16_t f_bitmap, uint16_t d_bitmap, uint32_t rbitmap,
                          struct afp_filedir_parms *filedir, struct afp_filedir_parms *filedir2)
{
    int ofs;
    int len;
    DSI *dsi;
    uint32_t temp;
    uint16_t bitmap;
    assert(conn && pos && filedir && filedir2);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CATSEARCH;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    temp = htonl(nbe);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    temp = 0;
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));	/* reserved */
    ofs += sizeof(temp);
    memcpy(dsi->commands + ofs, pos, 16);	/* cat pos (server stack)*/
    ofs += 16;
    bitmap = htons(f_bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    bitmap = htons(d_bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    temp = htonl(rbitmap);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    len = afp_filedir_pack(conn, dsi->commands + ofs + 2, filedir, rbitmap & 0xffff,
                           0);
    dsi->commands[ofs] = (uint8_t) len;
    ofs += len + 2;
    len = afp_filedir_pack(conn, dsi->commands + ofs + 2, filedir2,
                           rbitmap & 0xffff, 0);
    dsi->commands[ofs] = (uint8_t) len;
    ofs += len + 2;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPCatSearchExt(CONN *conn, uint16_t vol, uint32_t  nbe, char *pos,
                             uint16_t f_bitmap, uint16_t d_bitmap, uint32_t rbitmap,
                             struct afp_filedir_parms *filedir, struct afp_filedir_parms *filedir2)
{
    int ofs;
    int len;
    DSI *dsi;
    uint32_t temp;
    uint16_t bitmap;
    uint16_t i;
    assert(conn && pos && filedir && filedir2);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_CATSEARCH_EXT;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    temp = htonl(nbe);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    temp = 0;
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));	/* reserved */
    ofs += sizeof(temp);
    memcpy(dsi->commands + ofs, pos, 16);	/* cat pos (server stack)*/
    ofs += 16;
    bitmap = htons(f_bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    bitmap = htons(d_bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    temp = htonl(rbitmap);
    memcpy(dsi->commands + ofs, &temp, sizeof(temp));
    ofs += sizeof(temp);
    len = afp_filedir_pack(conn, dsi->commands + ofs + 2, filedir, rbitmap & 0xffff,
                           0);
    i = htons((uint16_t) len);
    memcpy(dsi->commands + ofs, &i, sizeof(i));
    ofs += len + 2;
    len = afp_filedir_pack(conn, dsi->commands + ofs + 2, filedir2,
                           rbitmap & 0xffff, 0);
    i = htons((uint16_t) len);
    memcpy(dsi->commands + ofs, &i, sizeof(i));
    ofs += len + 2;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPGetACL(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                       char *name)
{
    int ofs;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETACL;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    memset(dsi->commands + ofs, 0, 4);
    ofs += 4;
    ofs = FPset_name(conn, ofs, name);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* -------------------------------
*/
unsigned int AFPSetACL(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                       char *name, uint32_t ace_count, darwin_ace_t *aces)
{
    int ofs;
    DSI *dsi;
    assert(conn && name);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_SETACL;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    ofs = FPset_name(conn, ofs, name);

    /* Pad to even boundary */
    if (ofs & 1) {
        ofs++;
    }

    /* ACE count (network byte order) */
    uint32_t count_n = htonl(ace_count);
    memcpy(dsi->commands + ofs, &count_n, sizeof(uint32_t));
    ofs += sizeof(uint32_t);
    /* ACL flags (0 for our tests) */
    uint32_t acl_flags = 0;
    memcpy(dsi->commands + ofs, &acl_flags, sizeof(uint32_t));
    ofs += sizeof(uint32_t);

    /* ACEs (darwin_ace_t structures, already in network byte order) */
    if (ace_count > 0 && aces) {
        memcpy(dsi->commands + ofs, aces, ace_count * sizeof(darwin_ace_t));
        ofs += ace_count * sizeof(darwin_ace_t);
    }

    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_cmd_receive(dsi);
    return dsi->header.dsi_code;
}

/* --------------------------------
*/
unsigned int AFPGetExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                           int maxsize, char *pathname, char *attrname)
{
    int ofs;
    DSI *dsi;
    long long reqcount = -1;
    uint16_t len;
    assert(conn && pathname && attrname);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETEXTATTR;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    /* offset = 0, 8 bytes */
    memset(dsi->commands + ofs, 0, 8);
    ofs += 8;
    /* reqcount = -1, 8 bytes */
    reqcount = bswap_64(reqcount);
    memcpy(dsi->commands + ofs, &reqcount, sizeof(reqcount));
    ofs += sizeof(reqcount);
    /* max reply size */
    maxsize = htonl(maxsize);
    memcpy(dsi->commands + ofs, &maxsize, sizeof(maxsize));
    ofs += sizeof(maxsize);
    ofs = FPset_name(conn, ofs, pathname);

    /* pad to an even boundary */
    if (ofs & 1) {
        ofs++;
    }

    len = (uint16_t) strlen(attrname);
    len = htons(len);
    memcpy(dsi->commands + ofs, &len, sizeof(len));
    ofs += sizeof(len);
    len = ntohs(len);
    memcpy(&dsi->commands[ofs], attrname, len);
    ofs += len;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}


/* --------------------------------
*/
unsigned int AFPListExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                            int maxsize, char *pathname)
{
    int ofs;
    DSI *dsi;
    assert(conn && pathname);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_LISTEXTATTR;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    /* reqcount / startindex = 0, 6 bytes */
    memset(dsi->commands + ofs, 0, 6);
    ofs += 6;
    /* max reply size */
    maxsize = htonl(maxsize);
    memcpy(dsi->commands + ofs, &maxsize, sizeof(maxsize));
    ofs += sizeof(maxsize);
    ofs = FPset_name(conn, ofs, pathname);
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

/* --------------------------------
*/
unsigned int AFPSetExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                           char *pathname, char *attrname, char *data)
{
    int ofs;
    DSI *dsi;
    uint16_t len;
    uint32_t datalen;
    assert(conn && pathname && attrname && data);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_SETEXTATTR;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    /* offset = 0, 8 bytes */
    memset(dsi->commands + ofs, 0, 8);
    ofs += 8;
    ofs = FPset_name(conn, ofs, pathname);

    /* pad to an even boundary */
    if (ofs & 1) {
        ofs++;
    }

    len = (uint16_t) strlen(attrname);
    len = htons(len);
    memcpy(dsi->commands + ofs, &len, sizeof(len));
    ofs += sizeof(len);
    len = ntohs(len);
    memcpy(&dsi->commands[ofs], attrname, len);
    ofs += len;
    datalen = (uint32_t) strlen(data);
    datalen = htonl(datalen);
    memcpy(dsi->commands + ofs, &datalen, sizeof(datalen));
    ofs += sizeof(datalen);
    datalen = ntohl(datalen);
    memcpy(&dsi->commands[ofs], data, datalen);
    ofs += datalen;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}

unsigned int AFPRemoveExtAttr(CONN *conn, uint16_t vol, int did,
                              uint16_t bitmap, char *pathname, char *attrname)
{
    int ofs;
    DSI *dsi;
    uint16_t len;
    assert(conn && pathname && attrname);
    dsi = &conn->dsi;
    SendInit(dsi);
    ofs = 0;
    dsi->commands[ofs++] = AFP_REMOVEATTR;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(bitmap);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    ofs = FPset_name(conn, ofs, pathname);

    /* pad to an even boundary */
    if (ofs & 1) {
        ofs++;
    }

    len = (uint16_t) strlen(attrname);
    len = htons(len);
    memcpy(dsi->commands + ofs, &len, sizeof(len));
    ofs += sizeof(len);
    len = ntohs(len);
    memcpy(&dsi->commands[ofs], attrname, len);
    ofs += len;
    SetLen(dsi, ofs);
    dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    dsi_data_receive(dsi);
    return dsi->header.dsi_code;
}
