/*
 * Private declarations shared between etc/afpd/spotlight.c and the
 * pluggable Spotlight search backends.  Not a public API.
 */

#ifndef SPOTLIGHT_PRIVATE_H
#define SPOTLIGHT_PRIVATE_H

#include <atalk/spotlight.h>

/*
 * Add metadata for one search result to the fm_array.
 * Defined in etc/afpd/spotlight.c.
 */
extern bool add_filemeta(sl_array_t *reqinfo,
                         sl_array_t *fm_array,
                         const char *path,
                         const struct stat *sp);

#endif /* SPOTLIGHT_PRIVATE_H */
