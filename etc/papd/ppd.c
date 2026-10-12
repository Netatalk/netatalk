/*
 * Copyright (c) 1995 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief PPD file reading for the printer of a papd job
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netatalk/at.h>
#include <atalk/atp.h>

#include "printer.h"
#include "ppd.h"

struct ppd_font		*ppd_fonts = NULL;

struct ppd_feature	ppd_features[] = {
    { "*LanguageLevel",	NULL },
    { "*PSVersion",	NULL },
#ifdef HAVE_CUPS
    { "*FreeVM",	"33554432" },
#else
    { "*FreeVM",	NULL },
#endif
    { "*Product",	NULL },
    { "*PCFileName",	NULL },
    { "*ModelName",	NULL },
    { "*NickName",	NULL },
    { "*ColorDevice",	NULL },
    { "*FaxSupport",	NULL },
    { "*TTRasterizer",	NULL },
    { "*Resolution",    NULL },
    { NULL, NULL },
};

struct ppdent {
    char	*pe_main;
    char	*pe_option;
    char	*pe_translation;
    char	*pe_value;
};

#ifndef SHOWPPD
static int ppd_inited;

/*!
 * @brief Read the PPD file of the printer of this process, once
 */
static void ppd_init(void)
{
    if (ppd_inited) {
        return;
    }

    ppd_inited++;

    if (printer->p_ppdfile) {
        read_ppd(printer->p_ppdfile, 0);
    }
}
#endif /* SHOWPPD */


/*!
 * @brief Read a line that ends with CR or LF, for PPD files with Mac line ends
 *
 * A CR line end becomes LF. A line longer than the buffer spans calls, and
 * only the call that reaches its end returns a line end.
 *
 * @param[out] buf      line buffer, NUL-terminated on return
 * @param[in]  bufsize  size of buf
 * @param[in]  stream   PPD file
 * @returns buf, or NULL at the end of the file
 */
static char *my_fgets(char *buf, size_t bufsize, FILE *stream)
{
    int p = 0;
    size_t count = 0;

    while (count < (bufsize - 1) && EOF != (p = fgetc(stream))) {
        buf[count] = p;
        count++;

        if (p == '\r' || p == '\n') {
            break;
        }
    }

    if (p == EOF && count == 0) {
        return NULL;
    }

    /* translate line endings */
    if (buf[count - 1] == '\r') {
        buf[count - 1] = '\n';
    }

    buf[count] = 0;
    return buf;
}

/*!
 * @brief Read the next main keyword entry of a PPD file
 *
 * An entry has the form `*Main Option/Translation: Value`, and the option,
 * translation, and value are optional. The call skips comments, queries,
 * lines that do not start with `*`, and lines over 1023 bytes.
 *
 * @param[in] stream  PPD file
 * @returns the entry in a static buffer that the next call overwrites, or NULL
 *          at the end of the file
 */
static struct ppdent *getppdent(FILE *stream)
{
    static char buf[1024];
    static struct ppdent ppdent;
    char *p;
    char *q;
    ppdent.pe_main = NULL;
    ppdent.pe_option = NULL;
    ppdent.pe_translation = NULL;
    ppdent.pe_value = NULL;

    while ((p = my_fgets(buf, sizeof(buf), stream)) != NULL) {
        /* main key word */
        if (*p != '*') {
            continue;
        }

        if (p[strlen(p) - 1] != '\n') {
            LOG(log_error, logtype_papd, "getppdent: line too long");
            continue;
        }

        q = p;

        while ((*p != ' ') && (*p != '\t') && (*p != ':') && (*p != '\n')) {
            p++;
        }

        /* comments & queries */
        if ((*(q + 1) == '%') || (*(q + 1) == '?')) {
            continue;
        }

        ppdent.pe_main = q;

        if (*p == '\n') {
            *p = '\0';
            ppdent.pe_option = ppdent.pe_translation = ppdent.pe_value = NULL;
            return &ppdent;
        }

        /* option key word */
        if (*p != ':') {
            *p++ = '\0';

            while ((*p == ' ') || (*p == '\t')) {
                p++;
            }

            q = p;

            while ((*p != ':') && (*p != '/') && (*p != '\n')) {
                p++;
            }

            if (*p == '\n') {
                continue;
            }

            ppdent.pe_option = q;

            /* translation string */
            if (*p == '/') {
                *p++ = '\0';
                q = p;

                while (*p != ':' && *p != '\n') {
                    p++;
                }

                if (*p != ':') {
                    continue;
                }

                ppdent.pe_translation = q;
            } else {
                ppdent.pe_translation = NULL;
            }
        }

        *p++ = '\0';

        while ((*p == ' ') || (*p == '\t')) {
            p++;
        }

        /* value */
        q = p;

        while (*p != '\n') {
            p++;
        }

        *p = '\0';
        ppdent.pe_value = q;
        return &ppdent;
    }

    return NULL;
}

/*!
 * @brief Read the fonts and the known features of a PPD file
 *
 * Each `*Font` entry adds its name to ppd_fonts once, and an entry for a
 * feature of the ppd_features table stores its value. An `*Include` entry
 * reads the named file, up to 20 levels deep.
 *
 * @param[in] file  PPD file path
 * @param[in] fcnt  include depth, 0 for the top file
 * @returns 0, or -1 when the file does not open or the includes go too deep
 */
int read_ppd(char *file, int fcnt)
{
    FILE		*ppdfile;
    struct ppdent	*pe;
    struct ppd_feature	*pfe;
    struct ppd_font	*pfo;

    if (fcnt > 20) {
        LOG(log_error, logtype_papd, "read_ppd: %s: Too many files!", file);
        return -1;
    }

    if ((ppdfile = fopen(file, "r")) == NULL) {
        LOG(log_error, logtype_papd, "read_ppd %s: %s", file, strerror(errno));
        return -1;
    }

    while ((pe = getppdent(ppdfile)) != NULL) {
        /* *Include files */
        if (strcmp(pe->pe_main, "*Include") == 0) {
            read_ppd(pe->pe_value, fcnt + 1);
            continue;
        }

        /* *Font */
        if (strcmp(pe->pe_main, "*Font") == 0 && pe->pe_option) {
            for (pfo = ppd_fonts; pfo; pfo = pfo->pd_next) {
                if (strcmp(pfo->pd_font, pe->pe_option) == 0) {
                    break;
                }
            }

            if (pfo) {
                continue;
            }

            if ((pfo = (struct ppd_font *)malloc(sizeof(struct ppd_font)))
                    == NULL) {
                LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
                exit(1);
            }

            if ((pfo->pd_font =
                        (char *)malloc(strlen(pe->pe_option) + 1)) == NULL) {
                LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
                exit(1);
            }

            strcpy(pfo->pd_font, pe->pe_option);
            pfo->pd_next = ppd_fonts;
            ppd_fonts = pfo;
            continue;
        }

        /* Features */
        for (pfe = ppd_features; pfe->pd_name; pfe++) {
            if (strcmp(pe->pe_main, pfe->pd_name) == 0) {
                break;
            }
        }

        if (pfe->pd_name && pe->pe_value) {
            if ((pfe->pd_value =
                        (char *)malloc(strlen(pe->pe_value) + 1)) == NULL) {
                LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
                exit(1);
            }

            strcpy(pfe->pd_value, pe->pe_value);
            continue;
        }
    }

    fclose(ppdfile);
    return 0;
}

/*!
 * @brief Look up a font of the PPD file by name
 *
 * The first lookup reads the PPD file.
 *
 * @param[in] font  font name
 * @returns the font entry, or NULL for a font the PPD file does not list
 */
struct ppd_font *ppd_font(char *font)
{
    struct ppd_font	*pfo;
#ifndef SHOWPPD

    if (! ppd_inited) {
        ppd_init();
    }

#endif /* SHOWPPD */

    for (pfo = ppd_fonts; pfo; pfo = pfo->pd_next) {
        if (strcmp(pfo->pd_font, font) == 0) {
            return pfo;
        }
    }

    return NULL;
}

/*!
 * @brief Look up a feature of the PPD file by its main keyword
 *
 * The keyword runs up to the CR or LF that has to follow within len + 1
 * bytes, and len cannot exceed 255. Only a feature with a value matches. The
 * first lookup reads the PPD file.
 *
 * @param[in] feature  keyword such as `*PSVersion`, followed by a line end
 * @param[in] len      length of the keyword
 * @returns the feature, or NULL for an unknown keyword or one without a value
 */
struct ppd_feature *ppd_feature(const char *feature, int len)
{
    struct ppd_feature	*pfe;
    char		ppd_feature_main[256];
    const char		*end, *p;
    char 		*q;
#ifndef SHOWPPD

    if (! ppd_inited) {
        ppd_init();
    }

#endif /* SHOWPPD */

    if (len > sizeof(ppd_feature_main) - 1) {
        return NULL;
    }

    for (end = feature + len, p = feature, q = ppd_feature_main;
            (p <= end) && (*p != '\n') && (*p != '\r'); p++, q++) {
        *q = *p;
    }

    if (p > end) {
        return NULL;
    }

    *q = '\0';

    for (pfe = ppd_features; pfe->pd_name; pfe++) {
        if ((strcmp(pfe->pd_name, ppd_feature_main) == 0) && pfe->pd_value) {
            return pfe;
        }
    }

    return NULL;
}
