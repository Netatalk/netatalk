/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * This file handles byte ordering.
 */

#ifndef _ATALK_ENDIAN_H_
#define _ATALK_ENDIAN_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <netinet/in.h>

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
