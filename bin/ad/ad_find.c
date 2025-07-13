/*
   Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/bstrlib.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/unicode.h>
#include <atalk/util.h>

#include "ad.h"

static volatile sig_atomic_t alarmed;

/*
  SIGNAL handling:
  catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.
*/

static void sig_handler(int signo _U_)
{
    alarmed = 1;
    return;
}

static void set_signal(void)
{
    struct sigaction sv;
    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGTERM, &sv, NULL) < 0) {
        ERROR("error in sigaction(SIGTERM): %s", strerror(errno));
    }

    if (sigaction(SIGINT, &sv, NULL) < 0) {
        ERROR("error in sigaction(SIGINT): %s", strerror(errno));
    }

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0) {
        ERROR("error in sigaction(SIGABRT): %s", strerror(errno));
    }

    if (sigaction(SIGHUP, &sv, NULL) < 0) {
        ERROR("error in sigaction(SIGHUP): %s", strerror(errno));
    }

    if (sigaction(SIGQUIT, &sv, NULL) < 0) {
        ERROR("error in sigaction(SIGQUIT): %s", strerror(errno));
    }
}

static void usage_find(void)
{
    printf(
        "Usage: ad find [-v VOLUME_PATH] NAME\n"
    );
}

/*!
 * @brief Push a bstring to the end of a list
 */
int bstr_list_push(struct bstrList *sl, bstring bs)
{
    if (sl->qty == sl->mlen && bstrListAlloc(sl, sl->qty + 1) != BSTR_OK) {
        return BSTR_ERR;
    }

    sl->entry[sl->qty] = bs;
    sl->qty++;
    return BSTR_OK;
}

/*!
 * @brief Create an empty list with preallocated storage for at least 'min' members
 */
struct bstrList *bstr_list_create_min(int min)
{
    struct bstrList *sl = NULL;

    if ((sl = bstrListCreate()) == NULL) {
        return NULL;
    }

    if ((bstrListAlloc(sl, min)) != BSTR_OK) {
        bstrListDestroy(sl);
        return NULL;
    }

    return sl;
}

/*!
 * @brief Inverse bjoin
 */
bstring bjoin_inv(const struct bstrList *bl, const bstring sep)
{
    bstring b;
    int i, j, c, v;

    if (bl == NULL || bl->qty < 0) {
        return NULL;
    }

    if (sep != NULL && (sep->slen < 0 || sep->data == NULL)) {
        return NULL;
    }

    for (i = 0, c = 1; i < bl->qty; i++) {
        v = bl->entry[i]->slen;

        if (v < 0) {
            /* Invalid input */
            return NULL;
        }

        c += v;

        if (c < 0) {
            /* Wrap around ?? */
            return NULL;
        }
    }

    if (sep != NULL) {
        c += (bl->qty - 1) * sep->slen;
    }

    b = (bstring) malloc(sizeof(struct tagbstring));

    if (NULL == b) {
        return NULL;    /* Out of memory */
    }

    b->data = (unsigned char *) malloc(c);

    if (b->data == NULL) {
        free(b);
        return NULL;
    }

    b->mlen = c;
    b->slen = c - 1;

    for (i = bl->qty - 1, c = 0, j = 0; i >= 0; i--, j++) {
        if (j > 0 && sep != NULL) {
            memcpy(b->data + c, sep->data, sep->slen);
            c += sep->slen;
        }

        v = bl->entry[i]->slen;
        memcpy(b->data + c, bl->entry[i]->data, v);
        c += v;
    }

    b->data[c] = (unsigned char) '\0';
    return b;
}

int ad_find(int argc, char **argv, AFPObj *obj)
{
    int c, ret;
    afpvol_t vol;
    char *srchvol = strdup(getcwdpath());

    while ((c = getopt(argc - 1, &argv[1], ":v:")) != -1) {
        switch (c) {
        case 'v':
            free((void *)srchvol);
            srchvol = strdup(optarg);
            break;

        case ':':
        case '?':
            usage_find();
            free((void *)srchvol);
            exit(1);
            break;
        }
    }

    optind++;

    if ((argc - optind) != 1) {
        usage_find();
        free((void *)srchvol);
        exit(1);
    }

    set_signal();
    cnid_init();

    if (openvol(obj, srchvol, &vol) != 0) {
        ERROR("Can't open volume \"%s\"", srchvol);
    }

    free((void *)srchvol);
    uint16_t flags = CONV_TOLOWER;
    char namebuf[MAXPATHLEN + 1];

    if (convert_charset(CH_UNIX,
                        CH_UNIX,
                        vol.vol->v_maccharset,
                        argv[optind],
                        strlen(argv[optind]),
                        namebuf,
                        MAXPATHLEN,
                        &flags) == (size_t) -1) {
        ERROR("conversion error");
    }

    int count;
    char resbuf[DBD_MAX_SRCH_RSLTS * sizeof(cnid_t)];

    if ((count = cnid_find(vol.vol->v_cdb,
                           namebuf,
                           strlen(namebuf),
                           resbuf,
                           sizeof(resbuf))) < 1) {
        ret = 1;
    } else {
        ret = 0;
        cnid_t cnid;
        char *bufp = resbuf;
        bstring sep = bfromcstr("/");

        while (count--) {
            memcpy(&cnid, bufp, sizeof(cnid_t));
            bufp += sizeof(cnid_t);
            bstring path = NULL;
            bstring volpath = bfromcstr(vol.vol->v_path);
            BSTRING_STRIP_SLASH(volpath);
            char buffer[12 + MAXPATHLEN + 1];
            int buflen = 12 + MAXPATHLEN + 1;
            char *name;
            cnid_t did = cnid;
            struct bstrList *pathlist = bstr_list_create_min(32);

            while (did != DIRDID_ROOT) {
                if ((name = cnid_resolve(vol.vol->v_cdb, &did, buffer, buflen)) == NULL) {
                    goto next;
                }

                if (bstr_list_push(pathlist, bfromcstr(name)) < 0) {
                    ERROR("bstr_list_push failed for \"%s\"", name);
                    bstrListDestroy(pathlist);
                    bdestroy(volpath);
                    return 1;
                }
            }

            if (bstr_list_push(pathlist, volpath) < 0) {
                ERROR("bstr_list_push failed for volume path");
                bstrListDestroy(pathlist);
                bdestroy(volpath);
                return 1;
            }

            path = bjoin_inv(pathlist, sep);
            printf("%s\n", cfrombstr(path));
next:
            bstrListDestroy(pathlist);
            bdestroy(path);
        }

        bdestroy(sep);
    }

    closevol(&vol);
    return ret;
}
