#ifndef AFPTEST_UAM_H
#define AFPTEST_UAM_H

#include <stdint.h>

typedef struct CONN CONN;

/* `-A clrtxt` or `-A "Cleartxt Passwrd"` selects each runner's legacy login path. */
int afptest_uam_uses_legacy_login(const char *uam);

unsigned int afptest_uam_login(CONN *conn, const char *vers,
                               const char *uam, const char *username,
                               const char *password);

#endif
