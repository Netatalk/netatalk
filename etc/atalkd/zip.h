/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_ZIP_H
#define ATALKD_ZIP_H 1

#include <sys/cdefs.h>

struct ziptab {
    struct ziptab	*zt_next,
			*zt_prev;
    u_char		zt_len;
    char		*zt_name;
    u_char		*zt_bcast;
    struct list		*zt_rt;
};

extern struct ziptab	*ziptab, *ziplast;
struct ziptab	*newzt __P((const int, const char *));

#endif /* atalkd/zip.h */
