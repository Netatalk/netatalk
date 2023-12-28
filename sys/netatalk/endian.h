/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * This file handles both byte ordering and integer sizes.
 */

#ifndef _ATALK_ENDIAN_H_
#define _ATALK_ENDIAN_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <netinet/in.h>


#ifdef _ISOC9X_SOURCE
#include <inttypes.h>
#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__
typedef uint8_t        u_int8_t;
typedef uint16_t       u_int16_t;
typedef uint32_t       u_int32_t;
typedef uint64_t       u_int64_t;
#endif /* ! __BIT_TYPES_DEFINED__ */
#else

#if defined(HAVE_32BIT_LONGS) || defined(HAVE_64BIT_LONGS)
#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__
typedef unsigned char  u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int   u_int32_t;
typedef int            int32_t;
#endif
#endif /* HAVE_32BIT_LONGS || HAVE_64BIT_LONGS */


#ifdef HAVE_64BIT_LONGS
typedef unsigned long u_int64_t;
#else /* HAVE_64BIT_LONGS */
/* check for long long support. currently, i assume that if 64-bit
 * ints exist that their made available via long long */
#ifdef linux
#include <endian.h> /* i think this is here for libc4 */
#else /* linux */
#if defined(HAVE_32BIT_LONGS) && !(defined(BSD4_4) || \
				  defined(NO_LARGE_VOL_SUPPORT))
typedef unsigned long long  u_int64_t;
#endif /* HAVE_32BIT_LONGS || !BSD4_4 || NO_LARGE_VOL_SUPPORT */
#endif /* linux */
#endif /* HAVE_64BIT_LONGS */
#endif /* ISOC9X */

# ifndef BYTE_ORDER
#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define PDP_ENDIAN	3412


#if defined(WORDS_BIGENDIAN) || defined(_BIG_ENDIAN)
#define BYTE_ORDER	BIG_ENDIAN
#else
#define BYTE_ORDER	LITTLE_ENDIAN
#endif /* WORDS_BIGENDIAN */

#endif /* BYTE_ORDER */

#endif /* _ATALK_ENDIAN_H_ */

