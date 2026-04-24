/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * Copyright (c) 2024,2026 Daniel Markstedt <daniel@mindani.net>
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_ICON_H
#define AFPD_ICON_H 1

/* Icon resource sizes */
#define ICN_HASH_SIZE 256   /* ICN# resource: 32x32 1-bit icon + 32x32 mask */
#define ICL4_SIZE     512   /* icl4 resource: 32x32 4-bit color icon */
#define ICL8_SIZE     1024  /* icl8 resource: 32x32 8-bit color icon */

/* Resource type codes */
#define RESTYPE_ICNH  0x49434E23  /* 'ICN#' */
#define RESTYPE_ICL4  0x69636C34  /* 'icl4' */
#define RESTYPE_ICL8  0x69636C38  /* 'icl8' */

/* Monochrome icons (ICN#) */
extern const unsigned char declogo_icon[];
extern const unsigned char daemon_icon[];
extern const unsigned char fileserver_icon[];
extern const unsigned char globe_icon[];
extern const unsigned char nas_icon[];
extern const unsigned char sdcard_icon[];
extern const unsigned char sunlogo_icon[];
extern const unsigned char viking_icon[];

/* Colorized icons (icl4 = 4-bit, icl8 = 8-bit) */
extern const unsigned char declogo_icon_icl4[];
extern const unsigned char declogo_icon_icl8[];
extern const unsigned char daemon_icon_icl4[];
extern const unsigned char daemon_icon_icl8[];
extern const unsigned char fileserver_icon_icl4[];
extern const unsigned char fileserver_icon_icl8[];
extern const unsigned char globe_icon_icl4[];
extern const unsigned char globe_icon_icl8[];
extern const unsigned char nas_icon_icl4[];
extern const unsigned char nas_icon_icl8[];
extern const unsigned char sdcard_icon_icl4[];
extern const unsigned char sdcard_icon_icl8[];
extern const unsigned char sunlogo_icon_icl4[];
extern const unsigned char sunlogo_icon_icl8[];
extern const unsigned char viking_icon_icl4[];
extern const unsigned char viking_icon_icl8[];

#endif /* AFPD_ICON_H */
