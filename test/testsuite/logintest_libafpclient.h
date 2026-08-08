#ifndef LOGINTEST_LIBAFPCLIENT_H
#define LOGINTEST_LIBAFPCLIENT_H

#include <stdbool.h>

struct logintest_libafpclient_session;

int logintest_libafpclient_init(void);
unsigned int logintest_libafpclient_uam_mask(const char *uam);
const char *logintest_libafpclient_resolve_uam(const char *uam);
unsigned int logintest_libafpclient_discover_uams(const char *host, int port,
        int afp_version,
        const char *user,
        const char *password,
        bool *known);
int logintest_libafpclient_connect(
    struct logintest_libafpclient_session **session,
    const char *host, int port,
    int afp_version, const char *uam,
    const char *user, const char *password);
unsigned int logintest_libafpclient_supported_uams(
    const struct logintest_libafpclient_session *session);
unsigned int logintest_libafpclient_using_uam(
    const struct logintest_libafpclient_session *session);
int logintest_libafpclient_using_version(
    const struct logintest_libafpclient_session *session);
int logintest_libafpclient_smoke(
    struct logintest_libafpclient_session *session);
void logintest_libafpclient_close(
    struct logintest_libafpclient_session **session);

#endif
