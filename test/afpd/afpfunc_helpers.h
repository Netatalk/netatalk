/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifndef AFPFUNC_HELPERS
#define AFPFUNC_HELPERS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/bstrlib.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "hash.h"
#include "subtests.h"
#include "test.h"
#include "volume.h"

extern char **cnamewrap(const char *name);

extern int getfiledirparms(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int createdir(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int createfile(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int delete(AFPObj *obj, uint16_t vid, cnid_t did, const char *name);
extern int enumerate(AFPObj *obj, uint16_t vid, cnid_t did);
extern uint16_t openvol(AFPObj *obj, const char *name);

#endif  /* AFPFUNC_HELPERS */
