/*
  Declarations for pluggable Spotlight search backend ops instances.
  Include this header wherever a backend ops pointer is needed.
*/

#ifndef SPOTLIGHT_BACKENDS_H
#define SPOTLIGHT_BACKENDS_H

#include <atalk/spotlight.h>

#ifdef SPOTLIGHT_BACKEND_CNID
extern const sl_backend_ops sl_cnid_ops;
#endif

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
extern const sl_backend_ops sl_localsearch_ops;
#endif

#ifdef SPOTLIGHT_BACKEND_XAPIAN
extern const sl_backend_ops sl_xapian_ops;
#endif

#endif /* SPOTLIGHT_BACKENDS_H */
