/*
 * MacChineseSimp
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
 * @brief Mac Chinese Simplified charset conversion
 * @sa Reference
 * http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>

#if HAVE_USABLE_ICONV

#include "generic_cjk.h"
#include "mac_chinese_simp.h"

static size_t mac_chinese_simp_pull(void *, char **, size_t *, char **,
                                    size_t *);
static size_t mac_chinese_simp_push(void *, char **, size_t *, char **,
                                    size_t *);

struct charset_functions charset_mac_chinese_simp = {
    "MAC_CHINESE_SIMP",
    25,
    mac_chinese_simp_pull,
    mac_chinese_simp_push,
    CHARSET_ICONV | CHARSET_MULTIBYTE | CHARSET_PRECOMPOSED | CHARSET_CLIENT,
    "EUC-CN",
    NULL,
    NULL
};

/*!
 * @brief Map one UCS2 character, or a pair with an Apple hint character, to
 * MAC_CHINESE_SIMP bytes
 *
 * ASCII passes as is. When a hint character in the range 0xf870 to 0xf87f
 * follows, the composition table first combines the pair. Private-use
 * characters and characters without a table entry return 0, so the caller
 * converts them through EUC-CN.
 *
 * @param[out]    out   receives one or two bytes
 * @param[in]     in    UCS2 input
 * @param[in,out] size  characters available, which receives the characters used
 * @returns the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with
 *          EILSEQ
 */
static size_t mac_chinese_simp_char_push(uint8_t *out, const ucs2_t *in,
                                         size_t *size)
{
    ucs2_t wc = in[0];

    if (wc <= 0x7f) {
        *size = 1;
        out[0] = (uint8_t)wc;
        return 1;
    } else if ((wc & 0xf000) == 0xe000) {
        *size = 1;
        return 0;
    } else if (*size >= 2 && (in[1] & ~15) == 0xf870) {
        ucs2_t comp = cjk_compose(wc, in[1], mac_chinese_simp_compose,
                                  sizeof(mac_chinese_simp_compose) / sizeof(uint32_t));

        if (comp) {
            wc = comp;
            *size = 2;
        } else {
            *size = 1;
        }
    } else {
        *size = 1;
    }

    return cjk_char_push(cjk_lookup(wc, mac_chinese_simp_uni2_index,
                                    mac_chinese_simp_uni2_charset), out);
}

/*!
 * @brief Convert UCS2 to MAC_CHINESE_SIMP through cjk_generic_push
 *
 * @param[in]     cd            EUC-CN iconv descriptor for the fallback
 * @param[in,out] inbuf         UCS2 input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_chinese_simp_push(void *cd, char **inbuf, size_t *inbytesleft,
                                    char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_push(mac_chinese_simp_char_push,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

/*!
 * @brief Map one MAC_CHINESE_SIMP character of one or two bytes to UCS2
 *
 * ASCII passes as is. A lead byte from 0xa1 to 0xfc needs a trail byte from
 * 0xa1 to 0xfe, and other bytes stand alone. The table value expands
 * through the composition table into up to CJK_PULL_BUFFER characters.
 *
 * @param[out]    out   receives the UCS2 characters
 * @param[in]     in    input bytes
 * @param[in,out] size  bytes available, which receives the bytes used
 * @returns the number of characters, 0 for the iconv fallback, or `(size_t)-1`
 *          with EILSEQ or EINVAL
 */
static size_t mac_chinese_simp_char_pull(ucs2_t *out, const uint8_t *in,
                                         size_t *size)
{
    uint16_t c = in[0];

    if (c <= 0x7f) {
        *size = 1;
        *out = c;
        return 1;
    } else if (c >= 0xa1 && c <= 0xfc) {
        if (*size >= 2) {
            uint8_t c2 = in[1];

            if (c2 >= 0xa1 && c2 <= 0xfe) {
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

    return cjk_char_pull(cjk_lookup(c, mac_chinese_simp_2uni_index,
                                    mac_chinese_simp_2uni_charset),
                         out, mac_chinese_simp_compose);
}

/*!
 * @brief Convert MAC_CHINESE_SIMP to UCS2 through cjk_generic_pull
 *
 * @param[in]     cd            EUC-CN iconv descriptor for the fallback
 * @param[in,out] inbuf         input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        UCS2 output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_chinese_simp_pull(void *cd, char **inbuf, size_t *inbytesleft,
                                    char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_pull(mac_chinese_simp_char_pull,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}
#endif
