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

/* dynamic datastore element types */
typedef enum {
    DDT_INT64,
    DDT_BOOL,
    DDT_DATE,
    DDT_UUID,
    DDT_FLOAT,
    DDT_STRING,
    DDT_UTF16_STRING,
    DDT_ARRAY,
    DDT_DICTIONARY
} dde_type_e;

/* one dynamic datastore element */
typedef struct {
    dde_type_e dde_type;    /* type */
    void       *dde_val;    /* void pointer to value */
} dde_t;

/* dynamic datastore */
typedef struct {
    int   dd_count;         /* number of elements */
    dde_t **dd_elem;        /* talloc'ed array of elements */
} dd_t;
