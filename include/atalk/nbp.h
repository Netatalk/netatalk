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

#ifndef _ATALK_NBP_H
#define _ATALK_NBP_H 1

#define NBP_UNRGSTR_4ARGS 1
#define ATP_OPEN_2ARGS 1

#include <sys/types.h>
#include <sys/types.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>

/* Described in IAT 7-14 */

struct nbphdr {
#if BYTE_ORDER == BIG_ENDIAN
    uint32_t	nh_op : 4,
    		nh_cnt : 4,
#else /* BYTE_ORDER != BIG_ENDIAN */
    uint32_t	nh_cnt : 4,
    		nh_op : 4,
#endif /* BYTE_ORDER */
    		nh_id : 8;
};

#define SZ_NBPHDR	2


/* NBP tuple described in IAT 7-16 */
/* struct nbptuple is misnamed; describes only the entity address */
struct nbptuple {
    uint16_t   nt_net;
    uint8_t    nt_node;
    uint8_t    nt_port;
    uint8_t    nt_enum;
};
#define SZ_NBPTUPLE	5

#define NBPSTRLEN	32
/*
 * Name Binding Protocol Network Visible Entity
 * This is the rest of NBP tuple in IAT 7-16
 */
struct nbpnve {
    struct sockaddr_at	nn_sat;
    uint8_t		nn_objlen;
    char		nn_obj[ NBPSTRLEN ];
    uint8_t		nn_typelen;
    char		nn_type[ NBPSTRLEN ];
    uint8_t		nn_zonelen;
    char		nn_zone[ NBPSTRLEN ];
};

/*
 * Note that NBPOP_ERROR is not standard. As Apple adds more NBPOPs,
 * we need to check for collisions with our extra values.  */
#define NBPOP_BRRQ	 0x1
#define NBPOP_LKUP	 0x2
#define NBPOP_LKUPREPLY	 0x3
#define NBPOP_FWD	 0x4
#define NBPOP_RGSTR	 0x7
#define NBPOP_UNRGSTR	 0x8
#define NBPOP_CONFIRM    0x9
#define NBPOP_OK	 0xa  /* NBPOP_STATUS_REPLY */
#define NBPOP_CLOSE_NOTE 0xb

#define NBPOP_ERROR	 0xf

#define NBPMATCH_NOGLOB	(1<<1)
#define NBPMATCH_NOZONE	(1<<2)

extern int nbp_name (const char *, char **, char **, char **);
extern int nbp_lookup (const char *, const char *, const char *,
			   struct nbpnve *, const int,
			   const struct at_addr *);
extern int nbp_rgstr (struct sockaddr_at *,
			  const char *, const char *, const char *);
extern int nbp_unrgstr (const char *, const char *, const char *,
			    const struct at_addr *);

#endif
