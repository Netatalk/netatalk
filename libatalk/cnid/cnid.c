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
 * also lands in the database flags. A caller at euid 0 opens a private
 * volume as its owner, through become_owner(), and a failed switch fails
 * the open.
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

    /* private state is its owner's: euid 0, an admin session or root's
     * tool, becomes the owner for the open */
    const bool as_owner = (vol->v_flags & AFPVOL_PRIVATE) && geteuid() == 0;

    gid_t egid = 0;

    if (as_owner && become_owner(vol->v_private_uid, &egid) != 0) {
        LOG(log_error, logtype_afpd,
            "cnid_open: can't open volume '%s' as its owner, uid %ju: %s",
            vol->v_path, (uintmax_t)vol->v_private_uid, strerror(errno));
        return NULL;
    }

    db = mod->cnid_open(&args);

    if (as_owner) {
        unbecome_owner(egid);
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

/* folder tables a database keeps: one per listing enumerate() saves, and one
 * for the volume root's listing, which no slot keeps */
#define DIRTABS 9

enum dirtab_state { DIRTAB_FREE, DIRTAB_ARMED, DIRTAB_READY, DIRTAB_OFF };

/*! a folder's row, its name kept as a hash */
struct cnid_dirrow {
    uint64_t hash;
    uint64_t dev;
    uint64_t ino;
    cnid_t id;
};

/*! the rows of one folder, indexed by name hash */
struct cnid_dirtab {
    cnid_t did;
    enum dirtab_state state;
    unsigned long armed;       /*!< arm order; the oldest makes room */
    unsigned int calls;        /*!< cnid_add calls while armed */
    time_t loaded;             /*!< the second before the load began */
    uint32_t count;
    uint32_t cap;
    struct cnid_dirrow *rows;
    uint32_t *index;           /*!< row number + 1, 0 for a free slot */
    uint32_t mask;
};

static unsigned long dirtab_clock;

/*!
 * @brief FNV-1a hash of a name
 *
 * @param[in] name  the name
 * @param[in] len   its length
 * @returns the hash
 */
static uint64_t dirtab_hash(const char *name, size_t len)
{
    uint64_t h = 14695981039346656037ULL;

    for (size_t i = 0; i < len; i++) {
        h = (h ^ (unsigned char)name[i]) * 1099511628211ULL;
    }

    return h;
}

/*!
 * @brief Free a table's rows and mark it free
 *
 * @param[in,out] t  the table
 */
static void dirtab_empty(struct cnid_dirtab *t)
{
    free(t->rows);
    free(t->index);
    memset(t, 0, sizeof(*t));
}

/*!
 * @brief The table a folder holds, or NULL
 *
 * @param[in] cdb  CNID database
 * @param[in] did  the folder
 * @returns the table, or NULL
 */
static struct cnid_dirtab *dirtab_of(const struct _cnid_db *cdb, cnid_t did)
{
    for (int i = 0; cdb->cnid_dirtabs != NULL && i < DIRTABS; i++) {
        if (cdb->cnid_dirtabs[i].state != DIRTAB_FREE
                && cdb->cnid_dirtabs[i].did == did) {
            return &cdb->cnid_dirtabs[i];
        }
    }

    return NULL;
}

/*!
 * @brief Take one row of a folder into its table
 *
 * @param[in] ctx   the table
 * @param[in] id    the row's CNID
 * @param[in] name  the row's name
 * @param[in] len   the name's length
 * @param[in] dev   the row's device
 * @param[in] ino   the row's inode
 * @returns 0, or -1 when the rows cannot grow
 */
static int dirtab_take(void *ctx, uint64_t id, const char *name, size_t len,
                       uint64_t dev, uint64_t ino)
{
    struct cnid_dirtab *t = ctx;

    /* a row a backend lookup refuses, or one without its name, stays the
     * backend's to answer */
    if (name == NULL || id == 0 || id > UINT32_MAX) {
        return 0;
    }

    if (t->count == t->cap) {
        uint32_t cap = t->cap ? t->cap * 2 : 1024;
        struct cnid_dirrow *rows = realloc(t->rows, cap * sizeof(*rows));

        if (rows == NULL) {
            return -1;
        }

        t->rows = rows;
        t->cap = cap;
    }

    t->rows[t->count].hash = dirtab_hash(name, len);
    t->rows[t->count].dev = dev;
    t->rows[t->count].ino = ino;
    t->rows[t->count].id = htonl((uint32_t)id);
    t->count++;
    return 0;
}

/*!
 * @brief Load a folder's rows from one backend query
 *
 * A failed query or allocation turns the table off, and the folder's
 * names take the backend; so does a row that finds no free slot within
 * CNID_DIRTAB_PROBES of its own, which names chosen to collide make.
 *
 * @param[in]     cdb  CNID database
 * @param[in,out] t    the folder's table
 */
static void dirtab_load(struct _cnid_db *cdb, struct cnid_dirtab *t)
{
    uint32_t slots = 16;
    t->state = DIRTAB_OFF;
    /* a file's ctime can trail time() by a clock tick */
    t->loaded = time(NULL) - 1;

    if (cdb->cnid_dirlist(cdb, t->did, CNID_DIRTAB_ROWS, dirtab_take,
                          t) != 0) {
        return;
    }

    while (slots < 2 * t->count) {
        slots *= 2;
    }

    if ((t->index = calloc(slots, sizeof(*t->index))) == NULL) {
        return;
    }

    t->mask = slots - 1;

    for (uint32_t r = 0; r < t->count; r++) {
        uint32_t s = (uint32_t)t->rows[r].hash & t->mask;
        int probes = 1;

        while (t->index[s] != 0) {
            if (probes++ == CNID_DIRTAB_PROBES) {
                return;
            }

            s = (s + 1) & t->mask;
        }

        t->index[s] = r + 1;
    }

    t->state = DIRTAB_READY;
}

/*!
 * @brief A loaded table's CNID for a name whose row holds the object's
 *        device and inode
 *
 * @param[in] t     the folder's table
 * @param[in] name  the name
 * @param[in] len   its length
 * @param[in] dev   the object's device, as the backend stores it
 * @param[in] ino   the object's inode
 * @returns the CNID, or CNID_INVALID
 */
static cnid_t dirtab_lookup(const struct cnid_dirtab *t, const char *name,
                            size_t len, uint64_t dev, uint64_t ino)
{
    uint64_t h = dirtab_hash(name, len);
    uint32_t s = (uint32_t)h & t->mask;
    uint32_t r;

    /* the load put every row within CNID_DIRTAB_PROBES slots of its own */
    for (int probes = 0; probes < CNID_DIRTAB_PROBES
            && (r = t->index[s]) != 0; probes++) {
        const struct cnid_dirrow *row = &t->rows[r - 1];

        if (row->hash == h) {
            return row->dev == dev && row->ino == ino ? row->id : CNID_INVALID;
        }

        s = (s + 1) & t->mask;
    }

    return CNID_INVALID;
}

/*!
 * @brief Turn the loaded tables off: a CNID write may have moved a row
 *
 * @param[in] cdb  CNID database
 */
static void dirtab_off(const struct _cnid_db *cdb)
{
    for (int i = 0; cdb->cnid_dirtabs != NULL && i < DIRTABS; i++) {
        if (cdb->cnid_dirtabs[i].state == DIRTAB_READY) {
            cdb->cnid_dirtabs[i].state = DIRTAB_OFF;
        }
    }
}

/*!
 * @brief Let one backend query answer a folder's cnid_add calls
 *
 * The rows load at the folder's (CNID_DIRTAB_MISSES + 1)-th call. Arming a
 * folder again drops its rows, and the least recently armed table makes
 * room.
 *
 * @param[in] cdb  CNID database, where a backend without cnid_dirlist
 *                 arms nothing
 * @param[in] did  the folder
 */
void cnid_dir_arm(struct _cnid_db *cdb, cnid_t did)
{
    struct cnid_dirtab *t;

    if (cdb == NULL || cdb->cnid_dirlist == NULL) {
        return;
    }

    if (cdb->cnid_dirtabs == NULL
            && (cdb->cnid_dirtabs = calloc(DIRTABS, sizeof(*t))) == NULL) {
        return;
    }

    if ((t = dirtab_of(cdb, did)) == NULL) {
        t = &cdb->cnid_dirtabs[0];

        for (int i = 1; i < DIRTABS && t->state != DIRTAB_FREE; i++) {
            if (cdb->cnid_dirtabs[i].state == DIRTAB_FREE
                    || cdb->cnid_dirtabs[i].armed < t->armed) {
                t = &cdb->cnid_dirtabs[i];
            }
        }
    }

    dirtab_empty(t);
    t->did = did;
    t->state = DIRTAB_ARMED;
    t->armed = ++dirtab_clock;
}

/*!
 * @brief Free a folder's table
 *
 * @param[in] cdb  CNID database, or NULL
 * @param[in] did  the folder
 */
void cnid_dir_forget(struct _cnid_db *cdb, cnid_t did)
{
    struct cnid_dirtab *t;

    if (cdb != NULL && (t = dirtab_of(cdb, did)) != NULL) {
        dirtab_empty(t);
    }
}

/*!
 * @brief Let a folder's table answer cnid_add() while its listing's batch
 *        runs, and no table outside one
 *
 * @param[in] cdb  CNID database, or NULL
 * @param[in] did  the folder, CNID_INVALID when the batch ends
 */
void cnid_dir_serve(struct _cnid_db *cdb, cnid_t did)
{
    if (cdb != NULL) {
        cdb->cnid_dirtab_did = did;
    }
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

    for (int i = 0; db->cnid_dirtabs != NULL && i < DIRTABS; i++) {
        dirtab_empty(&db->cnid_dirtabs[i]);
    }

    free(db->cnid_dirtabs);
    db->cnid_close(db);
}

/*!
 * @brief Add an object to the CNID database or find its existing CNID
 *
 * The call clears errno before it asks the backend, so errno reports the
 * classification of this call only. The result passes through valide.
 * While the folder's listing batch runs, its table answers for an object
 * last changed before the second preceding the table's load, whose row
 * holds the object's device and inode, unless hint names another CNID.
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

    struct cnid_dirtab *t = did == cdb->cnid_dirtab_did
                            ? dirtab_of(cdb, did) : NULL;

    if (t != NULL && t->state == DIRTAB_ARMED
            && ++t->calls > CNID_DIRTAB_MISSES) {
        dirtab_load(cdb, t);
    }

    if (t != NULL && t->state == DIRTAB_READY && st->st_ctime < t->loaded) {
        uint64_t dev = cdb->cnid_db_flags & CNID_FLAG_NODEV
                       ? 0 : (uint64_t)st->st_dev;

        if ((ret = dirtab_lookup(t, name, len, dev, (uint64_t)st->st_ino))
                != CNID_INVALID && (hint == CNID_INVALID || hint == ret)) {
            return valide(ret);
        }
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
    dirtab_off(cdb);
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
 * A stored name of "." or ".." means a corrupted database, and the call then
 * fails with CNID_ERR_CORRUPT.
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

    if (ret && (!strcmp(ret, ".") || !strcmp(ret, ".."))) {
        LOG(log_error, logtype_afpd,
            "cnid_resolve: name is '%s', corrupted db? ", ret);
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
    dirtab_off(cdb);
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
    dirtab_off(cdb);

    if (cdb->cnid_wipe) {
        ret = cdb->cnid_wipe(cdb);
    }

    return ret;
}
