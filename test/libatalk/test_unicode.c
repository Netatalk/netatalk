/*
 * TAP tests for Unicode and filename character conversion
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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
 */

/*!
 * @file
 * Exercise public libatalk conversion APIs without an AFP server. Expected
 * bytes are specified independently of the converters, including AFP's
 * normalization exceptions. Regression cases check filename punctuation,
 * empty-output termination, and decomposition buffer bounds.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/unicode.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define GUARD_SIZE 16
#define SENTINEL 0xa5
#define OUTPUT_SIZE (MAXPATHLEN + 2)

static int tests_run;
static int tests_failed;

struct conversion_case {
    const char *name;
    const char *from;
    const char *to;
    const char *input;
    size_t input_len;
    const char *expected;
    size_t expected_len;
    uint16_t options;
    uint16_t expected_flags;
    size_t capacity;
    int expected_errno;
};

#define SUCCESS(name, from, to, input, expected, options, result_flags) \
    {name, from, to, input, sizeof(input) - 1, expected, sizeof(expected) - 1, \
     options, (options) | (result_flags), 64, 0}
#define FAILURE(name, from, to, input, options, capacity, error) \
    {name, from, to, input, sizeof(input) - 1, "", 0, \
     options, options, capacity, error}

static const struct conversion_case filename_cases[] = {
    SUCCESS("UTF8 ASCII identity", "UTF8", "UTF8", "hello", "hello", 0, 0),
    SUCCESS("Mac Roman e acute to UTF8",
    "MAC_ROMAN", "UTF8", "\x8e", "\xc3\xa9", 0, 0),
    SUCCESS("UTF8 e acute to Mac Roman",
    "UTF8", "MAC_ROMAN", "\xc3\xa9", "\x8e", 0, 0),
    SUCCESS("Mac Roman Apple logo to UTF8",
    "MAC_ROMAN", "UTF8", "\xf0", "\xef\xa3\xbf", 0, 0),
    SUCCESS("Mac UTF8 precomposes e acute",
    "UTF8-MAC", "UTF8", "e\xcc\x81", "\xc3\xa9", 0, 0),
    SUCCESS("Mac UTF8 target decomposes e acute",
    "UTF8", "UTF8-MAC", "\xc3\xa9", "e\xcc\x81", 0, 0),
    SUCCESS("explicit precomposition",
    "UTF8", "UTF8", "e\xcc\x81", "\xc3\xa9", CONV_PRECOMPOSE, 0),
    SUCCESS("explicit decomposition",
    "UTF8", "UTF8", "\xc3\xa9", "e\xcc\x81", CONV_DECOMPOSE, 0),
    SUCCESS("recursive decomposition of U with diaeresis and macron",
    "UTF8", "UTF8-MAC", "\xc7\x95", "U\xcc\x88\xcc\x84", 0, 0),
    SUCCESS("Hangul decomposition",
    "UTF8", "UTF8-MAC", "\xea\xb0\x81",
    "\xe1\x84\x80\xe1\x85\xa1\xe1\x86\xa8", 0, 0),
    SUCCESS("Hangul composition",
    "UTF8-MAC", "UTF8", "\xe1\x84\x80\xe1\x85\xa1\xe1\x86\xa8",
    "\xea\xb0\x81", 0, 0),
    SUCCESS("AFP leaves Angstrom sign undecomposed",
    "UTF8", "UTF8-MAC", "\xe2\x84\xab", "\xe2\x84\xab", 0, 0),
    SUCCESS("supplementary character survives normalization",
    "UTF8", "UTF8-MAC", "\xf0\x9f\x98\x80", "\xf0\x9f\x98\x80", 0, 0),
    SUCCESS("uppercase ASCII",
    "UTF8", "UTF8", "hello", "HELLO", CONV_TOUPPER, 0),
    SUCCESS("lowercase ASCII",
    "UTF8", "UTF8", "HELLO", "hello", CONV_TOLOWER, 0),
    SUCCESS("uppercase e acute",
    "UTF8", "UTF8", "\xc3\xa9", "\xc3\x89", CONV_TOUPPER, 0),
    SUCCESS("lowercase e acute",
    "UTF8", "UTF8", "\xc3\x89", "\xc3\xa9", CONV_TOLOWER, 0),
    SUCCESS("uppercase Deseret surrogate pair",
    "UTF8", "UTF8", "\xf0\x90\x90\xa8", "\xf0\x90\x90\x80",
    CONV_TOUPPER, 0),
    SUCCESS("lowercase Deseret surrogate pair",
    "UTF8", "UTF8", "\xf0\x90\x90\x80", "\xf0\x90\x90\xa8",
    CONV_TOLOWER, 0),
    SUCCESS("uppercase takes precedence over lowercase",
    "UTF8", "UTF8", "Hello", "HELLO", CONV_TOUPPER | CONV_TOLOWER, 0),
    SUCCESS("Unix slash remains a slash", "UTF8", "UTF8", "a/b", "a/b", 0, 0),
    SUCCESS("Mac UTF8 slash becomes Unix colon",
    "UTF8-MAC", "UTF8", "a/b", "a:b", 0, 0),
    SUCCESS("Mac Roman slash becomes Unix colon",
    "MAC_ROMAN", "UTF8", "a/b", "a:b", 0, 0),
    SUCCESS("Unix colon becomes Mac UTF8 slash",
    "UTF8", "UTF8-MAC", "a:b", "a/b", 0, 0),
    SUCCESS("Unix colon becomes Mac Roman slash",
    "UTF8", "MAC_ROMAN", "a:b", "a/b", 0, 0),
    SUCCESS("Unix colon remains a colon", "UTF8", "UTF8", "a:b", "a:b", 0, 0),
    /* Regression: a Mac-to-Mac conversion must not apply Unix slash mapping. */
    SUCCESS("Mac UTF8 identity preserves slash",
    "UTF8-MAC", "UTF8-MAC", "a/b", "a/b", 0, 0),
    SUCCESS("Mac Roman identity preserves slash",
    "MAC_ROMAN", "MAC_ROMAN", "a/b", "a/b", 0, 0),
    SUCCESS("Mac UTF8 to Mac Roman preserves slash",
    "UTF8-MAC", "MAC_ROMAN", "a/b", "a/b", 0, 0),
    SUCCESS("Mac Roman to Mac UTF8 preserves slash",
    "MAC_ROMAN", "UTF8-MAC", "a/b", "a/b", 0, 0),
    SUCCESS("escape colon and slash",
    "UTF8", "UTF8", ":/", ":3a:2f", CONV_ESCAPEHEX, 0),
    SUCCESS("escape leading dot",
    "UTF8", "UTF8", ".hidden", ":2ehidden", CONV_ESCAPEDOTS,
    CONV_REQESCAPE),
    SUCCESS("interior dot stays literal",
    "UTF8", "UTF8", "a.b", "a.b", CONV_ESCAPEDOTS, 0),
    SUCCESS("escape unmappable e acute through Mac Roman",
    "UTF8", "ASCII", "\xc3\xa9", ":8e", CONV_ESCAPEHEX, CONV_REQESCAPE),
    SUCCESS("unescape Mac Roman e acute",
    "UTF8", "UTF8", ":8e", "\xc3\xa9", CONV_UNESCAPEHEX, 0),
    SUCCESS("unescape adjacent hex bytes",
    "UTF8", "UTF8", ":41:42", "AB", CONV_UNESCAPEHEX, 0),
    SUCCESS("unescape accepts uppercase hex digits",
    "UTF8", "UTF8", ":8E", "\xc3\xa9", CONV_UNESCAPEHEX, 0),
    SUCCESS("ignore unmappable suffix and request mangling",
    "UTF8", "ASCII", "a\xc3\xa9z", "a", CONV_IGNORE, CONV_REQMANGLE),
    SUCCESS("ignore invalid source suffix and request mangling",
    "UTF8", "UTF8", "a\xffz", "a", CONV_IGNORE, CONV_REQMANGLE),
    SUCCESS("force returns prefix of invalid source",
    "UTF8", "UTF8", "a\xffz", "a", CONV_FORCE, 0),
    SUCCESS("replace invalid source byte with underscore",
    "UTF8", "UTF8", "a\xffz", "a_z", CONV__EILSEQ, 0),
    SUCCESS("ignored invalid suffix still escapes leading dot",
    "UTF8", "UTF8", ".\xff", ":2e", CONV_IGNORE | CONV_ESCAPEDOTS,
    CONV_REQMANGLE | CONV_REQESCAPE),
    SUCCESS("forced invalid suffix still escapes leading dot",
    "UTF8", "UTF8", ".\xff", ":2e", CONV_FORCE | CONV_ESCAPEDOTS,
    CONV_REQESCAPE),
    FAILURE("invalid UTF8 source fails",
    "UTF8", "UTF8", "a\xffz", 0, 64, EILSEQ),
    FAILURE("truncated UTF8 source fails",
    "UTF8", "UTF8", "\xc3", 0, 64, EILSEQ),
    FAILURE("unmappable target character fails",
    "UTF8", "ASCII", "\xc3\xa9", 0, 64, EILSEQ),
    FAILURE("invalid hex escape fails",
    "UTF8", "UTF8", ":zz", CONV_UNESCAPEHEX, 64, EILSEQ),
    FAILURE("incomplete hex escape fails",
    "UTF8", "UTF8", ":8", CONV_UNESCAPEHEX, 64, EILSEQ),
    FAILURE("zero output capacity fails", "UTF8", "UTF8", "a", 0, 0, E2BIG),
    FAILURE("one output byte fails", "UTF8", "UTF8", "a", 0, 1, E2BIG),
    FAILURE("output must reserve two terminator bytes",
    "UTF8", "UTF8", "a", 0, 2, E2BIG),
    {
        "exact filename output capacity", "UTF8", "UTF8",
        "a", 1, "a", 1, 0, 0, 3, 0
    },
    /* Regression: use a dirty destination, unlike netacnv's static buffer. */
    SUCCESS("empty filename writes both terminator bytes",
            "UTF8-MAC", "UTF8", "", "", 0, 0),
    FAILURE("empty filename rejects zero output capacity",
            "UTF8-MAC", "UTF8", "", 0, 0, E2BIG),
    FAILURE("empty filename rejects one output byte",
            "UTF8-MAC", "UTF8", "", 0, 1, E2BIG),
    {
        "empty filename fits exactly two terminator bytes", "UTF8-MAC", "UTF8",
        "", 0, "", 0, 0, 0, 2, 0
    },
    SUCCESS("ignore invalid first byte terminates empty result",
            "UTF8", "UTF8", "\xff", "", CONV_IGNORE, CONV_REQMANGLE),
    SUCCESS("force invalid first byte terminates empty result",
            "UTF8", "UTF8", "\xff", "", CONV_FORCE, 0),
};

static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

static int filled_with(const unsigned char *bytes, size_t len,
                       unsigned char value)
{
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] != value) {
            return 0;
        }
    }

    return 1;
}

/*!
 * @brief Check exact output bytes, terminators, and guards for a bounded call
 *
 * A failure may leave a converted prefix, but must not write outside the
 * advertised capacity. No strlen() is used on a possibly unterminated result.
 */
static int check_output(const struct conversion_case *tc, size_t result,
                        int error, const unsigned char *storage,
                        size_t terminators)
{
    const unsigned char *dest = storage + GUARD_SIZE;
    size_t total = GUARD_SIZE + OUTPUT_SIZE + GUARD_SIZE;
    int passed = filled_with(storage, GUARD_SIZE, SENTINEL)
                 && filled_with(dest + tc->capacity,
                                total - GUARD_SIZE - tc->capacity, SENTINEL);

    if (!passed) {
        printf("# write outside destination capacity %zu\n", tc->capacity);
    }

    if (tc->expected_errno) {
        if (result != (size_t) -1 || error != tc->expected_errno) {
            printf("# returned %zu, errno %d; expected failure with errno %d\n",
                   result, error, tc->expected_errno);
            passed = 0;
        }
    } else if (result != tc->expected_len) {
        printf("# returned %zu, errno %d; expected %zu bytes\n",
               result, error, tc->expected_len);
        passed = 0;
    } else if (tc->expected_len + terminators > tc->capacity
               || memcmp(dest, tc->expected, tc->expected_len) != 0
               || !filled_with(dest + tc->expected_len, terminators, 0)) {
        printf("# expected bytes:");

        for (size_t i = 0; i < tc->expected_len + terminators; i++) {
            unsigned char byte = i < tc->expected_len ? tc->expected[i] : 0;
            printf(" %02x", byte);
        }

        printf("\n# actual bytes:");

        for (size_t i = 0; i < tc->expected_len + terminators
                && i < tc->capacity; i++) {
            printf(" %02x", dest[i]);
        }

        printf("\n");
        passed = 0;
    }

    return passed;
}

static void test_filename_case(const struct conversion_case *tc)
{
    unsigned char storage[GUARD_SIZE + OUTPUT_SIZE + GUARD_SIZE];
    uint16_t flags = tc->options;
    size_t result;
    int error;
    int passed;
    charset_t from = add_charset(tc->from);
    charset_t to = add_charset(tc->to);
    char *dest = (char *)storage + GUARD_SIZE;
    memset(storage, SENTINEL, sizeof(storage));
    errno = 0;
    result = convert_charset(from, to, CH_MAC, tc->input, tc->input_len,
                             dest, tc->capacity, &flags);
    error = errno;
    passed = check_output(tc, result, error, storage, 2);

    if (flags != tc->expected_flags) {
        printf("# flags 0x%04x; expected 0x%04x\n", flags, tc->expected_flags);
        passed = 0;
    }

    report(passed, tc->name);
}

static void test_string_apis(void)
{
    static const struct conversion_case cases[] = {
        SUCCESS("ASCII identity", "UTF8", "UTF8", "hello", "hello", 0, 0),
        SUCCESS("Mac Roman to UTF8",
        "MAC_ROMAN", "UTF8", "\x8e", "\xc3\xa9", 0, 0),
        SUCCESS("Mac UTF8 precomposition",
        "UTF8-MAC", "UTF8", "e\xcc\x81", "\xc3\xa9", 0, 0),
        SUCCESS("Mac UTF8 decomposition",
        "UTF8", "UTF8-MAC", "\xc3\xa9", "e\xcc\x81", 0, 0),
        SUCCESS("string APIs leave filename punctuation literal",
        "UTF8-MAC", "UTF8", "a/b:c", "a/b:c", 0, 0),
        SUCCESS("embedded NUL with explicit byte length",
        "UTF8", "UTF8", "a\0b", "a\0b", 0, 0),
        FAILURE("invalid UTF8", "UTF8", "UTF8", "\xff", 0, 64, EILSEQ),
        FAILURE("truncated UTF8", "UTF8", "UTF8", "\xc3", 0, 64, EINVAL),
        FAILURE("unmappable ASCII", "UTF8", "ASCII", "\xc3\xa9", 0, 64, EILSEQ),
    };

    for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
        const struct conversion_case *tc = &cases[i];
        unsigned char storage[GUARD_SIZE + OUTPUT_SIZE + GUARD_SIZE];
        char name[160];
        char *allocated = NULL;
        charset_t from = add_charset(tc->from);
        charset_t to = add_charset(tc->to);
        size_t result;
        int error;
        int passed;
        memset(storage, SENTINEL, sizeof(storage));
        errno = 0;
        result = convert_string(from, to, tc->input, tc->input_len,
                                storage + GUARD_SIZE, tc->capacity);
        error = errno;
        snprintf(name, sizeof(name), "convert_string: %s", tc->name);
        report(check_output(tc, result, error, storage, 1), name);
        errno = 0;
        result = convert_string_allocate(from, to, tc->input, tc->input_len,
                                         &allocated);
        error = errno;

        if (tc->expected_errno) {
            passed = result == (size_t) -1 && error == tc->expected_errno
                     && allocated == NULL;
        } else {
            passed = result == tc->expected_len && allocated != NULL
                     && memcmp(allocated, tc->expected, tc->expected_len) == 0
                     && allocated[tc->expected_len] == 0;
        }

        if (!passed) {
            printf("# allocated conversion returned %zu, errno %d; "
                   "expected %zu bytes, errno %d\n",
                   result, error, tc->expected_len, tc->expected_errno);
        }

        snprintf(name, sizeof(name), "convert_string_allocate: %s", tc->name);
        report(passed, name);
        free(allocated);
    }
}

/*!
 * @brief Exercise decoded filename syntax and error policies through libatalk
 */
static void test_wide_filename_cases(void)
{
    static const ucs2_t native[] = {'.', ':', '/', 0x012e, 0x012f, 0x013a};
    struct conversion_case tc = {
        "native UCS2 source scans characters rather than bytes", "UCS-2", "UTF8",
        (const char *)native, sizeof(native), ".:/\xc4\xae\xc4\xaf\xc4\xba", 9,
        0, 0, 64, 0
    };
    test_filename_case(&tc);
    tc.name = "native UCS2 source accepts an unaligned buffer";
    char unaligned[sizeof(native) + 1];
    memcpy(unaligned + 1, native, sizeof(native));
    tc.input = unaligned + 1;
    test_filename_case(&tc);
    tc.name = "native UCS2 truncated source fails";
    tc.input_len--;
    tc.expected_errno = EILSEQ;
    test_filename_case(&tc);
    tc.name = "native UCS2 truncated source can be replaced";
    tc.expected_errno = 0;
    tc.options = CONV__EILSEQ;
    tc.expected_flags = CONV__EILSEQ;
    tc.expected = ".:/\xc4\xae\xc4\xaf_";
    tc.expected_len = 8;
    test_filename_case(&tc);
#ifdef HAVE_USABLE_ICONV
    static const struct conversion_case cases[] = {
        SUCCESS("UTF16LE punctuation bytes inside other characters",
        "UTF-16LE", "UTF8", "\x2e\x01\x2f\x01\x3a\x01",
        "\xc4\xae\xc4\xaf\xc4\xba", CONV_ESCAPEHEX | CONV_ESCAPEDOTS, 0),
        SUCCESS("UTF16BE punctuation bytes inside other characters",
        "UTF-16BE", "UTF8", "\x01\x2e\x01\x2f\x01\x3a",
        "\xc4\xae\xc4\xaf\xc4\xba", 0, 0),
        SUCCESS("UTF32LE source with punctuation",
        "UTF-32LE", "UTF8", "A\0\0\0:\0\0\0/\0\0\0", "A:/", 0, 0),
        SUCCESS("UTF16LE source escapes actual punctuation",
        "UTF-16LE", "UTF8", ":\0/\0", ":3a:2f", CONV_ESCAPEHEX, 0),
        SUCCESS("UTF16LE source escapes leading dot",
        "UTF-16LE", "UTF8", ".\0a\0", ":2ea", CONV_ESCAPEDOTS, CONV_REQESCAPE),
        SUCCESS("UTF16LE source unescapes hex digits",
        "UTF-16LE", "UTF8", ":\0" "8\0e\0", "\xc3\xa9", CONV_UNESCAPEHEX, 0),
        SUCCESS("UTF16LE source maps colon for a Mac target",
        "UTF-16LE", "MAC_ROMAN", "a\0:\0b\0", "a/b", 0, 0),
        SUCCESS("UTF16LE source retains slash for a Unix target",
        "UTF-16LE", "UTF8", "a\0/\0b\0", "a/b", 0, 0),
        SUCCESS("UTF16LE source precomposes decoded characters",
        "UTF-16LE", "UTF8", "e\0\x01\x03", "\xc3\xa9", 0, 0),
        SUCCESS("UTF16LE source uppercases decoded characters",
        "UTF-16LE", "UTF8", "a\0", "A", CONV_TOUPPER, 0),
        SUCCESS("UTF16LE source preserves embedded Unicode NUL",
        "UTF-16LE", "UTF8", "a\0\0\0b\0", "a\0b", 0, 0),
        FAILURE("UTF16LE malformed source fails",
        "UTF-16LE", "UTF8", "A\0\0\xd8" "B\0", 0, 64, EILSEQ),
        SUCCESS("UTF16LE malformed source IGNORE keeps prefix",
        "UTF-16LE", "UTF8", "A\0\0\xd8" "B\0", "A", CONV_IGNORE, CONV_REQMANGLE),
        SUCCESS("UTF16LE malformed source FORCE keeps prefix",
        "UTF-16LE", "UTF8", "A\0\0\xd8" "B\0", "A", CONV_FORCE, 0),
        SUCCESS("UTF16LE malformed source replacement preserves alignment",
        "UTF-16LE", "UTF8", "A\0\0\xd8" "B\0", "A_B", CONV__EILSEQ, 0),
        SUCCESS("UTF32LE truncated source replacement",
        "UTF-32LE", "UTF8", "A\0\0\0B\0\0", "A_", CONV__EILSEQ, 0),
        SUCCESS("UTF16 BOM byte order survives replacement",
        "UTF-16", "UTF8", "\xff\xfe" "A\0\0\xd8" "B\0", "A_B", CONV__EILSEQ, 0),
        SUCCESS("UTF16 big-endian BOM after little-endian conversion",
        "UTF-16", "UTF8", "\xfe\xff\0B", "B", 0, 0),
        SUCCESS("UTF16 little-endian BOM after big-endian conversion",
        "UTF-16", "UTF8", "\xff\xfe" "A\0", "A", 0, 0),
        SUCCESS("UTF16 big-endian BOM byte order survives replacement",
        "UTF-16", "UTF8", "\xfe\xff\0A\xd8\0\0B", "A_B", CONV__EILSEQ, 0),
        SUCCESS("UTF32 little-endian BOM source",
        "UTF-32", "UTF8", "\xff\xfe\0\0A\0\0\0", "A", 0, 0),
        SUCCESS("UTF32 big-endian BOM after little-endian conversion",
        "UTF-32", "UTF8", "\0\0\xfe\xff\0\0\0B", "B", 0, 0),
        SUCCESS("UTF32 little-endian BOM after big-endian conversion",
        "UTF-32", "UTF8", "\xff\xfe\0\0A\0\0\0", "A", 0, 0),
        SUCCESS("UTF32 BOM truncated code unit replacement",
        "UTF-32", "UTF8", "\0\0\xfe\xff\0\0\0A\0\0\0", "A_",
        CONV__EILSEQ, 0),
        SUCCESS("UTF32 little-endian BOM truncated code unit replacement",
        "UTF-32", "UTF8", "\xff\xfe\0\0A\0\0\0B\0\0", "A_",
        CONV__EILSEQ, 0),
        FAILURE("UTF16LE source truncation fails",
        "UTF-16LE", "UTF8", "A\0B", 0, 64, EILSEQ),
        SUCCESS("UTF16LE source truncation replacement",
        "UTF-16LE", "UTF8", "A\0B", "A_", CONV__EILSEQ, 0),
        FAILURE("UTF16LE source malformed hex escape fails",
        "UTF-16LE", "UTF8", ":\0z\0z\0", CONV_UNESCAPEHEX, 64, EILSEQ),
        SUCCESS("UTF16LE source malformed hex escape IGNORE keeps prefix",
        "UTF-16LE", "UTF8", "A\0:\0z\0z\0", "A",
        CONV_UNESCAPEHEX | CONV_IGNORE, CONV_REQMANGLE),
        SUCCESS("ISO2022 encoded slash is not filename punctuation",
        "ISO-2022-JP", "UTF8", "\x1b$B1/\x1b(B", "\xe4\xb8\x91", CONV_ESCAPEHEX, 0),
        SUCCESS("ISO2022 source can finish in a shifted state",
        "ISO-2022-JP", "UTF8", "\x1b$B1/", "\xe4\xb8\x91", 0, 0),
        SUCCESS("ISO2022 shift state is reset before the next filename",
        "ISO-2022-JP", "UTF8", "plain", "plain", 0, 0),
        SUCCESS("system UTF8 malformed source replacement resumes decoding",
        "UTF-8", "UTF8", "A\xff" "B", "A_B", CONV__EILSEQ, 0),
    };

    for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
        atalk_iconv_t descriptor = atalk_iconv_open("UCS-2", cases[i].from);

        if (descriptor == (atalk_iconv_t) -1) {
            printf("ok %d - %s # SKIP charset unavailable\n", ++tests_run, cases[i].name);
            continue;
        }

        atalk_iconv_close(descriptor);
        test_filename_case(&cases[i]);
    }

#endif
}

/*!
 * @brief Cover the fixed decomposition terminator regression from afpdtest
 *
 * A destination holding only the payload is too small. Both the recursive
 * BMP expansion and the surrogate-pair branch must reject before writing,
 * and must succeed with exactly one additional UTF-16 terminator slot.
 */
static void test_decomposition_bounds(void)
{
    ucs2_t bmp[] = {0x01d5};
    ucs2_t surrogate[] = {0xd800, 0xdc00};
    const ucs2_t bmp_expected[] = {0x0055, 0x0308, 0x0304, 0};
    const ucs2_t surrogate_expected[] = {0xd800, 0xdc00, 0};

    for (size_t i = 0; i < 2; i++) {
        ucs2_t storage[8];
        ucs2_t *input = i == 0 ? bmp : surrogate;
        const ucs2_t *expected = i == 0 ? bmp_expected : surrogate_expected;
        size_t input_len = i == 0 ? sizeof(bmp) : sizeof(surrogate);
        size_t expected_len = i == 0 ? sizeof(bmp_expected)
                              : sizeof(surrogate_expected);
        size_t payload_len = expected_len - sizeof(ucs2_t);
        size_t outlen = payload_len;
        size_t result;
        int error;
        int untouched;
        memset(storage, SENTINEL, sizeof(storage));
        errno = 0;
        result = decompose_w(input, input_len, storage + 1, &outlen);
        error = errno;
        untouched = filled_with((unsigned char *)storage, sizeof(storage),
                                SENTINEL);
        report(result == (size_t) -1 && error == E2BIG && untouched,
               i == 0 ? "BMP decomposition reserves terminator before writing"
               : "surrogate decomposition reserves terminator before writing");
        outlen = payload_len + 2;
        result = decompose_w(input, input_len, storage + 1, &outlen);
        report(result == payload_len && outlen == 2
               && memcmp(storage + 1, expected, payload_len + 2) == 0
               && storage[0] == 0xa5a5
               && filled_with((unsigned char *)(storage + 1) + payload_len + 2,
                              sizeof(storage) - 2 - payload_len - 2, SENTINEL),
               i == 0 ? "BMP decomposition fits payload plus terminator"
               : "surrogate decomposition fits payload plus terminator");
    }
}

static void test_composition_bounds(void)
{
    ucs2_t input[] = {0x0065, 0x0301};
    ucs2_t output[4];
    size_t outlen = sizeof(input);
    size_t result;
    memset(output, SENTINEL, sizeof(output));
    result = precompose_w(input, sizeof(input), output + 1, &outlen);
    report(result == 2 && outlen == 2 && output[0] == 0xa5a5
           && output[1] == 0x00e9 && output[2] == 0 && output[3] == 0xa5a5,
           "precompose_w combines e acute and reserves terminator");
}

static void test_path_length_boundaries(void)
{
    char input[MAXPATHLEN + 2];
    struct conversion_case tc = {
        "filename at maximum output payload", "UTF8", "UTF8", input,
        MAXPATHLEN - 2, input, MAXPATHLEN - 2, 0, 0, MAXPATHLEN, 0
    };
    memset(input, 'a', sizeof(input));
    input[sizeof(input) - 1] = 0;
    test_filename_case(&tc);
    char escaped[3 * (MAXPATHLEN - 2)];

    for (size_t i = 0; i < sizeof(escaped); i += 3) {
        memcpy(escaped + i, ":61", 3);
    }

    struct conversion_case escaped_tc = tc;

    escaped_tc.name = "long escaped input fits after unescaping";
    escaped_tc.input = escaped;
    escaped_tc.input_len = sizeof(escaped);
    escaped_tc.options = CONV_UNESCAPEHEX;
    escaped_tc.expected_flags = CONV_UNESCAPEHEX;
    test_filename_case(&escaped_tc);
    tc.name = "filename output payload one byte too large";
    tc.input_len++;
    tc.expected_errno = E2BIG;
    test_filename_case(&tc);
    tc.name = "filename exceeds intermediate Unicode buffer";
    tc.input_len = MAXPATHLEN + 1;
    tc.capacity = OUTPUT_SIZE;
    test_filename_case(&tc);
}

int main(void)
{
    printf("TAP version 13\n");

    /* Set fixed names before add_charset() caches conversion descriptors. */
    if (set_charset_name(CH_UNIX, "UTF8") != 0
            || set_charset_name(CH_MAC, "MAC_ROMAN") != 0
            || add_charset("UTF8") == (charset_t) -1
            || add_charset("UTF8-MAC") == (charset_t) -1
            || add_charset("MAC_ROMAN") == (charset_t) -1
            || add_charset("ASCII") == (charset_t) -1) {
        printf("Bail out! cannot initialize built-in charsets\n");
        return 1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(filename_cases); i++) {
        test_filename_case(&filename_cases[i]);
    }

    test_string_apis();
    test_wide_filename_cases();
    test_decomposition_bounds();
    test_composition_bounds();
    test_path_length_boundaries();
    printf("1..%d\n", tests_run);
    free_charset_names();
    return tests_failed == 0 ? 0 : 1;
}
