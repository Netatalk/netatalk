#ifndef TESTSUITE_LIBAFPCLIENT_H
#define TESTSUITE_LIBAFPCLIENT_H

#include <stdbool.h>

struct testsuite_libafpclient_session;

int testsuite_libafpclient_init(void);
unsigned int testsuite_libafpclient_uam_mask(const char *uam);
const char *testsuite_libafpclient_resolve_uam(const char *uam);
unsigned int testsuite_libafpclient_discover_uams(const char *host, int port,
        int afp_version,
        const char *user,
        const char *password,
        bool *known);
int testsuite_libafpclient_connect(struct testsuite_libafpclient_session
                                   **session,
                                   const char *host, int port,
                                   int afp_version, const char *uam,
                                   const char *user, const char *password);
unsigned int testsuite_libafpclient_supported_uams(
    const struct testsuite_libafpclient_session *session);
unsigned int testsuite_libafpclient_using_uam(
    const struct testsuite_libafpclient_session *session);
int testsuite_libafpclient_using_version(
    const struct testsuite_libafpclient_session *session);
int testsuite_libafpclient_smoke(struct testsuite_libafpclient_session
                                 *session);
void testsuite_libafpclient_close(
    struct testsuite_libafpclient_session **session);

#endif
