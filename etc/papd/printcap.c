/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <atalk/logger.h>
#include <atalk/compat.h>

#include "printcap.h"

#ifndef BUFSIZ
#define	BUFSIZ	1024
#endif /* ! BUFSIZ */
#define MAXHOP	32	/*!< max number of tc= indirections */

/*!
 * @file
 * @brief routines for dealing with the terminal capability data base based on termcap
 *
 * @bug		Should use a "last" pointer in tbuf, so that searching
 *		for capabilities alphabetically would not be a n**2/2
 *		process when large numbers of capabilities are given.
 * @note	If we add a last pointer now we will screw up the
 *		tc capability. We really should compile termcap.
 *
 * Essentially all the work here is scanning and decoding escapes
 * in string capabilities.  We don't use stdio because the editor
 * doesn't, and because living w/o it is not hard.
 */

#define PRINTCAP

#ifdef PRINTCAP
#define tgetent	pgetent
#define tskip	pskip
#define tgetstr	pgetstr
#define tdecode pdecode
#define tgetnum	pgetnum
#define	tgetflag pgetflag
#define tdecode pdecode
#define tnchktc	pnchktc
#define	tnamatch pnamatch
#define V6
#endif /* PRINTCAP */

static	FILE *pfp = NULL;	/*!< printcap data base file pointer */
static	char *tbuf;
static	int hopcount;		/*!< detect infinite loops in termcap, init 0 */

/*!
 * @brief Read the next entry of papd.conf or a printcap file into a buffer
 *
 * The first call opens the file, and the end of the file closes it again. A
 * backslash before a newline joins lines, and lines that start with `#` are
 * comments. The call drops spaces and tabs at the start of an entry. An entry
 * that fills the buffer ends there with a log line, and the call closes the
 * file.
 *
 * @param[in]  cap      file path
 * @param[out] bp       buffer that receives the entry, which tbuf then points
 *                      at
 * @param[in]  bufsize  size of bp
 * @returns 1 for an entry, 0 at the end of the file, or -1 when the file does
 *          not open
 */
int getprent(char *cap, char *bp, int bufsize)
{
    int c;
    int skip = 0;
    int i;

    if (pfp == NULL && (pfp = fopen(cap, "r")) == NULL) {
        return -1;
    }

    tbuf = bp;
    i = 0;

    for (;;) {
        switch (c = getc(pfp)) {
        case EOF:
            if (bp != tbuf) {
                *bp = '\0';
                return 1;
            }

            fclose(pfp);
            pfp = NULL;
            return 0;

        case '\n':
            if (bp == tbuf) {
                skip = 0;
                continue;
            }

            if (bp[-1] == '\\') {
                bp--;
                continue;
            }

            *bp = '\0';
            return 1;

        case '#':
            if (bp == tbuf) {
                skip++;
            }

        default:
            if (skip) {
                continue;
            }

            if (bp == tbuf && (c == ' ' || c == '\t')) {
                continue;
            }

            if (bp >= tbuf + bufsize) {
                LOG(log_error, logtype_papd, "Termcap entry too long");
                *bp = '\0';
                return 1;
            }

            *bp++ = c;

            if (++i >= bufsize) {
                LOG(log_error, logtype_papd, "config file too large");
                fclose(pfp);
                pfp = NULL;
                *bp = '\0';
                return 1;
            }
        }
    }
}

/*!
 * @brief Close the file that getprent reads
 */
void endprent(void)
{
    if (pfp != NULL) {
        fclose(pfp);
    }
}

/*!
 * @brief Find the entry with a given name in a printcap-style file
 *
 * The call reads the file entry by entry into bp, joining lines that end with
 * a backslash and skipping `#` comments, until pnamatch accepts name. The name
 * check of pnamatch and the capability readers use tbuf, which only getprent
 * sets. A tc= capability at the end of the entry pulls in the named entry
 * through pnchktc.
 *
 * @param[in]  cap   file path
 * @param[out] bp    buffer of BUFSIZ bytes that receives the entry
 * @param[in]  name  entry name to look for
 * @returns 1 for a match, 0 when the name is missing or a tc= reference fails,
 *          or -1 when the file does not open
 */
int tgetent(char *cap, char *bp, const char *name)
{
    char *cp;
    int c;
    int i = 0;
    int cnt = 0;
    char ibuf[BUFSIZ];
    int tf;
    int skip;
#ifndef V6
    cp = getenv("TERMCAP");

    /*
     * TERMCAP can have one of two things in it. It can be the
     * name of a file to use instead of /etc/termcap. In this
     * case it better start with a "/". Or it can be an entry to
     * use so we don't have to read the file. In this case it
     * has to already have the newlines crunched out.
     */
    if (cp && *cp) {
        if (*cp != '/') {
            cp2 = getenv("TERM");

            if (cp2 == (char *) 0 || strcmp(name, cp2) == 0) {
                strcpy(bp, cp);
                return tnchktc(cap);
            } else {
                tf = open(cap, 0);
            }
        } else {
            tf = open(cp, 0);
        }
    }

    if (tf == 0) {
        tf = open(cap, 0);
    }

#else /* V6 */
    tf = open(cap, 0);
#endif /* V6 */

    if (tf < 0) {
        return -1;
    }

    for (;;) {
        cp = bp;
        skip = 0;

        for (;;) {
            if (i == cnt) {
                cnt = read(tf, ibuf, BUFSIZ);

                if (cnt <= 0) {
                    close(tf);
                    return 0;
                }

                i = 0;
            }

            c = ibuf[i++];

            if (c == '\n') {
                if (!skip && cp > bp && cp[-1] == '\\') {
                    cp--;
                    continue;
                }

                skip = 0;

                if (cp == bp) {
                    continue;
                } else {
                    break;
                }
            }

            if (c == '#' && cp == bp) {
                skip++;
            }

            if (skip) {
                continue;
            }

            if (cp >= bp + BUFSIZ) {
                LOG(log_error, logtype_papd, "Termcap entry too long");
                break;
            } else {
                *cp++ = c;
            }
        }

        *cp = 0;

        /*
         * The real work for the match.
         */
        if (tnamatch(name)) {
            close(tf);
            return tnchktc(cap);
        }
    }
}

/*!
 * @brief Resolve a tc= capability that ends the entry in tbuf
 *
 * When the last field is tc=name, the call looks the named entry up with
 * pgetent. Its fields, without its names, then take the place of the tc=
 * field. A field of the entry comes before an included field of the same
 * name, since lookups scan from the left. More than MAXHOP references fail,
 * counted over the life of the process.
 *
 * @param[in] cap  file path for the lookup
 * @returns 1 when the entry is complete, or 0 for a bad entry, too many
 *          references, or a missing entry
 */
int tnchktc(char *cap)
{
    char *p;
    char *q;
    /* name of similar terminal */
    char tcname[16];
    char *tcbuf = calloc(1, BUFSIZ);

    if (tcbuf == NULL) {
        return 0;
    }

    char *holdtbuf = tbuf;
    int l;
    /* before the last colon */
    p = tbuf + strlen(tbuf) - 2;

    while (*--p != ':')
        if (p < tbuf) {
            LOG(log_error, logtype_papd, "Bad termcap entry");
            free(tcbuf);
            return 0;
        }

    p++;

    /* p now points to beginning of last field */
    if (p[0] != 't' || p[1] != 'c') {
        free(tcbuf);
        return 1;
    }

    strlcpy(tcname, p + 3, sizeof(tcname));
    q = tcname;

    while (q && *q != ':') {
        q++;
    }

    *q = 0;

    if (++hopcount > MAXHOP) {
        LOG(log_error, logtype_papd, "Infinite tc= loop");
        free(tcbuf);
        return 0;
    }

    if (tgetent(cap, tcbuf, tcname) != 1) {
        free(tcbuf);
        return 0;
    }

    for (q = tcbuf; q && *q != ':'; q++)
        ;

    l = p - holdtbuf + strlen(q);

    if (l > BUFSIZ) {
        LOG(log_error, logtype_papd, "Termcap entry too long");
        q[BUFSIZ - (p - tbuf)] = 0;
    }

    strcpy(p, q + 1);
    free(tcbuf);
    tbuf = holdtbuf;
    return 1;
}

/*!
 * @brief Check whether the entry in tbuf lists a name among its names
 *
 * The first field of an entry lists its names, separated by `|`, and the
 * first `:` ends the list. An entry that starts with `#` matches nothing.
 *
 * @param[in] np  name to look for
 * @returns 1 for a match, or 0
 */
int tnamatch(const char *np)
{
    const char *Np;
    const char *Bp;
    Bp = tbuf;

    if (*Bp == '#') {
        return 0;
    }

    for (;;) {
        for (Np = np; *Np && *Bp == *Np; Bp++, Np++) {
            continue;
        }

        if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0)) {
            return 1;
        }

        while (*Bp && *Bp != ':' && *Bp != '|') {
            Bp++;
        }

        if (*Bp == 0 || *Bp == ':') {
            return 0;
        }

        Bp++;
    }
}

/*!
 * @brief Skip to the next field of an entry
 *
 * The call moves past the next run of `:` and does not know about escaped
 * colons, so a value spells a colon as `\072`.
 *
 * @param[in] bp  position in the entry
 * @returns the start of the next field, or the end of the entry
 */
static char *tskip(char *bp)
{
    while (*bp && *bp != ':') {
        bp++;
    }

    while (*bp && *bp == ':') {
        bp++;
    }

    return bp;
}

/*!
 * @brief Return the value of a numeric capability of the entry in tbuf
 *
 * A numeric capability looks like `pc#200`, and a value with a leading 0 is
 * octal. A capability written as `pc@` counts as missing.
 *
 * @param[in] id  two-letter capability name
 * @returns the value, or -1 when the entry lacks the capability
 */
int tgetnum(char *id)
{
    int i;
    int base;
    char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);

        if (*bp == 0) {
            return -1;
        }

        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1]) {
            continue;
        }

        if (*bp == '@') {
            return -1;
        }

        if (*bp != '#') {
            continue;
        }

        bp++;
        base = 10;

        if (*bp == '0') {
            base = 8;
        }

        i = 0;

        while (isdigit(*bp)) {
            i *= base, i += *bp++ - '0';
        }

        return i;
    }
}

/*!
 * @brief Check a flag capability of the entry in tbuf
 *
 * A flag stands alone between colons, as in `:sp:`, and `sp@` turns it off.
 *
 * @param[in] id  two-letter capability name
 * @returns 1 when the entry has the flag, or 0
 */
int tgetflag(char *id)
{
    char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);

        if (!*bp) {
            return 0;
        }

        if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
            if (!*bp || *bp == ':') {
                return 1;
            } else if (*bp == '@') {
                return 0;
            }
        }
    }
}

/*!
 * @brief Decode the value of a string capability into an area and advance the
 * area
 *
 * The value runs to the next `:`. `^X` becomes a control character, and a
 * backslash introduces `\E`, `\^`, `\\`, `\:`, `\n`, `\r`, `\t`, `\b`, `\f`,
 * or up to three octal digits. The call does not check the area for room.
 *
 * @param[in]     str   start of the value
 * @param[in,out] area  output area, advanced past the decoded string
 * @returns the decoded string
 */
static char *
tdecode(char *str, char **area)
{
    char *cp;
    int c;
    const char *dp;
    int i;
    cp = *area;

    while ((c = *str++) && c != ':') {
        switch (c) {
        case '^':
            c = *str++ & 037;
            break;

        case '\\':
            dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
            c = *str++;
nextc:

            if (*dp++ == c) {
                c = *dp++;
                break;
            }

            dp++;

            if (*dp) {
                goto nextc;
            }

            if (isdigit(c)) {
                c -= '0', i = 2;

                do {
                    c <<= 3, c |= *str++ - '0';
                } while (--i && isdigit(*str));
            }

            break;
        }

        *cp++ = c;
    }

    *cp++ = 0;
    str = *area;
    *area = cp;
    return str;
}

/*!
 * @brief Return the value of a string capability of the entry in tbuf
 *
 * A string capability looks like `pr=lp`, and pdecode turns its escapes into
 * bytes in area. A capability written as `pr@` counts as missing. The call
 * does not check the area for room.
 *
 * @param[in]     id    two-letter capability name
 * @param[in,out] area  output area, advanced past the value
 * @returns the decoded value, or NULL when the entry lacks the capability
 */
char *
tgetstr(char *id, char **area)
{
    char *bp = tbuf;

    for (;;) {
        bp = tskip(bp);

        if (!*bp) {
            return NULL;
        }

        if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1]) {
            continue;
        }

        if (*bp == '@') {
            return NULL;
        }

        if (*bp != '=') {
            continue;
        }

        bp++;
        return tdecode(bp, area);
    }
}

/*!
 * @brief Decode the first name of an entry into an area and advance the area
 *
 * The name runs to the first `:` or `|`, takes at most bufsize - 1 bytes, and
 * uses the escapes of pdecode.
 *
 * @param[in]     str      start of the entry
 * @param[in,out] area     output area, advanced past the decoded name
 * @param[in]     bufsize  room in the area
 * @returns the decoded name
 */
static char *
decodename(char *str, char **area, int bufsize)
{
    char *cp;
    int c;
    const char *dp;
    int i;
    cp = *area;

    while ((c = *str++) && --bufsize && c != ':' && c != '|') {
        switch (c) {
        case '^':
            c = *str++ & 037;
            break;

        case '\\':
            dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
            c = *str++;
nextc:

            if (*dp++ == c) {
                c = *dp++;
                break;
            }

            dp++;

            if (*dp) {
                goto nextc;
            }

            if (isdigit(c)) {
                c -= '0', i = 2;

                do {
                    c <<= 3, c |= *str++ - '0';
                } while (--i && isdigit(*str));
            }

            break;
        }

        *cp++ = c;
    }

    *cp++ = 0;
    str = *area;
    *area = cp;
    return str;
}

/*!
 * @brief Return the first name of the entry in tbuf, the printer name of a
 * papd.conf entry
 *
 * @param[in,out] area     output area, advanced past the name
 * @param[in]     bufsize  room in the area
 * @returns the decoded name
 */
char *
getpname(char **area, int bufsize)
{
    return decodename(tbuf, area, bufsize);
}
