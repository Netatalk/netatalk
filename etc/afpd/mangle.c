/* 
 * $Id: mangle.c,v 1.16 2003-03-09 20:37:27 didg Exp $ 
 *
 * Copyright (c) 2002. Joe Marcus Clarke (marcus@marcuscom.com)
 * All Rights Reserved.  See COPYRIGHT.
 *
 * mangle, demangle (filename):
 * mangle or demangle filenames if they are greater than the max allowed
 * characters for a given version of AFP.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef FILE_MANGLING
#include "mangle.h"

char *
demangle(const struct vol *vol, char *mfilename) {
	char *filename = NULL;
	char *ext = NULL;
	size_t ext_len = 0;
	char *mangle;

	/* Is this really a mangled file? */
	mangle = strstr(mfilename, MANGLE_CHAR);
	if (!mangle) {
	    return mfilename;
	}

	if (NULL != (ext = strrchr(mfilename, '.')) ) {
	    ext_len = strlen(ext);
	}
	if (strlen(mangle) != strlen(MANGLE_CHAR) + MANGLE_LENGTH + ext_len) {
	    return mfilename;
	}

	filename = cnid_mangle_get(vol->v_db, mfilename);

	/* No unmangled filename was found. */
	if (filename == NULL) {
#ifdef DEBUG
	    LOG(log_debug, logtype_afpd, "demangle: Unable to lookup %s in the mangle database", mfilename);
#endif
	    return mfilename;
	}
	return filename;
}

/* -----------------------
   with utf8 filename not always round trip
   filename   mac filename too long or with unmatchable utf8 replaced with _
   uname      unix filename 
*/
char *
mangle(const struct vol *vol, char *filename, char *uname, int flags) {
    char *ext = NULL;
    char *tf = NULL;
    char *m = NULL;
    static char mfilename[MAX_LENGTH + 1];
    char mangle_suffix[MANGLE_LENGTH + 1];
    size_t ext_len = 0;
    int mangle_suffix_int = 0;

    /* Do we really need to mangle this filename? */
    if (!flags && strlen(filename) <= vol->max_filename) {
	return filename;
    }
    /* First, attempt to locate a file extension. */
    if (NULL != (ext = strrchr(filename, '.')) ) {
	ext_len = strlen(ext);
	if (ext_len > MAX_EXT_LENGTH) {
	    /* Do some bounds checking to prevent an extension overflow. */
	    ext_len = MAX_EXT_LENGTH;
	}
    }

    /* Check to see if we already have a mangled filename by this name. */
    while (1) {
	m = mfilename;
    	strncpy(m, filename, MAX_LENGTH - strlen(MANGLE_CHAR) - MANGLE_LENGTH - ext_len);
	m[MAX_LENGTH - strlen(MANGLE_CHAR) - MANGLE_LENGTH - ext_len] = '\0';

    	strcat(m, MANGLE_CHAR);
    	(void)sprintf(mangle_suffix, "%03d", mangle_suffix_int);
    	strcat(m, mangle_suffix);

    	if (ext) {
		strncat(m, ext, ext_len);
    	}

	tf = cnid_mangle_get(vol->v_db, m);
	if (tf == NULL || (strcmp(tf, uname)) == 0) {
	    break;
	}
	else {
	    if (++mangle_suffix_int > MAX_MANGLE_SUFFIX_LENGTH) {
		LOG(log_error, logtype_afpd, "mangle: Failed to find a free mangle suffix; returning original filename");
		return filename;
	    }
	}
    }

    if (cnid_mangle_add(vol->v_db, m, uname) < 0) {
	return filename;
    }

    return m;
}
#endif /* FILE_MANGLING */
