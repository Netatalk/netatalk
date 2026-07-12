/*
 * Compatibility wrapper for bstring.
 *
 * illumos declares a legacy bgets() function in libgen.h with a signature
 * that conflicts with bstring's bgets().  Netatalk does not use bstring's
 * bgets(), so hide that declaration while processing the system header.
 */

#ifndef ATALK_BSTRLIB_COMPAT_H
#define ATALK_BSTRLIB_COMPAT_H 1

#ifdef __sun
#define bgets netatalk_unused_bstring_bgets
#endif

#include <bstrlib.h>

#ifdef __sun
#undef bgets
#endif

#endif /* ATALK_BSTRLIB_COMPAT_H */
