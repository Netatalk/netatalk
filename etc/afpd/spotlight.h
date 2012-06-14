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

#include <stdint.h>

#include <atalk/dalloc.h>
#include <atalk/globals.h>

typedef DALLOC_CTX     sl_array_t;    /* an array of elements                                           */
typedef DALLOC_CTX     sl_dict_t;     /* an array of key/value elements                                 */
typedef DALLOC_CTX     sl_filemeta_t; /* an array of elements                                           */
typedef int            sl_nil_t;      /* a nil element                                                  */
typedef bool           sl_bool_t;     /* a boolean, we avoid bool_t as it's a define for something else */
typedef struct timeval sl_time_t;     /* a boolean, we avoid bool_t as it's a define for something else */
typedef struct {
    char sl_uuid[16];
}                      sl_uuid_t;     /* a UUID                                                         */
typedef struct {
    uint16_t   ca_unkn1;
    uint32_t   ca_unkn2;
    DALLOC_CTX *ca_cnids;
}                      sl_cnids_t;    /* an array of CNID                                               */

extern int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen);

#endif /* SPOTLIGHT_H */
