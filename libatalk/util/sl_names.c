/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*!
 * @file
 * @brief Spotlight name comparisons: reading a query and matching a name
 *
 * A value follows Apple's File Metadata Query Expression Syntax: '*' matches
 * any run of characters, '?' one character, a backslash makes the next
 * character literal, and an end without a wildcard is anchored. Folding
 * lowers a character for 'c', takes its canonical decomposition from the
 * tables of the UTF8-MAC conversion and drops the combining marks for 'd',
 * and composes the result again.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/logger.h>
#include <atalk/sl_names.h>
#include <atalk/unicode.h>

/* UTF-16 units one decomposed character takes at most */
#define SL_NAME_UNITS 16

/* The name index's fold, besides SL_NAMES_CASE and SL_NAMES_DIACRITIC:
 * Hangul syllables decompose too, and nothing composes */
#define SL_NAME_INDEX (1 << 8)

/* A query node that matches no name: a comparison the policy refuses */
#define SL_NAME_NEVER 'N'

enum {
    SL_TOK_CHAR,
    SL_TOK_ONE,
    SL_TOK_ANY
};

/* One unit of a folded value, or a wildcard */
struct sl_name_tok {
    ucs2_t ch;
    unsigned char kind;
    bool run_end;           /*!< last unit of a run of literal characters */
};

struct sl_name_pattern {
    unsigned int flags;
    size_t literals;        /*!< bytes of the value's folded literals */
    size_t req;             /*!< first token of the longest literal run */
    size_t reqlen;          /*!< its tokens, 0 without a literal */
    char *like;             /*!< LIKE pattern every match satisfies */
    char *index_glob;       /*!< GLOB pattern over names sl_name_fold() folds */
    char *index_like;       /*!< LIKE pattern over them, ESCAPE '!' */
    size_t ntok;
    struct sl_name_tok tok[];
};

/* A query's name comparisons: a comparison, two joined by && or ||, or a
 * comparison that matches nothing; the root holds the charset of the names it
 * matches */
struct sl_name_query {
    char op;
    charset_t charset;
    struct sl_name_pattern *pattern;
    struct sl_name_query *left;
    struct sl_name_query *right;
};

/* Name comparisons a query holds at most, repeats included, which bounds the
 * nodes the matcher evaluates for every name */
#define SL_NAME_NODES 32

/* Parentheses a query nests at most: the parser recurses into each */
#define SL_NAME_DEPTH 64

/* The parser's state for one query */
struct sl_name_parse {
    const char *p;                  /*!< position in the UTF-8 query */
    size_t min;                     /*!< literal bytes a comparison other
                                         than != needs */
    int max;                        /*!< distinct comparisons kept */
    int nseen;                      /*!< comparisons compiled in seen */
    int nodes;                      /*!< name comparisons read, repeats
                                         included */
    int depth;                      /*!< parentheses open */
    bool failed;                    /*!< no name can match the query */
    struct sl_name_pattern **seen;  /*!< the distinct comparisons so far */
};

static struct sl_name_query *sl_name_or(struct sl_name_parse *ps);

/*!
 * @brief Tell whether text of a charset reads as UTF-8 only once converted
 *
 * UTF-8 and UTF8-MAC read as they are. UCS-2 never holds a name, so a query
 * that names no charset, its root zeroed, takes it for UTF-8 too.
 *
 * @param[in] charset  the charset
 * @returns true for every other charset
 */
static bool sl_name_converts(charset_t charset)
{
    return charset != CH_UCS2 && charset != CH_UTF8 && charset != CH_UTF8_MAC;
}

/*!
 * @brief Tell whether a unit is a combining mark that 'd' drops
 *
 * The Hebrew and Arabic ranges are Unicode 17.0's nonspacing marks of those
 * blocks: the points, the cantillation marks and the harakat, and the hamza
 * and madda of a letter that decomposes with them, so أ folds to ا.
 *
 * @param[in] u  UTF-16 unit
 * @returns true for a combining diacritical mark, a kana voicing mark, or a
 *          nonspacing mark of Hebrew or Arabic
 */
static bool sl_name_mark(ucs2_t u)
{
    if (u < 0x0300) {
        return false;
    }

    return u <= 0x036f || (u >= 0x0591 && u <= 0x05bd) || u == 0x05bf
           || u == 0x05c1 || u == 0x05c2 || u == 0x05c4 || u == 0x05c5
           || u == 0x05c7 || (u >= 0x0610 && u <= 0x061a)
           || (u >= 0x064b && u <= 0x065f) || u == 0x0670
           || (u >= 0x06d6 && u <= 0x06dc) || (u >= 0x06df && u <= 0x06e4)
           || u == 0x06e7 || u == 0x06e8 || (u >= 0x06ea && u <= 0x06ed)
           || (u >= 0x1ab0 && u <= 0x1aff) || (u >= 0x1dc0 && u <= 0x1dff)
           || (u >= 0x20d0 && u <= 0x20ff) || (u >= 0xfe20 && u <= 0xfe2f)
           || u == 0x3099 || u == 0x309a;
}

/*!
 * @brief Tell whether a unit joins the character before it
 *
 * @param[in] u  UTF-16 unit
 * @returns true for a mark sl_name_mark() knows, or a combining mark of Thai,
 *          Lao, Myanmar or Khmer
 */
static bool sl_name_joins(ucs2_t u)
{
    if (u < 0x0300) {
        return false;
    }

    return sl_name_mark(u) || u == 0x0e31 || (u >= 0x0e34 && u <= 0x0e3a)
           || (u >= 0x0e47 && u <= 0x0e4e) || u == 0x0eb1
           || (u >= 0x0eb4 && u <= 0x0ebc) || (u >= 0x0ec8 && u <= 0x0ece)
           || (u >= 0x102b && u <= 0x103e) || (u >= 0x1056 && u <= 0x1059)
           || (u >= 0x105e && u <= 0x1060) || (u >= 0x1062 && u <= 0x1064)
           || (u >= 0x1067 && u <= 0x106d) || (u >= 0x1071 && u <= 0x1074)
           || (u >= 0x1082 && u <= 0x108d) || u == 0x108f
           || (u >= 0x109a && u <= 0x109d) || (u >= 0x17b4 && u <= 0x17d3)
           || u == 0x17dd;
}

/*!
 * @brief Decode the next UTF-8 character
 *
 * A byte that starts no valid sequence in its shortest form decodes alone,
 * as U+DC80 to U+DCFF, so the bytes of a name that is not UTF-8 stay apart.
 *
 * @param[in,out] p    position, moved past the character
 * @param[in]     end  end of the string
 * @returns the code point
 */
static uint32_t sl_name_next(const unsigned char **p,
                             const unsigned char *end)
{
    static const uint32_t least[] = { 0, 0x80, 0x800, 0x10000 };
    const unsigned char *s = *p;
    uint32_t cp = *s++;
    int more = cp >= 0xf0 ? 3 : cp >= 0xe0 ? 2 : cp >= 0xc0 ? 1 : 0;

    if (cp < 0x80) {
        *p = s;
        return cp;
    }

    if (more == 0 || cp >= 0xf8) {
        goto invalid;
    }

    cp &= 0x3fu >> more;

    for (int i = 0; i < more; i++) {
        if (s >= end || (*s & 0xc0) != 0x80) {
            goto invalid;
        }

        cp = cp << 6 | (*s++ & 0x3fu);
    }

    if (cp < least[more] || cp > 0x10ffff || (cp >= 0xd800 && cp < 0xe000)) {
        goto invalid;
    }

    *p = s;
    return cp;
invalid:
    cp = 0xdc00u + **p;
    (*p)++;
    return cp;
}

/*!
 * @brief Fold the case of a unit
 *
 * The tree's lowercase map leaves a few letters apart from the ones they
 * fold to: the micro sign, long s, final sigma, the Greek symbol forms of
 * beta, theta, phi, pi, kappa, rho and epsilon, the Cyrillic small letter
 * variants, long s with dot above and Greek prosgegrammeni.
 *
 * @param[in] u  UTF-16 unit
 * @returns the unit that a letter of either case folds to
 */
static ucs2_t sl_name_case(ucs2_t u)
{
    u = tolower_w(u);

    switch (u) {
    case 0x00b5:
        return 0x03bc;

    case 0x017f:
        return 's';

    case 0x03c2:
        return 0x03c3;

    case 0x03d0:
        return 0x03b2;

    case 0x03d1:
        return 0x03b8;

    case 0x03d5:
        return 0x03c6;

    case 0x03d6:
        return 0x03c0;

    case 0x03f0:
        return 0x03ba;

    case 0x03f1:
        return 0x03c1;

    case 0x03f5:
        return 0x03b5;

    case 0x1c80:
        return 0x0432;

    case 0x1c81:
        return 0x0434;

    case 0x1c82:
        return 0x043e;

    case 0x1c83:
        return 0x0441;

    case 0x1c84:
    case 0x1c85:
        return 0x0442;

    case 0x1c86:
        return 0x044a;

    case 0x1c87:
        return 0x0463;

    case 0x1c88:
        return 0xa64b;

    case 0x1e9b:
        return 0x1e61;

    case 0x1fbe:
        return 0x03b9;

    default:
        return u;
    }
}

/*!
 * @brief Tell whether a unit is a CJK ideograph or a Hangul syllable
 *
 * A comparison needs neither decomposed: an ideograph has no canonical
 * decomposition, a syllable composes again from its jamo, and no composition
 * takes either as its second character.
 *
 * @param[in] u  the unit
 * @returns true for U+3400 to U+9FFF and U+AC00 to U+D7A3
 */
static bool sl_name_atomic(ucs2_t u)
{
    return (u >= 0x3400 && u <= 0x9fff) || (u >= 0xac00 && u <= 0xd7a3);
}

/*!
 * @brief Fold UTF-8 characters into UTF-16 units
 *
 * SL_NAMES_CASE folds the case of each character first, so a letter that
 * lowers to a precomposed one decomposes as that one does; I with dot above
 * lowers to i and its dot, as its decomposition does. SL_NAMES_DIACRITIC
 * then decomposes each character but a CJK ideograph or a Hangul syllable
 * and drops the marks sl_name_mark() knows. A variation selector only picks
 * a glyph and folds away, so ❤ and its emoji form ❤️ fold alike. The units
 * then compose, so a name stored precomposed and one stored decomposed in
 * canonical order fold alike; precompose_w() composes neighbours only, so
 * marks in another order, or a letter stored part composed, fold apart
 * wherever the marks stay. Units below U+0300, ideographs and syllables never
 * compose with the unit before them, so a fold of those alone skips
 * composing. Under SL_NAME_INDEX a Hangul syllable decomposes too and nothing
 * composes: a dropped mark could otherwise let its neighbours compose, which
 * a run of a value, folded alone, does not.
 *
 * @param[in]  str    the characters
 * @param[in]  len    bytes of str
 * @param[in]  flags  SL_NAMES_CASE, SL_NAMES_DIACRITIC and SL_NAME_INDEX
 * @param[out] out    receives the units, 2 * len + SL_NAME_UNITS of room
 * @param[out] tmp    scratch of the same room
 * @returns the number of units
 */
static size_t sl_name_fold_units(const char *str, size_t len,
                                 unsigned int flags, ucs2_t *out, ucs2_t *tmp)
{
    const unsigned char *p = (const unsigned char *)str;
    const unsigned char *end = p + len;
    size_t room = (2 * len + SL_NAME_UNITS) * sizeof(ucs2_t);
    size_t n = 0;
    size_t bytes;
    bool compose = false;

    for (; p < end && *p < 0x80; p++) {
        out[n++] = (ucs2_t)((flags & SL_NAMES_CASE) && *p >= 'A' && *p <= 'Z'
                            ? *p + ('a' - 'A') : *p);
    }

    if (p == end) {
        return n;
    }

    memcpy(tmp, out, n * sizeof(ucs2_t));

    while (p < end) {
        uint32_t cp = sl_name_next(&p, end);
        ucs2_t in[2] = { (ucs2_t)cp, 0 };
        ucs2_t dec[SL_NAME_UNITS];
        size_t left = sizeof(dec);
        size_t k = 1;
        const bool dot = (flags & SL_NAMES_CASE) && cp == 0x0130;

        /* Unicode's Variation_Selector characters */
        if ((cp >= 0x180b && cp <= 0x180d) || cp == 0x180f
                || (cp >= 0xfe00 && cp <= 0xfe0f)
                || (cp >= 0xe0100 && cp <= 0xe01ef)) {
            continue;
        }

        if (cp >= 0x10000) {
            in[0] = (ucs2_t)(0xd800 + ((cp - 0x10000) >> 10));
            in[1] = (ucs2_t)(0xdc00 + ((cp - 0x10000) & 0x3ff));
            k = 2;
        }

        if ((flags & SL_NAMES_CASE) && k == 2) {
            uint32_t pair = tolower_sp((uint32_t)in[0] << 16 | in[1]);
            in[0] = (ucs2_t)(pair >> 16);
            in[1] = (ucs2_t)(pair & 0xffff);
        } else if (flags & SL_NAMES_CASE) {
            in[0] = sl_name_case(in[0]);
        }

        if ((flags & SL_NAMES_DIACRITIC) && in[0] >= 0x80
                && (!sl_name_atomic(in[0])
                    || ((flags & SL_NAME_INDEX) && in[0] >= 0xac00))
                && (bytes = decompose_w(in, k * sizeof(ucs2_t), dec, &left))
                != (size_t) -1) {
            for (size_t i = 0; i < bytes / sizeof(ucs2_t); i++) {
                if (!sl_name_mark(dec[i])) {
                    tmp[n++] = dec[i];
                }
            }
        } else {
            tmp[n++] = in[0];

            if (k == 2) {
                tmp[n++] = in[1];
            }
        }

        if (dot && !(flags & SL_NAMES_DIACRITIC)) {
            tmp[n++] = 0x0307;
        }
    }

    for (size_t i = 0; i < n && !compose && !(flags & SL_NAME_INDEX); i++) {
        compose = tmp[i] >= 0x300 && !sl_name_atomic(tmp[i]);
    }

    if (!compose
            || (bytes = precompose_w(tmp, n * sizeof(ucs2_t), out, &room))
            == (size_t) -1) {
        memcpy(out, tmp, n * sizeof(ucs2_t));
        return n;
    }

    return bytes / sizeof(ucs2_t);
}

/*!
 * @brief Step past a unit or a surrogate pair and the units that join it
 *
 * @param[in] s  folded units
 * @param[in] i  start of the character, below n
 * @param[in] n  number of units
 * @returns the position after the character
 */
static size_t sl_name_char_end(const ucs2_t *s, size_t i, size_t n)
{
    i += (s[i] >= 0xd800 && s[i] < 0xdc00 && i + 1 < n) ? 2 : 1;

    while (i < n && sl_name_joins(s[i])) {
        i++;
    }

    return i;
}

/* The units above U+007F that Unicode 17.0 classes as punctuation, symbols,
 * separators, controls or format characters (general categories P, S, Z, Cc
 * and Cf), in ranges of the BMP */
static const uint16_t sl_name_nonword[][2] = {
    {0x0080, 0x00a9}, {0x00ab, 0x00b1}, {0x00b4, 0x00b4}, {0x00b6, 0x00b8},
    {0x00bb, 0x00bb}, {0x00bf, 0x00bf}, {0x00d7, 0x00d7}, {0x00f7, 0x00f7},
    {0x02c2, 0x02c5}, {0x02d2, 0x02df}, {0x02e5, 0x02eb}, {0x02ed, 0x02ed},
    {0x02ef, 0x02ff}, {0x0375, 0x0375}, {0x037e, 0x037e}, {0x0384, 0x0385},
    {0x0387, 0x0387}, {0x03f6, 0x03f6}, {0x0482, 0x0482}, {0x055a, 0x055f},
    {0x0589, 0x058a}, {0x058d, 0x058f}, {0x05be, 0x05be}, {0x05c0, 0x05c0},
    {0x05c3, 0x05c3}, {0x05c6, 0x05c6}, {0x05f3, 0x05f4}, {0x0600, 0x060f},
    {0x061b, 0x061f}, {0x066a, 0x066d}, {0x06d4, 0x06d4}, {0x06dd, 0x06de},
    {0x06e9, 0x06e9}, {0x06fd, 0x06fe}, {0x0700, 0x070d}, {0x070f, 0x070f},
    {0x07f6, 0x07f9}, {0x07fe, 0x07ff}, {0x0830, 0x083e}, {0x085e, 0x085e},
    {0x0888, 0x0888}, {0x0890, 0x0891}, {0x08e2, 0x08e2}, {0x0964, 0x0965},
    {0x0970, 0x0970}, {0x09f2, 0x09f3}, {0x09fa, 0x09fb}, {0x09fd, 0x09fd},
    {0x0a76, 0x0a76}, {0x0af0, 0x0af1}, {0x0b70, 0x0b70}, {0x0bf3, 0x0bfa},
    {0x0c77, 0x0c77}, {0x0c7f, 0x0c7f}, {0x0c84, 0x0c84}, {0x0d4f, 0x0d4f},
    {0x0d79, 0x0d79}, {0x0df4, 0x0df4}, {0x0e3f, 0x0e3f}, {0x0e4f, 0x0e4f},
    {0x0e5a, 0x0e5b}, {0x0f01, 0x0f17}, {0x0f1a, 0x0f1f}, {0x0f34, 0x0f34},
    {0x0f36, 0x0f36}, {0x0f38, 0x0f38}, {0x0f3a, 0x0f3d}, {0x0f85, 0x0f85},
    {0x0fbe, 0x0fc5}, {0x0fc7, 0x0fcc}, {0x0fce, 0x0fda}, {0x104a, 0x104f},
    {0x109e, 0x109f}, {0x10fb, 0x10fb}, {0x1360, 0x1368}, {0x1390, 0x1399},
    {0x1400, 0x1400}, {0x166d, 0x166e}, {0x1680, 0x1680}, {0x169b, 0x169c},
    {0x16eb, 0x16ed}, {0x1735, 0x1736}, {0x17d4, 0x17d6}, {0x17d8, 0x17db},
    {0x1800, 0x180a}, {0x180e, 0x180e}, {0x1940, 0x1940}, {0x1944, 0x1945},
    {0x19de, 0x19ff}, {0x1a1e, 0x1a1f}, {0x1aa0, 0x1aa6}, {0x1aa8, 0x1aad},
    {0x1b4e, 0x1b4f}, {0x1b5a, 0x1b6a}, {0x1b74, 0x1b7f}, {0x1bfc, 0x1bff},
    {0x1c3b, 0x1c3f}, {0x1c7e, 0x1c7f}, {0x1cc0, 0x1cc7}, {0x1cd3, 0x1cd3},
    {0x1fbd, 0x1fbd}, {0x1fbf, 0x1fc1}, {0x1fcd, 0x1fcf}, {0x1fdd, 0x1fdf},
    {0x1fed, 0x1fef}, {0x1ffd, 0x1ffe}, {0x2000, 0x2064}, {0x2066, 0x206f},
    {0x207a, 0x207e}, {0x208a, 0x208e}, {0x20a0, 0x20c1}, {0x2100, 0x2101},
    {0x2103, 0x2106}, {0x2108, 0x2109}, {0x2114, 0x2114}, {0x2116, 0x2118},
    {0x211e, 0x2123}, {0x2125, 0x2125}, {0x2127, 0x2127}, {0x2129, 0x2129},
    {0x212e, 0x212e}, {0x213a, 0x213b}, {0x2140, 0x2144}, {0x214a, 0x214d},
    {0x214f, 0x214f}, {0x218a, 0x218b}, {0x2190, 0x2429}, {0x2440, 0x244a},
    {0x249c, 0x24e9}, {0x2500, 0x2775}, {0x2794, 0x2b73}, {0x2b76, 0x2bff},
    {0x2ce5, 0x2cea}, {0x2cf9, 0x2cfc}, {0x2cfe, 0x2cff}, {0x2d70, 0x2d70},
    {0x2e00, 0x2e2e}, {0x2e30, 0x2e5d}, {0x2e80, 0x2e99}, {0x2e9b, 0x2ef3},
    {0x2f00, 0x2fd5}, {0x2ff0, 0x3004}, {0x3008, 0x3020}, {0x3030, 0x3030},
    {0x3036, 0x3037}, {0x303d, 0x303f}, {0x309b, 0x309c}, {0x30a0, 0x30a0},
    {0x30fb, 0x30fb}, {0x3190, 0x3191}, {0x3196, 0x319f}, {0x31c0, 0x31e5},
    {0x31ef, 0x31ef}, {0x3200, 0x321e}, {0x322a, 0x3247}, {0x3250, 0x3250},
    {0x3260, 0x327f}, {0x328a, 0x32b0}, {0x32c0, 0x33ff}, {0x4dc0, 0x4dff},
    {0xa490, 0xa4c6}, {0xa4fe, 0xa4ff}, {0xa60d, 0xa60f}, {0xa673, 0xa673},
    {0xa67e, 0xa67e}, {0xa6f2, 0xa6f7}, {0xa700, 0xa716}, {0xa720, 0xa721},
    {0xa789, 0xa78a}, {0xa828, 0xa82b}, {0xa836, 0xa839}, {0xa874, 0xa877},
    {0xa8ce, 0xa8cf}, {0xa8f8, 0xa8fa}, {0xa8fc, 0xa8fc}, {0xa92e, 0xa92f},
    {0xa95f, 0xa95f}, {0xa9c1, 0xa9cd}, {0xa9de, 0xa9df}, {0xaa5c, 0xaa5f},
    {0xaa77, 0xaa79}, {0xaade, 0xaadf}, {0xaaf0, 0xaaf1}, {0xab5b, 0xab5b},
    {0xab6a, 0xab6b}, {0xabeb, 0xabeb}, {0xfb29, 0xfb29}, {0xfbb2, 0xfbd2},
    {0xfd3e, 0xfd4f}, {0xfd90, 0xfd91}, {0xfdc8, 0xfdcf}, {0xfdfc, 0xfdff},
    {0xfe10, 0xfe19}, {0xfe30, 0xfe52}, {0xfe54, 0xfe66}, {0xfe68, 0xfe6b},
    {0xfeff, 0xfeff}, {0xff01, 0xff0f}, {0xff1a, 0xff20}, {0xff3b, 0xff40},
    {0xff5b, 0xff65}, {0xffe0, 0xffe6}, {0xffe8, 0xffee}, {0xfff9, 0xfffd},
};

/* The same classes in ranges above the BMP, the emoji among them */
static const uint32_t sl_name_nonword_sp[][2] = {
    {0x10100, 0x10102}, {0x10137, 0x1013f}, {0x10179, 0x10189},
    {0x1018c, 0x1018e}, {0x10190, 0x1019c}, {0x101a0, 0x101a0},
    {0x101d0, 0x101fc}, {0x1039f, 0x1039f}, {0x103d0, 0x103d0},
    {0x1056f, 0x1056f}, {0x10857, 0x10857}, {0x10877, 0x10878},
    {0x1091f, 0x1091f}, {0x1093f, 0x1093f}, {0x10a50, 0x10a58},
    {0x10a7f, 0x10a7f}, {0x10ac8, 0x10ac8}, {0x10af0, 0x10af6},
    {0x10b39, 0x10b3f}, {0x10b99, 0x10b9c}, {0x10d6e, 0x10d6e},
    {0x10d8e, 0x10d8f}, {0x10ead, 0x10ead}, {0x10ed0, 0x10ed8},
    {0x10f55, 0x10f59}, {0x10f86, 0x10f89}, {0x11047, 0x1104d},
    {0x110bb, 0x110c1}, {0x110cd, 0x110cd}, {0x11140, 0x11143},
    {0x11174, 0x11175}, {0x111c5, 0x111c8}, {0x111cd, 0x111cd},
    {0x111db, 0x111db}, {0x111dd, 0x111df}, {0x11238, 0x1123d},
    {0x112a9, 0x112a9}, {0x113d4, 0x113d5}, {0x113d7, 0x113d8},
    {0x1144b, 0x1144f}, {0x1145a, 0x1145b}, {0x1145d, 0x1145d},
    {0x114c6, 0x114c6}, {0x115c1, 0x115d7}, {0x11641, 0x11643},
    {0x11660, 0x1166c}, {0x116b9, 0x116b9}, {0x1173c, 0x1173f},
    {0x1183b, 0x1183b}, {0x11944, 0x11946}, {0x119e2, 0x119e2},
    {0x11a3f, 0x11a46}, {0x11a9a, 0x11a9c}, {0x11a9e, 0x11aa2},
    {0x11b00, 0x11b09}, {0x11be1, 0x11be1}, {0x11c41, 0x11c45},
    {0x11c70, 0x11c71}, {0x11ef7, 0x11ef8}, {0x11f43, 0x11f4f},
    {0x11fd5, 0x11ff1}, {0x11fff, 0x11fff}, {0x12470, 0x12474},
    {0x12ff1, 0x12ff2}, {0x13430, 0x1343f}, {0x16a6e, 0x16a6f},
    {0x16af5, 0x16af5}, {0x16b37, 0x16b3f}, {0x16b44, 0x16b45},
    {0x16d6d, 0x16d6f}, {0x16e97, 0x16e9a}, {0x16fe2, 0x16fe2},
    {0x1bc9c, 0x1bc9c}, {0x1bc9f, 0x1bca3}, {0x1cc00, 0x1ccef},
    {0x1ccfa, 0x1ccfc}, {0x1cd00, 0x1ceb3}, {0x1ceba, 0x1ced0},
    {0x1cee0, 0x1cef0}, {0x1cf50, 0x1cfc3}, {0x1d000, 0x1d0f5},
    {0x1d100, 0x1d126}, {0x1d129, 0x1d164}, {0x1d16a, 0x1d16c},
    {0x1d173, 0x1d17a}, {0x1d183, 0x1d184}, {0x1d18c, 0x1d1a9},
    {0x1d1ae, 0x1d1ea}, {0x1d200, 0x1d241}, {0x1d245, 0x1d245},
    {0x1d300, 0x1d356}, {0x1d6c1, 0x1d6c1}, {0x1d6db, 0x1d6db},
    {0x1d6fb, 0x1d6fb}, {0x1d715, 0x1d715}, {0x1d735, 0x1d735},
    {0x1d74f, 0x1d74f}, {0x1d76f, 0x1d76f}, {0x1d789, 0x1d789},
    {0x1d7a9, 0x1d7a9}, {0x1d7c3, 0x1d7c3}, {0x1d800, 0x1d9ff},
    {0x1da37, 0x1da3a}, {0x1da6d, 0x1da74}, {0x1da76, 0x1da83},
    {0x1da85, 0x1da8b}, {0x1e14f, 0x1e14f}, {0x1e2ff, 0x1e2ff},
    {0x1e5ff, 0x1e5ff}, {0x1e95e, 0x1e95f}, {0x1ecac, 0x1ecac},
    {0x1ecb0, 0x1ecb0}, {0x1ed2e, 0x1ed2e}, {0x1eef0, 0x1eef1},
    {0x1f000, 0x1f02b}, {0x1f030, 0x1f093}, {0x1f0a0, 0x1f0ae},
    {0x1f0b1, 0x1f0bf}, {0x1f0c1, 0x1f0cf}, {0x1f0d1, 0x1f0f5},
    {0x1f10d, 0x1f1ad}, {0x1f1e6, 0x1f202}, {0x1f210, 0x1f23b},
    {0x1f240, 0x1f248}, {0x1f250, 0x1f251}, {0x1f260, 0x1f265},
    {0x1f300, 0x1f6d8}, {0x1f6dc, 0x1f6ec}, {0x1f6f0, 0x1f6fc},
    {0x1f700, 0x1f7d9}, {0x1f7e0, 0x1f7eb}, {0x1f7f0, 0x1f7f0},
    {0x1f800, 0x1f80b}, {0x1f810, 0x1f847}, {0x1f850, 0x1f859},
    {0x1f860, 0x1f887}, {0x1f890, 0x1f8ad}, {0x1f8b0, 0x1f8bb},
    {0x1f8c0, 0x1f8c1}, {0x1f8d0, 0x1f8d8}, {0x1f900, 0x1fa57},
    {0x1fa60, 0x1fa6d}, {0x1fa70, 0x1fa7c}, {0x1fa80, 0x1fa8a},
    {0x1fa8e, 0x1fac6}, {0x1fac8, 0x1fac8}, {0x1facd, 0x1fadc},
    {0x1fadf, 0x1faea}, {0x1faef, 0x1faf8}, {0x1fb00, 0x1fb92},
    {0x1fb94, 0x1fbef}, {0x1fbfa, 0x1fbfa}, {0xe0001, 0xe0001},
    {0xe0020, 0xe007f},
};

/*!
 * @brief Tell whether a folded unit belongs to a word
 *
 * @param[in] u  UTF-16 unit
 * @returns true for an ASCII letter or digit, and for a non-ASCII unit that
 *          sl_name_nonword holds no range of: a letter, a mark, a number, a
 *          surrogate, a private-use unit or one Unicode leaves unassigned
 */
static bool sl_name_alnum(ucs2_t u)
{
    size_t lo = 0;
    size_t hi = sizeof(sl_name_nonword) / sizeof(sl_name_nonword[0]);

    if (u < 0x80) {
        return (u >= '0' && u <= '9') || (u >= 'a' && u <= 'z')
               || (u >= 'A' && u <= 'Z');
    }

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (u < sl_name_nonword[mid][0]) {
            hi = mid;
        } else if (u > sl_name_nonword[mid][1]) {
            lo = mid + 1;
        } else {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Tell whether a folded unit starts a character of an unspaced script
 *
 * @param[in] u  UTF-16 unit
 * @returns true for Thai, Lao, Myanmar, Khmer, kana, Bopomofo, the CJK
 *          ideographs and the fullwidth and halfwidth forms; Hangul's
 *          compatibility and halfwidth jamo stay out, as Korean spaces its
 *          words
 */
static bool sl_name_unspaced(ucs2_t u)
{
    if (u < 0x0e00) {
        return false;
    }

    return u <= 0x0eff || (u >= 0x1000 && u <= 0x109f)
           || (u >= 0x1780 && u <= 0x17ff) || (u >= 0x3040 && u <= 0x312f)
           || (u >= 0x3190 && u <= 0x31ff) || (u >= 0x3400 && u <= 0x4dbf)
           || (u >= 0x4e00 && u <= 0x9fff) || (u >= 0xf900 && u <= 0xfaff)
           || (u >= 0xff00 && u <= 0xff9f) || (u >= 0xffe0 && u <= 0xffef)
           || (u >= 0xd840 && u <= 0xd8bf);
}

/*!
 * @brief Tell whether a folded unit is the low half of a surrogate pair
 *
 * @param[in] s  folded units
 * @param[in] i  position
 * @returns true for a low surrogate after a high one
 */
static bool sl_name_pair_tail(const ucs2_t *s, size_t i)
{
    return i > 0 && s[i] >= 0xdc00 && s[i] < 0xe000 && s[i - 1] >= 0xd800
           && s[i - 1] < 0xdc00;
}

/*!
 * @brief Tell whether the character at a position belongs to a word
 *
 * A surrogate pair takes the class of its code point, so an emoji ends a
 * word as a BMP symbol does.
 *
 * @param[in] s  folded units
 * @param[in] i  start of the character, below n
 * @param[in] n  number of units
 * @returns true for a letter, a mark, a number, a surrogate, a private-use or
 *          an unassigned code point
 */
static bool sl_name_word(const ucs2_t *s, size_t i, size_t n)
{
    size_t lo = 0;
    size_t hi = sizeof(sl_name_nonword_sp) / sizeof(sl_name_nonword_sp[0]);
    uint32_t cp;

    if (i + 1 >= n || !sl_name_pair_tail(s, i + 1)) {
        return sl_name_alnum(s[i]);
    }

    cp = 0x10000 + ((uint32_t)(s[i] - 0xd800) << 10) + (s[i + 1] - 0xdc00);

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (cp < sl_name_nonword_sp[mid][0]) {
            hi = mid;
        } else if (cp > sl_name_nonword_sp[mid][1]) {
            lo = mid + 1;
        } else {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Tell whether a position of folded units is a word boundary
 *
 * Inside the text of a script written without spaces every character starts
 * a word, as a search for a word within such text expects. A character of
 * no word stands alone, so "(2)" starts after the space of "photo (2).jpg".
 * The character before a position takes the class of its base, whatever
 * marks join it.
 *
 * @param[in] s  folded units
 * @param[in] i  position
 * @param[in] n  number of units
 * @returns true at either end, and between two characters when either is of
 *          no word or of a script written without spaces
 */
static bool sl_name_boundary(const ucs2_t *s, size_t i, size_t n)
{
    size_t lead;

    if (i == 0 || i == n) {
        return true;
    }

    if (sl_name_joins(s[i]) || sl_name_pair_tail(s, i)) {
        return false;
    }

    lead = i - 1;

    while (lead > 0 && (sl_name_joins(s[lead]) || sl_name_pair_tail(s, lead))) {
        lead--;
    }

    return !sl_name_word(s, lead, n) || !sl_name_word(s, i, n)
           || sl_name_unspaced(s[lead]) || sl_name_unspaced(s[i]);
}

/*!
 * @brief Match a value against folded units
 *
 * The value takes every unit up to the end, or with @p word up to any word
 * boundary, and an anchored start with @p word may fall on any word
 * boundary. A '*' backtracks one whole character at a time, and a start
 * moves to the next word boundary. A literal run cannot end on a character
 * that a joining unit follows.
 *
 * @param[in] pat   the value
 * @param[in] s     folded units
 * @param[in] n     number of units
 * @param[in] word  the value's anchored ends fall on word boundaries
 * @returns true when the value matches
 */
static bool sl_name_glob(const struct sl_name_pattern *pat, const ucs2_t *s,
                         size_t n, bool word)
{
    bool lead = word && !(pat->ntok > 0 && pat->tok[0].kind == SL_TOK_ANY);
    size_t star = SIZE_MAX;
    size_t from = 0;
    size_t p = 0;
    size_t i = 0;

    for (;;) {
        const struct sl_name_tok *t = p < pat->ntok ? &pat->tok[p] : NULL;

        if (t == NULL && (word ? sl_name_boundary(s, i, n) : i == n)) {
            return true;
        }

        if (t != NULL && i < n && t->kind == SL_TOK_CHAR && t->ch == s[i]
                && !(t->run_end && i + 1 < n && sl_name_joins(s[i + 1]))) {
            i++;
            p++;
        } else if (t != NULL && i < n && t->kind == SL_TOK_ONE) {
            i = sl_name_char_end(s, i, n);
            p++;
        } else if (t != NULL && t->kind == SL_TOK_ANY) {
            star = p++;
            from = i;
        } else if (star != SIZE_MAX && from < n) {
            p = star + 1;
            from = sl_name_char_end(s, from, n);
            i = from;
        } else if (star == SIZE_MAX && lead && from < n) {
            do {
                from = sl_name_char_end(s, from, n);
            } while (!sl_name_boundary(s, from, n));

            p = 0;
            i = from;
        } else {
            return false;
        }
    }
}

/*!
 * @brief Write folded units as UTF-8, a stray byte's unit as U+FFFD
 *
 * U+DC80 to U+DCFF stand for the bytes that start no UTF-8 character. The
 * index text and its patterns then hold whole characters only, which
 * SQLite's GLOB and trigrams read one by one, as the matcher reads the
 * units; a stray byte written as itself would merge with the bytes after
 * it.
 *
 * @param[out] o  output position
 * @param[in]  u  the units
 * @param[in]  n  number of units
 * @returns the output position after them
 */
static char *sl_name_put_utf8(char *o, const ucs2_t *u, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint32_t cp = u[i];

        if (cp >= 0xd800 && cp < 0xdc00 && i + 1 < n) {
            cp = 0x10000 + ((cp - 0xd800) << 10) + (u[++i] - 0xdc00u);
        } else if (cp >= 0xdc80 && cp <= 0xdcff) {
            cp = 0xfffd;
        }

        if (cp < 0x80) {
            *o++ = (char)cp;
        } else if (cp < 0x800) {
            *o++ = (char)(0xc0 | cp >> 6);
            *o++ = (char)(0x80 | (cp & 0x3f));
        } else if (cp < 0x10000) {
            *o++ = (char)(0xe0 | cp >> 12);
            *o++ = (char)(0x80 | (cp >> 6 & 0x3f));
            *o++ = (char)(0x80 | (cp & 0x3f));
        } else {
            *o++ = (char)(0xf0 | cp >> 18);
            *o++ = (char)(0x80 | (cp >> 12 & 0x3f));
            *o++ = (char)(0x80 | (cp >> 6 & 0x3f));
            *o++ = (char)(0x80 | (cp & 0x3f));
        }
    }

    return o;
}

/*!
 * @brief Fold a run of literal characters into tokens and the index patterns
 *
 * @param[in,out] pat     the value, receiving the tokens and their folded
 *                        bytes
 * @param[in]     run     the characters
 * @param[in,out] runlen  bytes of run, reset to 0
 * @param[out]    units   scratch of 2 * run bytes + SL_NAME_UNITS units
 * @param[out]    tmp     scratch of the same room
 * @param[in,out] gp      end of the GLOB index pattern, moved past the run
 * @param[in,out] ip      end of the LIKE index pattern, moved past the run
 */
static void sl_name_flush(struct sl_name_pattern *pat, const char *run,
                          size_t *runlen, ucs2_t *units, ucs2_t *tmp,
                          char **gp, char **ip)
{
    size_t k = sl_name_fold_units(run, *runlen, pat->flags, units, tmp);
    char *end;

    for (size_t i = 0; i < k; i++) {
        ucs2_t u = units[i];

        /* the folded bytes: 4 for a surrogate pair, 1 for a lone byte */
        if (u >= 0xd800 && u < 0xdc00 && i + 1 < k) {
            pat->literals += 4;
        } else if (!sl_name_pair_tail(units, i)) {
            pat->literals += u < 0x80 || (u >= 0xdc80 && u <= 0xdcff) ? 1
                             : u < 0x800 ? 2 : 3;
        }

        pat->tok[pat->ntok].kind = SL_TOK_CHAR;
        pat->tok[pat->ntok++].ch = units[i];
    }

    k = sl_name_fold_units(run, *runlen,
                           SL_NAMES_CASE | SL_NAMES_DIACRITIC | SL_NAME_INDEX,
                           units, tmp);
    end = sl_name_put_utf8(*gp, units, k);

    /* LIKE escapes its wildcards with '!'; a GLOB wildcard becomes '?', which
     * matches it among others */
    for (char *c = *gp; c < end; c++) {
        if (*c == '%' || *c == '_' || *c == '!') {
            *(*ip)++ = '!';
        }

        *(*ip)++ = *c;

        if (*c == '*' || *c == '?' || *c == '[') {
            *c = '?';
        }
    }

    *gp = end;
    *runlen = 0;
}

/*!
 * @brief Compile a value with its modifiers
 *
 * The LIKE pattern keeps the value's literal ASCII characters but letters,
 * each '%', '_' and '!' escaped with '!', between '%': a collation can fold a
 * letter otherwise than the matcher, or read two letters as one, but compares
 * the others as themselves, and the matcher drops variation selectors, and
 * under SL_NAMES_DIACRITIC combining marks, between any two characters. So
 * every match satisfies the pattern under any collation.
 *
 * @param[in] value  the value between its quotes, escapes included
 * @param[in] len    bytes of value
 * @param[in] flags  SL_NAMES_* flags
 * @returns the comparison, to sl_name_pattern_free(), or NULL when out of
 *          memory
 */
struct sl_name_pattern *sl_name_pattern_new(const char *value, size_t len,
                                            unsigned int flags)
{
    const unsigned char *p = (const unsigned char *)value;
    const unsigned char *end = p + len;
    const size_t room = 2 * len + SL_NAME_UNITS;
    struct sl_name_pattern *pat = calloc(1, sizeof(*pat)
                                         + room * sizeof(pat->tok[0]));
    ucs2_t *units = malloc(2 * room * sizeof(ucs2_t));
    char *run = malloc(len + 1);
    size_t runlen = 0;
    char *lp;
    char *gp;
    char *ip;

    /* like: '!', the byte and '%' for each byte of the value; the index
     * patterns: 3 bytes of UTF-8 a folded unit, twice that with escapes */
    if (pat == NULL || units == NULL || run == NULL
            || (pat->like = malloc(3 * len + 3)) == NULL
            || (pat->index_glob = malloc(3 * room + 3)) == NULL
            || (pat->index_like = malloc(6 * room + 3)) == NULL) {
        free(units);
        free(run);
        sl_name_pattern_free(pat);
        return NULL;
    }

    pat->flags = flags;
    lp = pat->like;
    gp = pat->index_glob;
    ip = pat->index_like;

    /* the index's patterns anchor where the value does, w aside: its folded
     * names start and end as the matcher's do */
    if ((flags & SL_NAMES_WORD) || len == 0 || *p == '*' || *p == '?') {
        *gp++ = '*';
        *ip++ = '%';
    }

    *lp++ = '%';

    while (p < end) {
        const unsigned char *c;

        if (*p == '*' || *p == '?') {
            unsigned char kind = *p++ == '*' ? SL_TOK_ANY : SL_TOK_ONE;

            if (runlen > 0) {
                sl_name_flush(pat, run, &runlen, units, units + room, &gp,
                              &ip);
                *gp++ = '*';
                *ip++ = '%';
            }

            if (kind == SL_TOK_ONE || pat->ntok == 0
                    || pat->tok[pat->ntok - 1].kind != SL_TOK_ANY) {
                pat->tok[pat->ntok++].kind = kind;
            }

            continue;
        }

        if (*p == '\\' && p + 1 < end) {
            p++;
        }

        c = p;
        sl_name_next(&p, end);

        /* the run holds bytes of the value, its escapes left out, so it
         * never outgrows the value */
        if ((size_t)(p - c) > len - runlen) {
            break;
        }

        memcpy(run + runlen, c, (size_t)(p - c));
        runlen += (size_t)(p - c);

        if (p - c > 1 || *c >= 0x80 || isalpha(*c)) {
            continue;
        }

        if (*c == '%' || *c == '_' || *c == '!') {
            *lp++ = '!';
        }

        *lp++ = (char)c[0];
        *lp++ = '%';
    }

    if (runlen > 0) {
        sl_name_flush(pat, run, &runlen, units, units + room, &gp, &ip);

        if (flags & SL_NAMES_WORD) {
            *gp++ = '*';
            *ip++ = '%';
        }
    }

    *lp = '\0';
    *gp = '\0';
    *ip = '\0';
    free(units);
    free(run);

    for (size_t i = 0, seq = 0; i < pat->ntok; i++) {
        bool last = i + 1 == pat->ntok || pat->tok[i + 1].kind != SL_TOK_CHAR;
        seq = pat->tok[i].kind == SL_TOK_CHAR ? seq + 1 : 0;
        pat->tok[i].run_end = seq > 0 && last;

        if (seq > pat->reqlen) {
            pat->req = i + 1 - seq;
            pat->reqlen = seq;
        }
    }

    if (flags & SL_NAMES_NEGATE) {
        memcpy(pat->like, "%", 2);
        memcpy(pat->index_glob, "*", 2);
        memcpy(pat->index_like, "%", 2);
    }

    return pat;
}

/*!
 * @brief Free a compiled name comparison
 *
 * @param[in] pattern  the comparison, or NULL
 */
void sl_name_pattern_free(struct sl_name_pattern *pattern)
{
    if (pattern != NULL) {
        free(pattern->like);
        free(pattern->index_glob);
        free(pattern->index_like);
        free(pattern);
    }
}

/*!
 * @brief Tell whether folded units hold a value's longest literal run
 *
 * @param[in] pat  the value
 * @param[in] s    folded units
 * @param[in] n    number of units
 * @returns true when the run occurs, as it does in every match, or when the
 *          value has no literal
 */
static bool sl_name_holds(const struct sl_name_pattern *pat, const ucs2_t *s,
                          size_t n)
{
    const struct sl_name_tok *r = pat->tok + pat->req;

    for (size_t i = 0; i + pat->reqlen <= n; i++) {
        size_t k = 0;

        while (k < pat->reqlen && s[i + k] == r[k].ch) {
            k++;
        }

        if (k == pat->reqlen) {
            return true;
        }
    }

    return pat->reqlen == 0;
}

/*!
 * @brief Tell whether a compiled comparison is a !=
 *
 * @param[in] pattern  the comparison
 * @returns true for SL_NAMES_NEGATE
 */
static bool sl_name_pattern_negated(const struct sl_name_pattern *pattern)
{
    return (pattern->flags & SL_NAMES_NEGATE) != 0;
}

/*!
 * @brief Tell whether a folded name matches a compiled comparison
 *
 * @param[in] pattern  the comparison
 * @param[in] s        the name's units, folded as the comparison asks
 * @param[in] n        number of units
 * @returns whether the name matches, the opposite for SL_NAMES_NEGATE
 */
static bool sl_name_units_match(const struct sl_name_pattern *pattern,
                                const ucs2_t *s, size_t n)
{
    return (sl_name_holds(pattern, s, n)
            && sl_name_glob(pattern, s, n,
                            (pattern->flags & SL_NAMES_WORD) != 0))
           != sl_name_pattern_negated(pattern);
}

/*!
 * @brief Tell whether a name matches a compiled comparison
 *
 * With SL_NAMES_WORD an end of the value without a wildcard falls on a word
 * boundary of the name instead of its end. A name over MAXPATHLEN bytes
 * matches no comparison but a !=.
 *
 * @param[in] pattern  the comparison
 * @param[in] name     UTF-8 name
 * @param[in] len      bytes of name
 * @returns whether the name matches, the opposite for SL_NAMES_NEGATE
 */
bool sl_name_pattern_match(const struct sl_name_pattern *pattern,
                           const char *name, size_t len)
{
    ucs2_t s[2 * MAXPATHLEN + SL_NAME_UNITS];
    ucs2_t tmp[2 * MAXPATHLEN + SL_NAME_UNITS];
    size_t n;

    if (len > MAXPATHLEN) {
        return sl_name_pattern_negated(pattern);
    }

    n = sl_name_fold_units(name, len, pattern->flags, s, tmp);
    return sl_name_units_match(pattern, s, n);
}

/* Bytes of a name that one match of a query folds once for each set of
 * modifiers, so the four foldings fill 4 KiB; a longer name folds for each
 * comparison */
#define SL_NAME_KEEP 248
/* The modifiers a folding depends on */
#define SL_NAME_FOLDING (SL_NAMES_CASE | SL_NAMES_DIACRITIC)

/* A name and its foldings, one for each set of SL_NAMES_CASE (bit 0) and
 * SL_NAMES_DIACRITIC (bit 1) */
struct sl_name_folds {
    const char *name;
    size_t len;
    size_t n[4];            /*!< units of each folding, SIZE_MAX until made */
    ucs2_t s[4][2 * SL_NAME_KEEP + SL_NAME_UNITS];
};

/*!
 * @brief Tell whether the name of @p f matches a compiled comparison
 *
 * The folding for the comparison's modifiers is made once and kept in @p f,
 * but for a name over SL_NAME_KEEP bytes, which folds anew.
 *
 * @param[in,out] f        the name and its foldings so far
 * @param[in]     pattern  the comparison
 * @returns whether the name matches, the opposite for SL_NAMES_NEGATE
 */
static bool sl_name_folds_match(struct sl_name_folds *f,
                                const struct sl_name_pattern *pattern)
{
    const unsigned int k = pattern->flags & SL_NAME_FOLDING;
    ucs2_t tmp[2 * SL_NAME_KEEP + SL_NAME_UNITS];

    if (f->len > SL_NAME_KEEP) {
        return sl_name_pattern_match(pattern, f->name, f->len);
    }

    if (f->n[k] == SIZE_MAX) {
        f->n[k] = sl_name_fold_units(f->name, f->len, k, f->s[k], tmp);
    }

    return sl_name_units_match(pattern, f->s[k], f->n[k]);
}

/*!
 * @brief Bytes of a compiled comparison's literals, as its modifiers fold them
 *
 * @param[in] pattern  the comparison
 * @returns the UTF-8 bytes, escapes, wildcards and the marks 'd' drops left
 *          out
 */
size_t sl_name_pattern_literals(const struct sl_name_pattern *pattern)
{
    return pattern->literals;
}

/*!
 * @brief Fold a name for case and diacritics, decomposed, as the index does
 *
 * @param[in] name  UTF-8 name
 * @param[in] len   bytes of name
 * @returns the folded UTF-8 name, to free(), or NULL when out of memory
 */
char *sl_name_fold(const char *name, size_t len)
{
    const size_t room = 2 * len + SL_NAME_UNITS;
    ucs2_t *units = malloc(2 * room * sizeof(ucs2_t));
    /* a folded unit takes 3 bytes of UTF-8 at most */
    char *out = malloc(3 * room + 1);

    if (units != NULL && out != NULL) {
        size_t k = sl_name_fold_units(name, len,
                                      SL_NAMES_CASE | SL_NAMES_DIACRITIC
                                      | SL_NAME_INDEX, units, units + room);
        *sl_name_put_utf8(out, units, k) = '\0';
    } else {
        free(out);
        out = NULL;
    }

    free(units);
    return out;
}

/*!
 * @brief Fold a name of a charset as sl_name_fold() folds its UTF-8
 *
 * @param[in] name     the name
 * @param[in] len      bytes of name
 * @param[in] charset  its charset; a name that does not convert folds its
 *                     bytes
 * @returns the folded name, to free(), or NULL when out of memory
 */
char *sl_name_fold_in(const char *name, size_t len, charset_t charset)
{
    char utf8[4 * MAXPATHLEN + 1];
    size_t n;

    if (sl_name_converts(charset)
            && (n = convert_string_quiet(charset, CH_UTF8, name, len, utf8,
                                         sizeof(utf8))) != (size_t) -1) {
        return sl_name_fold(utf8, n);
    }

    return sl_name_fold(name, len);
}

/*!
 * @brief The GLOB pattern that the folded name of every match satisfies
 *
 * '*' joins the value's runs of folded literal characters, so a '%' or a '_'
 * in them compares as itself; a '*', '?' or '[' in them stands as '?'. An
 * end of the value that a wildcard or w leaves open takes '*' as well.
 *
 * @param[in] pattern  the comparison
 * @returns the pattern, "*" for a !=, which lasts as long as @p pattern
 */
const char *sl_name_pattern_index_glob(const struct sl_name_pattern *pattern)
{
    return pattern->index_glob;
}

/*!
 * @brief The LIKE pattern, ESCAPE '!', that every match's folded name satisfies
 *
 * '%' joins the value's runs of folded literal characters, each '%', '_' and
 * '!' of them escaped with '!', for a column of folded names that compares
 * bytes. An end of the value that a wildcard or w leaves open takes '%' as
 * well.
 *
 * @param[in] pattern  the comparison
 * @returns the pattern, "%" for a !=, which lasts as long as @p pattern
 */
const char *sl_name_pattern_index_like(const struct sl_name_pattern *pattern)
{
    return pattern->index_like;
}

/*!
 * @brief The LIKE pattern, ESCAPE '!', every match satisfies under any collation
 *
 * @param[in] pattern  the comparison
 * @returns the pattern, "%" for a !=, which lasts as long as @p pattern
 */
const char *sl_name_pattern_like(const struct sl_name_pattern *pattern)
{
    return pattern->like;
}

/*!
 * @brief Tell whether two compiled comparisons match the same names
 *
 * @param[in] a  a comparison
 * @param[in] b  another
 * @returns true for the same modifiers and the same folded value
 */
static bool sl_name_pattern_same(const struct sl_name_pattern *a,
                                 const struct sl_name_pattern *b)
{
    if (a->flags != b->flags || a->ntok != b->ntok) {
        return false;
    }

    for (size_t i = 0; i < a->ntok; i++) {
        if (a->tok[i].kind != b->tok[i].kind || a->tok[i].ch != b->tok[i].ch) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Skip white space
 *
 * @param[in,out] ps  parser state
 */
static void sl_name_space(struct sl_name_parse *ps)
{
    while (ps->p[0] == ' ' || ps->p[0] == '\t' || ps->p[0] == '\n'
            || ps->p[0] == '\r') {
        ps->p++;
    }
}

/*!
 * @brief Take a token after white space
 *
 * @param[in,out] ps     parser state
 * @param[in]     token  the token
 * @returns true when the token follows and is taken
 */
static bool sl_name_take(struct sl_name_parse *ps, const char *token)
{
    size_t len = strlen(token);
    sl_name_space(ps);

    if (strncmp(ps->p, token, len) != 0) {
        return false;
    }

    ps->p += len;
    return true;
}

/*!
 * @brief Read past a quoted string, its escapes included
 *
 * @param[in,out] ps  parser state, at the opening quote; failed for a string
 *                    without its closing quote
 * @returns the position of the closing quote, or of the end of the query
 */
static const char *sl_name_skip_string(struct sl_name_parse *ps)
{
    const char *close;

    for (ps->p++; ps->p[0] != '\0' && ps->p[0] != '"'; ps->p++) {
        if (ps->p[0] == '\\' && ps->p[1] != '\0') {
            ps->p++;
        }
    }

    close = ps->p;

    if (ps->p[0] == '"') {
        ps->p++;
    } else {
        ps->failed = true;
    }

    return close;
}

/*!
 * @brief Read past an operand the matcher does not take
 *
 * A word, a number, a string or a call such as $time.iso(...).
 *
 * @param[in,out] ps  parser state; failed for a call without its closing
 *                    parenthesis
 */
static void sl_name_skip_operand(struct sl_name_parse *ps)
{
    int depth = 0;

    while (ps->p[0] != '\0') {
        if (ps->p[0] == '"') {
            sl_name_skip_string(ps);
            continue;
        }

        if (ps->p[0] == '(') {
            depth++;
        } else if (ps->p[0] == ')' && depth == 0) {
            return;
        } else if (ps->p[0] == ')') {
            depth--;
        } else if (depth == 0 && (isspace((unsigned char)ps->p[0])
                                  || ps->p[0] == '&' || ps->p[0] == '|'
                                  || ps->p[0] == ',')) {
            return;
        }

        ps->p++;
    }

    if (depth > 0) {
        ps->failed = true;
    }
}

/*!
 * @brief Read modifier letters
 *
 * @param[in,out] ps  parser state
 * @returns the SL_NAMES_* flags of c, d and w in either case; other letters
 *          add none
 */
static unsigned int sl_name_modifiers(struct sl_name_parse *ps)
{
    unsigned int flags = 0;

    for (; isalpha((unsigned char)ps->p[0]); ps->p++) {
        switch (tolower((unsigned char)ps->p[0])) {
        case 'c':
            flags |= SL_NAMES_CASE;
            break;

        case 'd':
            flags |= SL_NAMES_DIACRITIC;
            break;

        case 'w':
            flags |= SL_NAMES_WORD;
            break;

        default:
            break;
        }
    }

    return flags;
}

/*!
 * @brief Drop the Finder's phrase delimiters from a value
 *
 * The Finder sends a phrase as \"…\"* or *\"…\"*; without the trailing '*',
 * as anywhere else, an escaped quote is a quote.
 *
 * @param[in]  value  the value between its quotes
 * @param[in]  len    bytes of value
 * @param[out] out    receives the value, len bytes of room
 * @returns the bytes written to out
 */
static size_t sl_name_phrase(const char *value, size_t len, char *out)
{
    size_t start = 0;
    size_t end = len;

    while (start < len && value[start] == '*') {
        start++;
    }

    while (end > start && value[end - 1] == '*') {
        end--;
    }

    if (end < len && end - start >= 4 && value[start] == '\\'
            && value[start + 1] == '"'
            && value[end - 2] == '\\' && value[end - 1] == '"') {
        memcpy(out, value, start);
        memcpy(out + start, value + start + 2, end - start - 4);
        memcpy(out + end - 4, value + end, len - end);
        return len - 4;
    }

    memcpy(out, value, len);
    return len;
}

/*!
 * @brief Compare bytes, ASCII letters in either case, whatever the locale
 *
 * strncasecmp() folds through the locale, and a Turkish one keeps I apart
 * from i, so a query that a volume of `casefold = tolower` lowered would
 * name no attribute.
 *
 * @param[in] a  bytes
 * @param[in] b  bytes
 * @param[in] n  number of bytes
 * @returns true when the bytes agree
 */
static bool sl_name_ascii_same(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned char x = (unsigned char)a[i];
        unsigned char y = (unsigned char)b[i];

        if (x >= 'A' && x <= 'Z') {
            x += 'a' - 'A';
        }

        if (y >= 'A' && y <= 'Z') {
            y += 'a' - 'A';
        }

        if (x != y) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Make the node of a comparison that matches no name
 *
 * @param[in,out] ps  parser state, failed when out of memory
 * @returns the node, or NULL when out of memory
 */
static struct sl_name_query *sl_name_never(struct sl_name_parse *ps)
{
    struct sl_name_query *node = calloc(1, sizeof(*node));

    if (node != NULL) {
        node->op = SL_NAME_NEVER;
    } else {
        ps->failed = true;
    }

    return node;
}

/*!
 * @brief Read one comparison, a node when it compares a name with a value
 *
 * A name attribute, ==, = or != and a quoted value with its modifiers make a
 * node; any other comparison, function or operand is read to its end and
 * makes none. A repeat of a comparison, one that compiles to the same
 * modifiers and folded value, takes no room again. A comparison past
 * ps->max, or one without SL_NAMES_NEGATE under ps->min bytes of folded
 * literal characters, matches no name. A string or a call without its
 * closing quote or parenthesis, a name attribute without an operator, an
 * operator without a value, modifiers in brackets without the closing
 * bracket, the SL_NAME_NODES + 1st comparison, a token that starts no
 * comparison, the end of the query and a closing parenthesis among them, and
 * a failed allocation, fail the query.
 *
 * @param[in,out] ps  parser state
 * @returns the node, or NULL
 */
static struct sl_name_query *sl_name_comparison(struct sl_name_parse *ps)
{
    static const char *const attrs[] = {
        "kMDItemFSName", "kMDItemDisplayName", "_kMDItemFileName", "*", NULL
    };
    struct sl_name_query *node;
    const char *attr;
    const char *value;
    const char *close;
    unsigned int flags = 0;
    bool name = false;
    bool seen = false;
    size_t attrlen;
    size_t len;
    char *phrase;
    sl_name_space(ps);
    attr = ps->p;

    while (isalnum((unsigned char)ps->p[0])
            || (ps->p[0] != '\0' && strchr("_:.$*", ps->p[0]) != NULL)) {
        ps->p++;
    }

    attrlen = (size_t)(ps->p - attr);

    /* no comparison starts here: the end of the query or a ')' among them */
    if (attrlen == 0) {
        ps->failed = true;
        return NULL;
    }

    for (int i = 0; attrs[i] != NULL; i++) {
        name = name || (strlen(attrs[i]) == attrlen
                        && sl_name_ascii_same(attrs[i], attr, attrlen));
    }

    sl_name_space(ps);

    if (ps->p[0] == '(') {
        sl_name_skip_operand(ps);
        return NULL;
    }

    if (sl_name_take(ps, "!=")) {
        flags = SL_NAMES_NEGATE;
    } else if (!sl_name_take(ps, "==") && !sl_name_take(ps, "=")) {
        const char *op = ps->p;

        while (ps->p[0] != '\0' && strchr("<>=!", ps->p[0]) != NULL) {
            ps->p++;
        }

        if (name && ps->p == op) {
            ps->failed = true;
            return NULL;
        }

        name = false;
    }

    if (ps->p[0] == '[') {
        ps->p++;
        flags |= sl_name_modifiers(ps);

        if (ps->p[0] != ']') {
            ps->failed = true;
            return NULL;
        }

        ps->p++;
    }

    sl_name_space(ps);

    if (ps->p[0] != '"') {
        const char *operand = ps->p;
        sl_name_skip_operand(ps);

        if (ps->p == operand) {
            ps->failed = true;
        }

        return NULL;
    }

    value = ps->p + 1;
    close = sl_name_skip_string(ps);
    flags |= sl_name_modifiers(ps);

    if (!name || close[0] != '"') {
        return NULL;
    }

    if (++ps->nodes > SL_NAME_NODES) {
        LOG(log_debug, logtype_sl,
            "sl_name_query_parse: a query of more than %d name comparisons "
            "matches no name", SL_NAME_NODES);
        ps->failed = true;
        return NULL;
    }

    if ((phrase = malloc((size_t)(close - value) + 1)) == NULL) {
        ps->failed = true;
        return NULL;
    }

    len = sl_name_phrase(value, (size_t)(close - value), phrase);

    if ((node = sl_name_query_value(phrase, len, flags)) == NULL) {
        free(phrase);
        ps->failed = true;
        return NULL;
    }

    for (int i = 0; i < ps->nseen && !seen; i++) {
        seen = sl_name_pattern_same(ps->seen[i], node->pattern);
    }

    if (!seen && ps->nseen >= ps->max) {
        LOG(log_debug, logtype_sl,
            "sl_name_query_parse: a comparison past the maximum matches no "
            "name");
        free(phrase);
        sl_name_query_free(node);
        return sl_name_never(ps);
    }

    if (!(flags & SL_NAMES_NEGATE)
            && sl_name_pattern_literals(node->pattern) < ps->min) {
        LOG(log_debug, logtype_sl,
            "sl_name_query_parse: a value of %zu folded bytes, under %zu, "
            "matches no name", sl_name_pattern_literals(node->pattern),
            ps->min);
        free(phrase);
        sl_name_query_free(node);
        return sl_name_never(ps);
    }

    /* a copy, as an && with a side that matches no name frees the other */
    if (!seen) {
        if ((ps->seen[ps->nseen] = sl_name_pattern_new(phrase, len, flags))
                == NULL) {
            free(phrase);
            sl_name_query_free(node);
            ps->failed = true;
            return NULL;
        }

        ps->nseen++;
    }

    free(phrase);
    return node;
}

/*!
 * @brief Tell whether a chain of one operator holds a comparison
 *
 * @param[in] chain  the chain's node
 * @param[in] op     '&' or '|'
 * @param[in] leaf   a comparison's node
 * @returns true when a comparison of the chain matches the same names
 */
static bool sl_name_chain_holds(const struct sl_name_query *chain, char op,
                                const struct sl_name_query *leaf)
{
    if (chain->op == op) {
        return sl_name_chain_holds(chain->left, op, leaf)
               || sl_name_chain_holds(chain->right, op, leaf);
    }

    return chain->op != '&' && chain->op != '|'
           && sl_name_pattern_same(chain->pattern, leaf->pattern);
}

/*!
 * @brief Join two nodes
 *
 * A side that the reading dropped gives way to the other. A side that
 * matches no name makes an && match none and gives way in an ||. A
 * comparison that its chain of the same operator holds already, as the
 * Finder's pairs of name attributes send it, gives way too, so the match
 * reads it once.
 *
 * @param[in,out] ps  parser state, failed when out of memory
 * @param[in]     op  '&' or '|'
 * @param[in]     a   left node, or NULL
 * @param[in]     b   right node, or NULL
 * @returns the joined node, the one that is not NULL, or NULL
 */
static struct sl_name_query *sl_name_join(struct sl_name_parse *ps, char op,
                                          struct sl_name_query *a,
                                          struct sl_name_query *b)
{
    struct sl_name_query *node;

    if (a == NULL || b == NULL) {
        return a != NULL ? a : b;
    }

    if (a->op == SL_NAME_NEVER || b->op == SL_NAME_NEVER) {
        /* an && keeps the side that matches no name, an || the other */
        const bool keep_a = (op == '&') == (a->op == SL_NAME_NEVER);

        if (keep_a) {
            sl_name_query_free(b);
            return a;
        }

        sl_name_query_free(a);
        return b;
    }

    if (b->op != '&' && b->op != '|' && sl_name_chain_holds(a, op, b)) {
        sl_name_query_free(b);
        return a;
    }

    if ((node = calloc(1, sizeof(*node))) == NULL) {
        sl_name_query_free(a);
        sl_name_query_free(b);
        ps->failed = true;
        return NULL;
    }

    node->op = op;
    node->left = a;
    node->right = b;
    return node;
}

/*!
 * @brief Read a comparison or a parenthesised expression
 *
 * An expression without its closing parenthesis, or one nested deeper than
 * SL_NAME_DEPTH, fails the query.
 *
 * @param[in,out] ps  parser state
 * @returns the node, or NULL
 */
static struct sl_name_query *sl_name_primary(struct sl_name_parse *ps)
{
    struct sl_name_query *node;

    if (!sl_name_take(ps, "(")) {
        return sl_name_comparison(ps);
    }

    if (ps->depth == SL_NAME_DEPTH) {
        ps->failed = true;
        return NULL;
    }

    ps->depth++;
    node = sl_name_or(ps);
    ps->depth--;

    if (!sl_name_take(ps, ")")) {
        ps->failed = true;
    }

    return node;
}

/*!
 * @brief Read comparisons joined by &&
 *
 * @param[in,out] ps  parser state
 * @returns the node, or NULL
 */
static struct sl_name_query *sl_name_and(struct sl_name_parse *ps)
{
    struct sl_name_query *node = sl_name_primary(ps);

    while (sl_name_take(ps, "&&")) {
        node = sl_name_join(ps, '&', node, sl_name_primary(ps));
    }

    return node;
}

/*!
 * @brief Read conjunctions joined by ||
 *
 * @param[in,out] ps  parser state
 * @returns the node, or NULL
 */
static struct sl_name_query *sl_name_or(struct sl_name_parse *ps)
{
    struct sl_name_query *node = sl_name_and(ps);

    while (sl_name_take(ps, "||")) {
        node = sl_name_join(ps, '|', node, sl_name_and(ps));
    }

    return node;
}

/*!
 * @brief Read the name comparisons of a Spotlight query
 *
 * kMDItemFSName, kMDItemDisplayName, _kMDItemFileName and the any-attribute
 * '*' compare names; every other comparison drops out of the && or || that
 * holds it. A comparison other than != under the minimum, or one past the
 * maximum, matches no name; a repeated one counts once. A query that does
 * not read to its end, lacks a closing parenthesis or an operand, nests
 * parentheses deeper than SL_NAME_DEPTH, holds more than SL_NAME_NODES name
 * comparisons or runs out of memory matches no name. Attribute names and
 * modifiers read without regard to case: a volume that folds its names to
 * one case converts the query the same way.
 *
 * @param[in] query            the query string
 * @param[in] min_literals     folded literal bytes a comparison without !=
 *                             needs
 * @param[in] max_comparisons  comparisons the query keeps at most
 * @returns the comparisons, to sl_name_query_free(), or NULL when the query
 *          holds no name comparison or no name can match
 */
struct sl_name_query *sl_name_query_parse(const char *query,
                                          size_t min_literals,
                                          int max_comparisons)
{
    return sl_name_query_parse_in(query, min_literals, max_comparisons,
                                  CH_UTF8);
}

/* Bytes of a query that convert at once: the conversion holds MAXPATHLEN
 * UCS-2 units, which a byte of most charsets fills one at a time */
#define SL_NAME_PIECE (MAXPATHLEN / 2)

/* Bytes the UTF-8 of a piece takes at most: 3 a unit, and the NUL */
#define SL_NAME_PIECE_UTF8 (3 * MAXPATHLEN + 2)

/*!
 * @brief Convert a query of a charset to UTF-8, whatever its length
 *
 * A longer query converts in pieces of SL_NAME_PIECE bytes at most. A piece
 * that ends inside a character of more bytes does not convert, as a character
 * cut short is no character of an ASCII-compatible charset, so it ends up to
 * 3 bytes earlier instead, before that character. A piece that still does not
 * convert halves, for a charset whose character decodes to several units, as
 * one of Mac Japanese decodes to five. A try that fails logs at the debug
 * level alone.
 *
 * @param[in] query    the query, NUL-terminated
 * @param[in] charset  its charset
 * @returns the UTF-8 query, to free(), or NULL when a byte of it does not
 *          convert, or out of memory
 */
static char *sl_name_utf8_query(const char *query, charset_t charset)
{
    const size_t len = strlen(query);
    char *utf8 = malloc(SL_NAME_PIECE_UTF8);
    char *out = malloc(1);
    size_t done = 0;
    size_t at = 0;

    while (utf8 != NULL && out != NULL && at < len) {
        size_t piece = len - at < SL_NAME_PIECE ? len - at : SL_NAME_PIECE;
        size_t n = (size_t) -1;
        char *grown;

        while (n == (size_t) -1 && piece > 0) {
            for (size_t cut = 0; cut < 4 && cut < piece && n == (size_t) -1;
                    cut++) {
                if ((n = convert_string_quiet(charset, CH_UTF8, query + at,
                                              piece - cut, utf8,
                                              SL_NAME_PIECE_UTF8))
                        != (size_t) -1) {
                    piece -= cut;
                }
            }

            if (n == (size_t) -1) {
                piece = piece > 4 ? piece / 2 : 0;
            }
        }

        if (n == (size_t) -1 || (grown = realloc(out, done + n + 1)) == NULL) {
            break;
        }

        memcpy(grown + done, utf8, n);
        out = grown;
        done += n;
        at += piece;
    }

    free(utf8);

    if (out == NULL || at < len) {
        free(out);
        return NULL;
    }

    out[done] = '\0';
    return out;
}

/*!
 * @brief Read the name comparisons of a Spotlight query in a charset
 *
 * The query converts to UTF-8 before it is read, so a byte that continues a
 * character, such as the trailing 0x5C of a Shift_JIS character, escapes
 * nothing; each name converts as the matcher reads it, and one that does not
 * convert compares its bytes.
 *
 * @param[in] query            the query string
 * @param[in] min_literals     folded literal bytes a comparison without !=
 *                             needs
 * @param[in] max_comparisons  comparisons the query keeps at most
 * @param[in] charset          charset of the query and the names
 * @returns the comparisons, to sl_name_query_free(), or NULL when the query
 *          holds no name comparison, no name can match or the query does not
 *          convert
 */
struct sl_name_query *sl_name_query_parse_in(const char *query,
                                             size_t min_literals,
                                             int max_comparisons,
                                             charset_t charset)
{
    char *utf8 = NULL;
    struct sl_name_parse ps = {
        .p = query, .min = min_literals, .max = max_comparisons
    };
    struct sl_name_query *node = NULL;

    if (sl_name_converts(charset)) {
        if ((utf8 = sl_name_utf8_query(query, charset)) == NULL) {
            LOG(log_debug, logtype_sl,
                "sl_name_query_parse: the query does not convert to UTF-8");
            return NULL;
        }

        ps.p = utf8;
    }

    if (max_comparisons > 0 && (ps.seen = calloc((size_t)max_comparisons,
                                                 sizeof(*ps.seen))) == NULL) {
        free(utf8);
        return NULL;
    }

    node = sl_name_or(&ps);
    sl_name_space(&ps);

    if (ps.p[0] != '\0' || ps.failed) {
        LOG(log_debug, logtype_sl,
            "sl_name_query_parse: the query does not read on at \"%s\"", ps.p);
        sl_name_query_free(node);
        node = NULL;
    } else if (node != NULL && node->op == SL_NAME_NEVER) {
        sl_name_query_free(node);
        node = NULL;
    }

    if (node != NULL) {
        node->charset = charset;
    }

    for (int i = 0; i < ps.nseen; i++) {
        sl_name_pattern_free(ps.seen[i]);
    }

    free(ps.seen);
    free(utf8);
    return node;
}

/*!
 * @brief Make a query of one name comparison
 *
 * @param[in] value  the value, escapes included
 * @param[in] len    bytes of value
 * @param[in] flags  SL_NAMES_* flags
 * @returns the query, to sl_name_query_free(), or NULL when out of memory
 */
struct sl_name_query *sl_name_query_value(const char *value, size_t len,
                                          unsigned int flags)
{
    struct sl_name_query *node = calloc(1, sizeof(*node));

    if (node != NULL
            && (node->pattern = sl_name_pattern_new(value, len, flags))
            == NULL) {
        free(node);
        node = NULL;
    }

    return node;
}

/*!
 * @brief Make a query of one name comparison from a search term
 *
 * A term for names that contain it keeps its wildcards and escapes, and a
 * backslash at its end means itself. A term for whole names takes every
 * character literally.
 *
 * @param[in] term   the term
 * @param[in] len    bytes of term, MAXPATHLEN at most
 * @param[in] exact  whether the term is a whole name
 * @param[in] flags  SL_NAMES_* flags
 * @returns the query, to sl_name_query_free(), or NULL when out of memory or
 *          for a longer term
 */
struct sl_name_query *sl_name_query_term(const char *term, size_t len,
                                         bool exact, unsigned int flags)
{
    return sl_name_query_term_in(term, len, exact, flags, CH_UTF8);
}

/*!
 * @brief Make a query of one name comparison from a search term in a charset
 *
 * The term converts to UTF-8 first, and keeps its bytes when it does not.
 *
 * @param[in] term     the term
 * @param[in] len      bytes of term, MAXPATHLEN at most, as UTF-8 too
 * @param[in] exact    whether the term is a whole name
 * @param[in] flags    SL_NAMES_* flags
 * @param[in] charset  charset of the term and the names
 * @returns the query, to sl_name_query_free(), or NULL when out of memory or
 *          for a longer term
 */
struct sl_name_query *sl_name_query_term_in(const char *term, size_t len,
                                            bool exact, unsigned int flags,
                                            charset_t charset)
{
    char value[2 * MAXPATHLEN + 3];
    char utf8[MAXPATHLEN + 1];
    struct sl_name_query *query;
    size_t n = 0;

    if (sl_name_converts(charset)
            && (n = convert_string(charset, CH_UTF8, term, len, utf8,
                                   sizeof(utf8))) != (size_t) -1) {
        term = utf8;
        len = n;
    }

    n = 0;

    if (len > MAXPATHLEN) {
        return NULL;
    }

    if (!exact) {
        value[n++] = '*';
    }

    for (size_t i = 0; i < len; i++) {
        if (!exact && term[i] == '\\' && i + 1 < len) {
            value[n++] = term[i++];
        } else if (term[i] == '\\'
                   || (exact && (term[i] == '*' || term[i] == '?'))) {
            value[n++] = '\\';
        }

        value[n++] = term[i];
    }

    if (!exact) {
        value[n++] = '*';
    }

    if ((query = sl_name_query_value(value, n, flags)) != NULL) {
        query->charset = charset;
    }

    return query;
}

/*!
 * @brief Free a query's name comparisons
 *
 * @param[in] query  the comparisons, or NULL
 */
void sl_name_query_free(struct sl_name_query *query)
{
    if (query != NULL) {
        sl_name_query_free(query->left);
        sl_name_query_free(query->right);
        sl_name_pattern_free(query->pattern);
        free(query);
    }
}

/*!
 * @brief Tell whether a folded name satisfies a node of a query
 *
 * @param[in]     query  the node
 * @param[in,out] f      the name and its foldings so far
 * @returns the truth of the && and || of the node's comparisons
 */
static bool sl_name_node_match(const struct sl_name_query *query,
                               struct sl_name_folds *f)
{
    switch (query->op) {
    case '&':
        return sl_name_node_match(query->left, f)
               && sl_name_node_match(query->right, f);

    case '|':
        return sl_name_node_match(query->left, f)
               || sl_name_node_match(query->right, f);

    default:
        return sl_name_folds_match(f, query->pattern);
    }
}

/*!
 * @brief Tell whether a name satisfies a query's name comparisons
 *
 * A name of SL_NAME_KEEP bytes at most folds once for each set of modifiers.
 *
 * @param[in] query  the comparisons
 * @param[in] name   the name, in the charset of the query
 * @param[in] len    bytes of name
 * @returns the truth of the && and || of the comparisons
 */
bool sl_name_query_match(const struct sl_name_query *query, const char *name,
                         size_t len)
{
    struct sl_name_folds f;
    /* the conversion holds MAXPATHLEN UCS-2 units, 3 bytes of UTF-8 each */
    char utf8[4 * MAXPATHLEN + 1];
    size_t n;

    /* a name that does not convert compares its bytes, quietly: a volume that
     * other tools write can hold many */
    if (sl_name_converts(query->charset)
            && (n = convert_string_quiet(query->charset, CH_UTF8, name, len,
                                         utf8, sizeof(utf8))) != (size_t) -1) {
        name = utf8;
        len = n;
    }

    f.name = name;
    f.len = len;

    for (int i = 0; i < 4; i++) {
        f.n[i] = SIZE_MAX;
    }

    return sl_name_node_match(query, &f);
}

/*!
 * @brief Collect the seeds that cover every match of a node
 *
 * A seed is a positive comparison the backend narrows by, collected once;
 * every match of the node satisfies one of them. Either side of an && covers
 * its matches: the left one when the backend narrows by all of its seeds,
 * else the right one when it does. Each side is read once, and the backend
 * asked once about each seed, so the work grows with the nodes of the query.
 *
 * @param[in]     query    the node
 * @param[out]    seeds    receives the comparisons
 * @param[in,out] n        comparisons in seeds
 * @param[in]     max      room in seeds
 * @param[in]     narrows  the backend's test of a seed, or NULL
 * @param[in]     ctx      the test's context
 * @returns false when no such set fits, as under a !=, a comparison the
 *          backend does not narrow by, or an && neither side of which fits
 */
static bool sl_name_seed(const struct sl_name_query *query,
                         const struct sl_name_pattern **seeds, int *n,
                         int max, sl_name_narrows_fn *narrows, void *ctx)
{
    int mark = *n;

    switch (query->op) {
    case '&':
        if (sl_name_seed(query->left, seeds, n, max, narrows, ctx)) {
            return true;
        }

        *n = mark;
        return sl_name_seed(query->right, seeds, n, max, narrows, ctx);

    case '|':
        return sl_name_seed(query->left, seeds, n, max, narrows, ctx)
               && sl_name_seed(query->right, seeds, n, max, narrows, ctx);

    default:
        if (sl_name_pattern_negated(query->pattern)) {
            return false;
        }

        for (int i = 0; i < *n; i++) {
            if (sl_name_pattern_same(seeds[i], query->pattern)) {
                return true;
            }
        }

        if (*n == max || (narrows != NULL && !narrows(query->pattern, ctx))) {
            return false;
        }

        seeds[(*n)++] = query->pattern;
        return true;
    }
}

/*!
 * @brief Pick the comparisons a database search runs for a query
 *
 * @param[in]  query    the comparisons
 * @param[out] seeds    receives positive comparisons that every match
 *                      satisfies one of, which belong to @p query and last
 *                      until sl_name_query_free() frees it
 * @param[in]  max      room in seeds, of which SL_NAME_NODES at most are used
 * @param[in]  narrows  tells whether the backend narrows a search by a seed,
 *                      or NULL
 * @param[in]  ctx      context for narrows
 * @returns the number of seeds, every one of which narrows the search, or 0
 *          when every name is a candidate
 */
int sl_name_query_seeds(const struct sl_name_query *query,
                        const struct sl_name_pattern **seeds, int max,
                        sl_name_narrows_fn *narrows, void *ctx)
{
    int n = 0;
    return sl_name_seed(query, seeds, &n, max, narrows, ctx) ? n : 0;
}
