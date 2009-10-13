/*
 * $Id: zip.h,v 1.4 2009-10-13 22:55:37 didg Exp $
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
struct ziptab	*newzt (const int, const char *);

int addzone ( struct rtmptab *, int, char * );
int zip_getnetinfo ( struct interface * );
int zip_packet(struct atport *ap,struct sockaddr_at *from, char *data, int len);

#endif /* atalkd/zip.h */
