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

#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include <atalk/dalloc.h>

typedef struct {
    uint16_t   ca_unkn1;
    uint32_t   ca_unkn2;
    DALLOC_CTX *ca_cnids;
} cnid_array_t;

#endif /* SPOTLIGHT_H */
