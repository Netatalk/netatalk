/*
 * $Id: at_var.h,v 1.2 2001-06-29 14:14:47 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 *
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef _ATVAR_H
#define _ATVAR_H 1

/*
 * For phase2, we need to keep not only our address on an interface,
 * but also the legal networks on the interface.
 */
struct at_ifaddr {
    struct ifaddr	aa_ifa;
# define aa_ifp			aa_ifa.ifa_ifp
#ifdef BSD4_4
    struct sockaddr_at	aa_addr;
    struct sockaddr_at	aa_broadaddr;
    struct sockaddr_at	aa_netmask;
#else /* BSD4_4 */
# define aa_addr		aa_ifa.ifa_addr
# define aa_broadaddr		aa_ifa.ifa_broadaddr
# define aa_dstaddr		aa_ifa.ifa_dstaddr
#endif /* BSD4_4 */
    int			aa_flags;
    u_short		aa_firstnet, aa_lastnet;
    int			aa_probcnt;
    struct at_ifaddr	*aa_next;
};

#ifdef BSD4_4
struct at_aliasreq {
	char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	sockaddr_at ifra_addr;
	struct	sockaddr_at ifra_broadaddr;
#define ifra_dstaddr ifra_broadaddr
	struct	sockaddr_at ifra_mask;
};
#endif /* BSD4_4 */

#define AA_SAT(aa) \
    ((struct sockaddr_at *)&((struct at_ifaddr *)(aa))->aa_addr)
#define satosat(sa)	((struct sockaddr_at *)(sa))

#define AFA_ROUTE	0x0001
#define AFA_PROBING	0x0002
#define AFA_PHASE2	0x0004

#ifdef KERNEL
struct at_ifaddr	*at_ifaddr;
struct ifqueue		atintrq1, atintrq2;
int			atdebug;
#endif /* KERNEL */

#endif /* _ATVAR_H */
