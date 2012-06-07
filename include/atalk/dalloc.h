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

#define dalloc_add(dd, obj, type)                                       \
    dalloc_add_talloc_chunk((dd), talloc((dd), type), (obj), sizeof(type));

#define dd_get_count(dd) talloc_array_length(dd->dd_talloc_array)

extern int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj, size_t size);


#endif  /* DALLOC_H */
