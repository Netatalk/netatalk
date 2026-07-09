#ifndef UAMTEST_LIBAFPCLIENT_H
#define UAMTEST_LIBAFPCLIENT_H

#include <stdbool.h>

struct uamtest_session;

int uamtest_libafpclient_init(void);
unsigned int uamtest_libafpclient_uam_mask(const char *uam);
const char *uamtest_libafpclient_resolve_uam(const char *uam);
unsigned int uamtest_libafpclient_discover_uams(const char *host, int port,
        int afp_version,
        const char *user,
        const char *password,
        bool *known);
int uamtest_libafpclient_connect(struct uamtest_session **session,
                                 const char *host, int port,
                                 int afp_version, const char *uam,
                                 const char *user, const char *password);
unsigned int uamtest_libafpclient_supported_uams(
    const struct uamtest_session *session);
unsigned int uamtest_libafpclient_using_uam(
    const struct uamtest_session *session);
int uamtest_libafpclient_using_version(
    const struct uamtest_session *session);
int uamtest_libafpclient_smoke(struct uamtest_session *session);
void uamtest_libafpclient_close(struct uamtest_session **session);

#endif
