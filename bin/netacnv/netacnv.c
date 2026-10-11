/*
 * Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
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
 * @brief The netacnv command: convert a string between two charsets
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atalk/unicode.h>

#define MACCHARSET "MAC_ROMAN"

#define FLAG(x) {x, #x}

struct flag_map {
    uint16_t flag;
    const char *flagname;
};

static const struct flag_map flag_map[] = {
    FLAG(CONV_ESCAPEHEX),
    FLAG(CONV_UNESCAPEHEX),
    FLAG(CONV_ESCAPEDOTS),
    FLAG(CONV_IGNORE),
    FLAG(CONV_FORCE),
    FLAG(CONV__EILSEQ),
    FLAG(CONV_TOUPPER),
    FLAG(CONV_TOLOWER),
    FLAG(CONV_PRECOMPOSE),
    FLAG(CONV_DECOMPOSE)
};

/*!
 * @brief Print the usage of netacnv, its defaults, and the names of its
 * conversion options to the chosen stream
 *
 * @param[in] stream  stdout for help, stderr for a usage error
 */
static void usage(FILE *stream)
{
    fprintf(stream, "Usage: netacnv [-hvx] [-o <conversion option> [...]]\n"
                    "               [-f <from charset>] [-t <to charset>]\n"
                    "               [-m <legacy Mac charset>] [--] <string>\n"
                    "Defaults: -f UTF8-MAC, -t UTF8, -m MAC_ROMAN\n"
                    "  -h  Show this help\n"
                    "  -x  Read input as hexadecimal byte pairs\n"
                    "  -v  Print charset names, byte lengths, hex bytes, "
                    "and flags to stderr\n"
                    "Input and output are limited to %d bytes; "
                    "expansion may exceed the limit.\n"
                    "Available conversion options:\n", MAXPATHLEN);

    for (size_t i = 0; i < sizeof(flag_map) / sizeof(struct flag_map); i++) {
        fprintf(stream, "%s\n", flag_map[i].flagname);
    }
}

/*!
 * @brief Validate both conversion directions before registering a charset
 *
 * add_charset returns an existing fixed slot by name without checking that
 * its descriptors loaded. Probe the actual converters first so a matching
 * name alone does not make an unusable charset appear valid.
 *
 * @param[in] name  charset name
 * @param[in] role  name of the command-line option for an error message
 * @returns 0 on success, or -1 after a diagnostic
 */
static int validate_charset(const char *name, const char *role)
{
    if (*name == '\0') {
        fprintf(stderr, "netacnv: %s charset name must not be empty\n", role);
        return -1;
    }

    for (int direction = 0; direction < 2; direction++) {
        atalk_iconv_t descriptor = atalk_iconv_open(
                                       direction == 0 ? "UCS-2" : name,
                                       direction == 0 ? name : "UCS-2");

        if (descriptor == (atalk_iconv_t) -1) {
            fprintf(stderr, "netacnv: cannot load %s charset '%s': %s\n",
                    role, name, strerror(errno));
            return -1;
        }

        atalk_iconv_close(descriptor);
    }

    return 0;
}

/*!
 * @brief Decode hexadecimal byte pairs, optionally separated by ASCII whitespace
 *
 * @param[in]  text    command-line hex input
 * @param[out] bytes   buffer with MAXPATHLEN bytes and a trailing NUL slot
 * @param[out] length  decoded byte count
 * @returns 0 on success, or -1 after a diagnostic
 */
static int decode_hex_input(const char *text, char *bytes, size_t *length)
{
    int high = -1;
    *length = 0;

    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        int digit;

        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'
                || *p == '\f' || *p == '\v') {
            if (high < 0) {
                continue;
            }

            fprintf(stderr, "netacnv: hex input requires complete byte pairs\n");
            return -1;
        }

        if (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } else {
            fprintf(stderr, "netacnv: invalid character in hex input at byte %zu\n",
                    (size_t)(p - (const unsigned char *)text) + 1);
            return -1;
        }

        if (high < 0) {
            high = digit;
        } else {
            if (*length == MAXPATHLEN) {
                fprintf(stderr, "netacnv: hex input exceeds maximum of %d bytes\n",
                        MAXPATHLEN);
                return -1;
            }

            memset(bytes + (*length)++, (high << 4) | digit, 1);
            high = -1;
        }
    }

    if (high >= 0) {
        fprintf(stderr, "netacnv: hex input requires complete byte pairs\n");
        return -1;
    }

    bytes[*length] = '\0';
    return 0;
}

/*!
 * @brief Print exact bytes and their length without interpreting the charset
 *
 * @param[in] label   label for the diagnostic line
 * @param[in] bytes   byte buffer
 * @param[in] length  byte count, excluding the terminator
 */
static void print_bytes(const char *label, const char *bytes, size_t length)
{
    fprintf(stderr, "%s (%zu bytes):", label, length);

    for (size_t i = 0; i < length; i++) {
        fprintf(stderr, " %02x", (unsigned char)bytes[i]);
    }

    fputc('\n', stderr);
}

/*!
 * @brief Print option and result flags returned by convert_charset
 *
 * @param[in] flags  conversion flags after the call
 */
static void print_flags(uint16_t flags)
{
    fprintf(stderr, "flags: 0x%04x", (unsigned int)flags);

    for (size_t i = 0; i < sizeof(flag_map) / sizeof(struct flag_map); i++) {
        if (flags & flag_map[i].flag) {
            fprintf(stderr, " %s", flag_map[i].flagname);
        }
    }

    if (flags & CONV_REQMANGLE) {
        fprintf(stderr, " CONV_REQMANGLE");
    }

    if (flags & CONV_REQESCAPE) {
        fprintf(stderr, " CONV_REQESCAPE");
    }

    fputc('\n', stderr);
}

/*!
 * @brief Convert a string between two charsets with convert_charset and print
 * both forms
 *
 * The -f and -t options name the source and the target charsets. Their
 * defaults are UTF8-MAC and UTF8. The -m option names the legacy Mac
 * charset, MAC_ROMAN by default. Each -o option adds a CONV_* flag by its
 * name; unknown names and conflicting case or composition options fail.
 * -x decodes hexadecimal byte pairs, allowing embedded NULs and wide input.
 * The input and output are bounded by MAXPATHLEN bytes. -v prints byte
 * lengths, hex bytes, and result flags to stderr; -h prints help.
 *
 * @param[in] argc  argument count
 * @param[in] argv  argument vector
 * @returns EXIT_SUCCESS, or EXIT_FAILURE for an argument, charset,
 *          conversion, or output error
 */
int main(int argc, char **argv)
{
    int opt;
    int conversion_errno;
    int verbose = 0;
    int hex_input = 0;
    uint16_t flags = 0;
    const char *string;
    const char *mac_name = MACCHARSET;
    const char *from_name = "UTF8-MAC";
    const char *to_name = "UTF8";
    char input_buffer[MAXPATHLEN + 1];
    char buffer[MAXPATHLEN + 2] = {0};
    size_t input_len;
    size_t output_len;
    charset_t from;
    charset_t to;
    charset_t mac;
    int ret = EXIT_FAILURE;

    while ((opt = getopt(argc, argv, "hvxf:m:o:t:")) != -1) {
        switch (opt) {
        case 'h':
            usage(stdout);
            return fflush(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

        case 'v':
            verbose = 1;
            break;

        case 'x':
            hex_input = 1;
            break;

        case 'f':
            from_name = optarg;
            break;

        case 'm':
            mac_name = optarg;
            break;

        case 'o': {
            size_t i;

            for (i = 0; i < sizeof(flag_map) / sizeof(struct flag_map); i++) {
                if (strcmp(flag_map[i].flagname, optarg) == 0) {
                    flags |= flag_map[i].flag;
                    break;
                }
            }

            if (i == sizeof(flag_map) / sizeof(struct flag_map)) {
                fprintf(stderr, "netacnv: unknown conversion option '%s'\n",
                        optarg);
                usage(stderr);
                return EXIT_FAILURE;
            }

            break;
        }

        case 't':
            to_name = optarg;
            break;

        case '?':
        default:
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 1) {
        fprintf(stderr, "netacnv: exactly one input string is required\n");
        usage(stderr);
        return EXIT_FAILURE;
    }

    if (((flags & CONV_TOUPPER) && (flags & CONV_TOLOWER))
            || ((flags & CONV_PRECOMPOSE) && (flags & CONV_DECOMPOSE))) {
        fprintf(stderr, "netacnv: conflicting conversion options\n");
        return EXIT_FAILURE;
    }

    string = argv[optind];

    if (hex_input) {
        if (decode_hex_input(string, input_buffer, &input_len) != 0) {
            return EXIT_FAILURE;
        }

        string = input_buffer;
    } else {
        input_len = strlen(string);
    }

    if (input_len > MAXPATHLEN) {
        fprintf(stderr, "netacnv: input is %zu bytes; maximum is %d\n",
                input_len, MAXPATHLEN);
        return EXIT_FAILURE;
    }

    if (validate_charset(from_name, "source") != 0
            || validate_charset(to_name, "target") != 0
            || validate_charset(mac_name, "legacy Mac") != 0) {
        return EXIT_FAILURE;
    }

    /* Keep fixed filename charset roles independent of the escape charset. */
    if (set_charset_name(CH_UNIX, "UTF8") != 0
            || set_charset_name(CH_MAC, MACCHARSET) != 0) {
        fprintf(stderr, "netacnv: cannot initialize charset names\n");
        goto cleanup;
    }

    from = add_charset(from_name);
    to = add_charset(to_name);
    mac = add_charset(mac_name);

    if (from == (charset_t) -1 || to == (charset_t) -1
            || mac == (charset_t) -1) {
        fprintf(stderr, "netacnv: cannot register conversion charsets\n");
        goto cleanup;
    }

    output_len = convert_charset(from, to, mac, string, input_len,
                                 buffer, sizeof(buffer), &flags);
    conversion_errno = errno;

    if (verbose) {
        fprintf(stderr, "from charset: %s\nto charset: %s\n"
                        "legacy Mac charset: %s\n",
                from_name, to_name, mac_name);
        print_bytes("from bytes", string, input_len);

        if (output_len != (size_t) -1) {
            print_bytes("to bytes", buffer, output_len);
        }

        print_flags(flags);
    }

    if (output_len == (size_t) -1) {
        fprintf(stderr, "netacnv: conversion from '%s' to '%s' failed: %s\n",
                from_name, to_name, conversion_errno
                ? strerror(conversion_errno) : "no error code supplied");
        goto cleanup;
    }

    /* Explicit lengths preserve wide encodings and embedded NULs on both sides. */
    if (fputs("from: ", stdout) == EOF
            || fwrite(string, 1, input_len, stdout) != input_len
            || fputs("\nto: ", stdout) == EOF
            || fwrite(buffer, 1, output_len, stdout) != output_len
            || fputc('\n', stdout) == EOF || fflush(stdout) == EOF) {
        perror("netacnv: output error");
        goto cleanup;
    }

    ret = EXIT_SUCCESS;
cleanup:
    free_charset_names();
    return ret;
}
