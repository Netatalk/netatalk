/*
 * $Id: atserv.h,v 1.4 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */
#ifndef ATALKD_ATSERV_H
#define ATALKD_ATSERV_H 1

struct atport {
    int			ap_fd;
    struct atport	*ap_next;
    struct interface	*ap_iface;
    u_char		ap_port;
    int			(*ap_packet)(struct atport *ap, struct sockaddr_at *from, char *data, int len);
};

struct atserv {
    char	*as_name;
    u_char	as_port;		/* Used as a fall back */
    int		(*as_packet)(struct atport *ap, struct sockaddr_at *from, char *data, int len);
};

#endif
