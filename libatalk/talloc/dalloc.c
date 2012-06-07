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

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/bstrlib.h>
#include <atalk/dalloc.h>

/* Use dalloc_add_obj() macro, not this function */
int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj, size_t size)
{
    AFP_ASSERT(talloc_chunk);

    dd->dd_talloc_array = talloc_realloc(dd, dd->dd_talloc_array, void *, talloc_array_length(dd->dd_talloc_array) + 1);
    memcpy(talloc_chunk, obj, size);
    dd->dd_talloc_array[talloc_array_length(dd->dd_talloc_array) - 1] = talloc_chunk;

    return 0;
}
