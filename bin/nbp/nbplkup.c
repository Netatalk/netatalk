/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/nbp.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MACCHARSET "MAC_ROMAN"

static char *Obj = "=";
static char *Type = "=";
static char *Zone = "*";

static void Usage(char *av0) {
    char *p = strrchr(av0, '/');

    if (p == NULL) {
        p = av0;
    } else {
        p++;
    }

    printf("Usage:\t%s [-A address] [-r responses] [-m Mac charset] [-s] [obj:type@zone]\n",
           p);
    exit(1);
}

int main(int ac, char **av) {
    struct nbpnve *nn;
    char *name;
    int i, c, nresp = 1000;
    struct at_addr addr;
    char *obj = NULL;
    char *type = NULL;
    size_t obj_len;
    size_t type_len;
    charset_t chMac = CH_MAC;
    char *convname;
    bool script_friendly_output = false;
    extern char *optarg;
    extern int optind;
    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, MACCHARSET);
    memset(&addr, 0, sizeof(addr));

    while ((c = getopt(ac, av, "sA:m:r:")) != EOF) {
        switch (c) {
        case 'A':
            if (!atalk_aton(optarg, &addr)) {
                fprintf(stderr, "Bad address.\n");
                exit(1);
            }

            break;

        case 'r' :
            nresp = atoi(optarg);
            break;

        case 'm':
            chMac = add_charset(optarg);

            if ((charset_t) -1 == chMac) {
                fprintf(stderr, "Invalid Mac charset.\n");
                exit(1);
            }

            break;

        case 's':
            script_friendly_output = true;
            break;

        default:
            Usage(av[0]);
            exit(1);
        }
    }

    nn = (struct nbpnve *)malloc(nresp * sizeof(struct nbpnve));

    if (nn == NULL) {
        perror("malloc");
        exit(1);
    }

    if (ac - optind > 1) {
        Usage(av[0]);
        exit(1);
    }

    /*
     * Get default values from the environment. We need to copy out
     * the results, here, since nbp_name returns it's parameters
     * in static space, and we'll clobber them when we call it again
     * later.
     */
    if ((name = getenv("NBPLKUP")) != NULL) {
        if (nbp_name(name, &Obj, &Type, &Zone)) {
            fprintf(stderr,
                    "Environment variable syntax error: NBPLKUP = %s\n",
                    name);
            exit(1);
        }

        Obj = strndup(Obj, 32);

        if (Obj == NULL) {
            perror("strndup(Obj)");
            exit(1);
        }

        Type = strndup(Type, 32);

        if (Type == NULL) {
            perror("strndup(Type)");
            exit(1);
        }

        Zone = strndup(Zone, 32);

        if (Type == NULL) {
            perror("strndup(Zone)");
            exit(1);
        }
    }

    if (ac - optind == 1) {
        if ((size_t)(-1) == convert_string_allocate(CH_UNIX, chMac,
                av[optind], -1, &convname)) {
            convname = av[optind];
        }

        if (nbp_name(convname, &Obj, &Type, &Zone)) {
            Usage(av[0]);
            exit(1);
        }
    }

    c = nbp_lookup(Obj, Type, Zone, nn, nresp, &addr);

    if (c < 0) {
        perror("nbp_lookup");
        exit(-1);
    }

    for (i = 0; i < c; i++) {
        obj_len = convert_string_allocate(chMac, CH_UNIX, nn[i].nn_obj,
                                          nn[i].nn_objlen, &obj);
        type_len = convert_string_allocate(chMac, CH_UNIX, nn[i].nn_type,
                                           nn[i].nn_typelen, &type);

        if ((size_t)(-1) == obj_len) {
            obj_len = nn[i].nn_objlen;
            obj = strdup(nn[i].nn_obj);

            if (obj == NULL) {
                perror("strdup");
                exit(1);
            }
        }

        if (script_friendly_output) {
            printf("%u.%u:%u %s:%s\n",
                   ntohs(nn[i].nn_sat.sat_addr.s_net),
                   nn[i].nn_sat.sat_addr.s_node,
                   nn[i].nn_sat.sat_port,
                   obj, type);
        } else {
            printf("%31.*s:%-34.*s %u.%u:%u\n",
                   (int)obj_len, obj,
                   (int)type_len, type,
                   ntohs(nn[i].nn_sat.sat_addr.s_net),
                   nn[i].nn_sat.sat_addr.s_node,
                   nn[i].nn_sat.sat_port);
        }

        free(obj);
        free(type);
    }

    free(nn);
    return 0;
}
