/*
   Unix SMB/CIFS implementation.
   minimal iconv implementation
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Jelmer Vernooij 2002,2003
   Copyright (C) Panos Christeas 2006

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

   From samba 3.0 beta and GNU libiconv-1.8
   It's bad but most of the time we can't use libc iconv service:
   - it doesn't round trip for most encoding
   - it doesn't know about Apple extension

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>
#include <arpa/inet.h>

#include <atalk/unicode.h>

#include "mac_greek.h"
#include "generic_mb.h"

static size_t   mac_greek_pull(void *,char **, size_t *, char **, size_t *);
static size_t   mac_greek_push(void *,char **, size_t *, char **, size_t *);

struct charset_functions charset_mac_greek =
{
	"MAC_GREEK",
	6,
	mac_greek_pull,
	mac_greek_push,
	CHARSET_CLIENT | CHARSET_MULTIBYTE,
	NULL,
	NULL, NULL
};

/* ------------------------ */
static int
char_ucs2_to_mac_greek ( unsigned char *r, ucs2_t wc)
{
  unsigned char c = 0;
  if (wc < 0x0080) {
    *r = wc;
    return 1;
  }
  else if (wc >= 0x00a0 && wc < 0x0100)
    c = mac_greek_page00[wc-0x00a0];
  else if (wc == 0x0153)
    c = 0xcf;
  else if (wc >= 0x0380 && wc < 0x03d0)
    c = mac_greek_page03[wc-0x0380];
  else if (wc >= 0x2010 && wc < 0x2038)
    c = mac_greek_page20[wc-0x2010];
  else if (wc == 0x2122)
    c = 0x93;
  else if (wc >= 0x2248 && wc < 0x2268)
    c = mac_greek_page22[wc-0x2248];
  if (c != 0) {
    *r = c;
    return 1;
  }
 return 0;
 }

static size_t mac_greek_push( void *cd, char **inbuf, size_t *inbytesleft,
                         char **outbuf, size_t *outbytesleft)
{
	/* No special handling required */
	return (size_t) mb_generic_push( char_ucs2_to_mac_greek, cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

/* ------------------------ */
static int
char_mac_greek_to_ucs2 (ucs2_t *pwc, const unsigned char *s)
{
  unsigned char c = *s;
  if (c < 0x80) {
    *pwc = (ucs2_t) c;
    return 1;
  }
  else {
    unsigned short wc = mac_greek_2uni[c-0x80];
    if (wc != 0xfffd) {
      *pwc = (ucs2_t) wc;
      return 1;
    }
  }
  return 0;
}

static size_t mac_greek_pull ( void *cd, char **inbuf, size_t *inbytesleft,
                         char **outbuf, size_t *outbytesleft)
{
	return (size_t) mb_generic_pull( char_mac_greek_to_ucs2, cd, inbuf, inbytesleft, outbuf, outbytesleft);
}
