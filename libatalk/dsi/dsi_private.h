/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifndef _DSI_PRIVATE_H
#define _DSI_PRIVATE_H 1

/* this header handles interactions between protocol-specific code and
 * dsi initialization. only dsi_init.c and dsi_<proto>.c should
 * include it.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>

extern int dsi_tcp_init __P((DSI *, const char * /*host*/, 
			     const char * /*address*/,
			     const u_int16_t /*port*/,
			     const int /*proxy*/));

#endif /* _DSI_PRIVATE_H */
