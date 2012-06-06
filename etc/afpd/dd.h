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

/* dynamic datastore */
typedef struct {
    void **dd_talloc_array;
} dd_t;

#define dd_init(dd) (dd)->dd_talloc_array = NULL;

#define dd_add_obj(dd, obj, type)                                   \
    _dd_add_obj((dd), talloc((dd), type), (obj), sizeof(type));

#define dd_get_count(dd) talloc_array_length(dd)
