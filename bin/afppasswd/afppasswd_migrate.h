/*!
 * @file
 * @brief SRP verifier migration entry point for afppasswd
 */

#ifndef NETATALK_AFPPASSWD_MIGRATE_H
#define NETATALK_AFPPASSWD_MIGRATE_H

#include <sys/types.h>

int afppasswd_migrate_srp(const char *path, uid_t administrator_uid);

#endif /* NETATALK_AFPPASSWD_MIGRATE_H */
