/*
   Unix SMB/CIFS implementation.
   SMB Byte handling
   Copyright (C) Andrew Tridgell 1992-1998

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef _BYTEORDER_H
#define _BYTEORDER_H
#include <arpa/inet.h>

/*!
   @file
   Macros for machine independent short and int manipulation.

   Derived from Samba's byteorder.h by Andrew Tridgell, where SVAL/IVAL
   always decode little-endian SMB wire format using byte-order independent
   arithmetic, and RSVAL/RIVAL always decode big-endian (NMB) wire format.

   The netatalk version replaces this with a big-endian conditional
   that changes the semantics: SVAL/IVAL read in host-native byte order
   (safe unaligned access), and RSVAL/RIVAL read in the opposite byte
   order. This was done to support AFP (a big-endian protocol) and
   native-order in-memory Unicode on big-endian Unix servers.
*/

#define CVAL(buf,pos) ((unsigned)(((const unsigned char *)(buf))[pos]))
#define CVAL_NC(buf,pos) (((unsigned char *)(buf))[pos]) /*!< Non-const version of CVAL */
#define PVAL(buf,pos) (CVAL(buf,pos))
#define SCVAL(buf,pos,val) (CVAL_NC(buf,pos) = (val))

#ifdef WORDS_BIGENDIAN

#define SVAL(buf,pos) (PVAL(buf,(pos)+1)|PVAL(buf,pos)<<8)
#define SVALS(buf,pos) ((int16_t)SVAL(buf,pos))
#define IVAL(buf,pos) (SVAL(buf,(pos)+2)|SVAL(buf,pos)<<16)
#define IVALS(buf,pos) ((int32_t)IVAL(buf,pos))
#define LVAL(buf,pos) (IVAL(buf,(pos)+4)|((uint64_t)IVAL(buf,pos))<<32)
#define LVALS(buf,pos) ((int64_t)LVAL(buf,pos))

#define SSVALX(buf,pos,val) (CVAL_NC(buf,pos+1)=(unsigned char)((val)&0xFF),CVAL_NC(buf,pos)=(unsigned char)((val)>>8))
#define SIVALX(buf,pos,val) (SSVALX(buf,pos,(val)>>16),SSVALX(buf,pos+2,((val)&0xFFFF)))
#define SLVALX(buf,pos,val) (SIVALX(buf,pos,(val)>>32),SIVALX(buf,pos+4,((val)&0xFFFFFFFF)))

#else

#define SVAL(buf,pos) (PVAL(buf,pos)|PVAL(buf,(pos)+1)<<8)
#define SVALS(buf,pos) ((int16_t)SVAL(buf,pos))
#define IVAL(buf,pos) (SVAL(buf,pos)|SVAL(buf,(pos)+2)<<16)
#define IVALS(buf,pos) ((int32_t)IVAL(buf,pos))
#define LVAL(buf,pos) (IVAL(buf,pos)|((uint64_t)IVAL(buf,(pos)+4))<<32)
#define LVALS(buf,pos) ((int64_t)LVAL(buf,pos))

#define SSVALX(buf,pos,val) (CVAL_NC(buf,pos)=(unsigned char)((val)&0xFF),CVAL_NC(buf,pos+1)=(unsigned char)((val)>>8))
#define SIVALX(buf,pos,val) (SSVALX(buf,pos,((val)&0xFFFF)),SSVALX(buf,pos+2,(val)>>16))
#define SLVALX(buf,pos,val) (SIVALX(buf,pos,((val)&0xFFFFFFFF)),SIVALX(buf,pos+4,(val)>>32))

#endif /* WORDS_BIGENDIAN */

#define SSVAL(buf,pos,val) SSVALX((buf),(pos),((uint16_t)(val)))
#define SSVALS(buf,pos,val) SSVALX((buf),(pos),((int16_t)(val)))
#define SIVAL(buf,pos,val) SIVALX((buf),(pos),((uint32_t)(val)))
#define SIVALS(buf,pos,val) SIVALX((buf),(pos),((int32_t)(val)))
#define SLVAL(buf,pos,val) SLVALX((buf),(pos),((uint64_t)(val)))
#define SLVALS(buf,pos,val) SLVALX((buf),(pos),((int64_t)(val)))

/* now the reverse routines */
#define SREV(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define IREV(x) ((SREV((uint32_t)(x))<<16) | (SREV(((uint32_t)(x))>>16)))
#define LREV(x) (((uint64_t)IREV(x)<<32) | (IREV((x)>>32)))

#define RSVAL(buf,pos) SREV(SVAL(buf,pos))
#define RSVALS(buf,pos) SREV(SVALS(buf,pos))
#define RIVAL(buf,pos) IREV(IVAL(buf,pos))
#define RIVALS(buf,pos) IREV(IVALS(buf,pos))
#define RLVAL(buf,pos) LREV(LVAL(buf,pos))
#define RLVALS(buf,pos) LREV(LVALS(buf,pos))

#define RSSVAL(buf,pos,val) SSVAL(buf,pos,SREV(val))
#define RSSVALS(buf,pos,val) SSVALS(buf,pos,SREV(val))
#define RSIVAL(buf,pos,val) SIVAL(buf,pos,IREV(val))
#define RSIVALS(buf,pos,val) SIVALS(buf,pos,IREV(val))

#define RSLVAL(buf,pos,val) SLVAL(buf,pos,LREV(val))
#define RSLVALS(buf,pos,val) SLVALS(buf,pos,LREV(val))

/* Alignment macros. */
#define ALIGN4(p,base) ((p) + ((4 - (PTR_DIFF((p), (base)) & 3)) & 3))
#define ALIGN2(p,base) ((p) + ((2 - (PTR_DIFF((p), (base)) & 1)) & 1))

#endif /* _BYTEORDER_H */
