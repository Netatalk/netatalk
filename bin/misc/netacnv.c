#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/unicode.h>

#define MACCHARSET "MAC_ROMAN"

#define flag(x) {x, #x}

struct flag_map {
    int flag;
    const char *flagname;
};

struct flag_map flag_map[] = {
    flag(CONV_ESCAPEHEX),
    flag(CONV_UNESCAPEHEX),
    flag(CONV_ESCAPEDOTS),
    flag(CONV_IGNORE),
    flag(CONV_FORCE),
    flag(CONV__EILSEQ),
    flag(CONV_TOUPPER),
    flag(CONV_TOLOWER),
    flag(CONV_PRECOMPOSE),
    flag(CONV_DECOMPOSE)
};

char buffer[MAXPATHLEN +2];

static void usage(void)
{
    printf("Usage: netacnv [-o <conversion option> [...]] [-f <from charset>] [-t <to charset>] [-m legacy Mac charset] <string>\n");
    printf("Defaults: -f: UTF8-MAC, -t: UTF8, -m MAC_ROMAN\n");
    printf("Available conversion options:\n");
    for (int i = 0; i < (sizeof(flag_map)/sizeof(struct flag_map) - 1); i++) {
        printf("%s\n", flag_map[i].flagname);
    }
}

int main(int argc, char **argv)
{
    int opt;
    uint16_t flags = 0;
    char *string;
    char *macName = strdup(MACCHARSET);
    char *f = NULL;
    char *t = NULL;
    charset_t from;
    charset_t to;
    charset_t mac;
    int ret = 0;

    while ((opt = getopt(argc, argv, "m:o:f:t:")) != -1) {
        switch(opt) {
        case 'm':
            if (macName) free((void *)macName);
            macName = strdup(optarg);
            break;
        case 'o':
            for (int i = 0; i < sizeof(flag_map)/sizeof(struct flag_map) - 1; i++)
                if ((strcmp(flag_map[i].flagname, optarg)) == 0)
                    flags |= flag_map[i].flag;
            break;
        case 'f':
            if (f) free((void *)f);
            f = strdup(optarg);
            break;
        case 't':
            if (t) free((void *)t);
            t = strdup(optarg);
            break;
        case '?':
        default:
            usage();
            ret = 1;
            goto cleanup;
        }
    }

    if ((optind + 1) != argc) {
        usage();
        ret = 1;
        goto cleanup;
    }
    string = argv[optind];

    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, macName);

    if ( (charset_t) -1 == (from = add_charset(f ? f : "UTF8-MAC")) ) {
        fprintf( stderr, "Setting codepage %s as from codepage failed\n", f ? f : "UTF8-MAC");
        ret = -1;
        goto cleanup;
    }

    if ( (charset_t) -1 == (to = add_charset(t ? t : "UTF8")) ) {
        fprintf( stderr, "Setting codepage %s as to codepage failed\n", t ? t : "UTF8");
        ret = -1;
        goto cleanup;
    }

    if ( (charset_t) -1 == (mac = add_charset(macName)) ) {
        fprintf( stderr, "Setting codepage %s as Mac codepage failed\n", MACCHARSET);
        ret = -1;
        goto cleanup;
    }

    if ((size_t)-1 == (convert_charset(from, to, mac,
                                       string, strlen(string),
                                       buffer, MAXPATHLEN,
                                       &flags)) ) {
        perror("Conversion error");
        ret = 1;
        goto cleanup;
    }

    printf("from: %s\nto: %s\n", string, buffer);

cleanup:

    if (f) free((void *)f);
    if (t) free((void *)t);
    free((void *)macName);

    return ret;
}
