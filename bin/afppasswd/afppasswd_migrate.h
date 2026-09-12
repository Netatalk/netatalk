/*
 * Internal support for migrating the legacy flat SRP verifier file.
 */

#ifndef NETATALK_AFPPASSWD_MIGRATE_H
#define NETATALK_AFPPASSWD_MIGRATE_H

#include <sys/types.h>

struct afppasswd_migrate_hooks {
    int (*lookup_uid)(const char *name, uid_t *uid, void *context);
    ssize_t (*write_data)(int fd, const void *buf, size_t count,
                          void *context);
    int (*sync_fd)(int fd, void *context);
    void *context;
};

int afppasswd_migrate_srp(const char *path, uid_t administrator_uid,
                          const struct afppasswd_migrate_hooks *hooks);

#endif /* NETATALK_AFPPASSWD_MIGRATE_H */
