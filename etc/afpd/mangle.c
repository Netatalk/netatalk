/* 
 * $Id: mangle.c,v 1.1 2002-05-29 18:02:59 jmarcus Exp $ 
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
	char dir[MAXPATHLEN+1];
	char *mangle;

	/* Is this really a mangled file? */
	mangle = strstr(mfilename, MANGLE_CHAR);
	if (!mangle) {
	    return mfilename;
	}

	ext = strrchr(mfilename, '.');
	if (strlen(mangle) != strlen(MANGLE_CHAR) + MANGLE_LENGTH + strlen(ext)) {
	    return mfilename;
	}

	getcwd(dir, MAXPATHLEN);

	if (strlen(dir) > MAXPATHLEN - strlen(mfilename) - 1) {
	    LOG(log_error, logtype_default, "demangle: Path is too large");
	    return mfilename;
	}

	strcat(dir, "/");
	strcat(dir, mfilename);

	filename = cnid_mangle_get(vol->v_db, dir);

	/* No unmangled filename was found. */
	if (filename == NULL) {
	    LOG(log_debug, logtype_default, "demangle: Unable to lookup %s in the mangle database", dir);
	    return mfilename;
	}

	return filename;
}
char *
mangle(const struct vol *vol, char *filename) {
    char *ext = NULL;
    char mfilename[MAX_LENGTH+1];
    char mangle_suffix[MANGLE_LENGTH+1];
    char dir[MAXPATHLEN+1];
    char tmp[MAXPATHLEN+1];
    int mangle_suffix_int = 0;

    /* Do we really need to mangle this filename? */
    if (strlen(filename) <= MAX_LENGTH) {
	return filename;
    }

    /* First, attmept to locate a file extension. */
    ext = strrchr(filename, '.');

    getcwd(dir, MAXPATHLEN);

    if (strlen(dir) > MAXPATHLEN - strlen(filename) - 1) {
	LOG(log_error, logtype_default, "mangle: path is too large");
	return filename;
    }

    /* Check to see if we already have a mangled filename by this name. */
    while (1) {
    	strcpy(tmp, dir);
    	strncpy(mfilename, filename, MAX_LENGTH - strlen(MANGLE_CHAR) - MANGLE_LENGTH - ((ext != NULL)?strlen(ext):0));
    	strcat(mfilename, MANGLE_CHAR);
    	(void)sprintf(mangle_suffix, "%03d", mangle_suffix_int);
    	strcat(mfilename, mangle_suffix);

    	if (ext) {
		strcat(mfilename, ext);
    	}
	
	strcat(tmp, "/");
	strcat(tmp, mfilename);

	if (!cnid_mangle_get(vol->v_db, tmp)) {
	    break;
	}
	else {
	    if (++mangle_suffix_int > MAX_MANGLE_SUFFIX_LENGTH) {
		LOG(log_error, logtype_default, "mangle: Failed to find a free mangle suffix; returning original filename");
		return filename;
	    }
	}
    }

    if (cnid_mangle_add(vol->v_db, mfilename, tmp) < 0) {
	return filename;
    }

    filename = mfilename;

    return filename;
}
#endif /* FILE_MANGLING */
