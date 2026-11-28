/*
 * Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*!
 * @file
 * @brief Volume scan of dbd: CNID and AppleDouble checks and repairs
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/compat.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/netatalk_conf.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "cmd_dbd.h"

/* Some defines to ease code parsing */
#define ADDIR_OK (addir_ok == 0)
#define ADFILE_OK (adfile_ok == 0)


static char           cwdbuf[MAXPATHLEN + 1];
static struct vol     *vol;
static dbd_flags_t    dbd_flags;
static char           stamp[ADEDLEN_PRIVSYN];
static char           *netatalk_dirs[] = {
    ".AppleDB",
    ".AppleDesktop",
    NULL
};
static char           *special_dirs[] = {
    ".zfs",
    NULL
};
static jmp_buf jmp;

/*!
 * @brief Tell whether a name is one of the netatalk folders .AppleDB and
 * .AppleDesktop
 *
 * @param[in] name  directory entry name
 * @returns the matching table entry, or NULL
 */
static const char *check_netatalk_dirs(const char *name)
{
    const int max_dirs = 2;

    for (int c = 0; c < max_dirs && netatalk_dirs[c]; c++) {
        if ((strcmp(name, netatalk_dirs[c])) == 0) {
            return netatalk_dirs[c];
        }
    }

    return NULL;
}

/*!
 * @brief Tell whether a name is a special directory that the scan leaves out,
 * such as .zfs
 *
 * @param[in] name  directory entry name
 * @returns the matching table entry, or NULL
 */
static const char *check_special_dirs(const char *name)
{
    const int max_dirs = 1;

    for (int c = 0; c < max_dirs && special_dirs[c]; c++) {
        if ((strcmp(name, special_dirs[c])) == 0) {
            return special_dirs[c];
        }
    }

    return NULL;
}

/*!
 * @brief Move the CNID of a renamed entry from its old name to its new name
 *
 * An entry that the database does not know needs no update.
 *
 * @param[in] did      parent directory ID
 * @param[in] sp       stat result of the entry
 * @param[in] oldname  name before the rename
 * @param[in] newname  name after the rename
 * @returns 0, or -1 when the update fails
 */
static int update_cnid(cnid_t did, const struct stat *sp, const char *oldname,
                       const char *newname)
{
    cnid_t id;

    /* Query the database */
    if ((id = cnid_lookup(vol->v_cdb, sp, did, (char *)oldname,
                          strlen(oldname))) == CNID_INVALID) {
        return errno == 0 || CNID_ERRNO() == CNID_ERR_NOTFOUND ? 0 : -1;
    }

    /* Update the database */
    if (cnid_update(vol->v_cdb, id, sp, did, (char *)newname,
                    strlen(newname)) < 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Check the AppleDouble file of an entry, or convert it to extended
 * attributes with -c
 *
 * On an adouble:ea volume the call converts the adouble:v2 data with -c and
 * otherwise does nothing. On an adouble:v2 volume a missing AppleDouble file
 * gets a log line. Outside -s, the call creates it with the Mac name and the
 * owner of the entry. An existing file has to open. Under -s it is opened
 * strictly read-only, so an empty header is reported rather than repaired.
 *
 * @param[in]  fname    entry name in the current directory
 * @param[in]  st       stat result of the entry
 * @param[out] newname  receives the new name when the conversion renames the
 *                      entry, or NULL
 * @returns 0, or -1 for an access error, a missing file during -s, or a file
 *          that fails to open or create
 */
static int check_adfile(const char *fname, const struct stat *st,
                        const char **newname)
{
    int adflags = ADFLAGS_HF;
    struct adouble ad;
    const char *adname;
    *newname = NULL;

    if (vol->v_adouble == AD_VERSION_EA) {
        if (!(dbd_flags & DBD_FLAGS_V2TOEA)) {
            return 0;
        }

        if (ad_convert(fname, st, vol, newname) != 0) {
            switch (errno) {
            case ENOENT:
                break;

            default:
                dbd_log(LOGSTD, "Conversion error for \"%s/%s\": %s", cwdbuf, fname,
                        strerror(errno));
                break;
            }
        }

        return 0;
    }

    if (S_ISDIR(st->st_mode)) {
        adflags |= ADFLAGS_DIR;
    }

    adname = vol->ad_path(fname, adflags);
    int fd = open(adname, O_RDONLY | O_NOFOLLOW);

    if (fd == -1) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error for ad-file '%s/%s': %s",
                    cwdbuf, adname, strerror(errno));
            return -1;
        }

        /* Missing. Log and create it */
        dbd_log(LOGSTD, "Missing AppleDouble file '%s/%s'", cwdbuf, adname);

        if (dbd_flags & DBD_FLAGS_SCAN) {
            /* Scan only requested, don't change anything */
            return -1;
        }

        /* Create ad file */
        ad_init(&ad, vol);

        if ((ad_open(&ad, fname, adflags | ADFLAGS_CREATE | ADFLAGS_RDWR,
                     0666)) != 0) {
            dbd_log(LOGSTD, "Error creating AppleDouble file '%s/%s': %s",
                    cwdbuf, adname, strerror(errno));
            return -1;
        }

        /* Set name in ad-file */
        ad_setname(&ad, convert_utf8_to_mac(vol, fname));
        ad_flush(&ad);
        ad_close(&ad, ADFLAGS_HF);
        fd = open(adname, O_RDWR | O_NOFOLLOW);

        if (fd != -1) {
            if (fchown(fd, st->st_uid, st->st_gid) < 0) {
                dbd_log(LOGSTD, "Could not chown \"%s\" (%s)", adname, strerror(errno));
            }

            close(fd);
        }
    } else {
        int oflags = (dbd_flags & DBD_FLAGS_SCAN)
                     ? ADFLAGS_RDONLY_STRICT : ADFLAGS_RDONLY;
        ad_init(&ad, vol);

        if (ad_open(&ad, fname, adflags | oflags) != 0) {
            dbd_log(LOGSTD, "Error opening AppleDouble file for '%s/%s'", cwdbuf, fname);
            close(fd);
            return -1;
        }

        ad_close(&ad, ADFLAGS_HF);
        close(fd);
    }

    return 0;
}

/*!
 * @brief Delete the extended attribute files of an entry from the .AppleDouble
 * directory
 *
 * The call deletes every file in .AppleDouble whose name contains
 * `name::EA`.
 *
 * @param[in] name  entry name
 * @param[in] ea    EA state of the entry, unused
 */
static void remove_eafiles(const char *name, struct ea *ea _U_)
{
    DIR *dp;
    int addir_fd;
    struct dirent *ep;
    char eaname[MAXPATHLEN];
    strlcpy(eaname, name, sizeof(eaname));

    if (strlcat(eaname, "::EA", sizeof(eaname)) >= sizeof(eaname)) {
        dbd_log(LOGSTD, "name too long: '%s/%s/%s'", cwdbuf, ADv2_DIRNAME, name);
        return;
    }

    addir_fd = open(ADv2_DIRNAME,
                    O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

    if (addir_fd < 0) {
        dbd_log(LOGSTD, "Couldn't open '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        return;
    }

    if ((dp = fdopendir(addir_fd)) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        close(addir_fd);
        return;
    }

    while ((ep = readdir(dp))) {
        /* fdopendir() transferred ownership to dp. Validate its descriptor
         * in the same iteration as the descriptor-relative operations. */
        const int dir_fd = dirfd(dp);

        if (dir_fd < 0) {
            dbd_log(LOGSTD, "Invalid directory descriptor for '%s/%s'",
                    cwdbuf, ADv2_DIRNAME);
            break;
        }

        if (strstr(ep->d_name, eaname) != NULL) {
            dbd_log(LOGSTD, "Removing EA file: '%s/%s/%s'",
                    cwdbuf, ADv2_DIRNAME, ep->d_name);

            if ((unlinkat(dir_fd, ep->d_name, 0)) != 0) {
                dbd_log(LOGSTD, "Error unlinking EA file '%s/%s/%s': %s",
                        cwdbuf, ADv2_DIRNAME, ep->d_name, strerror(errno));
            }
        } /* if */
    } /* while */

    closedir(dp);
}

/*!
 * @brief Check the extended attribute files of an entry against its EA header
 *
 * An EA whose file is missing, or whose size differs from the header, drops
 * out of the header. The call also deletes a file of the wrong size. Under
 * -s it reports those discrepancies without changing the EA or its header.
 *
 * @param[in] fname  entry name
 * @returns 0, or -1 when the EA header does not open
 */
static int check_eafiles(const char *fname)
{
    unsigned int  count = 0;
    int ret = 0, remove;
    struct ea ea;
    struct stat st;
    char *eaname;
    bool scan_only = (dbd_flags & DBD_FLAGS_SCAN) != 0;

    if ((ret = ea_open(vol, fname, scan_only ? EA_RDONLY : EA_RDWR, &ea)) != 0) {
        if (errno == ENOENT) {
            return 0;
        }

        dbd_log(LOGSTD, "Error calling ea_open for file: %s/%s",
                cwdbuf, fname);

        if (!scan_only) {
            remove_eafiles(fname, &ea);
        }

        return -1;
    }

    /* Check all EAs */
    while (count < ea.ea_count) {
        dbd_log(LOGDEBUG, "EA: %s", (*ea.ea_entries)[count].ea_name);
        remove = 0;
        eaname = ea_path(&ea, (*ea.ea_entries)[count].ea_name, 0);

        if (lstat(eaname, &st) != 0) {
            if (errno == ENOENT) {
                dbd_log(LOGSTD, "Missing EA: %s/%s", cwdbuf, eaname);
            } else {
                dbd_log(LOGSTD, "Bogus EA: %s/%s", cwdbuf, eaname);
            }

            remove = 1;
        } else if (st.st_size != (*ea.ea_entries)[count].ea_size) {
            dbd_log(LOGSTD, "Bogus EA: %s/%s", cwdbuf, eaname);
            remove = 1;

            if (!scan_only && unlink(eaname) != 0)
                dbd_log(LOGSTD, "Error removing EA file '%s/%s': %s",
                        cwdbuf, eaname, strerror(errno));
        }

        if (remove && !scan_only) {
            /* Be CAREFUL here! This should do what ea_delentry does. ea_close relies on it !*/
            free((*ea.ea_entries)[count].ea_name);
            (*ea.ea_entries)[count].ea_name = NULL;
        }

        count++;
    } /* while */

    ea_close(&ea);
    return ret;
}

/*!
 * @brief Tell whether an open error means that a non-directory holds the
 * .AppleDouble name
 *
 * A regular file gives ENOTDIR, and a symbolic link gives the O_NOFOLLOW
 * error of the platform.
 *
 * @param[in] err  errno of the failed open
 * @returns true for such an error
 */
static bool adouble_dir_unusable(int err)
{
    return err == ENOTDIR || err == OPEN_NOFOLLOW_ERRNO;
}

/* A same-directory rename in the scanned directory or .AppleDouble. */
struct dbd_rename {
    struct dbd_rename *next;
    int dirfd;
    char *oldname;
    char *newname;
};

/*!
 * @brief Check whether a rename destination is available
 *
 * Any existing entry occupies the destination, including a dangling symbolic
 * link. The call sets `errno` to `EEXIST` when it finds an entry.
 *
 * @param[in] fd    directory descriptor or `AT_FDCWD`
 * @param[in] name  destination name relative to that directory
 * @returns 0 when the lookup reports `ENOENT`, or -1 for an occupied
 *          destination or any other lookup error
 */
static int rename_destination(int fd, const char *name)
{
    struct stat st;

    if (fstatat(fd, name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
        errno = EEXIST;
        return -1;
    }

    return errno == ENOENT ? 0 : -1;
}

/*!
 * @brief Rename an entry within its directory if the destination is free
 *
 * The call uses an exclusive rename where supported. The fallback checks the
 * destination before calling `renameat()` and relies on the maintenance scan
 * running with other writers stopped.
 *
 * @param[in] fd       directory descriptor or `AT_FDCWD`
 * @param[in] oldname  source name relative to that directory
 * @param[in] newname  destination name relative to that directory
 * @returns 0 when the entry is renamed, or -1 on failure with `errno` set
 */
static int rename_unoccupied(int fd, const char *oldname, const char *newname)
{
#if defined(__APPLE__) && defined(RENAME_EXCL)

    if (renameatx_np(fd, oldname, fd, newname, RENAME_EXCL) == 0) {
        return 0;
    }

    if (errno != ENOTSUP && errno != EINVAL) {
        return -1;
    }

#elif defined(__linux__) && defined(RENAME_NOREPLACE)

    if (renameat2(fd, oldname, fd, newname, RENAME_NOREPLACE) == 0) {
        return 0;
    }

    if (errno != ENOSYS && errno != ENOTSUP && errno != EINVAL) {
        return -1;
    }

#endif

    /* The maintenance scan requires other writers to be stopped. */
    if (rename_destination(fd, newname) != 0) {
        return -1;
    }

    return renameat(fd, oldname, fd, newname);
}

/*!
 * @brief Queue a same-directory rename for later execution
 *
 * For an optional source, a lookup that reports `ENOENT` or `ENAMETOOLONG`
 * skips the rename. Otherwise the call checks the destination, copies both
 * names, and prepends the record. The caller keeps the directory descriptor
 * open while the record is in use.
 *
 * @param[in,out] pending   head of the pending rename list
 * @param[in]     fd        directory descriptor or `AT_FDCWD`
 * @param[in]     oldname   source name relative to that directory
 * @param[in]     newname   destination name relative to that directory
 * @param[in]     optional  whether to check for an optional metadata source
 * @returns 0 when queued or skipped, or -1 on failure with `errno` set
 */
static int queue_rename(struct dbd_rename **pending, int fd,
                        const char *oldname,
                        const char *newname, bool optional)
{
    struct stat st;
    struct dbd_rename *entry;

    if (optional && fstatat(fd, oldname, &st, AT_SYMLINK_NOFOLLOW) != 0) {
        return errno == ENOENT || errno == ENAMETOOLONG ? 0 : -1;
    }

    if (rename_destination(fd, newname) != 0) {
        return -1;
    }

    entry = malloc(sizeof(*entry));

    if (entry == NULL) {
        return -1;
    }

    entry->oldname = strdup(oldname);
    entry->newname = strdup(newname);

    if (entry->oldname == NULL || entry->newname == NULL) {
        free(entry->oldname);
        free(entry->newname);
        free(entry);
        return -1;
    }

    entry->dirfd = fd;
    entry->next = *pending;
    *pending = entry;
    return 0;
}

/*!
 * @brief Free a rename list and optionally undo its renames
 *
 * For rollback, the caller supplies completed renames in reverse execution
 * order. Each failed restore is logged and the call keeps processing the
 * remaining records.
 *
 * @param[in] entries   head of the list to free, or NULL
 * @param[in] rollback  whether to restore the original names before freeing
 * @returns 0, or -1 if any restore fails
 */
static int finish_renames(struct dbd_rename *entries, bool rollback)
{
    int ret = 0;

    while (entries != NULL) {
        struct dbd_rename *entry = entries;
        entries = entry->next;

        if (rollback && rename_unoccupied(entry->dirfd, entry->newname,
                                          entry->oldname) != 0) {
            dbd_log(LOGSTD, "Can't restore '%s/%s' from '%s': %s", cwdbuf,
                    entry->oldname, entry->newname, strerror(errno));
            ret = -1;
        }

        free(entry->oldname);
        free(entry->newname);
        free(entry);
    }

    return ret;
}

/*!
 * @brief Queue .AppleDouble renames for a non-directory entry
 *
 * The call queues the v2 metadata file if present. With `ea = ad`, it reads
 * the directory stream and queues the matching `::EA` header and `::EA::`
 * files, retaining their suffixes. The caller keeps the stream open through
 * execution and rollback and frees any queued records on failure.
 *
 * @param[in,out] pending  head of the pending rename list
 * @param[in,out] dir      open .AppleDouble directory stream
 * @param[in]     oldname  current name of the volume entry
 * @param[in]     newname  desired name of the volume entry
 * @returns 0 when collection completes, or -1 on failure with `errno` set
 */
static int queue_adouble_renames(struct dbd_rename **pending, DIR *dir,
                                 const char *oldname, const char *newname)
{
    size_t oldlen = strlen(oldname);
    const struct dirent *ep;
    const int fd = dirfd(dir);

    if (fd < 0) {
        return -1;
    }

    if (queue_rename(pending, fd, oldname, newname, true) != 0) {
        return -1;
    }

    if (vol->v_vfs_ea != AFPVOL_EA_AD) {
        return 0;
    }

    for (;;) {
        errno = 0;
        ep = readdir(dir);

        if (ep == NULL) {
            return errno == 0 ? 0 : -1;
        }

        if (strncmp(ep->d_name, oldname, oldlen) != 0) {
            continue;
        }

        const char *suffix = ep->d_name + oldlen;

        if (strcmp(suffix, "::EA") != 0 && strncmp(suffix, "::EA::", 6) != 0) {
            continue;
        }

        char target[MAXPATHLEN + 1];
        int len = snprintf(target, sizeof(target), "%s%s", newname, suffix);

        if (len < 0 || (size_t)len >= sizeof(target)) {
            errno = ENAMETOOLONG;
            return -1;
        }

        if (queue_rename(pending, fd, ep->d_name, target, false) != 0) {
            return -1;
        }
    }
}

/*!
 * @brief Rename an entry and its external metadata, keeping its CNID
 *
 * The call queues the renames, moves external metadata before the entry,
 * and updates the CNID database. Directory-owned .AppleDouble metadata and
 * native extended attributes travel with the entry. On failure the call
 * attempts to restore completed renames in reverse order.
 *
 * @param[in] did      CNID of the parent directory
 * @param[in] st       stat result of the entry
 * @param[in] oldname  current name of the entry
 * @param[in] newname  desired name of the entry
 * @returns 1 when renamed, 0 when skipped because a destination is occupied
 *          or a name is too long, or -1 for a migration or rollback error
 */
static int rename_euro(cnid_t did, const struct stat *st, const char *oldname,
                       const char *newname)
{
    struct dbd_rename *pending = NULL, *moved = NULL;
    DIR *addir = NULL;
    char oldsidecar[MAXPATHLEN + 3], newsidecar[MAXPATHLEN + 3];
    int ret = -1;
    int saved_errno;
    bool catalog_error = false;

    if (queue_rename(&pending, AT_FDCWD, oldname, newname, false) != 0) {
        goto done;
    }

    /* Sidecars go first. Directory-owned .AppleDouble metadata and native
     * extended attributes travel with the entry. */
    snprintf(oldsidecar, sizeof(oldsidecar), "._%s", oldname);
    snprintf(newsidecar, sizeof(newsidecar), "._%s", newname);

    if (queue_rename(&pending, AT_FDCWD, oldsidecar, newsidecar, true) != 0) {
        goto done;
    }

    if (!S_ISDIR(st->st_mode)) {
        int fd = open(ADv2_DIRNAME, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

        if (fd >= 0) {
            addir = fdopendir(fd);

            if (addir == NULL) {
                saved_errno = errno;
                close(fd);
                errno = saved_errno;
                goto done;
            }

            if (queue_adouble_renames(&pending, addir, oldname, newname) != 0) {
                goto done;
            }
        } else if (errno != ENOENT && (vol->v_adouble == AD_VERSION2
                                       || !adouble_dir_unusable(errno))) {
            goto done;
        }
    }

    while (pending != NULL) {
        struct dbd_rename *entry = pending;

        if (rename_unoccupied(entry->dirfd, entry->oldname, entry->newname) != 0) {
            goto done;
        }

        pending = entry->next;
        entry->next = moved;
        moved = entry;
    }

    catalog_error = update_cnid(did, st, oldname, newname) != 0;
    ret = catalog_error ? -1 : 1;
done:
    saved_errno = errno;
    finish_renames(pending, false);

    if (finish_renames(moved, ret < 0) != 0) {
        ret = -1;
    } else if (ret < 0 && !catalog_error
               && (saved_errno == EEXIST || saved_errno == ENAMETOOLONG)) {
        ret = 0;
    }

    if (addir != NULL) {
        closedir(addir);
    }

    if (ret < 0) {
        dbd_log(LOGSTD, "Can't migrate '%s/%s' to '%s': %s", cwdbuf, oldname,
                newname, catalog_error ? "CNID update failed" : strerror(saved_errno));
    } else if (ret == 0) {
        dbd_log(LOGSTD, "Skipping euro migration '%s/%s' to '%s': %s", cwdbuf,
                oldname, newname, strerror(saved_errno));
    } else {
        dbd_log(LOGSTD, "Migrated '%s/%s' to '%s'", cwdbuf, oldname, newname);
    }

    return ret;
}

/*!
 * @brief Migrate UTF-8 currency signs in an entry name to euro signs
 *
 * The call replaces every currency sign and skips names whose replacement
 * exceeds the buffer. Under -s it reports the proposed rename without
 * changing the entry.
 *
 * @param[in]  did     CNID of the parent directory
 * @param[in]  st      stat result of the entry
 * @param[in]  name    current name of the entry
 * @param[out] target  buffer holding the new entry name on a return of 1
 * @param[in]  size    capacity of the buffer in bytes, including the terminator
 * @returns 1 when renamed, 0 when no rename takes place, or -1 on failure
 */
static int migrate_euro(cnid_t did, const struct stat *st, const char *name,
                        char *target, size_t size)
{
    const char *currency = "\xc2\xa4";
    const char *euro = "\xe2\x82\xac";
    const char *pos = strstr(name, currency);
    const char *src = name;
    char *dst = target;

    if (pos == NULL) {
        return 0;
    }

    do {
        size_t prefix = (size_t)(pos - src);

        if (prefix + 3 >= size) {
            goto too_long;
        }

        memcpy(dst, src, prefix);
        memcpy(dst + prefix, euro, 3);
        dst += prefix + 3;
        size -= prefix + 3;
        src = pos + 2;
        pos = strstr(src, currency);
    } while (pos != NULL);

    if (strlcpy(dst, src, size) >= size) {
        goto too_long;
    }

    if (dbd_flags & DBD_FLAGS_SCAN) {
        dbd_log(LOGSTD, "Euro migration candidate '%s/%s' to '%s'", cwdbuf,
                name, target);
        return 0;
    }

    return rename_euro(did, st, name, target);
too_long:
    dbd_log(LOGSTD, "Skipping euro migration '%s/%s': %s", cwdbuf, name,
            strerror(ENAMETOOLONG));
    return 0;
}

/*!
 * @brief Check the .AppleDouble directory and its .Parent file in the current
 * directory, and create them when missing
 *
 * An adouble:ea volume needs neither. A non-directory under the .AppleDouble
 * name, which a non-AFP client may leave, skips the AppleDouble checks of the
 * directory. Under -s a missing directory fails and a missing .Parent passes.
 * Otherwise the call creates both with the Mac name of the directory and its
 * owner. A directory that the call created but cannot open again is an error.
 *
 * @param[in] volroot  nonzero in the volume root, unused
 * @returns 0, 1 to skip the AppleDouble checks of the directory, or -1 for an
 *          error
 */
static int check_addir(int volroot _U_)
{
    int addir_fd = -1;
    int parent_fd = -1;
    struct stat st;
    struct adouble ad;
    const char *mname = NULL;
    const char *ad_parent_path;

    if (vol->v_adouble == AD_VERSION_EA) {
        return 0;
    }

    /* Check for ad-dir */
    addir_fd = open(ADv2_DIRNAME,
                    O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

    if (addir_fd < 0) {
        if (adouble_dir_unusable(errno)) {
            /* Left by a non-AFP client: skip this directory's AppleDouble
             * checks rather than fail the volume */
            dbd_log(LOGSTD,
                    "Skipping AppleDouble checks in '%s': %s is not a directory",
                    cwdbuf, ADv2_DIRNAME);
            return 1;
        }

        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Open error for directory %s/%s: %s", cwdbuf, ADv2_DIRNAME,
                    strerror(errno));
            return -1;
        }

        dbd_log(LOGSTD, "Missing %s for '%s'", ADv2_DIRNAME, cwdbuf);
    }

    /* Check for ".Parent" */
    ad_parent_path = vol->ad_path(".", ADFLAGS_DIR);

    if (addir_fd >= 0) {
        parent_fd = openat(addir_fd, ".Parent", O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    }

    if (parent_fd < 0) {
        if (addir_fd >= 0 && errno != ENOENT) {
            dbd_log(LOGSTD, "Open error on '%s/%s': %s",
                    cwdbuf, ad_parent_path, strerror(errno));

            if (addir_fd >= 0) {
                close(addir_fd);
            }

            return -1;
        }

        dbd_log(LOGSTD, "Missing .AppleDouble/.Parent for '%s'", cwdbuf);
    }

    /* Is one missing ? */
    if ((addir_fd < 0) || (parent_fd < 0)) {
        /* Yes, but are we only scanning ? */
        if (dbd_flags & DBD_FLAGS_SCAN) {
            /* Yes:  missing .Parent is not a problem, but missing ad-dir
               causes later checking of ad-files to fail. So we have to return appropriately */
            if (addir_fd < 0) {
                if (parent_fd >= 0) {
                    close(parent_fd);
                }

                return -1;
            } else { /* (parent_fd < 0) */
                close(addir_fd);
                return 0;
            }
        }

        /* Create ad dir and set name */
        ad_init(&ad, vol);

        if (ad_open(&ad, ".", ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_CREATE | ADFLAGS_RDWR,
                    0777) != 0) {
            dbd_log(LOGSTD, "Error creating AppleDouble dir in %s: %s", cwdbuf,
                    strerror(errno));

            if (addir_fd >= 0) {
                close(addir_fd);
            }

            if (parent_fd >= 0) {
                close(parent_fd);
            }

            return -1;
        }

        /* Get basename of cwd from cwdbuf */
        mname = convert_utf8_to_mac(vol, strrchr(cwdbuf, '/') + 1);
        /* Update name in ad file */
        ad_setname(&ad, mname);
        ad_flush(&ad);
        ad_close(&ad, ADFLAGS_HF);

        /* Inherit owner/group from "." to ".AppleDouble" and ".Parent" */
        if ((lstat(".", &st)) != 0) {
            dbd_log(LOGSTD, "Couldn't stat %s: %s", cwdbuf, strerror(errno));

            if (addir_fd >= 0) {
                close(addir_fd);
            }

            if (parent_fd >= 0) {
                close(parent_fd);
            }

            return -1;
        }

        /* If directories were created, we need their new file descriptors */
        if (addir_fd < 0) {
            addir_fd = open(ADv2_DIRNAME,
                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

            if (addir_fd < 0) {
                /* parent_fd is still -1: .Parent opens only through an open
                 * .AppleDouble */
                dbd_log(LOGSTD,
                        "Couldn't open newly created directory \"%s\": %s",
                        ADv2_DIRNAME, strerror(errno));
                return -1;
            }

            if (fchown(addir_fd, st.st_uid, st.st_gid) < 0) {
                dbd_log(LOGSTD, "fchown failed on fd for \"%s\"", ADv2_DIRNAME);
            }
        }

        if (parent_fd < 0) {
            parent_fd = openat(addir_fd, ".Parent",
                               O_RDONLY | O_NOFOLLOW | O_CLOEXEC);

            if (parent_fd >= 0) {
                if (fchown(parent_fd, st.st_uid, st.st_gid) < 0) {
                    dbd_log(LOGSTD, "fchown failed on fd for \"%s\"", ad_parent_path);
                }
            } else {
                dbd_log(LOGSTD, "Couldn't open newly created file \"%s\"", ad_parent_path);
            }
        }
    }

    if (addir_fd >= 0) {
        close(addir_fd);
    }

    if (parent_fd >= 0) {
        close(parent_fd);
    }

    return 0;
}

/*!
 * @brief Check whether a .AppleDouble entry is an EA file and whether its data
 * file still exists
 *
 * Only a volume with `ea = ad` has EA files, and on others the call returns
 * 0. An EA file whose data file is gone gets a log line and goes away, except
 * under -s.
 *
 * @param[in] parent_fd  open current directory
 * @param[in] addir_fd   open .AppleDouble directory
 * @param[in] name       entry name in .AppleDouble
 * @returns 0 for a name without `::EA`, 1 for an EA file with its data file, or
 *          -1 for an orphan or an access error
 */
static int check_eafile_in_adouble(int parent_fd, int addir_fd,
                                   const char *name)
{
    int ret = 0;
    struct stat st;
    char *namep, *namedup = NULL;

    /* Check if this is an AFPVOL_EA_AD vol */
    if (vol->v_vfs_ea == AFPVOL_EA_AD) {
        /* Does the filename contain "::EA" ? */
        namedup = strdup(name);

        if ((namep = strstr(namedup, "::EA")) == NULL) {
            ret = 0;
            goto ea_check_done;
        } else {
            /* File contains "::EA" so it's an EA file. Check for data file  */
            /* Get string before "::EA" from EA filename */
            namep[0] = 0;

            if ((fstatat(parent_fd, namedup, &st, 0)) == 0) {
                ret = 1;
                goto ea_check_done;
            } else {
                ret = -1;

                if (errno != ENOENT) {
                    dbd_log(LOGSTD, "Access error for file '%s/%s': %s",
                            cwdbuf, name, strerror(errno));
                    goto ea_check_done;
                }

                /* Orphaned EA file*/
                dbd_log(LOGSTD, "Orphaned Extended Attribute file '%s/%s/%s'",
                        cwdbuf, ADv2_DIRNAME, name);

                if (dbd_flags & DBD_FLAGS_SCAN)
                    /* Scan only requested, don't change anything */
                {
                    goto ea_check_done;
                }

                if ((unlinkat(addir_fd, name, 0)) != 0) {
                    dbd_log(LOGSTD, "Error unlinking orphaned Extended Attribute file '%s/%s/%s'",
                            cwdbuf, ADv2_DIRNAME, name);
                }
            } /* if (access) */
        } /* if strstr */
    } /* if AFPVOL_EA_AD */

ea_check_done:

    if (namedup) {
        free(namedup);
    }

    return ret;
}

/*!
 * @brief Check the entries of the .AppleDouble directory of the current
 * directory
 *
 * Directories inside .AppleDouble only log, and EA files go to
 * check_eafile_in_adouble. An AppleDouble file whose data file is gone goes
 * away, except under -s. On an adouble:ea volume a missing or unusable
 * .AppleDouble passes.
 *
 * @returns 0, or -1 when a directory does not open
 */
static int read_addir(void)
{
    DIR *dp;
    int addir_fd;
    int parent_fd;
    int ret = 0;
    struct dirent *ep;
    struct stat st;
    parent_fd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (parent_fd < 0) {
        dbd_log(LOGSTD, "Couldn't open '%s': %s", cwdbuf, strerror(errno));
        return -1;
    }

    addir_fd = openat(parent_fd, ADv2_DIRNAME,
                      O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

    if (addir_fd < 0) {
        int saved_errno = errno;
        close(parent_fd);

        /* On an ea volume the directory is at most a v2 leftover, never a
         * reason to stop the rebuild */
        if (vol->v_adouble == AD_VERSION_EA) {
            if (saved_errno != ENOENT) {
                dbd_log(LOGSTD, "Skipping '%s/%s': %s",
                        cwdbuf, ADv2_DIRNAME, strerror(saved_errno));
            }

            return 0;
        }

        dbd_log(LOGSTD, "Couldn't open '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(saved_errno));
        return -1;
    }

    if ((dp = fdopendir(addir_fd)) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        close(addir_fd);
        close(parent_fd);
        return -1;
    }

    while ((ep = readdir(dp))) {
        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name)) {
            continue;
        }

        /* Skip ".Parent" */
        if (STRCMP(ep->d_name, ==, ".Parent")) {
            continue;
        }

        /* Use the stream's descriptor, checked within this iteration. */
        const int dir_fd = dirfd(dp);

        if (dir_fd < 0) {
            dbd_log(LOGSTD, "Invalid directory descriptor for '%s/%s'",
                    cwdbuf, ADv2_DIRNAME);
            ret = -1;
            break;
        }

        if ((fstatat(dir_fd, ep->d_name, &st, AT_SYMLINK_NOFOLLOW)) < 0) {
            dbd_log(LOGSTD,
                    "Lost file or dir while enumeratin dir '%s/%s/%s', probably removed: %s",
                    cwdbuf, ADv2_DIRNAME, ep->d_name, strerror(errno));
            continue;
        }

        /* Check for dirs */
        if (S_ISDIR(st.st_mode)) {
            dbd_log(LOGSTD, "Unexpected directory '%s' in AppleDouble dir '%s/%s'",
                    ep->d_name, cwdbuf, ADv2_DIRNAME);
            continue;
        }

        /* Check if for orphaned and corrupt Extended Attributes file */
        if (check_eafile_in_adouble(parent_fd, dir_fd, ep->d_name) != 0) {
            continue;
        }

        /* Check for data file */
        if ((fstatat(parent_fd, ep->d_name, &st, 0)) != 0) {
            if (errno != ENOENT) {
                dbd_log(LOGSTD, "Access error for file '%s/%s': %s",
                        cwdbuf, ep->d_name, strerror(errno));
                continue;
            }

            /* Orphaned ad-file*/
            dbd_log(LOGSTD, "Orphaned AppleDoube file '%s/%s/%s'",
                    cwdbuf, ADv2_DIRNAME, ep->d_name);

            /* Scan only requested, don't change anything */
            if (dbd_flags & DBD_FLAGS_SCAN) {
                continue;
            }

            if ((unlinkat(dir_fd, ep->d_name, 0)) != 0) {
                dbd_log(LOGSTD, "Error unlinking orphaned AppleDoube file '%s/%s/%s'",
                        cwdbuf, ADv2_DIRNAME, ep->d_name);
            }
        }
    }

    closedir(dp);
    close(parent_fd);
    return ret;
}

/*!
 * @brief Make sure that an entry has a CNID in the database and that its
 * AppleDouble file agrees
 *
 * The call reads the CNID hint of the AppleDouble file, unless -i ignores it.
 * Under -s, the metadata read cannot initialize or repair the header.
 * It then adds or looks up the entry in the database with that hint. A mismatch
 * rewrites the AppleDouble file with the database CNID, except under -s. An
 * emptied CNID table ends the scan through longjmp, since the rest of the
 * volume would get new IDs.
 *
 * @param[in] name       entry name
 * @param[in] did        CNID of the parent directory
 * @param[in] st         stat result of the entry
 * @param[in] adfile_ok  0 when the AppleDouble file of the entry is usable
 * @returns the CNID of the entry, or CNID_INVALID for an error
 */
static cnid_t check_cnid(const char *name, cnid_t did, struct stat *st,
                         int adfile_ok)
{
    int adflags = ADFLAGS_HF;
    int err;
    int oflags = (dbd_flags & DBD_FLAGS_SCAN)
                 ? ADFLAGS_RDONLY_STRICT : ADFLAGS_RDWR;
    cnid_t db_cnid, ad_cnid;
    struct adouble ad;
    adflags = ADFLAGS_HF | (S_ISDIR(st->st_mode) ? ADFLAGS_DIR : 0);
    /* Get CNID from ad-file */
    ad_cnid = CNID_INVALID;

    if (dbd_flags & DBD_FLAGS_STRIP_AD) {
        dbd_log(LOGDEBUG, "Ignoring AppleDouble CNID hint for '%s/%s'", cwdbuf, name);

        if (ADFILE_OK && vol->v_adouble == AD_VERSION_EA) {
            ad_init(&ad, vol);

            if (ad_open(&ad, name, adflags | oflags) != 0) {
                dbd_log(LOGDEBUG, "File without meta EA: \"%s/%s\"", cwdbuf, name);
                adfile_ok = 1;
            } else {
                ad_close(&ad, ADFLAGS_HF);
            }
        }
    } else if (ADFILE_OK) {
        ad_init(&ad, vol);

        if (ad_open(&ad, name, adflags | oflags) != 0) {
            if (vol->v_adouble != AD_VERSION_EA) {
                dbd_log(LOGSTD, "Error opening AppleDouble file for '%s/%s': %s", cwdbuf, name,
                        strerror(errno));
                return CNID_INVALID;
            }

            dbd_log(LOGDEBUG, "File without meta EA: \"%s/%s\"", cwdbuf, name);
            adfile_ok = 1;
        } else {
            ad_cnid = ad_getid(&ad, st->st_dev, st->st_ino, 0, stamp);

            if (ad_cnid == CNID_INVALID) {
                dbd_log(LOGSTD, "Bad CNID in adouble file of '%s/%s'", cwdbuf, name);
            } else {
                dbd_log(LOGDEBUG, "CNID from .AppleDouble file for '%s/%s': %u", cwdbuf, name,
                        ntohl(ad_cnid));
            }

            ad_close(&ad, ADFLAGS_HF);
        }
    }

    /* Get CNID from database */
    if ((db_cnid = cnid_add(vol->v_cdb, st, did, (char *)name, strlen(name),
                            ad_cnid)) == CNID_INVALID) {
        if (CNID_ERRNO() == CNID_ERR_RESET) {
            dbd_log(LOGSTD,
                    "CNID space exhausted at \"%s/%s\": the table was emptied "
                    "and this scan is incomplete. Rerun to renumber the volume.",
                    cwdbuf, name);
            /* Walking on would renumber every remaining file against the
             * emptied table, leaving the volume half old, half new ids */
            longjmp(jmp, 2);
        }

        return CNID_INVALID;
    }

    /* Compare CNID from db and adouble file */
    if (ad_cnid != db_cnid && adfile_ok == 0 && !(dbd_flags & DBD_FLAGS_SCAN)) {
        /* Mismatch, overwrite ad file with value from db */
        dbd_log(LOGSTD, "CNID mismatch for '%s/%s', CNID db: %u, ad-file: %u",
                cwdbuf, name, ntohl(db_cnid), ntohl(ad_cnid));
        ad_init(&ad, vol);

        if (ad_open(&ad, name, adflags | ADFLAGS_HF | ADFLAGS_RDWR) != 0) {
            dbd_log(LOGSTD, "Error opening AppleDouble file for '%s/%s': %s",
                    cwdbuf, name, strerror(errno));
            return CNID_INVALID;
        }

        err = ad_setid(&ad, st->st_dev, st->st_ino, db_cnid, did, stamp);

        if (err == -1) {
            dbd_log(LOGSTD, "Error setting new CNID, malformed adouble: '%s/%s'",
                    cwdbuf, name);
            ad_close(&ad, ADFLAGS_HF);
            return CNID_INVALID;
        }

        ad_flush(&ad);
        ad_close(&ad, ADFLAGS_HF);
    }

    return db_cnid;
}

/*!
 * @brief Delete a `._name` file whose `name` no longer exists
 *
 * Under -s the file is only reported.
 *
 * @param[in] name  name that starts with `._`
 * @returns 1 for an orphaned file, or 0
 */
static int check_orphaned(const char *name)
{
    int rc;
    struct stat sb;

    if (strlen(name) < 3) {
        return 0;
    }

    rc = lstat(&name[2], &sb);

    if (rc != 0 && errno == ENOENT) {
        dbd_log(LOGSTD, "Orphaned AppleDouble \"%s/%s\"", cwdbuf, name);

        if (!(dbd_flags & DBD_FLAGS_SCAN)) {
            unlink(name);
        }

        return 1;
    }

    return 0;
}

/*!
 * @brief Scan the current directory, check each entry, and descend into the
 * subdirectories
 *
 * The call checks the .AppleDouble directory first, then every entry. It
 * skips the netatalk folders, .zfs in the volume root, and names that the
 * volume does not accept. Types other than files, directories, and symbolic
 * links go away, except under -s. The call checks the AppleDouble file of
 * each entry, and for entries other than symbolic links the CNID and the EA
 * files. It descends into each directory that has a CNID.
 *
 * A signal ends the scan through longjmp.
 *
 * @param[in] volroot  nonzero in the volume root
 * @param[in] did      CNID of the current directory
 * @returns 0, or -1 for an error that stops the scan
 */
static int dbd_readdir(int volroot, cnid_t did)
{
    int cwd, ret = 0, adfile_ok, addir_ok;
    cnid_t cnid = 0;
    const char *name, *filename;
    char euro_name[MAXPATHLEN + 1];
    DIR *dp;
    struct dirent *ep;
    static struct stat st;      /* Save some stack space */

    /* Check again for .AppleDouble folder, check_adfile also checks/creates it.
     * Positive: skip this directory's AppleDouble checks; negative: fatal on a
     * rebuild run. */
    if ((addir_ok = check_addir(volroot)) < 0 && !(dbd_flags & DBD_FLAGS_SCAN)) {
        return -1;
    }

    /* Check AppleDouble files in AppleDouble folder, but only if it exists or could be created */
    if (ADDIR_OK && (read_addir()) != 0 && !(dbd_flags & DBD_FLAGS_SCAN)) {
        /* Fatal on rebuild run, continue if only scanning ! */
        return -1;
    }

    if ((dp = opendir(".")) == NULL) {
        if (errno == EACCES || errno == EPERM) {
            dbd_log(LOGSTD, "Skipping directory '%s': %s", cwdbuf, strerror(errno));
            return 0;
        }

        dbd_log(LOGSTD, "Couldn't open the directory '%s': %s", cwdbuf,
                strerror(errno));
        return -1;
    }

    while ((ep = readdir(dp))) {
        /* Check if we got a termination signal */
        if (alarmed) {
            /* this jumps back to cmd_dbd_scanvol() */
            longjmp(jmp, 1);
        }

        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name)) {
            continue;
        }

        /* Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop" */
        if ((name = check_netatalk_dirs(ep->d_name)) != NULL) {
            if (! volroot) {
                dbd_log(LOGSTD, "Nested %s in %s", name, cwdbuf);
            }

            continue;
        }

        /* Check for special folders in volume root e.g. ".zfs" */
        if (volroot && (name = check_special_dirs(ep->d_name)) != NULL) {
            dbd_log(LOGSTD, "Ignoring special dir \"%s\"", name);
            continue;
        }

        /* Skip .AppleDouble dir in this loop */
        if (STRCMP(ep->d_name, ==, ADv2_DIRNAME)) {
            continue;
        }

        if (!vol->vfs->vfs_validupath(vol, ep->d_name, -1)) {
            dbd_log(LOGDEBUG, "Ignoring \"%s\"", ep->d_name);
            continue;
        }

        if (lstat(ep->d_name, &st) < 0) {
            dbd_log(LOGSTD, "Lost file while reading dir '%s/%s', probably removed: %s",
                    cwdbuf, ep->d_name, strerror(errno));
            continue;
        }

        switch (st.st_mode & S_IFMT) {
        case S_IFREG:
        case S_IFDIR:
        case S_IFLNK:
            break;

        default:
            dbd_log(LOGSTD, "Bad filetype: %s/%s", cwdbuf, ep->d_name);

            if (!(dbd_flags & DBD_FLAGS_SCAN) && (unlink(ep->d_name)) != 0) {
                dbd_log(LOGSTD, "Error removing: %s/%s: %s", cwdbuf, ep->d_name,
                        strerror(errno));
            }

            continue;
        }

        /**************************************************************************
           Statistics
         **************************************************************************/
        static unsigned long long statcount = 0;
        static time_t t = 0;

        if (t == 0) {
            t = time(NULL);
        }

        statcount++;

        if ((statcount % 10000) == 0 && dbd_flags & DBD_FLAGS_STATS) {
            dbd_log(LOGSTD, "Scanned: %10llu, time: %10llu s",
                    statcount, (unsigned long long)(time(NULL) - t));
        }

        /**************************************************************************
           Tests
        **************************************************************************/

        /* Check for invalid names and orphaned ._ files */
        if (S_ISREG(st.st_mode) && (strncmp(ep->d_name, "._", strlen("._")) == 0)) {
            if (check_orphaned(ep->d_name)) {
                continue;
            }

            if (vol->vfs->vfs_validupath(vol, ep->d_name, -1)) {
                dbd_log(LOGSTD, "Bad AppleDouble \"%s/%s\"", cwdbuf, ep->d_name);
                continue;
            }
        }

        filename = ep->d_name;

        if (dbd_flags & DBD_FLAGS_EURO) {
            int migrated = migrate_euro(did, &st, filename, euro_name,
                                        sizeof(euro_name));

            if (migrated < 0) {
                closedir(dp);
                return -1;
            }

            if (migrated > 0) {
                filename = euro_name;
            }
        }

        name = NULL;
        adfile_ok = -1;

        /* Check for appledouble file, create if missing, but only if we have addir */
        if (ADDIR_OK) {
            adfile_ok = check_adfile(filename, &st, &name);
        }

        if (!S_ISLNK(st.st_mode)) {
            if (name == NULL) {
                name = filename;
            } else {
                update_cnid(did, &st, filename, name);
            }

            /* Check CNIDs */
            cnid = check_cnid(name, did, &st, adfile_ok);

            /* Check EA files, which live in the AppleDouble dir too */
            if (ADDIR_OK && vol->v_vfs_ea == AFPVOL_EA_AD) {
                check_eafiles(name);
            }
        }

        /**************************************************************************
          Recursion
        **************************************************************************/
        /* If we have no cnid for it we can't enter recursion */
        if (S_ISDIR(st.st_mode) && cnid) {
            strlcat(cwdbuf, "/", sizeof(cwdbuf));
            strlcat(cwdbuf, name, sizeof(cwdbuf));
            dbd_log(LOGDEBUG, "Entering directory: %s", cwdbuf);

            if (-1 == (cwd = open(".", O_RDONLY))) {
                dbd_log(LOGSTD, "Can't open directory '%s': %s", cwdbuf, strerror(errno));
                continue;
            }

            if (0 != chdir(name)) {
                dbd_log(LOGSTD, "Can't chdir to directory '%s': %s", cwdbuf, strerror(errno));
                close(cwd);
                continue;
            }

            ret = dbd_readdir(0, cnid);

            if (fchdir(cwd) < 0) {
                dbd_log(LOGSTD, "Cant chdir to directory '%i': %s", cwd, strerror(errno));
            }

            close(cwd);
            *(strrchr(cwdbuf, '/')) = 0;

            if (ret < 0) {
                closedir(dp);
                return -1;
            }
        }
    }

    /*
      Use results of previous checks
    */
    if ((vol->v_adouble == AD_VERSION_EA) && (dbd_flags & DBD_FLAGS_V2TOEA)
            && rmdir(ADv2_DIRNAME) != 0) {
        switch (errno) {
        case ENOENT:
            break;

        default:
            dbd_log(LOGSTD, "Error removing adouble dir \"%s/%s\": %s", cwdbuf,
                    ADv2_DIRNAME, strerror(errno));
            break;
        }
    }

    closedir(dp);
    return ret;
}

/*!
 * @brief Scan a whole volume from its root
 *
 * Only Unicode volumes qualify. The scan reads the stamp of the CNID database,
 * and with -c it converts the AppleDouble data of the root. It then runs
 * dbd_readdir from the root with CNID 2. The umask stays 0 during the scan, so
 * repaired AppleDouble files take their permissions from the files they belong
 * to. A signal ends the scan with 0.
 *
 * @param[in] vol_in  volume
 * @param[in] flags   DBD_FLAGS_* options
 * @returns 0, or -1 for a failed scan or an emptied CNID table
 */
int cmd_dbd_scanvol(struct vol *vol_in, dbd_flags_t flags)
{
    EC_INIT;
    struct stat st;
    /* Preserve repaired AppleDouble permissions derived by ad_open/ad_mkdir
     * from the volume's existing files and directories, regardless of shell umask. */
    mode_t saved_umask = umask(0); //NOSONAR
    /* Make vol accessible for all funcs */
    vol = vol_in;
    dbd_flags = (flags & DBD_FLAGS_SCAN) ? flags & ~DBD_FLAGS_V2TOEA : flags;

    /* We only support unicode volumes ! */
    if (vol->v_volcharset != CH_UTF8) {
        dbd_log(LOGSTD, "Not a Unicode volume: %s, %u != %u", vol->v_volcodepage,
                vol->v_volcharset, CH_UTF8);
        EC_FAIL;
    }

    /*
     * Get CNID database stamp, cnid_getstamp() passes the buffer,
     * then cnid_resolve() actually gets the value from the db
     */
    cnid_getstamp(vol->v_cdb, stamp, sizeof(stamp));

    switch (setjmp(jmp)) {
    case 1:
        EC_EXIT_STATUS(0); /* Got signal, jump from dbd_readdir */

    case 2:
        EC_EXIT_STATUS(-1); /* CNID table reset, jump from check_cnid */

    default:
        break;
    }

    strlcpy(cwdbuf, vol->v_path, sizeof(cwdbuf));

    if (chdir(vol->v_path) < 0) {
        dbd_log(LOGSTD, "Can't chdir to '%s': %s", vol->v_path, strerror(errno));
        EC_FAIL;
    }

    if ((vol->v_adouble == AD_VERSION_EA) && (dbd_flags & DBD_FLAGS_V2TOEA)) {
        if (lstat(".", &st) != 0) {
            EC_FAIL;
        }

        if (ad_convert(".", &st, vol, NULL) != 0) {
            switch (errno) {
            case ENOENT:
                break;

            default:
                dbd_log(LOGSTD, "Conversion error for \"%s\": %s", vol->v_path,
                        strerror(errno));
                break;
            }
        }
    }

    /* Start recursion */
    EC_NEG1(dbd_readdir(1, htonl(2)));    /* 2 = volumeroot CNID */
EC_CLEANUP:
    umask(saved_umask);
    EC_EXIT;
}
