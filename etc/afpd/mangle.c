/* 
 * $Id: mangle.c,v 1.8 2002-07-04 18:14:38 jmarcus Exp $ 
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
	int ext_len = 0;
	char *mangle;

	/* Is this really a mangled file? */
	mangle = strstr(mfilename, MANGLE_CHAR);
	if (!mangle) {
	    LOG(log_error, logtype_default, "demangle: %s is not a mangled filename", mfilename);
	    return mfilename;
	}

	if ((ext = strrchr(mfilename, '.')) != NULL) {
	    ext_len = strlen(ext);
	}
	if (strlen(mangle) != strlen(MANGLE_CHAR) + MANGLE_LENGTH + ext_len) {
	    LOG(log_error, logtype_default, "demangle: %s is not long enough to be a mangled filename", mfilename);
	    return mfilename;
	}

	filename = cnid_mangle_get(vol->v_db, mfilename);

	/* No unmangled filename was found. */
	if (filename == NULL) {
	    LOG(log_error, logtype_default, "demangle: Unable to lookup %s in the mangle database", mfilename);
	    return mfilename;
	}

	return filename;
}

char *
mangle(const struct vol *vol, char *filename) {
    char *ext = NULL;
    char *tf = NULL;
    char *m = NULL;
    static char mfilename[MAX_LENGTH + 1];
    char mangle_suffix[MANGLE_LENGTH + 1];
    int ext_len = 0;
    int mangle_suffix_int = 0;

    /* Do we really need to mangle this filename? */
    if (strlen(filename) <= MAX_LENGTH) {
	return filename;
    }

    /* First, attmept to locate a file extension. */
    if ((ext = strrchr(filename, '.')) != NULL) {
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
	if (tf == NULL || (strcmp(tf, filename)) == 0) {
	    break;
	}
	else {
	    if (++mangle_suffix_int > MAX_MANGLE_SUFFIX_LENGTH) {
		LOG(log_error, logtype_default, "mangle: Failed to find a free mangle suffix; returning original filename");
		return filename;
	    }
	}
    }

    if (cnid_mangle_add(vol->v_db, m, filename) < 0) {
	return filename;
    }

    return m;
}
#endif /* FILE_MANGLING */
