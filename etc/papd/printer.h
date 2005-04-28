/*
 * $Id: printer.h,v 1.6 2005-04-28 20:49:49 bfernhomberg Exp $
 *
 * Copyright (c) 1990,1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

struct printer {
    char		*p_name;
    char		*p_type;
    char		*p_zone;
    char		*p_u_name;
#ifdef notdef
    char		*p_fonts;
    char		*p_psetdir;
#endif /* notdef */
    char		*p_ppdfile;
    char		p_status[255];
    char		*p_authprintdir;
    int			p_flags;
    struct at_addr      p_addr;
    union {
	struct {
	    char		*pr_printer;
	    char		*pr_operator;
	    char		*pr_spool;
#ifdef ABS_PRINT
	    char		*pr_role;
	    double		pr_balance;
	    int			pr_srvid;
#endif /* ABS_PRINT */
	    int			pr_pagecost;
	    char		*pr_pagecost_msg;
	    char		*pr_lock;
	} pu_pr;
	char		*pu_cmd;
    } p_un;
    ATP			p_atp;
#ifdef HAVE_CUPS
    char 		*p_cupsoptions;
#endif
    struct printer	*p_next;
};
#define p_cmd		p_un.pu_cmd
#define p_printer	p_un.pu_pr.pr_printer
#define p_operator	p_un.pu_pr.pr_operator
#define p_spool		p_un.pu_pr.pr_spool
#ifdef ABS_PRINT
#define p_role		p_un.pu_pr.pr_role
#define p_balance	p_un.pu_pr.pr_balance
#define p_srvid		p_un.pu_pr.pr_srvid
#endif /* ABS_PRINT */
#define p_pagecost	p_un.pu_pr.pr_pagecost
#define p_pagecost_msg	p_un.pu_pr.pr_pagecost_msg
#define p_lock		p_un.pu_pr.pr_lock

#define P_PIPED		(1<<0)
#define P_SPOOLED	(1<<1)
#define P_REGISTERED	(1<<2)
#define P_ACCOUNT	(1<<3)
#define P_KRB		(1<<4)
#define P_AUTH		(1<<5)
#define P_AUTH_PSSP	(1<<6)
#define P_AUTH_CAP	(1<<7)
#define P_CUPS		(1<<8)
#define P_CUPS_PPD	(1<<9)
#define P_CUPS_AUTOADDED (1<<10)
#define P_FOOMATIC_HACK (1<<11)

extern struct printer	*printer;
