/*
 * Private declarations shared between etc/afpd/spotlight.c and the
 * pluggable Spotlight search backends.  Not a public API.
 */

#ifndef SPOTLIGHT_PRIVATE_H
#define SPOTLIGHT_PRIVATE_H

#include <atalk/spotlight.h>

/* Session-local query ownership and backend cleanup. */
extern void slq_add(slq_t *slq);
extern slq_t *slq_for_ctx(uint64_t ctx1, uint64_t ctx2);
extern void slq_destroy(slq_t *slq);
extern void slq_cancel(slq_t *slq);
extern int slq_close(slq_t *slq);
extern void slq_cancelled_cleanup(void);
extern void slq_idle_cleanup(void);
extern void slq_dump(void);
extern const char *slq_state_name(slq_state_t state);

/*
 * Add metadata for one search result to the fm_array.
 * Defined in etc/afpd/spotlight.c.
 */
extern bool add_filemeta(sl_array_t *reqinfo,
                         sl_array_t *fm_array,
                         const char *path,
                         const struct stat *sp);

/*
 * True when `path` equals `scope` or lies underneath it (component
 * boundary aware). Defined in etc/afpd/spotlight.c.
 */
extern bool sl_path_in_scope(const char *path, const char *scope);

/*
 * Validate and copy a client-supplied CNID array into a query.
 * Defined in etc/afpd/spotlight.c.
 */
extern int sl_createCNIDArray(slq_t *slq, const DALLOC_CTX *p);

#endif /* SPOTLIGHT_PRIVATE_H */
