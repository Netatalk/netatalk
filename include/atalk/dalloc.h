/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef DALLOC_H
#define DALLOC_H

#include <atalk/talloc.h>

/* dynamic datastore */
typedef struct {
    void **dd_talloc_array;
} DALLOC_CTX;

/* Use dalloc_add_copy() macro, not this function */
extern int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj, size_t size);

#define dalloc_add_copy(d, obj, type) dalloc_add_talloc_chunk((d), talloc((d), type), (obj), sizeof(type));
#define dalloc_add(d, obj, type) dalloc_add_talloc_chunk((d), NULL, (obj), 0);
extern void *dalloc_get(const DALLOC_CTX *d, ...);
extern void *dalloc_value_for_key(const DALLOC_CTX *d, ...);
extern int dalloc_size(DALLOC_CTX *d);
extern char *dalloc_strdup(const void *ctx, const char *string);
extern char *dalloc_strndup(const void *ctx, const char *string, size_t n);
#endif  /* DALLOC_H */
