#ifndef LIBATALK_ADOUBLE_AD_PRIVATE_H
#define LIBATALK_ADOUBLE_AD_PRIVATE_H 1

#include <atalk/adouble.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

#ifndef __inline__
#define __inline__
#endif

/* this is so that we can keep lists of fds referencing the same file
 * around. that way, we can honor locks created by the same process
 * with the same file. */
#ifdef USE_FLOCK_LOCKS
#define adf_lock_init(a)
#define adf_lock_free(a)
#else

#define adf_lock_init(a) do { \
	(a)->adf_lockmax = (a)->adf_lockcount = 0; \
	(a)->adf_lock = NULL; \
} while (0)

#define adf_lock_free(a) do { \
	if (!(a)->adf_lock) \
		break; \
	free((a)->adf_lock); \
	adf_lock_init(a); \
} while (0)
#endif

#endif /* libatalk/adouble/ad_private.h */
