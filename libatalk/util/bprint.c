#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/util.h>

static const char	hexdig[] = "0123456789ABCDEF";

#define BPXLEN	50
#define BPALEN	18

/*!
 * @brief Print a hex dump of a buffer to stdout, 16 bytes a line
 *
 * Each line holds the hex bytes and then the printable characters, with a dot
 * for any other byte. A final "(end)" line closes the dump.
 *
 * @param[in] data  bytes to print
 * @param[in] len   number of bytes
 */
void bprint(char *data, int len)
{
    char	xout[BPXLEN], aout[BPALEN];
    int         i;
    memset(xout, 0, BPXLEN);
    memset(aout, 0, BPALEN);

    for (i = 0; len; len--, data++, i++) {
        if (i == 16) {
            printf("%-48s\t%-16s\n", xout, aout);
            memset(xout, 0, BPXLEN);
            memset(aout, 0, BPALEN);
            i = 0;
        }

        if (isprint((unsigned char) * data)) {
            aout[i] = *data;
        } else {
            aout[i] = '.';
        }

        xout[i * 3] = hexdig[(*data & 0xf0) >> 4];
        xout[(i * 3) + 1] = hexdig[*data & 0x0f];
        xout[(i * 3) + 2] = ' ';
    }

    if (i) {
        printf("%-48s\t%-16s\n", xout, aout);
    }

    printf("(end)\n");
}
