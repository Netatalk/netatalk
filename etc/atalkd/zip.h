/*
 * $Id: zip.h,v 1.3 2001-06-25 20:13:45 rufustfirefly Exp $
 *
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

int addzone __P(( struct rtmptab *, int, char * ));
int zip_getnetinfo __P(( struct interface * ));

#endif /* atalkd/zip.h */
