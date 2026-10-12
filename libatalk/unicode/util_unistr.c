/*
  NOTE:
  The early netatalk 2.x was based on UCS-2.
  UCS-2 don't support chars above U+10000.
  Recent netatalk is based on UTF-16.
  UTF-16 can support chars above U+10000, using Surrogate Pair.
  However, Surrogate Pair is complex, dirty, filthy and disagreeable.
  There might still be latent bugs...
 */

/*!
 * @file
 * @brief UTF-16 string functions, with Unicode precomposition and decomposition
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <atalk/logger.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <atalk/unicode.h>
#include <atalk/byteorder.h>

#include "precompose.h"

/*!
 * @brief Lowercase a NUL-terminated UTF-16 string in place
 *
 * A surrogate pair changes as one character.
 *
 * @param[in,out] s  UTF-16 string
 * @returns 1 when a character changed, or 0
 */
int strlower_w(ucs2_t *s)
{
    int ret = 0;

    while (*s) {
        if ((0xD800 <= *s) && (*s < 0xDC00)) {
            if ((0xDC00 <= s[1]) && (s[1] < 0xE000)) {
                uint32_t s_sp = (uint32_t) * s << 16 | (uint32_t)s[1];
                uint32_t v_sp = tolower_sp(s_sp);

                if (v_sp != s_sp) {
                    *s = v_sp >> 16;
                    s++;
                    *s = v_sp & 0xFFFF;
                    ret = 1;
                }
            }
        } else {
            ucs2_t v = tolower_w(*s);

            if (v != *s) {
                *s = v;
                ret = 1;
            }
        }

        s++;
    }

    return ret;
}

/*!
 * @brief Uppercase a NUL-terminated UTF-16 string in place
 *
 * A surrogate pair changes as one character.
 *
 * @param[in,out] s  UTF-16 string
 * @returns 1 when a character changed, or 0
 */
int strupper_w(ucs2_t *s)
{
    int ret = 0;

    while (*s) {
        if ((0xD800 <= *s) && (*s < 0xDC00)) {
            if ((0xDC00 <= s[1]) && (s[1] < 0xE000)) {
                uint32_t s_sp = (uint32_t) * s << 16 | (uint32_t)s[1];
                uint32_t v_sp = toupper_sp(s_sp);

                if (v_sp != s_sp) {
                    *s = v_sp >> 16;
                    s++;
                    *s = v_sp & 0xFFFF;
                    ret = 1;
                }
            }
        } else {
            ucs2_t v = toupper_w(*s);

            if (v != *s) {
                *s = v;
                ret = 1;
            }
        }

        s++;
    }

    return ret;
}

/*!
 * @brief Count the UTF-16 units of a NUL-terminated string, as strlen does
 *
 * A surrogate pair counts as two units.
 *
 * @param[in] src  UTF-16 string
 * @returns the number of units before the NUL
 */
size_t strlen_w(const ucs2_t *src)
{
    size_t len;

    for (len = 0; *src++; len++);

    return len;
}

/*!
 * @brief Count the UTF-16 units of a string up to a limit, as strnlen does
 *
 * A surrogate pair counts as two units. Without an earlier NUL the call
 * reads the unit at index @p max before it stops.
 *
 * @param[in] src  UTF-16 string
 * @param[in] max  limit
 * @returns the number of units before the NUL, capped at @p max
 */
size_t strnlen_w(const ucs2_t *src, size_t max)
{
    size_t len;

    for (len = 0; *src++ && (len < max); len++);

    return len;
}

/*!
 * @brief Find a UTF-16 unit in a NUL-terminated string, as strchr does
 *
 * The search treats the two halves of a surrogate pair as separate units,
 * and a NUL @p c finds the terminator.
 *
 * @param[in] s  UTF-16 string
 * @param[in] c  unit to find
 * @returns the first match, or NULL
 */
ucs2_t *strchr_w(const ucs2_t *s, ucs2_t c)
{
    while (*s != 0) {
        if (c == *s) {
            return (ucs2_t *)s;
        }

        s++;
    }

    if (c == *s) {
        return (ucs2_t *)s;
    }

    return NULL;
}

/*!
 * @brief Find a UTF-16 unit in a string, ignoring case
 *
 * The search compares single units only, and strcasechr_sp covers surrogate
 * pairs.
 *
 * @param[in] s  UTF-16 string
 * @param[in] c  unit to find
 * @returns the first match, or NULL
 */
ucs2_t *strcasechr_w(const ucs2_t *s, ucs2_t c)
{
    while (*s != 0) {
        if (tolower_w(c) == tolower_w(*s)) {
            return (ucs2_t *)s;
        }

        s++;
    }

    if (c == *s) {
        return (ucs2_t *)s;
    }

    return NULL;
}

/*!
 * @brief Find a surrogate pair in a UTF-16 string, ignoring case
 *
 * The search compares the pair with each pair of neighboring units.
 *
 * @param[in] s     UTF-16 string
 * @param[in] c_sp  pair to find, with the high surrogate in bits 31 to 16
 * @returns the first match, or NULL
 */
ucs2_t *strcasechr_sp(const ucs2_t *s, uint32_t c_sp)
{
    if (*s == 0) {
        return NULL;
    }

    while (s[1] != 0) {
        if (tolower_sp(c_sp) == tolower_sp((uint32_t) * s << 16 | (uint32_t)s[1])) {
            return (ucs2_t *)s;
        }

        s++;
    }

    return NULL;
}

/*!
 * @brief Compare two NUL-terminated UTF-16 strings by unit value, as strcmp
 * does
 *
 * @param[in] a  first string
 * @param[in] b  second string
 * @returns the difference of the first differing units, or 0
 */
int strcmp_w(const ucs2_t *a, const ucs2_t *b)
{
    while (*b && *a == *b) {
        a++;
        b++;
    }

    return *a - *b;
    /* warning: if *a != *b and both are not 0 we retrun a random
       greater or lesser than 0 number not realted to which
       string is longer */
}

/*!
 * @brief Compare up to @p len units of two UTF-16 strings, as strncmp does
 *
 * @param[in] a    first string
 * @param[in] b    second string
 * @param[in] len  limit in units
 * @returns the difference of the first differing units, or 0
 */
int strncmp_w(const ucs2_t *a, const ucs2_t *b, size_t len)
{
    size_t n = 0;

    while ((n < len) && *b && *a == *b) {
        a++;
        b++;
        n++;
    }

    return (len - n) ? (*a - *b) : 0;
}

/*!
 * @brief Find a UTF-16 substring, as strstr does
 *
 * @param[in] s    UTF-16 string
 * @param[in] ins  substring to find
 * @returns the first match, or NULL for no match or an empty or NULL argument
 */
ucs2_t *strstr_w(const ucs2_t *s, const ucs2_t *ins)
{
    ucs2_t *r;
    size_t inslen;

    if (!s || !*s || !ins || !*ins) {
        return NULL;
    }

    inslen = strlen_w(ins);
    r = (ucs2_t *)s;

    while ((r = strchr_w(r, *ins))) {
        if (strncmp_w(r, ins, inslen) == 0) {
            return r;
        }

        r++;
    }

    return NULL;
}

/*!
 * @brief Find a UTF-16 substring, ignoring case, with surrogate pair support
 *
 * A substring that starts with a lone surrogate from 0xd800 to 0xdbff
 * matches nothing.
 *
 * @param[in] s    UTF-16 string
 * @param[in] ins  substring to find
 * @returns the first match, or NULL for no match or an empty or NULL argument
 */
ucs2_t *strcasestr_w(const ucs2_t *s, const ucs2_t *ins)
{
    ucs2_t *r;
    size_t inslen;

    if (!s || !*s || !ins || !*ins) {
        return NULL;
    }

    inslen = strlen_w(ins);
    r = (ucs2_t *)s;

    if ((0xD800 <= *ins) && (*ins < 0xDC00)) {
        if ((0xDC00 <= ins[1]) && (ins[1] < 0xE000)) {
            uint32_t ins_sp = (uint32_t) * ins << 16 | (uint32_t)ins[1];

            while ((r = strcasechr_sp(r, ins_sp))) {
                if (strncasecmp_w(r, ins, inslen) == 0) {
                    return r;
                }

                r++;
            }
        } else {
            return NULL; /* illegal sequence */
        }
    } else {
        while ((r = strcasechr_w(r, *ins))) {
            if (strncasecmp_w(r, ins, inslen) == 0) {
                return r;
            }

            r++;
        }
    }

    return NULL;
}

/*!
 * @brief Compare two NUL-terminated UTF-16 strings, ignoring case, with
 * surrogate pair support
 *
 * @param[in] a  first string
 * @param[in] b  second string
 * @returns the difference of the first differing lowercase forms, or 0
 */
int strcasecmp_w(const ucs2_t *a, const ucs2_t *b)
{
    int ret;

    while (*a && *b) {
        if ((0xD800 <= *a) && (*a < 0xDC00)) {
            if ((ret = tolower_sp((uint32_t) * a << 16 | (uint32_t)a[1]) - tolower_sp((
                    uint32_t) * b << 16 | (uint32_t)b[1]))) {
                return ret;
            }

            a++;
            b++;

            if (!(*a && *b)) {
                return (tolower_w(*a) - tolower_w(*b));    /* avoid buffer over run */
            }
        } else {
            if ((ret = tolower_w(*a) - tolower_w(*b))) {
                return ret;
            }
        }

        a++;
        b++;
    }

    return (tolower_w(*a) - tolower_w(*b));
}

/*!
 * @brief Compare up to @p len units of two UTF-16 strings, ignoring case
 *
 * A surrogate pair counts as two units. When @p len ends inside a pair,
 * the comparison covers both of its units.
 *
 * @param[in] a    first string
 * @param[in] b    second string
 * @param[in] len  limit in units
 * @returns the difference of the first differing lowercase forms, or 0
 */
int strncasecmp_w(const ucs2_t *a, const ucs2_t *b, size_t len)
{
    size_t n = 0;
    int ret;

    while ((n < len) && *a && *b) {
        if ((0xD800 <= *a) && (*a < 0xDC00)) {
            if ((ret = tolower_sp((uint32_t) * a << 16 | (uint32_t)a[1]) - tolower_sp((
                    uint32_t) * b << 16 | (uint32_t)b[1]))) {
                return ret;
            }

            a++;
            b++;
            n++;

            if (!((n < len) && *a && *b)) {
                return (tolower_w(*a) - tolower_w(*b));
            }
        } else {
            if ((ret = tolower_w(*a) - tolower_w(*b))) {
                return ret;
            }
        }

        a++;
        b++;
        n++;
    }

    return (len - n) ? (tolower_w(*a) - tolower_w(*b)) : 0;
}

/*!
 * @brief Duplicate the first @p len units of a UTF-16 string into a buffer from
 * malloc
 *
 * A @p len of 0 takes the full string. The call does not stop at an
 * earlier NUL and does not check for a split surrogate pair.
 *
 * @param[in] src  UTF-16 string
 * @param[in] len  number of units, or 0 for the full string
 * @returns the new string, or NULL when memory runs out
 */
ucs2_t *strndup_w(const ucs2_t *src, size_t len)
{
    ucs2_t *dest;

    if (!len) {
        len = strlen_w(src);
    }

    dest = (ucs2_t *)malloc((len + 1) * sizeof(ucs2_t));

    if (!dest) {
        LOG(log_error, logtype_default, "strdup_w: out of memory!");
        return NULL;
    }

    memcpy(dest, src, len * sizeof(ucs2_t));
    dest[len] = 0;
    return dest;
}

/*!
 * @brief Duplicate a NUL-terminated UTF-16 string into a buffer from malloc
 *
 * @param[in] src  UTF-16 string
 * @returns the new string, or NULL when memory runs out
 */
ucs2_t *strdup_w(const ucs2_t *src)
{
    return strndup_w(src, 0);
}

/*!
 * @brief Look up the precomposed BMP character for a base and a combining
 * character
 *
 * The call searches the sorted precomposition table.
 *
 * @param[in] base  base character
 * @param[in] comb  combining character
 * @returns the precomposed character, or 0 for no entry
 */
static ucs2_t do_precomposition(unsigned int base, unsigned int comb)
{
    int min = 0;
    int max = PRECOMP_COUNT - 1;
    int mid;
    uint32_t sought = (base << 16) | comb, that;

    /* binary search */
    while (max >= min) {
        mid = (min + max) / 2;
        that = (precompositions[mid].base << 16) | (precompositions[mid].comb);

        if (that < sought) {
            min = mid + 1;
        } else if (that > sought) {
            max = mid - 1;
        } else {
            return precompositions[mid].replacement;
        }
    }

    /* no match */
    return 0;
}

/*!
 * @brief Look up the precomposed surrogate pair for a base pair and a combining
 * pair
 *
 * Each pair sits in one 32-bit value, with the high surrogate in bits 31 to
 * 16.
 *
 * @param[in] base_sp  base pair
 * @param[in] comb_sp  combining pair
 * @returns the precomposed pair, or 0 for no entry
 */
static uint32_t do_precomposition_sp(unsigned int base_sp, unsigned int comb_sp)
{
    int min = 0;
    int max = PRECOMP_SP_COUNT - 1;
    int mid;
    uint64_t sought_sp = ((uint64_t)base_sp << 32) | (uint64_t)comb_sp, that_sp;

    /* binary search */
    while (max >= min) {
        mid = (min + max) / 2;
        that_sp = ((uint64_t)precompositions_sp[mid].base_sp << 32) | ((
                uint64_t)precompositions_sp[mid].comb_sp);

        if (that_sp < sought_sp) {
            min = mid + 1;
        } else if (that_sp > sought_sp) {
            max = mid - 1;
        } else {
            return precompositions_sp[mid].replacement_sp;
        }
    }

    /* no match */
    return 0;
}

/*!
 * @brief Look up the base and the combining character of a precomposed BMP
 * character
 *
 * @param[in] base  precomposed character
 * @returns the base in bits 31 to 16 and the combining character in bits 15 to
 *          0, or 0 for no entry
 */
static uint32_t do_decomposition(ucs2_t base)
{
    int min = 0;
    int max = DECOMP_COUNT - 1;
    int mid;
    uint32_t sought = base;
    uint32_t result, that;

    /* binary search */
    while (max >= min) {
        mid = (min + max) / 2;
        that = decompositions[mid].replacement;

        if (that < sought) {
            min = mid + 1;
        } else if (that > sought) {
            max = mid - 1;
        } else {
            result = (decompositions[mid].base << 16) | (decompositions[mid].comb);
            return result;
        }
    }

    /* no match */
    return 0;
}

/*!
 * @brief Look up the base pair and the combining pair of a precomposed
 * surrogate pair
 *
 * @param[in] base_sp  precomposed pair, with the high surrogate in bits 31 to
 *                     16
 * @returns the base pair in bits 63 to 32 and the combining pair in bits 31 to
 *          0, or 0 for no entry
 */
static uint64_t do_decomposition_sp(unsigned int base_sp)
{
    int min = 0;
    int max = DECOMP_SP_COUNT - 1;
    int mid;
    uint32_t sought_sp = base_sp;
    uint32_t that_sp;
    uint64_t result_sp;

    /* binary search */
    while (max >= min) {
        mid = (min + max) / 2;
        that_sp = decompositions_sp[mid].replacement_sp;

        if (that_sp < sought_sp) {
            min = mid + 1;
        } else if (that_sp > sought_sp) {
            max = mid - 1;
        } else {
            result_sp = ((uint64_t)decompositions_sp[mid].base_sp << 32) | ((
                    uint64_t)decompositions_sp[mid].comb_sp);
            return result_sp;
        }
    }

    /* no match */
    return 0;
}

/*!
 * @brief Precompose a UTF-16 string by the netatalk tables
 *
 * The call combines each base with the combining characters after it,
 * Hangul syllables included, and surrogate pairs through a separate table.
 * It does no singleton mapping or canonical ordering. It ignores the
 * composition exclusions, which would break round trips.
 *
 * The tables leave out U+2000 to U+2FFF, U+FE30 to U+FE4F, and U+2F800 to
 * U+2FA1F, as the AFP 3.x specification asks. The output ends with a NUL,
 * which the returned length leaves out.
 *
 * @param[in]     name    UTF-16 input
 * @param[in]     inplen  input length in bytes, an even number above 0
 * @param[out]    comp    output buffer
 * @param[in,out] outlen  output space in bytes, which receives the space left
 * @returns the output length in bytes, or `(size_t)-1` for an odd, empty, or
 *          oversized input or with E2BIG
 */
size_t precompose_w(ucs2_t *name, size_t inplen, ucs2_t *comp, size_t *outlen)
{
    size_t i;
    ucs2_t base, comb;
    uint32_t base_sp, comb_sp;
    ucs2_t *in, *out;
    ucs2_t lindex, vindex;
    ucs2_t result;
    uint32_t result_sp;
    size_t o_len = *outlen;

    if (!inplen || (inplen & 1) || inplen > o_len) {
        return (size_t) -1;
    }

    i = 0;
    in  = name;
    out = comp;
    base = *in;

    while (*outlen > 2) {
        i += 2;

        if (i == inplen) {
            *out = base;
            out++;
            *out = 0;
            *outlen -= 2;
            return o_len - *outlen;
        }

        in++;
        comb = *in;
        result = 0;

        /* Non-Combination Character */
        if (comb < 0x300);
        /* Unicode Standard Annex #15 A10.3 Hangul Composition */
        /* Step 1: leading consonant, vowel */
        else if ((VBASE <= comb) && (comb <= VBASE + VCOUNT)) {
            if ((LBASE <= base) && (base < LBASE + LCOUNT)) {
                result = 1;
                lindex = base - LBASE;
                vindex = comb - VBASE;
                base = SBASE + (lindex * VCOUNT + vindex) * TCOUNT;
            }
        }
        /* Step 2: leading vowel, trailing consonant */
        else if ((TBASE < comb) && (comb < TBASE + TCOUNT)) {
            if ((SBASE <= base) && (base < SBASE + SCOUNT)
                    && (((base - SBASE) % TCOUNT) == 0)) {
                result = 1;
                base += comb - TBASE;
            }
        }
        /* Binary Search for Surrogate Pair */
        else if ((0xD800 <= base) && (base < 0xDC00)) {
            if ((0xDC00 <= comb) && (comb < 0xE000) && (i + 6 <= inplen)) {
                base_sp = ((uint32_t)base << 16) | (uint32_t)comb;

                do {
                    comb_sp = ((uint32_t)in[1] << 16) | (uint32_t)in[2];

                    if ((result_sp = do_precomposition_sp(base_sp, comb_sp))) {
                        base_sp = result_sp;
                        i += 4;
                        in += 2;
                    }
                } while ((i + 6 <= inplen) && result_sp);

                *out = base_sp >> 16;
                out++;
                *outlen -= 2;

                if (*outlen <= 2) {
                    errno = E2BIG;
                    return (size_t) -1;
                }

                *out = base_sp & 0xFFFF;
                out++;
                *outlen -= 2;
                i += 2;

                if (i == inplen) {
                    *out = 0;
                    return o_len - *outlen;
                }

                in++;
                base = *in;
                result = 1;
            }
        }
        /* Binary Search for BMP */
        else if ((result = do_precomposition(base, comb))) {
            base = result;
        }

        if (!result) {
            *out = base;
            out++;
            *outlen -= 2;
            base = comb;
        }
    }

    errno = E2BIG;
    return (size_t) -1;
}

/*!
 * @brief Decompose a UTF-16 string by the netatalk tables
 *
 * The call splits each precomposed character into its base and its
 * combining characters, Hangul syllables included, and surrogate pairs
 * through a separate table. ASCII stays as is. The output ends with a NUL,
 * which the returned length leaves out.
 *
 * @param[in]     name    UTF-16 input
 * @param[in]     inplen  input length in bytes, an even number above 0
 * @param[out]    comp    output buffer
 * @param[in,out] outlen  output space in bytes, which receives the space left
 * @returns the output length in bytes, or `(size_t)-1` for an odd or empty
 *          input or with E2BIG
 */
size_t decompose_w(ucs2_t *name, size_t inplen, ucs2_t *comp, size_t *outlen)
{
    size_t i;
    size_t comblen;
    ucs2_t base, comb[COMBBUFLEN];
    uint32_t base_sp;
    ucs2_t sindex, tjamo;
    ucs2_t *in, *out;
    unsigned int result;
    uint64_t result_sp;
    size_t o_len = *outlen;

    if (!inplen || (inplen & 1)) {
        return (size_t) -1;
    }

    i = 0;
    in  = name;
    out = comp;

    while (i < inplen) {
        base = *in;
        comblen = 0;

        /* check ASCII first. this is frequent. */
        if (base <= 0x007f);
        /* Unicode Standard Annex #15 A10.2 Hangul Decomposition */
        else if ((SBASE <= base) && (base < SBASE + SCOUNT)) {
            sindex = base - SBASE;
            base = LBASE + sindex / NCOUNT;
            comb[COMBBUFLEN - 2] = VBASE + (sindex % NCOUNT) / TCOUNT;

            /* <L,V> */
            if ((tjamo = TBASE + sindex % TCOUNT) == TBASE) {
                comb[COMBBUFLEN - 1] = comb[COMBBUFLEN - 2];
                comblen = 1;
            }
            /* <L,V,T> */
            else {
                comb[COMBBUFLEN - 1] = tjamo;
                comblen = 2;
            }
        }
        /* Binary Search for Surrogate Pair */
        else if ((0xD800 <= base) && (base < 0xDC00)) {
            if (i + 2 < inplen) {
                base_sp = ((uint32_t)base << 16) | (uint32_t)in[1];

                do {
                    if (!(result_sp = do_decomposition_sp(base_sp))) {
                        break;
                    }

                    comblen += 2;
                    base_sp = result_sp >> 32;
                    comb[COMBBUFLEN - comblen] = (result_sp >> 16) & 0xFFFF; /* hi */
                    comb[COMBBUFLEN - comblen + 1] = result_sp & 0xFFFF;    /* lo */
                } while (comblen < MAXCOMBSPLEN);

                /* Reserve space for the lower code unit and terminator below. */
                if (*outlen < (comblen + 3) << 1) {
                    errno = E2BIG;
                    return (size_t) -1;
                }

                *out = base_sp >> 16;   /* hi */
                out++;
                *outlen -= 2;
                base = base_sp & 0xFFFF; /* lo */
                i += 2;
                in++;
            }
        }
        /* Binary Search for BMP */
        else {
            do {
                if (!(result = do_decomposition(base))) {
                    break;
                }

                comblen++;
                base = result  >> 16;
                comb[COMBBUFLEN - comblen] = result & 0xFFFF;
            } while ((0x007f < base) && (comblen < MAXCOMBLEN));
        }

        /* Reserve space for the UTF-16 terminator below. */
        if (*outlen < (comblen + 2) << 1) {
            errno = E2BIG;
            return (size_t) -1;
        }

        *out = base;
        out++;
        *outlen -= 2;

        while (comblen > 0) {
            *out = comb[COMBBUFLEN - comblen];
            out++;
            *outlen -= 2;
            comblen--;
        }

        i += 2;
        in++;
    }

    *out = 0;
    return o_len - *outlen;
}
