/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/logger.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <netatalk/at.h>

#include "file.h"
#include "comment.h"
#include "lp.h"

static int state = 0;

static volatile sig_atomic_t stop_requested = 0;

/*!
 * @brief Note SIGINT or SIGTERM, so the job loop ends the job with a shutdown
 * message
 *
 * @param[in] signo  signal number, unused
 */
static void sig_handler(int signo)
{
    (void)signo;
    stop_requested = 1;
}

/*!
 * @brief Refuse a job whose DSC comments do not parse
 *
 * The client gets a "Comments error, Ignoring job." error, the output ends,
 * and the spool file closes.
 *
 * @param[in,out] outfile  reply buffer to the client
 */
static void parser_error(struct papfile *outfile)
{
    spoolerror(outfile, "Comments error, Ignoring job.");
    outfile->pf_state |= PF_EOF;
    lp_close();
}

/*!
 * @brief Run the PostScript job loop over the data a PAP client sent
 *
 * The first call routes SIGINT and SIGTERM to sig_handler, after which the
 * loop answers "Server shutting down." and ends the job. At the start of a
 * job the loop drops lines until a `%!PS-Adobe-` magic line pushes its
 * handler. After that, the handler at the top of the comment stack takes each
 * line, and lines outside any comment go to lp_write.
 *
 * PF_STW opens the spool file, and the client gets "Ignoring job." once when
 * lp_open fails.
 *
 * @param[in,out] infile   data from the client
 * @param[in,out] outfile  reply data for the client
 * @param[in]     sat      client address
 * @returns CH_MORE when a handler needs more data, CH_ERROR on shutdown, or 0
 *          otherwise
 */
int ps(struct papfile *infile, struct papfile *outfile, struct sockaddr_at *sat)
{
    char			*start;
    int				linelength, crlflength;
    struct papd_comment		*comment;
    struct sigaction sa;
    static int inited = 0;

    if (!inited) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig_handler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        inited = 1;
    }

    for (;;) {
        if (stop_requested) {
            spoolerror(outfile, "Server shutting down.");
            outfile->pf_state |= PF_EOF;
            lp_close();
            return CH_ERROR;
        }

        if (infile->pf_state & PF_STW) {
            infile->pf_state &= ~PF_STW;

            /* set up spool file */
            if (lp_open(outfile, sat) < 0 && !state) {
                LOG(log_error, logtype_papd, "lp_open failed");
                spoolerror(outfile, "Ignoring job.");
            }

            state = 1;
        }

        if (infile->pf_state & PF_QUERY) {
            infile->pf_state |= PF_BOT;
        }

        if ((comment = compeek())) {
            switch ((*comment->c_handler)(infile, outfile, sat)) {
            case CH_DONE :
                continue;

            case CH_MORE :
                return CH_MORE;

            case CH_ERROR :
                parser_error(outfile);
                return 0;

            default :
                return CH_ERROR;
            }
        } else {
            switch (markline(infile, &start, &linelength, &crlflength)) {
            case 0 :
                /* eof on infile */
                outfile->pf_state |= PF_EOF;
                lp_close();
                return 0;

            case -2:
                parser_error(outfile);
                return 0;

            case -1 :
                spoolreply(outfile, "Processing...");
                return 0;
            }

            if (infile->pf_state & PF_BOT) {
                if ((comment = commatch(start, start + linelength, magics)) != NULL) {
                    compush(comment);
                    /* top of for (;;) */
                    continue;
                } else {
                    CONSUME(infile, linelength + crlflength);
                    /* clear out the input queue if client sent data before magic string */
                    continue;
                }
            }

            /* write to file */
            lp_write(infile, start, linelength + crlflength);
            CONSUME(infile, linelength + crlflength);
        }
    }
}

/*!
 * @brief Serve a query job that starts with `%!PS-Adobe-3.0 Query`
 *
 * The call skips the magic line, pushes the handler of each query comment it
 * finds, and drops other lines. The end of the input, or PF_QUERY at the end
 * of a query, pops the state.
 *
 * @param[in,out] in   data from the client
 * @param[in,out] out  reply data, receives a status while a line is incomplete
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, or CH_MORE while a line is incomplete
 */
int cm_psquery(struct papfile *in, struct papfile *out,
               struct sockaddr_at *sat _U_)
{
    struct papd_comment	*comment;
    char		*start;
    int			linelength, crlflength;

    for (;;) {
        if (in->pf_state & PF_QUERY) {
            /* handle eof at end of query job */
            compop();
            return CH_DONE;
        }

        switch (markline(in, &start, &linelength, &crlflength)) {
        case 0 :
            /* eof on infile */
            out->pf_state |= PF_EOF;
            compop();
            return CH_DONE;

        case -1 :
            spoolreply(out, "Processing...");
            return CH_MORE;

        case -2 :
            return CH_ERROR;
        }

        if (in->pf_state & PF_BOT) {
            in->pf_state &= ~PF_BOT;
        } else {
            if ((comment = commatch(start, start + linelength, queries)) != NULL) {
                compush(comment);
                return CH_DONE;
            }
        }

        CONSUME(in, linelength + crlflength);
    }
}

/*!
 * @brief Serve a print job that starts with `%!PS-Adobe-3.0`
 *
 * The call writes the magic line and every other line to the job, and pushes
 * the handler of each header comment it finds. The end of the input pops the
 * state.
 *
 * @param[in,out] in   data from the client
 * @param[in,out] out  reply data, receives PF_EOF at the end of the input
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, or CH_MORE while a line is incomplete
 */
int cm_psadobe(struct papfile *in, struct papfile *out,
               struct sockaddr_at *sat _U_)
{
    char		*start;
    int			linelength, crlflength;
    struct papd_comment	*comment = compeek();

    for (;;) {
        switch (markline(in, &start, &linelength, &crlflength)) {
        case 0 :
            /* eof on infile */
            out->pf_state |= PF_EOF;
            compop();
            return CH_DONE;

        case -1 :
            return CH_MORE;

        case -2 :
            return CH_ERROR;
        }

        if (in->pf_state & PF_BOT) {
            in->pf_state &= ~PF_BOT;
        } else {
            if ((comment = commatch(start, start + linelength, headers)) != NULL) {
                compush(comment);
                return CH_DONE;
            }
        }

        lp_write(in, start, linelength + crlflength);
        CONSUME(in, linelength + crlflength);
    }
}

char	*Query = "Query";

/*!
 * @brief Pick the handler for a job with another `%!PS-Adobe-` version
 *
 * When the word after the version starts with Query, the job becomes a query
 * job, and any other job goes to cm_psadobe. The line stays in the input for
 * the new handler, and a missing handler in the magics table exits papd.
 *
 * @param[in,out] in   data from the client
 * @param[in,out] out  reply data, receives PF_EOF at the end of the input
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the input, or CH_MORE while the line is
 *          incomplete
 */
int cm_psswitch(struct papfile *in, struct papfile *out,
                struct sockaddr_at *sat _U_)
{
    char		*start, *stop, *p;
    int			linelength, crlflength;

    if (in->pf_state & PF_QUERY) {
        /*handle eof at end of query job */
        in->pf_state &= ~PF_QUERY;
    }

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        /* eof on infile */
        out->pf_state |= PF_EOF;
        compop();
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    stop = start + linelength;

    for (p = start; p < stop; p++) {
        if (*p == ' ' || *p == '\t') {
            break;
        }
    }

    for (; p < stop; p++) {
        if (*p != ' ' && *p != '\t') {
            break;
        }
    }

    if ((size_t)(stop - p) >= strlen(Query) &&
            strncmp(p, Query, strlen(Query)) == 0) {
        if (comswitch(magics, cm_psquery) < 0) {
            LOG(log_error, logtype_papd, "cm_psswitch: can't find psquery!");
            exit(1);
        }
    } else {
        if (comswitch(magics, cm_psadobe) < 0) {
            LOG(log_error, logtype_papd, "cm_psswitch: can't find psadobe!");
            exit(1);
        }
    }

    return CH_DONE;
}


struct papd_comment	magics[] = {
    { "%!PS-Adobe-3.0 Query",	NULL,			cm_psquery, C_FULL },
    { "%!PS-Adobe-3.0",		NULL,			cm_psadobe, C_FULL },
    { "%!PS-Adobe-",		NULL,			cm_psswitch,	0 },
    { NULL,			NULL,			NULL,		0 },
};
