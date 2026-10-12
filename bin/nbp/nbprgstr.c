/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief The nbprgstr command: register an NBP name with atalkd
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

#include <netatalk/at.h>
#include <atalk/netddp.h>
#include <atalk/nbp.h>
#include <atalk/util.h>
#include <atalk/unicode.h>

#define MACCHARSET "MAC_ROMAN"

/*!
 * @brief Print the usage of nbprgstr to stderr and exit with 1
 *
 * @param[in] av0  program path, whose base name the usage line shows
 */
static void Usage(char *av0)
{
    char	*p;

    if ((p = strrchr(av0, '/')) == NULL) {
        p = av0;
    } else {
        p++;
    }

    fprintf(stderr,
            "Usage: %s [ -A address ] [-m Mac charset] [ -p port] obj:type@zone\n", p);
    exit(1);
}

/*!
 * @brief Register an NBP name for a DDP socket with the local atalkd
 *
 * The tool opens a DDP socket to learn the node address, from the -A address
 * when given. The name registers for that socket, or for the -p port, and the
 * socket closes before the tool exits. The -m option names the Mac charset
 * of the name, which needs an object and a type.
 *
 * @param[in] ac  argument count
 * @param[in] av  argument vector
 * @returns 0, or -1 for a socket error, while a failed registration exits with
 *          1
 */
int main(int ac, char **av)
{
    struct sockaddr_at	addr;
    struct at_addr      ataddr;
    char		*Obj = NULL, *Type = NULL, *Zone = NULL;
    char		*convname = NULL;
    int			s, c, port = 0;
    charset_t		chMac = CH_MAC;
    extern char		*optarg;
    extern int		optind;
    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, MACCHARSET);
    memset(&ataddr, 0, sizeof(ataddr));

    while ((c = getopt(ac, av, "p:A:m:")) != EOF) {
        switch (c) {
        case 'A':
            if (!atalk_aton(optarg, &ataddr)) {
                fprintf(stderr, "Bad address.\n");
                exit(1);
            }

            break;

        case 'm':
            if ((charset_t) -1 == (chMac = add_charset(optarg))) {
                fprintf(stderr, "Invalid Mac charset.\n");
                exit(1);
            }

            break;

        case 'p' :
            port = atoi(optarg);
            break;

        default :
            Usage(av[0]);
        }
    }

    if (ac - optind != 1) {
        Usage(av[0]);
    }

    /* Convert the name */
    if ((size_t)(-1) == convert_string_allocate(CH_UNIX, chMac,
                                                av[optind], -1, &convname)) {
        convname = av[optind];
    }

    /*
     * Get the name. If Type or Obj aren't specified, error.
     */
    if (nbp_name(convname, &Obj, &Type, &Zone) || !Obj || !Type) {
        Usage(av[0]);
    }

    memset(&addr, 0, sizeof(addr));
    memcpy(&addr.sat_addr, &ataddr, sizeof(addr.sat_addr));

    if ((s = netddp_open(&addr, NULL)) < 0) {
        return -1;
    }

    if (port) {
        addr.sat_port = port;
    }

    if (nbp_rgstr(&addr, Obj, Type, Zone) < 0) {
        perror("nbp_rgstr");
        fprintf(stderr, "Can't register %s:%s@%s\n", Obj, Type,
                Zone ? Zone : "*");
        exit(1);
    }

    netddp_close(s);
    return 0;
}
