/*
 * Copyright (c) 2003 the Netatalk Team
 * Copyright (c) 2003 Rafal Lewczuk <rlewczuk@pronet.pl>
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
#define USE_LIST

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <time.h>

#include <atalk/cnid.h>
#include <atalk/list.h>
#include <atalk/logger.h>
#include <atalk/volume.h>

/*! List of all registered modules. */
static struct list_head modules = ATALK_LIST_HEAD_INIT(modules);

/*!
 * @brief Registered backend module named @p name, or NULL
 */
static const cnid_module *cnid_module_named(const char *name)
{
    struct list_head *ptr;
    list_for_each(ptr, &modules) {
        if (0 == strcasecmp(list_entry(ptr, cnid_module, db_list)->name, name)) {
            return list_entry(ptr, cnid_module, db_list);
        }
    }
    return NULL;
}

bool cnid_scheme_registered(const char *name)
{
    return cnid_module_named(name) != NULL;
}

/* Registers new CNID backend module. */

/*! Once module has been registered, it cannot be unregistered. */
void cnid_register(struct _cnid_module *module)
{
    if (cnid_module_named(module->name) != NULL) {
        LOG(log_error, logtype_afpd, "Module with name [%s] is already registered !",
            module->name);
        return;
    }

    LOG(log_info, logtype_afpd, "Registering CNID module [%s]", module->name);
    list_add_tail(&module->db_list, &modules);
}

/*! Opens CNID database using particular back-end */
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

    db = mod->cnid_open(&args);

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
  protect against bogus value from the DB.
  adddir really doesn't like 2

  Every CNID_INVALID returned from this file carries its own errno: callers read
  the CNID_ERR_* values to choose between a permanent reply and a retryable one,
  and CNID_ERR_DB ends the session. No syscall overwrites those values and the
  logger preserves errno, so one persists until deliberately replaced.
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

/*! Closes CNID database. Currently it's just a wrapper around db->cnid_close(). */
void cnid_close(struct _cnid_db *db)
{
    if (NULL == db) {
        LOG(log_error, logtype_afpd, "Error: cnid_close called with NULL argument !");
        return;
    }

    db->cnid_close(db);
}

/* --------------- */
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

/* --------------- */
int cnid_delete(struct _cnid_db *cdb, cnid_t id)
{
    int ret;
    errno = 0;
    ret = cdb->cnid_delete(cdb, id);
    return ret;
}


/* --------------- */
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name,
                const size_t len)
{
    cnid_t ret;
    errno = 0;
    ret = valide(cdb->cnid_get(cdb, did, name, len));
    return ret;
}

/* --------------- */
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

/* --------------- */
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

/* --------------- */
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

/* --------------- */
int cnid_update(struct _cnid_db *cdb, const cnid_t id, const struct stat *st,
                const cnid_t did, char *name, const size_t len)
{
    int ret;
    errno = 0;
    ret = cdb->cnid_update(cdb, id, st, did, name, len);
    return ret;
}

/* --------------- */
int cnid_wipe(struct _cnid_db *cdb)
{
    int ret = 0;
    errno = 0;

    if (cdb->cnid_wipe) {
        ret = cdb->cnid_wipe(cdb);
    }

    return ret;
}
