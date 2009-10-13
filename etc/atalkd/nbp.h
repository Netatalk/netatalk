/*
 * $Id: nbp.h,v 1.3 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_NBP_H
#define ATALKD_NBP_H 1

struct nbptab {
    struct nbptab	*nt_prev, *nt_next;
    struct nbpnve	nt_nve;
    struct interface    *nt_iface;
};

extern struct nbptab	*nbptab;

int nbp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len);

#endif
