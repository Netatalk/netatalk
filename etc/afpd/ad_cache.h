#ifndef AD_CACHE_H
#define AD_CACHE_H

#include <stdbool.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
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

#endif /* AD_CACHE_H */
