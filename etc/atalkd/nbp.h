/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

struct nbptab {
    struct nbptab	*nt_prev, *nt_next;
    struct nbpnve	nt_nve;
    struct interface    *nt_iface;
};

extern struct nbptab	*nbptab;
