/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifndef _ATALK_ASP_H
#define _ATALK_ASP_H 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/afp.h>
#include <atalk/server_child.h>

#define ASP_HDRSIZ        4
#define ASP_CMDSIZ        578

#define ASP_MAXPACKETS    8
#define ASP_CMDMAXSIZ     (ASP_CMDSIZ + ASP_HDRSIZ)
#define ASP_DATASIZ       (ASP_CMDSIZ*ASP_MAXPACKETS)
#define ASP_DATAMAXSIZ    ((ASP_CMDSIZ + ASP_HDRSIZ)*ASP_MAXPACKETS)

typedef struct ASP {
    ATP			asp_atp;
    struct sockaddr_at	asp_sat;
    u_int8_t	        asp_wss;
    u_int8_t            asp_sid;
    union {
	struct {
	    char			*as_status;
	    int				as_slen;
	}			asu_status;
	u_int16_t		asu_seq;
    }			asp_u;
#define asp_status	asp_u.asu_status.as_status
#define asp_slen	asp_u.asu_status.as_slen
#define asp_seq		asp_u.asu_seq
    int			asp_flags;
    char		child, inited, *commands;
    char                cmdbuf[ASP_CMDMAXSIZ];
    char                data[ASP_DATAMAXSIZ];  
    size_t		cmdlen, datalen;
    off_t 		read_count, write_count;
} *ASP;

#define ASPFL_SLS	1
#define ASPFL_SSS	2

#define ASPFUNC_CLOSE	1
#define ASPFUNC_CMD	2
#define ASPFUNC_STAT	3
#define ASPFUNC_OPEN	4
#define ASPFUNC_TICKLE	5
#define ASPFUNC_WRITE	6
#define ASPFUNC_WRTCONT	7
#define ASPFUNC_ATTN	8

#define ASPERR_OK	0x0000
#define ASPERR_BADVERS	0xfbd6
#define ASPERR_BUFSMALL	0xfbd5
#define ASPERR_NOSESS	0xfbd4
#define ASPERR_NOSERV	0xfbd3
#define ASPERR_PARM	0xfbd2
#define ASPERR_SERVBUSY	0xfbd1
#define ASPERR_SESSCLOS	0xfbd0
#define ASPERR_SIZERR	0xfbcf
#define ASPERR_TOOMANY	0xfbce
#define ASPERR_NOACK	0xfbcd

extern ASP asp_init         (ATP);
extern void asp_setstatus   (ASP, char *, const int);
extern ASP asp_getsession   (ASP, server_child *, const int);
extern int asp_close        (ASP);
extern int asp_shutdown     (ASP);
extern int asp_attention    (ASP, AFPUserBytes);
extern int asp_getrequest   (ASP);
extern int asp_cmdreply     (ASP, int);
extern int asp_wrtcont      (ASP, char *, size_t *);
#define asp_wrtreply(a,b)   asp_cmdreply((a), (b))
extern void asp_kill        (int);
extern int asp_tickle      (ASP, const u_int8_t, struct sockaddr_at *);
extern void asp_stop_tickle (void);

#endif
