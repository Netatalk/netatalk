/*
 * $Id: mangle.h,v 1.3 2002-10-17 18:01:54 didg Exp $
 *
 */

#ifndef AFPD_MANGLE_H 
#define AFPD_MANGLE_H 1

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB */
#include <atalk/logger.h>

#include "globals.h"
#include "volume.h"
#include "directory.h"

#define MANGLE_CHAR "~"
#define MANGLE_LENGTH 3 /* XXX This really can't be changed. */
#define MAX_MANGLE_SUFFIX_LENGTH 999
#define MAX_EXT_LENGTH 4 /* XXX This cannot be greater than 27 */
#define MAX_LENGTH MACFILELEN 

extern char *mangle __P((const struct vol *, char *));
extern char *demangle __P((const struct vol *, char *));

#endif /* AFPD_MANGLE_H */
