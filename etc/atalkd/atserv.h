/*
 * $Id: atserv.h,v 1.2 2001-06-25 20:13:45 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

struct atserv {
    char	*as_name;
    u_char	as_port;		/* Used as a fall back */
    int		(*as_packet)();
};

struct atport {
    int			ap_fd;
    struct atport	*ap_next;
    struct interface	*ap_iface;
    u_char		ap_port;
    int			(*ap_packet)();
};

extern struct atserv	atserv[];
extern int		atservNATSERV;
