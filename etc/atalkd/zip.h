/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_ZIP_H
#define ATALKD_ZIP_H 1

#include <sys/types.h>

struct ziptab {
    struct ziptab	*zt_next,
                 *zt_prev;
    unsigned char		zt_len;
    char		*zt_name;
    unsigned char		*zt_bcast;
    struct list		*zt_rt;
};

extern struct ziptab	*ziptab, *ziplast;
struct ziptab	*newzt(const int, const char *);

int addzone(struct rtmptab *, int, char *);
int zip_getnetinfo(struct interface *);
int zip_packet(struct atport *ap, struct sockaddr_at *from, char *data,
               int len);

#endif /* atalkd/zip.h */
