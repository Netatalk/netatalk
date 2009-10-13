/*
 * $Id: nbp_conf.h,v 1.3 2009-10-13 22:55:37 didg Exp $
 *
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

#ifndef NBP_CONF_H
#define NBP_CONF_H 1

#include <sys/cdefs.h>
#include <atalk/nbp.h>

extern char		nbp_send[ 1024 ];
extern char		nbp_recv[ 1024 ];
extern u_char		nbp_port;
extern unsigned char    nbp_id;


int nbp_parse (char *, struct nbpnve *, int);
int nbp_match (struct nbpnve *, struct nbpnve *, int);

#endif /* NBP_CONF_H */
