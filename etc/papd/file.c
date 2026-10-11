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
#include <stdlib.h>
#include <string.h>

#include "file.h"

/*!
 * @brief Find the next line in the input buffer of a print job
 *
 * The line runs up to the first CR or LF. Every CR and LF that follows counts
 * as its line end, so a blank line never comes back as a separate line. At
 * the end of the job the call appends a newline to a last line that has none.
 *
 * @param[in,out] pf          input buffer of the job
 * @param[out]    start       receives the first byte of the line
 * @param[out]    linelength  receives the line length without the line end
 * @param[out]    crlflength  receives the length of the line end
 * @returns 1 for a line, 0 at the end of the job, or -1 while the line is
 *          incomplete
 */
int markline(struct papfile *pf, char **start, int *linelength, int *crlflength)
{
    char		*p;

    if (pf->pf_datalen == 0 && (pf->pf_state & PF_EOF)) {
        return 0;
    }

    *start = pf->pf_data;

    /* get a line */
    for (*linelength = 0; *linelength < pf->pf_datalen; (*linelength)++) {
        if (pf->pf_data[*linelength] == '\n' ||
                pf->pf_data[*linelength] == '\r') {
            break;
        }
    }

    if (*linelength >= pf->pf_datalen) {
        if (pf->pf_state & PF_EOF) {
            append(pf, "\n", 1);
        } else {
            return -1;
        }
    }

    p = pf->pf_data + *linelength;
    *crlflength = 0;

    while (*crlflength < pf->pf_datalen - *linelength &&
            (p[*crlflength] == '\r' || p[*crlflength] == '\n')) {
        (*crlflength)++;
    }

#if 0

    /* Apple LaserWriter 8 drivers don't respect line lengths */
    if (!*crlflength) {
        /* line is way too long, something fishy is going on, give up */
        LOG(log_error, logtype_papd, "markline: no crlf in comment, give up");
        return -2;
    }

#endif
    /* success */
    return 1;
}

/*!
 * @brief Append data to a papfile buffer, after moving the data to the buffer
 * start and growing the buffer as needed
 *
 * The buffer grows in steps of PF_MORESPACE bytes. A failed allocation exits
 * papd.
 *
 * @param[in,out] pf    papfile buffer
 * @param[in]     data  bytes to append
 * @param[in]     len   number of bytes
 */
void morespace(struct papfile *pf, const char *data, int len)
{
    char		*nbuf;
    int			nsize;

    if (pf->pf_data != pf->pf_buf) {			/* pull up */
        bcopy(pf->pf_data, pf->pf_buf, pf->pf_datalen);
        pf->pf_data = pf->pf_buf;
    }

    if (pf->pf_datalen + len > pf->pf_bufsize) {	/* make more space */
        nsize = ((pf->pf_bufsize + len) / PF_MORESPACE +
                 ((pf->pf_bufsize + len) % PF_MORESPACE != 0)) * PF_MORESPACE;

        if (pf->pf_buf) {
            if ((nbuf = (char *)realloc(pf->pf_buf, nsize)) == NULL) {
                exit(1);
            }
        } else {
            if ((nbuf = (char *)malloc(nsize)) == NULL) {
                exit(1);
            }
        }

        pf->pf_bufsize = nsize;
        pf->pf_data = nbuf + (pf->pf_data - pf->pf_buf);
        pf->pf_buf = nbuf;
    }

    bcopy(data, pf->pf_data + pf->pf_datalen, len);
    pf->pf_datalen += len;
}


/*!
 * @brief Append data to a papfile buffer
 *
 * Data that does not fit after the current data goes through morespace.
 */
void append(struct papfile *pf, const char *data, int len)
{
    if ((pf->pf_data + pf->pf_datalen + len) >
            (pf->pf_buf + pf->pf_bufsize)) {
        morespace(pf, data, len);
    } else {
        memcpy(pf->pf_data + pf->pf_datalen, data, len);
        pf->pf_datalen += len;
    }
}


/*!
 * @brief Append a PostScript status message for the client,
 * `%%[ status: str ]%%`
 *
 * @param[in,out] out  output buffer to the client
 * @param[in]     str  message, or NULL for the default `Spooler error.` text
 */
void spoolreply(struct papfile *out, char *str)
{
    char	*pserr1 = "%%[ status: ";
    char	*pserr2 = " ]%%\n";

    if (str == NULL) {
        str = "Spooler error.";
    }

    append(out, pserr1, strlen(pserr1));
    append(out, str, strlen(str));
    append(out, pserr2, strlen(pserr2));
}

/*!
 * @brief Append a PostScript error message for the client, `%%[ Error: str ]%%`
 *
 * @param[in,out] out  output buffer to the client
 * @param[in]     str  message, or NULL for the default `Spooler error.` text
 */
void spoolerror(struct papfile *out, char *str)
{
    char	*pserr1 = "%%[ Error: ";
    char	*pserr2 = " ]%%\n";

    if (str == NULL) {
        str = "Spooler error.";
    }

    append(out, pserr1, strlen(pserr1));
    append(out, str, strlen(str));
    append(out, pserr2, strlen(pserr2));
}
