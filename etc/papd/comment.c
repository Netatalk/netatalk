/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief DSC comment state stack of the papd PostScript parser
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/logger.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "comment.h"

struct comstate	*comstate;

char	*comcont = "%%+";

/*!
 * @brief Pop the top state off the comment stack and free it
 *
 * The stack must not be empty.
 */
void compop(void)
{
    struct comstate	*cs;
    cs = comstate;
    comstate = cs->cs_prev;
    free(cs);
}

/*!
 * @brief Push a new state for a DSC comment onto the comment stack
 *
 * The state starts with no flags. A failed allocation exits papd.
 *
 * @param[in] comment  comment table entry whose handler takes over
 */
void compush(struct papd_comment *comment)
{
    struct comstate	*cs;

    if ((cs = (struct comstate *)malloc(sizeof(struct comstate))) ==
            NULL) {
        LOG(log_error, logtype_papd, "malloc: %s", strerror(errno));
        exit(1);
    }

    cs->cs_comment = comment;
    cs->cs_prev = comstate;
    cs->cs_flags = 0;
    comstate = cs;
}

/*!
 * @brief Replace the top state of the comment stack with the table entry that
 * has a given handler
 *
 * The last matching entry of the table counts, and the new state starts with
 * no flags.
 *
 * @param[in] comments  comment table, ended by an entry without c_begin
 * @param[in] handler   handler to look for
 * @returns 0, or -1 when no entry has the handler
 */
int comswitch(struct papd_comment *comments, int (*handler)())
{
    struct papd_comment	*c, *comment = NULL;

    for (c = comments; c->c_begin; c++) {
        if (c->c_handler == handler) {
            comment = c;
        }
    }

    if (comment == NULL || comment->c_handler != handler) {
        LOG(log_error, logtype_papd, "comswitch: can't find handler!");
        return -1;
    }

    compop();
    compush(comment);
    return 0;
}

/*!
 * @brief Match a comment line against a keyword
 *
 * With C_FULL in how, the line has to equal str. Otherwise str has to be a
 * prefix of the line.
 *
 * @param[in] start  first byte of the line
 * @param[in] stop   end of the line, not included
 * @param[in] str    keyword, such as `%%Title:`
 * @param[in] how    comment flags, C_FULL for a full match
 * @returns 0 on a match, or 1
 */
int comcmp(char *start, char *stop, char *str, int how)
{
    int		cc, len;
    len = stop - start;
    cc = strlen(str);

    if (how & C_FULL) {
        if ((cc == len) && (strncmp(str, start, cc) == 0)) {
            return 0;
        }
    } else {
        if ((cc <= len) && (strncmp(str, start, cc) == 0)) {
            return 0;
        }
    }

    return 1;
}

/*!
 * @brief Find the first table entry whose keyword matches a comment line
 *
 * Each entry goes through comcmp with the c_flags that it carries.
 *
 * @param[in] start     first byte of the line
 * @param[in] stop      end of the line, not included
 * @param[in] comments  comment table, ended by an entry without c_begin
 * @returns the entry, or NULL without a match
 */
struct papd_comment *commatch(char *start, char *stop,
                              struct papd_comment comments[])
{
    struct papd_comment	*comment;

    for (comment = comments; comment->c_begin; comment++) {
        if (comcmp(start, stop, comment->c_begin, comment->c_flags) == 0) {
            break;
        }
    }

    if (comment->c_begin) {
        return comment;
    } else {
        return NULL;
    }
}
