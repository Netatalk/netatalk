/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

/*
 * Parent-directory fd cache: transparent acceleration for dircache
 * validation stats. pfd_ostat() is a drop-in for
 * ostat(cfrombstr(entry->d_fullpath), st, options) — identical result and
 * errno whenever the filesystem is quiescent; any impediment falls back
 * to the full-path form internally. Callers never see fds. Under
 * concurrent external mutation (rename-away + recreate of the parent)
 * the two forms may answer from different world views until the
 * re-ground probe converges — each answer is truthful for one view,
 * never a third state (see the divergence-window tests).
 *
 * Threading: pfd_ostat() runs on the main thread only. pfd_purge()/
 * pfd_purge_vol() additionally run on the idle worker under the
 * iw_grant/iw_revoke temporal-separation handshake (via
 * dircache_remove from the deferred chains) — never concurrently with
 * main-thread pfd calls, so the module needs no locks and MUST NOT grow
 * state shared outside that discipline.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/directory.h>
#include <atalk/logger.h>
#include <atalk/util.h>

#include "dircache.h"
#include "directory.h"
#include "pfd_cache.h"
#include "volume.h"

#define PFD_SLOTS 16

#if defined(O_PATH)
#define PFD_OPEN_FLAGS (O_PATH | O_DIRECTORY | O_NOFOLLOW)
#elif defined(O_SEARCH)
#define PFD_OPEN_FLAGS (O_SEARCH | O_DIRECTORY)
#else
/* Execute-only directories fail to open with O_RDONLY; pfd_get() then
 * returns -1 and the wrapper falls back to full-path ostat (correct,
 * slower). */
#define PFD_OPEN_FLAGS (O_RDONLY | O_DIRECTORY)
#endif

struct pfd_slot {
    /* network order, as stored in struct dir */
    cnid_t   did;
    uint16_t vid;
    /* -1 = empty slot */
    int      fd;
    /* identity of the object fd was opened on */
    dev_t    dev;
    ino_t    ino;
    /* parent->dcache_ino observed at fill time — lets the sync-check tell
     * "dircache learned since fill" (act) from "slot learned via the
     * probe, dircache lagging" (serve; no thrash) */
    ino_t    fill_dcache_ino;
    /* hits since the last re-ground probe */
    uint32_t uses;
};

/* slots[0] is MRU; a hit moves its slot to the front (memmove of at most
 * 15 small structs — cheaper than list links at this size). */
static struct pfd_slot pfd_slots[PFD_SLOTS];
static int pfd_initialized;

static struct pfd_stats pfd_stat;

static void pfd_init_once(void)
{
    if (!pfd_initialized) {
        for (int i = 0; i < PFD_SLOTS; i++) {
            pfd_slots[i].fd = -1;
        }

        pfd_initialized = 1;
    }
}

static void slot_retire(struct pfd_slot *s)
{
    if (s->fd >= 0) {
        close(s->fd);
    }

    s->fd = -1;
    s->did = CNID_INVALID;
    s->vid = 0;
    s->uses = 0;
}

/* Open parent's path, record the opened object's identity. -1 on failure
 * (slot left empty). */
static int slot_fill(struct pfd_slot *s, const struct vol *vol,
                     const struct dir *parent)
{
    struct stat st;
    int fd = open(cfrombstr(parent->d_fullpath), PFD_OPEN_FLAGS);

    if (fd < 0) {
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }

    s->fd = fd;
    s->dev = st.st_dev;
    s->ino = st.st_ino;
    s->fill_dcache_ino = parent->dcache_ino;
    s->did = parent->d_did;
    s->vid = vol->v_vid;
    s->uses = 0;
    pfd_stat.opens++;
    return fd;
}

static void slot_move_to_front(int i)
{
    if (i > 0) {
        struct pfd_slot tmp = pfd_slots[i];
        memmove(&pfd_slots[1], &pfd_slots[0], (size_t)i * sizeof(tmp));
        pfd_slots[0] = tmp;
    }
}

/* Resolve parent to a directory fd. -1 = no usable fd (caller falls back).
 * Main thread only. */
static int pfd_get(const struct vol *vol, const struct dir *parent)
{
    pfd_init_once();

    for (int i = 0; i < PFD_SLOTS; i++) {
        struct pfd_slot *s = &pfd_slots[i];

        if (s->fd < 0 || s->vid != vol->v_vid || s->did != parent->d_did) {
            continue;
        }

        /* Sync-check against the fill-time baseline, not the slot's own
         * ino: after a probe refill the slot is FRESHER than the dircache,
         * and comparing slot.ino to the still-stale dcache_ino directly
         * would close+open on every hit until the entry catches up.
         * dcache_ino unchanged since fill → serve; changed and matching
         * the slot → re-baseline; changed and different → the dircache
         * learned of a replacement this slot hasn't seen → refill. */
        if (parent->dcache_ino != s->fill_dcache_ino) {
            if (parent->dcache_ino == s->ino) {
                s->fill_dcache_ino = parent->dcache_ino;
            } else {
                slot_retire(s);

                if (slot_fill(s, vol, parent) < 0) {
                    return -1;
                }

                pfd_stat.sync_refreshes++;
            }
        }

        if (++s->uses >= PFD_REGROUND_INTERVAL) {
            /* Absolute-path probe: catches an external rename-away +
             * recreate of the parent, which no cache-vs-cache compare
             * can see. */
            struct stat gst;
            s->uses = 0;
            pfd_stat.regrounds++;

            if (ostat(cfrombstr(parent->d_fullpath), &gst,
                      vol_syml_opt(vol)) != 0
                    || gst.st_dev != s->dev || gst.st_ino != s->ino) {
                slot_retire(s);

                if (slot_fill(s, vol, parent) < 0) {
                    return -1;
                }

                pfd_stat.probe_refreshes++;
            }
        }

        slot_move_to_front(i);
        pfd_stat.hits++;
        return pfd_slots[0].fd;
    }

    /* Miss: prefer an empty slot (purges leave holes); evict the LRU
     * tail only when full. */
    {
        int target = PFD_SLOTS - 1;

        for (int i = 0; i < PFD_SLOTS; i++) {
            if (pfd_slots[i].fd < 0) {
                target = i;
                break;
            }
        }

        slot_retire(&pfd_slots[target]);

        if (slot_fill(&pfd_slots[target], vol, parent) < 0) {
            return -1;
        }

        slot_move_to_front(target);
    }
    return pfd_slots[0].fd;
}

/* The relative stat just proved (parent fd, basename) names the live
 * object, so entry's true path is parent's path + '/' + basename. An
 * internal rename re-paths the parent in place and defers the children:
 * until the worker runs, a child's d_fullpath still names the old
 * location. A relative stat cannot see that, so the serving side must
 * keep the string true itself: verify the parent-prefix invariant on
 * every success and rebuild d_fullpath in place on mismatch — same
 * object, no expunge.
 *
 * Cost when the path is intact (every non-rename call): one integer
 * length compare, one byte load, one short memcmp — noise next to the
 * fstatat it rides on. */
static void pfd_repair_path(struct dir *entry, const struct dir *parent)
{
    int plen = blength(parent->d_fullpath);
    int nlen = blength(entry->d_u_name);
    const char *ep = bdata(entry->d_fullpath);
    const char *pp = bdata(parent->d_fullpath);

    if (ep != NULL && pp != NULL
            && blength(entry->d_fullpath) == plen + 1 + nlen
            && ep[plen] == '/'
            && memcmp(ep, pp, (size_t)plen) == 0) {
        return;
    }

    /* Both lengths are known: one exactly-sized allocation, no strlen,
     * no realloc */
    bstring repaired = fullpath_join_blk(parent->d_fullpath,
                                         bdata(entry->d_u_name), nlen);

    if (repaired == NULL) {
        /* Keep the stale string; the next validation retries */
        return;
    }

    LOG(log_debug, logtype_afpd, "pfd_repair_path: \"%s\" -> \"%s\"",
        ep ? ep : "(null)", cfrombstr(repaired));
    bdestroy(entry->d_fullpath);
    entry->d_fullpath = repaired;
    pfd_stat.path_repairs++;
}

/*!
 * @brief ostat(cfrombstr(entry->d_fullpath), st, options), accelerated.
 *
 * Resolves entry's parent to a cached dir fd and fstatat()s the leaf name;
 * any impediment (no parent entry, ghost parent, open failure) falls back
 * to the full-path ostat internally. Callers cannot observe which path ran:
 * return value and errno are exactly what ostat would produce.
 *
 * On success the entry's d_fullpath is additionally repaired against the
 * parent's — the relative stat can outlive a rename the path string hasn't
 * caught up with (see pfd_repair_path).
 */
int pfd_ostat(const struct vol *vol, struct dir *entry,
              struct stat *st, int options)
{
    const struct dir *parent = NULL;

    if (entry->d_did != DIRDID_ROOT && entry->d_did != DIRDID_ROOT_PARENT
            && entry->d_u_name != NULL) {
        if (entry->d_pdid == DIRDID_ROOT) {
            /* The volume root is never in the dircache hash but is always
             * live on the vol with d_fullpath = v_path and dcache_ino
             * populated by dir_new() at afp_openvol(). */
            parent = vol->v_root;
        } else if (entry->d_pdid != DIRDID_ROOT_PARENT) {
            parent = dircache_lookup_parent(vol, entry->d_pdid);
        }
    }

    if (parent != NULL && parent->d_fullpath != NULL
            && parent->dcache_ino != 0) {
        int fd = pfd_get(vol, parent);

        if (fd >= 0) {
            /* fd is verified >= 0: ostatat(-1, ...) means AT_FDCWD (a
             * CWD-relative stat) and must never occur. */
            int rc = ostatat(fd, cfrombstr(entry->d_u_name), st, options);

            if (rc == 0) {
                pfd_repair_path(entry, parent);
            }

            return rc;
        }
    }

    pfd_stat.fallbacks++;
    return ostat(cfrombstr(entry->d_fullpath), st, options);
}

/* Retire the slot for (vid, did) if present. Keyed on the raw vid so the
 * unpublish choke point (dircache_remove) can purge from the entry's own
 * d_vid — several of its callers pass vol == NULL. Runs on the main thread
 * AND on the idle worker (deferred chains) under temporal separation. */
void pfd_purge(uint16_t vid, cnid_t did)
{
    if (!pfd_initialized) {
        return;
    }

    for (int i = 0; i < PFD_SLOTS; i++) {
        if (pfd_slots[i].fd >= 0 && pfd_slots[i].vid == vid
                && pfd_slots[i].did == did) {
            slot_retire(&pfd_slots[i]);
            pfd_stat.purges++;
            return;
        }
    }
}

void pfd_purge_vol(uint16_t vid)
{
    if (!pfd_initialized) {
        return;
    }

    for (int i = 0; i < PFD_SLOTS; i++) {
        if (pfd_slots[i].fd >= 0 && pfd_slots[i].vid == vid) {
            slot_retire(&pfd_slots[i]);
            pfd_stat.purges++;
        }
    }
}

void pfd_shutdown(void)
{
    if (!pfd_initialized) {
        return;
    }

    for (int i = 0; i < PFD_SLOTS; i++) {
        slot_retire(&pfd_slots[i]);
    }
}

void pfd_stats_get(struct pfd_stats *out)
{
    *out = pfd_stat;
}

void pfd_log_stats(void)
{
    /* Healthy shape: hits >> opens, refresh counters near zero. Either
     * refresh counter approaching hits means slots are being rebuilt
     * per-call — pathological; report it. */
    LOG(log_info, logtype_afpd,
        "pfd_cache: %llu hits, %llu opens, %llu sync-refreshes, "
        "%llu probe-refreshes, %llu regrounds, %llu fallbacks, %llu purges, "
        "%llu path-repairs",
        pfd_stat.hits, pfd_stat.opens, pfd_stat.sync_refreshes,
        pfd_stat.probe_refreshes, pfd_stat.regrounds, pfd_stat.fallbacks,
        pfd_stat.purges, pfd_stat.path_repairs);
}
