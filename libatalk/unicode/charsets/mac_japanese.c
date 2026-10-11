/*
 * MacJapanese
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
 * @sa Reference
 * http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdlib.h>

#if HAVE_USABLE_ICONV

#include "generic_cjk.h"
#include "mac_japanese.h"

static size_t mac_japanese_pull(void *, char **, size_t *, char **, size_t *);
static size_t mac_japanese_push(void *, char **, size_t *, char **, size_t *);

struct charset_functions charset_mac_japanese = {
    "MAC_JAPANESE",
    1,
    mac_japanese_pull,
    mac_japanese_push,
    CHARSET_ICONV | CHARSET_MULTIBYTE | CHARSET_PRECOMPOSED | CHARSET_CLIENT,
    "SHIFT_JIS",
    NULL,
    NULL
};

/*!
 * @brief Map one UCS2 character, or an Apple sequence, to MAC_JAPANESE bytes
 *
 * ASCII passes as is, except that a backslash becomes 0x80. Private-use
 * characters up to U+E98B map to the user-defined lead bytes from 0xf0.
 * An Apple sequence that starts with U+F860 to U+F867 composes first. So
 * does a pair with a hint from U+F870 to U+F87F or with U+20DD.
 * Characters without a table entry return 0 for the SHIFT_JIS fallback.
 *
 * @param[out]    out   receives one or two bytes
 * @param[in]     in    UCS2 input
 * @param[in,out] size  characters available, which receives the characters used
 * @returns the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with
 *          errno
 */
static size_t mac_japanese_char_push(uint8_t *out, const ucs2_t *in,
                                     size_t *size)
{
    ucs2_t wc = in[0];

    if (wc <= 0x7f) {
        *size = 1;
        out[0] = (uint8_t)(wc == 0x5c ? 0x80 : wc);
        return 1;
    } else if ((wc & 0xf000) == 0xe000) { /* user defined */
        *size = 1;

        if (wc > 0xe98b) {
            return 0;
        }

        wc -= 0xe000;
        out[0] = (uint8_t)(wc / 188 + 0xf0);
        out[1] = (uint8_t)(wc % 188 + 0x40);

        if (out[1] >= 0x7f) {
            ++out[1];
        }

        return 2;
    } else if ((wc & ~7) == 0xf860) {
        wc = cjk_compose_seq(in, size, mac_japanese_compose,
                             sizeof(mac_japanese_compose) / sizeof(uint32_t));

        if (!wc) {
            return (size_t) -1;
        }
    } else if (*size >= 2 && ((in[1] & ~15) == 0xf870 || in[1] == 0x20dd)) {
        ucs2_t comp = cjk_compose(wc, in[1], mac_japanese_compose,
                                  sizeof(mac_japanese_compose) / sizeof(uint32_t));

        if (comp) {
            wc = comp;
            *size = 2;
        } else {
            *size = 1;
        }
    } else {
        *size = 1;
    }

    return cjk_char_push(cjk_lookup(wc, mac_japanese_uni2_index,
                                    mac_japanese_uni2_charset), out);
}

/*!
 * @brief Convert UCS2 to MAC_JAPANESE through cjk_generic_push
 *
 * @param[in]     cd            SHIFT_JIS iconv descriptor for the fallback
 * @param[in,out] inbuf         UCS2 input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_japanese_push(void *cd, char **inbuf, size_t *inbytesleft,
                                char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_push(mac_japanese_char_push,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

/*!
 * @brief Map one MAC_JAPANESE character of one or two bytes to UCS2
 *
 * ASCII passes as is, except that 0x5c becomes U+00A5. A lead byte in 0x81
 * to 0x9f or 0xe0 to 0xfc needs a trail byte in 0x40 to 0x7e or 0x80 to
 * 0xfc. Lead bytes from 0xf0 map to the private-use range from U+E000.
 * Other values expand through the table and the composition table.
 *
 * @param[out]    out   receives the UCS2 characters
 * @param[in]     in    input bytes
 * @param[in,out] size  bytes available, which receives the bytes used
 * @returns the number of characters, 0 for the iconv fallback, or `(size_t)-1`
 *          with EILSEQ or EINVAL
 */
static size_t mac_japanese_char_pull(ucs2_t *out, const uint8_t *in,
                                     size_t *size)
{
    uint16_t c = in[0];

    if (c <= 0x7f) {
        *size = 1;
        *out = (c == 0x5c ? 0xa5 : c);
        return 1;
    } else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) {
        if (*size >= 2) {
            uint8_t c2 = in[1];

            if ((c2 >= 0x40 && c2 <= 0x7e) || (c2 >= 0x80 && c2 <= 0xfc)) {
                *size = 2;

                if (c >= 0xf0) { /* user defined */
                    *out = 0xe000 + (c - 0xf0) * 188 + c2 - (c2 < 0x80 ? 0x40 : 0x41);
                    return 1;
                }

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

    return cjk_char_pull(cjk_lookup(c, mac_japanese_2uni_index,
                                    mac_japanese_2uni_charset),
                         out, mac_japanese_compose);
}

/*!
 * @brief Convert MAC_JAPANESE to UCS2 through cjk_generic_pull
 *
 * @param[in]     cd            SHIFT_JIS iconv descriptor for the fallback
 * @param[in,out] inbuf         input position
 * @param[in,out] inbytesleft   input bytes left
 * @param[in,out] outbuf        UCS2 output position
 * @param[in,out] outbytesleft  output space
 * @returns 0, or `(size_t)-1` with errno
 */
static size_t mac_japanese_pull(void *cd, char **inbuf, size_t *inbytesleft,
                                char **outbuf, size_t *outbytesleft)
{
    return cjk_generic_pull(mac_japanese_char_pull,
                            cd, inbuf, inbytesleft, outbuf, outbytesleft);
}
#endif
