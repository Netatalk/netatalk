/*
  Copyright (c) 2009-2010 Frank Lahm <franklahm@gmail.com>
  Copyright (c) 2025-2026 Daniel Markstedt <daniel@mindani.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * @brief Reducing ACLs to the entries that mirror the mode bits, in the VFS
 * layer
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_ACL_LIBACL_H
#include <acl/libacl.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/afp.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>

#ifdef HAVE_NFSV4_ACLS

/*!
 * @brief Keep only the owner@, group@, and everyone@ entries in the NFSv4 ACL
 * of name
 *
 * @param[in] name  file or directory
 * @returns AFP_OK, AFPERR_ACCESS, AFPERR_NOITEM for a missing name, or
 *          AFPERR_MISC
 */
int remove_nfsv4_acl_vfs(const char *name)
{
    int ret, i, ace_count, trivial_aces, new_aces_count;
    ace_t *old_aces = NULL;
    ace_t *new_aces = NULL;
    LOG(log_debug9, logtype_afpd, "remove_acl: BEGIN");

    /* Get existing ACL and count trivial ACEs */
    if ((ace_count = get_nfsv4_acl(name, &old_aces)) == -1) {
        return AFPERR_MISC;
    }

    trivial_aces = 0;

    for (i = 0; i < ace_count; i++) {
        if (old_aces[i].a_flags & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)) {
            trivial_aces++;
        }
    }

    /* malloc buffer for new ACL */
    if ((new_aces = malloc(trivial_aces * sizeof(ace_t))) == NULL) {
        LOG(log_error, logtype_afpd, "remove_acl: malloc %s", strerror(errno));
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Now copy the trivial ACEs */
    new_aces_count = 0;

    for (i = 0; i < ace_count; i++) {
        if (old_aces[i].a_flags  & (ACE_OWNER | ACE_GROUP | ACE_EVERYONE)) {
            memcpy(&new_aces[new_aces_count], &old_aces[i], sizeof(ace_t));
            new_aces_count++;
        }
    }

    if ((acl(name, ACE_SETACL, trivial_aces, new_aces)) == 0) {
        ret = AFP_OK;
    } else {
        LOG(log_error, logtype_afpd, "set_acl: error setting acl: %s", strerror(errno));

        if (errno == EACCES || errno == EPERM) {
            ret = AFPERR_ACCESS;
        } else if (errno == ENOENT) {
            ret = AFPERR_NOITEM;
        } else {
            ret = AFPERR_MISC;
        }
    }

exit:
    free(old_aces);
    free(new_aces);
    LOG(log_debug9, logtype_afpd, "remove_acl: END");
    return ret;
}

#endif  /* HAVE_NFSV4_ACLS */

#ifdef HAVE_POSIX_ACLS
/*!
 * @brief Clear permissions from a POSIX ACL entry according to mode bits
 *
 * Use the low three bits of bits to limit read, write, and execute
 * permissions. The entry belongs to the caller's ACL and changes in place.
 *
 * @param[in,out] entry  ACL entry to restrict
 * @param[in]     bits   permitted rights in the low three bits
 * @returns 0 on success, or -1 on an ACL error
 */
static int limit_posix_acl_entry(acl_entry_t entry, mode_t bits)
{
    acl_permset_t perms;

    if (acl_get_permset(entry, &perms) < 0
            || (!(bits & S_IROTH) && acl_delete_perm(perms, ACL_READ) < 0)
            || (!(bits & S_IWOTH) && acl_delete_perm(perms, ACL_WRITE) < 0)
            || (!(bits & S_IXOTH) && acl_delete_perm(perms, ACL_EXECUTE) < 0)) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Limit the ACL's effective permissions to the destination mode
 *
 * For an extended ACL, the mask bounds the owning group and named entries.
 * For a basic ACL, the owning group entry holds the group permissions.
 *
 * @param[in,out] acl   valid POSIX access ACL to restrict in place
 * @param[in]     mode  destination file mode
 * @returns 1 for an extended ACL, 0 for a basic ACL, or -1 with errno set
 */
static int limit_posix_acl(acl_t acl, mode_t mode)
{
    acl_entry_t entry, group_entry = NULL, mask_entry = NULL;
    int status;

    for (int id = ACL_FIRST_ENTRY; (status = acl_get_entry(acl, id, &entry)) == 1;
            id = ACL_NEXT_ENTRY) {
        acl_tag_t tag;

        if (acl_get_tag_type(entry, &tag) < 0) {
            return -1;
        }

        switch (tag) {
        case ACL_USER_OBJ:
        case ACL_OTHER:
            if (limit_posix_acl_entry(entry, tag == ACL_USER_OBJ ? mode >> 6 : mode) < 0) {
                return -1;
            }

            break;

        case ACL_GROUP_OBJ:
            group_entry = entry;
            break;

        case ACL_MASK:
            mask_entry = entry;
            break;

        case ACL_USER:
        case ACL_GROUP:
            /* The ACL mask limits effective permissions for named entries. */
            break;

        default:
            errno = EINVAL;
            return -1;
        }
    }

    if (status < 0
            || limit_posix_acl_entry(mask_entry ? mask_entry : group_entry,
                                     mode >> 3) < 0) {
        return -1;
    }

    return mask_entry != NULL;
}

/*!
 * @brief Apply a basic POSIX access ACL as file mode bits
 *
 * Convert the owner, group, and other entries to permission bits and apply
 * them with fchmod, carrying over the destination's special mode bits.
 *
 * @param[in] fd    open destination file descriptor, kept open
 * @param[in] acl   basic access ACL with owner, group, and other entries
 * @param[in] mode  destination mode supplying the special bits
 * @returns 0 on success, or -1 with errno set
 */
static int apply_basic_posix_acl_mode(int fd, acl_t acl, mode_t mode)
{
    acl_entry_t entry;
    int status;
    mode &= S_ISUID | S_ISGID | S_ISVTX;

    for (int id = ACL_FIRST_ENTRY; (status = acl_get_entry(acl, id, &entry)) == 1;
            id = ACL_NEXT_ENTRY) {
        acl_tag_t tag;
        acl_permset_t perms;

        if (acl_get_tag_type(entry, &tag) < 0 || acl_get_permset(entry, &perms) < 0) {
            return -1;
        }

#ifdef HAVE_ACL_GET_PERM_NP
        int read = acl_get_perm_np(perms, ACL_READ);
        int write = acl_get_perm_np(perms, ACL_WRITE);
        int execute = acl_get_perm_np(perms, ACL_EXECUTE);
#else
        int read = acl_get_perm(perms, ACL_READ);
        int write = acl_get_perm(perms, ACL_WRITE);
        int execute = acl_get_perm(perms, ACL_EXECUTE);
#endif

        if (read < 0 || write < 0 || execute < 0) {
            return -1;
        }

        mode_t bits = (read ? S_IROTH : 0) | (write ? S_IWOTH : 0)
                      | (execute ? S_IXOTH : 0);

        switch (tag) {
        case ACL_USER_OBJ:
            mode |= bits << 6;
            break;

        case ACL_GROUP_OBJ:
            mode |= bits << 3;
            break;

        case ACL_OTHER:
            mode |= bits;
            break;

        default:
            errno = EINVAL;
            return -1;
        }
    }

    return status < 0 ? -1 : fchmod(fd, mode);
}

/*!
 * @brief Copy a file's native POSIX access ACL to a newly created file
 *
 * A basic source ACL replaces any inherited ACL. Honor the destination's
 * creation mode so the copy cannot widen permissions. If the destination
 * does not support ACLs, apply a basic ACL as mode bits.
 *
 * @param[in] srcfd  open source data fork, kept open
 * @param[in] dstfd  open destination data fork, kept open
 * @returns 0 on success or when the source has no ACL support, or -1 with
 *          errno on error
 */
int copy_posix_acl(int srcfd, int dstfd)
{
    struct stat st;
    acl_t acl = acl_get_fd(srcfd);

    if (acl == NULL) {
        return errno == EOPNOTSUPP || errno == ENOTSUP ? 0 : -1;
    }

    int ret = -1;

    if (fstat(dstfd, &st) == 0) {
        int extended = limit_posix_acl(acl, st.st_mode);

        if (extended >= 0) {
            ret = acl_set_fd(dstfd, acl);

            if (ret < 0 && !extended && (errno == EOPNOTSUPP || errno == ENOTSUP)) {
                ret = apply_basic_posix_acl_mode(dstfd, acl, st.st_mode);
            }
        }
    }

    int saved_errno = errno;
    acl_free(acl);
    errno = saved_errno;
    return ret;
}

/*!
 * @brief Remove any ACL_USER, ACL_GROUP, ACL_MASK
 * or ACL_TYPE_DEFAULT ACEs from an object
 *
 * @param name  filesystem object name
 *
 * @returns AFP error code, AFP_OK (= 0) on success, AFPERR_MISC on error
 */
int remove_posix_acl_vfs(const char *name)
{
    EC_INIT;
    struct stat st;
    acl_t acl = NULL;
    acl_entry_t e;
    acl_tag_t tag;
    int entry_id = ACL_FIRST_ENTRY;
    /* Remove default ACL if it's a dir */
    EC_ZERO_ERR(stat(name, &st), AFPERR_MISC);

    if (S_ISDIR(st.st_mode)) {
        EC_NULL_LOG_ERR(acl = acl_init(0), AFPERR_MISC);
        EC_ZERO_LOG_ERR(acl_set_file(name, ACL_TYPE_DEFAULT, acl), AFPERR_MISC);
        EC_ZERO_LOG_ERR(acl_free(acl), AFPERR_MISC);
        acl = NULL;
    }

    /* Now get ACL and remove ACL_MASK, ACL_USER or ACL_GROUP entries, then re-set
     * the ACL again. acl_calc_mask() must not be called because there is no need
     * for an ACL_MASK entry in a basic ACL. */
    EC_NULL_LOG_ERR(acl = acl_get_file(name, ACL_TYPE_ACCESS), AFPERR_MISC);

    for (; acl_get_entry(acl, entry_id, &e) == 1; entry_id = ACL_NEXT_ENTRY) {
        EC_ZERO_LOG_ERR(acl_get_tag_type(e, &tag), AFPERR_MISC);

        if (tag == ACL_USER || tag == ACL_GROUP || tag == ACL_MASK) {
            EC_ZERO_LOG_ERR(acl_delete_entry(acl, e), AFPERR_MISC);
        }
    }

    EC_ZERO_LOG_ERR(acl_valid(acl), AFPERR_MISC);
    EC_ZERO_LOG_ERR(acl_set_file(name, ACL_TYPE_ACCESS, acl), AFPERR_MISC);
EC_CLEANUP:

    if (errno == ENOENT) {
        EC_STATUS(0);
    }

    if (acl) {
        acl_free(acl);
    }

    EC_EXIT;
}
#endif /* HAVE_POSIX_ACLS */
