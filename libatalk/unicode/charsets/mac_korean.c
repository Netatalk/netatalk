/*
 * MacKorean
 * Copyright (C) TSUBAKIMOTO Hiroya <zorac@4000do.co.jp> 2004
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*!
 * @file
 * @brief MAC_KOREAN charset conversion
 * @sa Reference
 * http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>

#if HAVE_USABLE_ICONV

#include "generic_cjk.h"
#include "mac_korean.h"

static size_t mac_korean_pull(void *, char **, size_t *, char **, size_t *);
static size_t mac_korean_push(void *, char **, size_t *, char **, size_t *);

struct charset_functions charset_mac_korean = {
    "MAC_KOREAN",
    3,
    mac_korean_pull,
    mac_korean_push,
    CHARSET_ICONV | CHARSET_MULTIBYTE | CHARSET_PRECOMPOSED | CHARSET_CLIENT,
    "EUC-KR",
    NULL,
    NULL
};

/*!
 * @brief Map one UCS2 character, with the combining characters after it, to
 * MAC_KOREAN bytes
 *
 * An Apple sequence that starts with U+F860 to U+F867 composes first.
 * Otherwise the call keeps composing while a hint from U+F870 to U+F87F or
 * a combining mark follows and the composition table has the pair.
 * Private-use characters and characters without a table entry return 0 for
 * the EUC-KR fallback.
 *
 * @param[out]    out   receives one or two bytes
 * @param[in]     in    UCS2 input
 * @param[in,out] size  characters available, which receives the characters used
 * @returns the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with
 *          errno
 */
static size_t mac_korean_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
{
    ucs2_t wc = in[0];

    if ((wc & ~7) == 0xf860) {
        wc = cjk_compose_seq(in, size, mac_korean_compose,
                             sizeof(mac_korean_compose) / sizeof(uint32_t));

        if (!wc) {
            return (size_t) -1;
        }
    } else if ((wc & 0xf000) == 0xe000) {
        *size = 1;
        return 0;
    } else if (*size >= 2) {
        ucs2_t comb = in[1];
        size_t n = 1;

        while ((comb & ~15) == 0xf870 ||
                (comb >= 0x0300 && comb <= 0x036f) ||
                (comb >= 0x20d0 && comb <= 0x20ea)) {
            ucs2_t comp = cjk_compose(wc, comb, mac_korean_compose,
                                      sizeof(mac_korean_compose) / sizeof(uint32_t));

            if (!comp) {
                break;
            }

            wc = comp;

            if (++n == *size) {
                break;
            }

            comb = in[n];
        }

        *size = n;
    } else {
        *size = 1;
    }

    if (wc <= 0x7f) {
        out[0] = (uint8_t)wc;
        return 1;
    }

    return cjk_char_push(cjk_lookup(wc, mac_korean_uni2_index,
                                    mac_korean_uni2_charset), out);
}

/*!
 * @brief Convert UCS2 to MAC_KOREAN through cjk_generic_push
 *
 * @param[in]     cd            EUC-KR iconv descriptor for the fallback
 * @param[in,out] inbuf         UCS2 input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_korean_push(void *cd, char **inbuf, size_t *inbytesleft,
                              char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_push(mac_korean_char_push,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

/*!
 * @brief Map one MAC_KOREAN character of one or two bytes to UCS2
 *
 * ASCII passes as is, and bytes outside the lead byte range stand alone. A
 * lead byte from 0xa1 to 0xfe needs a trail byte in 0x41 to 0x7d or in 0x81
 * to 0xfe. The table value expands through the composition table into up to
 * CJK_PULL_BUFFER characters.
 *
 * @param[out]    out   receives the UCS2 characters
 * @param[in]     in    input bytes
 * @param[in,out] size  bytes available, which receives the bytes used
 * @returns the number of characters, 0 for the iconv fallback, or `(size_t)-1`
 *          with EILSEQ or EINVAL
 */
static size_t mac_korean_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
{
    uint16_t c = in[0];

    if (c <= 0x7f) {
        *size = 1;
        *out = c;
        return 1;
    } else if (c >= 0xa1 && c <= 0xfe) {
        if (*size >= 2) {
            uint8_t c2 = in[1];

            if ((c2 >= 0x41 && c2 <= 0x7d) || (c2 >= 0x81 && c2 <= 0xfe)) {
                *size = 2;
                c = (c << 8) + c2;
            } else {
                errno = EILSEQ;
                return (size_t) -1;
            }
        } else {
            errno = EINVAL;
            return (size_t) -1;
        }
    } else {
        *size = 1;
    }

    return cjk_char_pull(cjk_lookup(c, mac_korean_2uni_index,
                                    mac_korean_2uni_charset),
                         out, mac_korean_compose);
}

/*!
 * @brief Convert MAC_KOREAN to UCS2 through cjk_generic_pull
 *
 * @param[in]     cd            EUC-KR iconv descriptor for the fallback
 * @param[in,out] inbuf         input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        UCS2 output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_korean_pull(void *cd, char **inbuf, size_t *inbytesleft,
                              char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_pull(mac_korean_char_pull,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}
#endif
