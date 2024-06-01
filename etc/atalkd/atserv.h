/*
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */
#ifndef ATALKD_ATSERV_H
#define ATALKD_ATSERV_H 1

struct atport {
    int			ap_fd;
    struct atport	*ap_next;
    struct interface	*ap_iface;
    unsigned char		ap_port;
    int			(*ap_packet)(struct atport *ap, struct sockaddr_at *from, char *data, int len);
};

struct atserv {
    char	*as_name;
    unsigned char	as_port;		/* Used as a fall back */
    int		(*as_packet)(struct atport *ap, struct sockaddr_at *from, char *data, int len);
};

#endif
