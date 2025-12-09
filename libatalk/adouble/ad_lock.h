#ifndef LIBATALK_ADOUBLE_AD_PRIVATE_H
#define LIBATALK_ADOUBLE_AD_PRIVATE_H 1

#include <atalk/adouble.h>

/* this is so that we can keep lists of fds referencing the same file
 * around. that way, we can honor locks created by the same process
 * with the same file. */

#define adf_lock_init(a) do {   \
        (a)->adf_lockmax = 0;   \
        (a)->adf_lockcount = 0; \
        (a)->adf_lock = NULL;   \
    } while (0)

#define adf_lock_free(a) do {                                 \
        int i;                                                \
        if (!(a)->adf_lock)                                   \
            break;                                            \
        for (i = 0; i < (a)->adf_lockcount; i++) {            \
            adf_lock_t *lock = (a)->adf_lock + i;             \
            if (--(*lock->refcount) < 1) {                    \
                free(lock->refcount);                         \
                /* Release the fcntl lock to prevent leaks */ \
                if ((a)->adf_fd >= 0) {                       \
                    lock->lock.l_type = F_UNLCK;              \
                    if (fcntl((a)->adf_fd, F_SETLK, &lock->lock) == -1) { \
                        /* Log failure but continue cleanup - best effort */ \
                        LOG(log_warning, logtype_default,     \
                            "adf_lock_free: fcntl unlock failed on fd %d: %s", \
                            (a)->adf_fd, strerror(errno));    \
                    }                                         \
                }                                             \
            }                                                 \
        }                                                     \
        free((a)->adf_lock);                                  \
        adf_lock_init(a);                                     \
    } while (0)

#endif /* libatalk/adouble/ad_private.h */
