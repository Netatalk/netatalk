#ifndef AFPCLIENT_TRANSPORT_H
#define AFPCLIENT_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

struct CONN;

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
int afptest_libafpclient_login(struct CONN *conn, const char *host, int port,
                               const char *vers, const char *uam,
                               const char *user, const char *password);
int afptest_libafpclient_raw_command(struct CONN *conn,
                                     const void *payload, size_t payload_len,
                                     uint8_t dsi_command,
                                     uint32_t data_offset,
                                     uint32_t *dsi_code,
                                     void *reply, size_t reply_cap,
                                     size_t *reply_len);
int afptest_libafpclient_smoke(struct CONN *conn, const char *volume_name);
void afptest_libafpclient_logout(struct CONN *conn);
void afptest_libafpclient_close(struct CONN *conn);
#endif

#endif
