/*
 * $Id: at_proto.c,v 1.2 2001-06-29 14:14:47 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 *
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "at.h"

extern int		ddp_usrreq();
extern int		ddp_output();
extern int		ddp_init();

#ifdef ultrix
extern int		ddp_ifoutput();
extern int		ddp_ifinput();
extern int		ddp_ifioctl();
#endif /* ultrix */

struct protosw		atalksw[] = {
    {
	/* Identifiers */
	SOCK_DGRAM,	&atalkdomain,	ATPROTO_DDP,	PR_ATOMIC|PR_ADDR,
	/*
	 * protocol-protocol interface.
	 * fields are pr_input, pr_output, pr_ctlinput, and pr_ctloutput.
	 * pr_input can be called from the udp protocol stack for iptalk
	 * packets bound for a local socket.
	 * pr_output can be used by higher level appletalk protocols, should
	 * they be included in the kernel.
	 */
	0,		ddp_output,	0,		0,
	/* socket-protocol interface. */
	ddp_usrreq,
	/* utility routines. */
	ddp_init,	0,		0,		0,
#ifdef ultrix
	/* interface hooks */
	ddp_ifoutput,	ddp_ifinput,	ddp_ifioctl,	0,
#endif /* ultrix */
    },
};

struct domain		atalkdomain = {
    AF_APPLETALK,	"appletalk",	0,	0,	0,	atalksw,
    &atalksw[ sizeof( atalksw ) / sizeof( atalksw[ 0 ] ) ]
};
