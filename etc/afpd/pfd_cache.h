/* Parent-directory fd cache — module and threading contract documented in
 * pfd_cache.c */

#ifndef AFPD_PFD_CACHE_H
#define AFPD_PFD_CACHE_H 1

#include <sys/stat.h>

#include <atalk/directory.h>

#include "volume.h"

/* One absolute re-ground probe per this many uses of a slot — the
 * ground-truth bound on rename-away staleness (unit tests assert on it) */
#define PFD_REGROUND_INTERVAL 256

struct pfd_stats {
    unsigned long long hits;            /*!< fd served */
    unsigned long long opens;           /*!< slot fills (full path resolutions) */
    unsigned long long
    sync_refreshes;  /*!< dircache-learned mismatches, refilled */
    unsigned long long probe_refreshes; /*!< re-ground probe mismatches, refilled */
    unsigned long long regrounds;       /*!< absolute-path probes issued */
    unsigned long long fallbacks;       /*!< fell back to full-path ostat */
    unsigned long long purges;          /*!< slots retired by purge hooks */
    unsigned long long path_repairs;    /*!< stale d_fullpath rebuilt from parent */
};

extern int  pfd_ostat(const struct vol *vol, struct dir *entry,
                      struct stat *st, int options);
extern void pfd_purge(uint16_t vid, cnid_t did);
extern void pfd_purge_vol(uint16_t vid);
extern void pfd_shutdown(void);
extern void pfd_log_stats(void);
extern void pfd_stats_get(struct pfd_stats *out);

#endif /* AFPD_PFD_CACHE_H */
