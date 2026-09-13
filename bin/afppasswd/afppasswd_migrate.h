/*
 * Internal support for migrating the legacy flat SRP verifier file.
 */

#ifndef NETATALK_AFPPASSWD_MIGRATE_H
#define NETATALK_AFPPASSWD_MIGRATE_H

#include <sys/types.h>

int afppasswd_migrate_srp(const char *path, uid_t administrator_uid);

#endif /* NETATALK_AFPPASSWD_MIGRATE_H */
