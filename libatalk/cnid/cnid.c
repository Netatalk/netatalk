/*
 * Copyright (c) 2003 the Netatalk Team
 * Copyright (c) 2003 Rafal Lewczuk <rlewczuk@pronet.pl>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation version 2 of the License or later
 * version if explicitly stated by any of above copyright holders.
 *
 * 2024/07/05: Rafal Lewczuk gave permission to change the license
 * to a later verion of the GNU GPL in
 * https://github.com/Netatalk/netatalk/issues/1193
 *
 */

/*!
 * @file
 * @brief The CNID database front end and backend registry
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/*! Permanent registry of backend modules, kept in registration order. */
static cnid_module *modules;

/*!
 * @brief Find a registered CNID backend by name, ignoring case
 *
 * @param[in] name  backend name
 * @returns the backend module, or NULL
 */
static const cnid_module *cnid_module_named(const char *name)
{
    for (const cnid_module *module = modules; module; module = module->next) {
        if (0 == strcasecmp(module->name, name)) {
            return module;
        }
    }

    return NULL;
}

/*!
 * @brief Tell whether cnid_register added a CNID backend of a name
 *
 * @param[in] name  backend name, compared without case
 * @returns true when cnid_module_named finds it
 */
bool cnid_scheme_registered(const char *name)
{
    return cnid_module_named(name) != NULL;
}

/*!
 * @brief Add a CNID backend to the list of registered backends
 *
 * A second backend with the same name, compared without case, only logs an
 * error. A backend cannot leave the registry again and must remain alive
 * for the lifetime of the process.
 *
 * @param[in] module  backend to add, kept by pointer
 */
void cnid_register(struct _cnid_module *module)
{
    if (cnid_module_named(module->name) != NULL) {
        LOG(log_error, logtype_afpd, "Module with name [%s] is already registered !",
            module->name);
        return;
    }

    LOG(log_info, logtype_afpd, "Registering CNID module [%s]", module->name);
    cnid_module **tail = &modules;

    while (*tail != NULL) {
        tail = &(*tail)->next;
    }

    module->next = NULL;

    *tail = module;
}

/*!
 * @brief Open the CNID database of a volume with the backend that a scheme
 * names
 *
 * An unknown scheme falls back to DEFAULT_CNID_SCHEME with a warning. A build
 * without MySQL refuses the mysql scheme instead. CNID_FLAG_NODEV in flags
 * also lands in the database flags.
 *
 * @param[in] vol    volume of the database
 * @param[in] type   CNID scheme name
 * @param[in] flags  CNID_FLAG_* open flags
 * @returns the database, or NULL when no backend fits or the open fails
 */
struct _cnid_db *cnid_open(struct vol *vol, char *type, int flags)
{
    struct _cnid_db *db;
    const cnid_module *mod = cnid_module_named(type);

    if (NULL == mod) {
#ifndef CNID_BACKEND_MYSQL

        if (0 == strcasecmp(type, "mysql")) {
            LOG(log_error, logtype_afpd,
                "CNID backend [%s] for volume %s is not compiled in",
                type, vol->v_path);
            return NULL;
        }

#endif
        LOG(log_warning, logtype_afpd,
            "Unknown cnid scheme [%s] for volume %s, using [%s]",
            type, vol->v_path, DEFAULT_CNID_SCHEME);
        mod = cnid_module_named(DEFAULT_CNID_SCHEME);
    }

    if (NULL == mod) {
        LOG(log_error, logtype_afpd,
            "Cannot find module named [%s] in registered module list!",
            DEFAULT_CNID_SCHEME);
        return NULL;
    }

    struct cnid_open_args args =  {
        .cnid_args_flags = flags,
        .cnid_args_vol   = vol
    };

    /* private state is created as its owner: a process with euid 0, an admin
     * session or root's tool, takes the owner's groups and ids for the open */
    const bool as_owner = (vol->v_flags & AFPVOL_PRIVATE) && geteuid() == 0;

    const gid_t egid = getegid();

    if (as_owner) {
        const struct passwd *owner = getpwuid(vol->v_private_uid);

        if (owner == NULL) {
            LOG(log_error, logtype_afpd,
                "cnid_open: volume '%s': its owner, uid %ju, has no account",
                vol->v_path, (uintmax_t)vol->v_private_uid);
            return NULL;
        }

        if (initgroups(owner->pw_name, owner->pw_gid) != 0
                || setegid(owner->pw_gid) != 0
                || seteuid(vol->v_private_uid) != 0) {
            LOG(log_error, logtype_afpd,
                "cnid_open: can't open volume '%s' as its owner %s: %s",
                vol->v_path, owner->pw_name, strerror(errno));

            if (setegid(egid) != 0) {
                AFP_PANIC("cnid_open: can't restore the gid");
            }

            return NULL;
        }
    }

    db = mod->cnid_open(&args);

    if (as_owner && (seteuid(0) != 0 || setegid(egid) != 0)) {
        AFP_PANIC("cnid_open: can't return to root");
    }

    if (NULL == db) {
        LOG(log_error, logtype_afpd, "Cannot open CNID db at [%s].", vol->v_path);
        return NULL;
    }

    if (flags & CNID_FLAG_NODEV) {
        db->cnid_db_flags |= CNID_FLAG_NODEV;
    }

    return db;
}

/*!
 * @brief Reject a CNID from a backend that falls in the reserved range
 *
 * An ID under CNID_START means a corrupted database. The first such ID logs
 * an error, and each one sets errno to CNID_ERR_CORRUPT. CNID_INVALID passes
 * with the errno of the backend. Callers read the CNID_ERR_* value to choose
 * between a permanent reply and a retry, and CNID_ERR_DB ends the session.
 * The logger keeps errno, so the value stays until a later call replaces it.
 *
 * @param[in] id  CNID in network order
 * @returns id, or CNID_INVALID
 */
static cnid_t valide(cnid_t id)
{
    if (id == CNID_INVALID) {
        /* the backend classified this one */
        return id;
    }

    /* id is in network byte order; comparing it raw would reject 16 valid
     * CNIDs on little-endian hosts and wave the reserved ids 1-16 through */
    if (ntohl(id) < CNID_START) {
        static int err = 0;

        if (!err) {
            err = 1;
            LOG(log_error, logtype_afpd, "Error: Invalid cnid, corrupted DB?");
        }

        errno = CNID_ERR_CORRUPT;
        return CNID_INVALID;
    }

    return id;
}

/*!
 * @brief Close a CNID database through its backend
 *
 * @param[in] db  database, and a NULL one logs an error
 */
void cnid_close(struct _cnid_db *db)
{
    if (NULL == db) {
        LOG(log_error, logtype_afpd, "Error: cnid_close called with NULL argument !");
        return;
    }

    db->cnid_close(db);
}

/*!
 * @brief Add an object to the CNID database or find its existing CNID
 *
 * The call clears errno, so errno reports the classification of this call
 * only. The result passes through valide.
 *
 * @param[in] cdb   CNID database
 * @param[in] st    stat of the object, for the device and inode
 * @param[in] did   CNID of the parent directory
 * @param[in] name  name on disk
 * @param[in] len   length of name, where 0 fails with CNID_ERR_PARAM
 * @param[in] hint  CNID that the metadata of the object carries, or
 *                  CNID_INVALID
 * @returns the CNID, or CNID_INVALID with errno set to a CNID_ERR_* value or 0
 */
cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did,
                const char *name, const size_t len, cnid_t hint)
{
    cnid_t ret;

    if (len == 0) {
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    /* Cleared for every backend and caller here, so errno reads as this
     * operation's verdict. Not every backend classifies every failure. */
    errno = 0;
    ret = valide(cdb->cnid_add(cdb, st, did, name, len, hint));
    return ret;
}

/*!
 * @brief Remove a CNID from the database
 *
 * @param[in] cdb  CNID database
 * @param[in] id   CNID to remove
 * @returns the backend result, 0 on success
 */
int cnid_delete(struct _cnid_db *cdb, cnid_t id)
{
    int ret;
    errno = 0;
    ret = cdb->cnid_delete(cdb, id);
    return ret;
}


/*!
 * @brief Look up the CNID of a name in a directory
 *
 * @param[in] cdb   CNID database
 * @param[in] did   CNID of the directory
 * @param[in] name  name on disk
 * @param[in] len   length of name
 * @returns the CNID after valide, or CNID_INVALID
 */
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name,
                const size_t len)
{
    cnid_t ret;
    errno = 0;
    ret = valide(cdb->cnid_get(cdb, did, name, len));
    return ret;
}

/*!
 * @brief Read the stamp of a CNID database
 *
 * A backend without stamps gets the current time instead, which makes every
 * cached CNID stale.
 *
 * @param[in]  cdb     CNID database
 * @param[out] buffer  receives the stamp
 * @param[in]  len     size of buffer
 * @returns the backend result, or for a backend without stamps 0, or -1 when
 *          buffer cannot hold a time_t
 */
int cnid_getstamp(struct _cnid_db *cdb,  void *buffer, const size_t len)
{
    cnid_t ret;
    time_t t;

    if (!cdb->cnid_getstamp) {
        memset(buffer, 0, len);

        /* return the current time. it will invalide cache */
        if (len < sizeof(time_t)) {
            return -1;
        }

        t = time(NULL);
        memcpy(buffer, &t, sizeof(time_t));
        return 0;
    }

    errno = 0;
    ret = cdb->cnid_getstamp(cdb, buffer, len);
    return ret;
}

/*!
 * @brief Look up the CNID of an object by name and parent or by device and
 * inode
 *
 * @param[in] cdb   CNID database
 * @param[in] st    stat of the object
 * @param[in] did   CNID of the parent directory
 * @param[in] name  name on disk
 * @param[in] len   length of name
 * @returns the CNID after valide, or CNID_INVALID
 */
cnid_t cnid_lookup(struct _cnid_db *cdb, const struct stat *st,
                   const cnid_t did,
                   char *name, const size_t len)
{
    cnid_t ret;
    errno = 0;
    ret = valide(cdb->cnid_lookup(cdb, st, did, name, len));
    return ret;
}

/* --------------- */
/*!
 * @brief Search the CNID database for entries whose name contains a substring
 *
 * Centralises parameter validation so all CNID backends behave identically
 * against bad input.
 *
 * The optional @p more_available out-parameter, when non-NULL, is written
 * unconditionally on entry to false and again by the backend: false on
 * error, true iff the result set was truncated on success (more matches
 * exist than fit in @p buffer).
 *
 * @param[in]  cdb            CNID database handle
 * @param[in]  name           UTF-8 substring to search for, NUL-terminated
 * @param[in]  namelen        bytes in @p name, range 1..MAXPATHLEN
 * @param[out] buffer         caller-provided buffer for matching CNIDs in network byte order
 * @param[in]  buflen         capacity of @p buffer in bytes, must be >= CNID_FIND_MIN_BUFLEN
 * @param[out] more_available set to true iff result set was truncated, NULL to opt out
 *
 * @returns number of CNIDs written to @p buffer on success, -1 on failure
 *          (errno = CNID_ERR_PARAM for invalid arguments, otherwise the
 *          backend's CNID_ERR_* classification, or 0 where it makes none)
 */
int cnid_find(struct _cnid_db *cdb, const char *name, size_t namelen,
              void *buffer, size_t buflen, bool *more_available)
{
    return cnid_find_scoped(cdb, name, namelen, CNID_INVALID,
                            buffer, buflen, more_available);
}

/*!
 * @brief cnid_find() restricted to the subtree of a directory
 *
 * Identical contract to cnid_find(); additionally, when @p scope_did
 * is not CNID_INVALID only entries lying underneath that directory
 * match. The scope directory itself is not a result.
 *
 * @param[in]  cdb            CNID database handle
 * @param[in]  name           UTF-8 substring to search for, NUL-terminated
 * @param[in]  namelen        bytes in @p name, range 1..MAXPATHLEN
 * @param[in]  scope_did      CNID of the scope directory in network byte
 *                            order, or CNID_INVALID for the whole volume
 * @param[out] buffer         caller-provided buffer for matching CNIDs in network byte order
 * @param[in]  buflen         capacity of @p buffer in bytes, must be >= CNID_FIND_MIN_BUFLEN
 * @param[out] more_available set to true iff result set was truncated, NULL to opt out
 *
 * @returns number of CNIDs written to @p buffer on success, -1 on failure
 *          (errno = CNID_ERR_PARAM for invalid arguments, otherwise the
 *          backend's CNID_ERR_* classification, or 0 where it makes none)
 */
int cnid_find_scoped(struct _cnid_db *cdb, const char *name,
                     size_t namelen, cnid_t scope_did,
                     void *buffer, size_t buflen, bool *more_available)
{
    int ret;

    if (more_available) {
        *more_available = false;
    }

    if (cdb == NULL || cdb->cnid_find == NULL) {
        LOG(log_error, logtype_cnid,
            "cnid_find: backend does not support cnid_find");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (name == NULL || namelen == 0
            || namelen > (size_t)MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_find: invalid name (namelen=%zu, max=%zu)",
            namelen, (size_t)MAXPATHLEN);
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (buffer == NULL || buflen < CNID_FIND_MIN_BUFLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_find: buflen %zu must be >= %zu",
            buflen, (size_t)CNID_FIND_MIN_BUFLEN);
        errno = CNID_ERR_PARAM;
        return -1;
    }

    errno = 0;
    ret = cdb->cnid_find(cdb, name, namelen, scope_did,
                         buffer, buflen, more_available);
    return ret;
}

/*!
 * @brief Resolve a CNID to its name and parent directory
 *
 * A stored name of ".." means a corrupted database, and the call then fails
 * with CNID_ERR_CORRUPT.
 *
 * @param[in]     cdb     CNID database
 * @param[in,out] id      CNID to resolve, receives the parent CNID or
 *                        CNID_INVALID
 * @param[out]    buffer  work buffer for the name
 * @param[in]     len     size of buffer
 * @returns the name inside buffer, or NULL
 */
char *cnid_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
{
    char *ret;
    errno = 0;
    ret = cdb->cnid_resolve(cdb, id, buffer, len);

    if (ret && !strcmp(ret, "..")) {
        LOG(log_error, logtype_afpd, "cnid_resolve: name is '..', corrupted db? ");
        /* Match the backend failure contract: without these the caller reads
         * a stale errno and *id still holds the parent DID the backend wrote */
        *id = CNID_INVALID;
        errno = CNID_ERR_CORRUPT;
        ret = NULL;
    }

    return ret;
}

/*!
 * @brief Update the name, parent, and stat of a CNID entry
 *
 * @param[in] cdb   CNID database
 * @param[in] id    CNID to update
 * @param[in] st    new stat of the object
 * @param[in] did   new parent CNID
 * @param[in] name  new name on disk
 * @param[in] len   length of name
 * @returns the backend result, 0 on success
 */
int cnid_update(struct _cnid_db *cdb, const cnid_t id, const struct stat *st,
                const cnid_t did, char *name, const size_t len)
{
    int ret;
    errno = 0;
    ret = cdb->cnid_update(cdb, id, st, did, name, len);
    return ret;
}

/*!
 * @brief Empty a CNID database
 *
 * A backend without a wipe function leaves the database unchanged and
 * reports success.
 *
 * @param[in] cdb  CNID database
 * @returns the backend result, 0 on success
 */
int cnid_wipe(struct _cnid_db *cdb)
{
    int ret = 0;
    errno = 0;

    if (cdb->cnid_wipe) {
        ret = cdb->cnid_wipe(cdb);
    }

    return ret;
}
