/*
 * Copyright (c) 2011 Frank Lahm
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef ATALK_LOCKING_H
#define ATALK_LOCKING_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <inttypes.h>

extern int locktable_init(const char *path);

#endif  /* ATALK_LOCKING_H */
