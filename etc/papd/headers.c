/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <netatalk/at.h>
#include <atalk/logger.h>

#include "file.h"
#include "comment.h"
#include "lp.h"

int ch_title(struct papfile *, struct papfile *, struct sockaddr_at *);
int ch_for(struct papfile *, struct papfile *, struct sockaddr_at *);

/*!
 * @brief Extract the value of a DSC comment line, the text after the colon
 *
 * The call skips spaces and tabs after the colon. A value in parentheses
 * yields the text inside, with nested parentheses kept and without escape
 * handling. Any other value yields the rest of the line, and a line without a
 * colon yields an empty string.
 *
 * @param[in] start       first byte of the line
 * @param[in] linelength  line length without the line end
 * @returns a NUL-terminated string to free, or NULL for a line over 1023 bytes
 *          or a failed allocation
 */
static char *get_text(char *start, int linelength)
{
    char *p, *q;
    char *t, *ret;
    char *stop;

    /* 1023 is arbitrary 255 max for comment but some may be escape \xxx and space and keyword */

    if (linelength > 1023) {
        return NULL;
    }

    t = ret = calloc(1, linelength + 1);

    if (!ret) {
        return NULL;
    }

    stop = start + linelength;

    for (p = start; p < stop; p++) {
        if (*p == ':') {
            p++;
            break;
        }
    }

    for (; p < stop; p++) {
        if (*p != ' ' && *p != '\t') {
            break;
        }
    }

    if (p < stop && *p == '(') {
        int count;
        /* start with ( then it's a <text> */
        p++;

        for (q = p, count = 1; q < stop; q++, t++) {
            if (*q == '(') {
                count++;
            } else if (*q == ')') {
                count--;

                if (!count) {
                    break;
                }
            }

            *t = *q;
        }
    } else {
        /* it's a textline */
        for (q = p; q < stop; q++, t++) {
            *t = *q;
        }
    }

    return ret;
}

/*!
 * @brief Take the user name the job is for from a `%%For:` header comment
 *
 * The call passes the value to lp_for and writes the line to the job with
 * PF_TRANSLATE set. It then pops the comment state and consumes the line.
 *
 * @param[in,out] in   input buffer of the job
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_for(struct papfile *in, struct papfile *out _U_,
           struct sockaddr_at *sat _U_)
{
    char                *start, *cmt;
    int                 linelength, crlflength;

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    cmt = get_text(start, linelength);

    if (cmt) {
        lp_for(cmt);
        free(cmt);
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief Take the job name from a `%%Title:` header comment
 *
 * The call passes the value to lp_job and writes the line to the job with
 * PF_TRANSLATE set. It then pops the comment state and consumes the line.
 *
 * @param[in,out] in   input buffer of the job
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_title(struct papfile *in, struct papfile *out _U_,
             struct sockaddr_at *sat _U_)
{
    char		*start, *cmt;
    int			linelength, crlflength;

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    LOG(log_debug9, logtype_papd, "Parsing %%Title");
    cmt = get_text(start, linelength);

    if (cmt) {
        lp_job(cmt);
        free(cmt);
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief Tell the driver that made a job from its `%%Creator:` value
 *
 * The result picks the charset of the job name and user name: Mac Roman for
 * 1 and decomposed UTF-8 for 2.
 *
 * @param[in] creator  value of the comment
 * @returns 1 for an Apple LaserWriter driver, 2 for cgpdftops, or 0
 */
static int guess_creator(char *creator)
{
    if (strstr(creator, "LaserWriter")) {
        return 1;
    }

    if (strstr(creator, "cgpdftops")) {
        return 2;
    }

    return 0;
}


/*!
 * @brief Record the driver that made the job from a `%%Creator:` header comment
 *
 * The call stores the guess_creator result in the input buffer, passes it to
 * lp_origin, and writes the line to the job with PF_TRANSLATE set. It then
 * pops the comment state and consumes the line.
 *
 * @param[in,out] in   input buffer of the job, receives the origin
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_creator(struct papfile *in, struct papfile *out _U_,
               struct sockaddr_at *sat _U_)
{
    char		*start, *cmt;
    int			linelength, crlflength;

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    cmt = get_text(start, linelength);

    if (cmt) {
        in->origin = guess_creator(cmt);
        free(cmt);
        lp_origin(in->origin);
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief End the header comments of a job at `%%EndComments`
 *
 * The call sets PF_STW, so the job loop opens the spool file. It writes the
 * line to the job with PF_TRANSLATE set, pops the comment state, and consumes
 * the line.
 *
 * @param[in,out] in   input buffer of the job, receives PF_STW
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_endcomm(struct papfile *in, struct papfile *out _U_,
               struct sockaddr_at *sat _U_)
{
    char                *start;
    int                 linelength, crlflength;
    LOG(log_debug9, logtype_papd, "End Comment");
    in->pf_state |= PF_STW;

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief Start a translated section at `%%BeginFeature` or `%%BeginPageSetup`
 *
 * The call sets PF_TRANSLATE and leaves it set until the matching end
 * comment. PF_TRANSLATE makes lp_write turn CR line ends into LF for printers
 * with P_FOOMATIC_HACK. The call writes the line, pops the comment state, and
 * consumes the line.
 *
 * @param[in,out] in   input buffer of the job, receives PF_TRANSLATE
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_starttranslate(struct papfile *in, struct papfile *out _U_,
                      struct sockaddr_at *sat _U_)
{
    char                *start;
    int                 linelength, crlflength;
    LOG(log_debug9, logtype_papd, "Start translate");

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief End a translated section at `%%EndFeature` or `%%EndPageSetup`
 *
 * The call writes the line while PF_TRANSLATE is still set, then clears it.
 * It pops the comment state and consumes the line.
 *
 * @param[in,out] in   input buffer of the job, without PF_TRANSLATE afterwards
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_endtranslate(struct papfile *in, struct papfile *out _U_,
                    struct sockaddr_at *sat _U_)
{
    char                *start;
    int                 linelength, crlflength;
    LOG(log_debug9, logtype_papd, "EndTranslate");

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}

/*!
 * @brief Write any other `%%` header comment with PF_TRANSLATE set for that
 * line only
 *
 * The call pops the comment state and consumes the line.
 *
 * @param[in,out] in   input buffer of the job
 * @param[in]     out  reply buffer, unused
 * @param[in]     sat  client address, unused
 * @returns CH_DONE, also at the end of the job, or CH_MORE while the line is
 *          incomplete
 */
int ch_translateone(struct papfile *in, struct papfile *out _U_,
                    struct sockaddr_at *sat _U_)
{
    char                *start;
    int                 linelength, crlflength;
    LOG(log_debug9, logtype_papd, "TranslateOne");

    switch (markline(in, &start, &linelength, &crlflength)) {
    case 0 :
        return 0;

    case -1 :
        return CH_MORE;

    case -2 :
        return CH_ERROR;
    }

    in->pf_state |= PF_TRANSLATE;
    lp_write(in, start, linelength + crlflength);
    in->pf_state &= ~PF_TRANSLATE;
    compop();
    CONSUME(in, linelength + crlflength);
    return CH_DONE;
}




/*
 * "Header" comments.
 */
struct papd_comment	headers[] = {
    { "%%Title:",			NULL,		ch_title,	0 },
    { "%%For:",				NULL,		ch_for,		0 },
    { "%%Creator:",			NULL,		ch_creator,	0 },
    { "%%EndComments",			NULL,		ch_endcomm,	0 },
    { "%%BeginFeature",			NULL,		ch_starttranslate,  0 },
    { "%%EndFeature",			NULL,		ch_endtranslate,  0 },
    { "%%BeginPageSetup",		NULL,		ch_starttranslate, 0 },
    { "%%EndPageSetup",			NULL,		ch_endtranslate, 0 },
#if 0
    { "%%BeginSetup",			NULL,		ch_translateone,  0 },
    { "%%EndSetup",			NULL,		ch_translateone,  0 },
    { "%%BeginProlog",			NULL,		ch_translateone,  0 },
    { "%%EndProlog",			NULL,		ch_translateone,  0 },
    { "%%Page:",			NULL,		ch_translateone, 0 },
    { "%%PageTrailer",			NULL,		ch_translateone, 0 },
    { "%%Trailer",			NULL,		ch_translateone, 0 },
    { "%%EOF",				NULL,		ch_translateone, 0 },
#endif
    { "%%",				NULL,		ch_translateone, 0 },
    { NULL,				NULL,		NULL,		0 },
};
