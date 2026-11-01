/*!
 * @file
 * @brief AppleDouble metadata and resource fork cache interface
 */

#ifndef AD_CACHE_H
#define AD_CACHE_H

#include <stdbool.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/volume.h>

extern void ad_store_to_cache(struct adouble *adp, struct dir *cached);
extern void ad_rebuild_from_cache(struct adouble *adp,
                                  const struct dir *cached);
extern int  ad_metadata_cached(const char *name, int flags, struct adouble *adp,
                               const struct vol *vol, struct dir *dir,
                               bool strict, struct stat *recent_st);

/* AD cache statistics — defined in ad_cache.c, logged by dircache.c */
extern unsigned long long ad_cache_hits;
extern unsigned long long ad_cache_misses;
extern unsigned long long ad_cache_no_ad;

/* Tier 2: Resource Fork data cache helpers — defined in ad_cache.c */
extern int  rfork_cache_store_from_fd(struct dir *entry,
                                      struct adouble *adp, int eid);
extern void rfork_cache_free(struct dir *entry);
extern void rfork_cache_evict_to_budget(size_t needed);

/*! @brief The stamp a resource fork's stat gives its cached copy: its ctime */
static inline struct timespec rfork_stamp(const struct stat *st)
{
    return atalk_stat_ctime_timespec(st);
}

struct ea_cache_stats {
    unsigned long long hits;      /*!< names or a value served from an entry */
    unsigned long long misses;    /*!< lookups the entry could not serve */
    unsigned long long stored;    /*!< names lists and values stored */
    unsigned long long oversize;  /*!< lists over the cap, values out of room */
    unsigned long long dropped;   /*!< populated caches released */
    unsigned long long absent;    /*!< names a cached list proved missing */
};

extern struct ea_cache_stats ea_cache_stat;

/* Tier 3: EA names and values per entry — defined in ad_cache.c */
extern struct dir *ea_cache_entry(struct vol *vol, struct path *path,
                                  int oflag, const char *name);
extern const char *ea_cache_names(struct dir *entry, size_t *len);
extern void ea_cache_store_names(struct dir *entry, const char *names,
                                 size_t len);
extern void ea_cache_free(struct dir *entry);
extern const char *ea_cache_value(struct dir *entry, const char *name,
                                  size_t *len);
extern void ea_cache_store_value(struct dir *entry, const char *name,
                                 const char *value, size_t len);
extern bool ea_cache_absent(const struct dir *entry, const char *name);

#endif /* AD_CACHE_H */
