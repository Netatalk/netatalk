#ifndef LIBATALK_ADOUBLE_AD_PRIVATE_H
#define LIBATALK_ADOUBLE_AD_PRIVATE_H 1

#include <atalk/adouble.h>

/* this is so that we can keep lists of fds referencing the same file
 * around. that way, we can honor locks created by the same process
 * with the same file. */

#define adf_lock_free(a)                                      \
    do {                                                      \
        ;                                                     \
    } while (0)

#endif /* libatalk/adouble/ad_private.h */
