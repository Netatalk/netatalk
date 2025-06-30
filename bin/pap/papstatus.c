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
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <netatalk/at.h>
#include <errno.h>
#include <atalk/atp.h>
#include <atalk/pap.h>
#include <atalk/nbp.h>
#include <atalk/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _PATH_PAPRC	".paprc"

#define IMAGEWRITER "ImageWriter"
#define IMAGEWRITER_LQ "LQ"

#define COLOR_RIBBON_INSTALLED 0b10000000
#define SHEET_FEEDER_INSTALLED 0b01000000
#define PAPER_OUT_ERROR        0b00100000
#define COVER_OPEN_ERROR       0b00010000
#define PRINTER_OFF_LINE       0b00001000
#define PAPER_JAM_ERROR        0b00000100
#define PRINTER_FAULT          0b00000010
#define PRINTER_ACTIVE         0b00000001

/* Forward Declaration */
static void getstatus(ATP atp, struct sockaddr_at *sat, int is_imagewriter);

static void usage(char *path)
{
    char	*p;

    if ((p = strrchr(path, '/')) == NULL) {
        p = path;
    } else {
        p++;
    }

    fprintf(stderr,
            "Usage:\t%s [ -A address ] [ -p printername ]\n", p);
    exit(1);
}

static char *
paprc(void)
{
    static char	s[32 + 1 + 32 + 1 + 32];
    char	*name = NULL;
    FILE	*f;

    if ((f = fopen(_PATH_PAPRC, "r")) == NULL) {
        return NULL;
    }

    while (fgets(s, sizeof(s), f) != NULL) {
        s[strlen(s) - 1] = '\0';	/* remove trailing newline */

        if (*s == '#') {
            continue;
        }

        name = s;
        break;
    }

    fclose(f);
    return name;
}

static char			*printer = NULL;

static char			cbuf[8];
static struct nbpnve		nn;

static void print_status(char status, char mask, char *message)
{
    printf("%s", message);

    if (status & mask) {
        printf("True\n");
    } else {
        printf("False\n");
    }
}

int main(int ac, char **av)
{
    ATP			atp;
    int			wait, c, err = 0;
    char		*obj = NULL, *type = "LaserWriter", *zone = "*";
    struct at_addr      addr;
    extern char		*optarg;
    extern int		optind;
    int     is_imagewriter;
    memset(&addr, 0, sizeof(addr));

    while ((c = getopt(ac, av, "p:s:A:")) != EOF) {
        switch (c) {
        case 'A':
            if (!atalk_aton(optarg, &addr)) {
                fprintf(stderr, "Bad address.\n");
                exit(1);
            }

            break;

        case 'p' :
            printer = optarg;
            break;

        default :
            fprintf(stderr, "Unknown option: '%c'\n", c);
            err++;
        }
    }

    if (err) {
        usage(*av);
    }

    if (printer == NULL && ((printer = paprc()) == NULL)) {
        usage(*av);
    }

    /*
     * Open connection.
     */
    if (nbp_name(printer, &obj, &type, &zone) < 0) {
        fprintf(stderr, "%s: Bad name\n", printer);
        exit(1);
    }

    if (obj == NULL) {
        fprintf(stderr, "%s: Bad name\n", printer);
        exit(1);
    }

    if (nbp_lookup(obj, type, zone, &nn, 1, &addr) <= 0) {
        if (errno != 0) {
            perror("nbp_lookup");
        } else {
            fprintf(stderr, "%s:%s@%s: NBP Lookup failed\n", obj, type, zone);
        }

        exit(1);
    }

    if ((atp = atp_open(ATADDR_ANYPORT, &addr)) == NULL) {
        perror("atp_open");
        exit(1);
    }

    is_imagewriter = (0 == strcmp(IMAGEWRITER, type) ||
                      0 == strcmp(IMAGEWRITER_LQ, type));

    if (optind == ac) {
        getstatus(atp, &nn.nn_sat, is_imagewriter);
        exit(0);
    }

    if (optind - ac > 1) {
        usage(*av);
    }

    wait = atoi(av[optind]);

    for (;;) {
        getstatus(atp, &nn.nn_sat, is_imagewriter);
        sleep(wait);
    }

    return 0;
}

static void getstatus(ATP atp, struct sockaddr_at *sat, int is_imagewriter)
{
    struct iovec	iov;
    struct atp_block	atpb;
    char		rbuf[ATP_MAXDATA];
    cbuf[0] = 0;
    cbuf[1] = PAP_SENDSTATUS;
    cbuf[2] = cbuf[3] = 0;
    atpb.atp_saddr = sat;
    atpb.atp_sreqdata = cbuf;
    atpb.atp_sreqdlen = 4;		/* bytes in SendStatus request */
    atpb.atp_sreqto = 2;		/* retry timer */
    atpb.atp_sreqtries = 5;		/* retry count */

    if (atp_sreq(atp, &atpb, 1, ATP_XO) < 0) {
        perror("atp_sreq");
        exit(1);
    }

    iov.iov_base = rbuf;
    iov.iov_len = sizeof(rbuf);
    atpb.atp_rresiov = &iov;
    atpb.atp_rresiovcnt = 1;

    if (atp_rresp(atp, &atpb) < 0) {
        perror("atp_rresp");
        exit(1);
    }

    /* sanity */
    if (iov.iov_len < 8 ||
            rbuf[1] != PAP_STATUS) {
        fprintf(stderr, "Bad response!\n");
        return;	/* This is weird, since TIDs must match... */
    }

    if (is_imagewriter) {
        char status = rbuf[9];
        print_status(status, COLOR_RIBBON_INSTALLED, "Color Ribbon Installed: ");
        print_status(status, SHEET_FEEDER_INSTALLED, "Sheet Feeder Installed: ");
        print_status(status, PAPER_OUT_ERROR,        "Paper Out Error:        ");
        print_status(status, COVER_OPEN_ERROR,       "Cover Opened:           ");
        print_status(status, PRINTER_OFF_LINE,       "Printer Off-Line:       ");
        print_status(status, PAPER_JAM_ERROR,        "Paper Jammed:           ");
        print_status(status, PRINTER_FAULT,          "Printer in Fault:       ");
        print_status(status, PRINTER_ACTIVE,         "Printer Active:         ");
    }

    printf("%.*s\n", (int)iov.iov_len - 9, (char *) iov.iov_base + 9);
}
