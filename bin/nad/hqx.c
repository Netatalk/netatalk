/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * All Rights Reserved.  See COPYRIGHT.
 *
 * Macintosh file format transformer based on megatron by Charles Clark.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/param.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <netinet/in.h>

#include <atalk/adouble.h>
#include <netatalk/endian.h>

#include "nad.h"
#include "megatron.h"
#include "nad_adouble.h"
#include "hqx.h"
#include "crc16.h"

/*	String used to indicate standard input instead of a disk
	file.  Should be a string not normally used for a file
 */
#ifndef	STDIN
#	define	STDIN	"-"
#endif /* ! STDIN */

/*	Yes and no
 */
#define NOWAY		0
#define SURETHANG	1

/*	Looking for the first or any other line of a binhex file
 */
#define	FIRST		0
#define OTHER		1

/*	This is the binhex run length encoding character
 */
#define RUNCHAR		0x90

/*	These are field sizes in bytes of various pieces of the
	binhex header
 */
#define	BHH_VERSION		1
#define	BHH_TCSIZ		8
#define	BHH_FLAGSIZ		2
#define	BHH_DATASIZ		4
#define	BHH_RESSIZ		4
#define BHH_CRCSIZ		2
#define BHH_HEADSIZ		21

static struct hqx_file_data {
    uint32_t		forklen[ NUMFORKS ];
    unsigned short		forkcrc[ NUMFORKS ];
    int                 forkdone[ NUMFORKS ];
    char		path[ MAXPATHLEN + 1];
    unsigned short		headercrc;
    int			filed;
    int                 owns_fd;
    unsigned char       outbuf[3];
    int                 outcnt;
    int                 linecol;
    int                 writing;
} 		hqx;

extern char	*forkname[];
static unsigned char	hqx7_buf[8192];
static unsigned char	*hqx7_first;
static unsigned char	*hqx7_last;
static int	first_flag;
static const unsigned char hqxchars[] =
    "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";

static int hqx_write_all(const void *buf, size_t len)
{
    const char *p = buf;

    while (len > 0) {
        ssize_t cc = write(hqx.filed, p, len);

        if (cc <= 0) {
            return -1;
        }

        p += cc;
        len -= cc;
    }

    return 0;
}

static int hqx_put_char(unsigned char c)
{
    if (hqx.linecol >= 63) {
        if (hqx_write_all("\n", 1) < 0) {
            return -1;
        }

        hqx.linecol = 0;
    }

    if (hqx_write_all(&c, 1) < 0) {
        return -1;
    }

    hqx.linecol++;
    return 0;
}

static int hqx_emit_6bit(unsigned char b)
{
    hqx.outbuf[hqx.outcnt++] = b;

    if (hqx.outcnt == 3) {
        unsigned char out[4];
        out[0] = hqxchars[(hqx.outbuf[0] >> 2) & 0x3f];
        out[1] = hqxchars[((hqx.outbuf[0] << 4) | (hqx.outbuf[1] >> 4)) & 0x3f];
        out[2] = hqxchars[((hqx.outbuf[1] << 2) | (hqx.outbuf[2] >> 6)) & 0x3f];
        out[3] = hqxchars[hqx.outbuf[2] & 0x3f];

        for (int i = 0; i < 4; i++) {
            if (hqx_put_char(out[i]) < 0) {
                return -1;
            }
        }

        hqx.outcnt = 0;
    }

    return 0;
}

static int hqx_emit_byte(unsigned char b)
{
    if (hqx_emit_6bit(b) < 0) {
        return -1;
    }

    if (b == RUNCHAR) {
        return hqx_emit_6bit(0);
    }

    return 0;
}

static int hqx_emit_data(const void *buf, size_t len)
{
    const unsigned char *p = buf;

    while (len-- > 0) {
        if (hqx_emit_byte(*p++) < 0) {
            return -1;
        }
    }

    return 0;
}

static int hqx_flush_6bit(void)
{
    unsigned char out[3];
    int outlen;

    if (hqx.outcnt == 0) {
        return 0;
    }

    hqx.outbuf[1] = hqx.outcnt > 1 ? hqx.outbuf[1] : 0;
    hqx.outbuf[2] = 0;
    out[0] = hqxchars[(hqx.outbuf[0] >> 2) & 0x3f];
    out[1] = hqxchars[((hqx.outbuf[0] << 4) | (hqx.outbuf[1] >> 4)) & 0x3f];
    out[2] = hqxchars[(hqx.outbuf[1] << 2) & 0x3f];
    outlen = hqx.outcnt + 1;

    for (int i = 0; i < outlen; i++) {
        if (hqx_put_char(out[i]) < 0) {
            return -1;
        }
    }

    hqx.outcnt = 0;
    return 0;
}

static int hqx_finish_fork(int fork)
{
    uint16_t crc;

    if (hqx.forkdone[fork]) {
        return 0;
    }

    if (hqx.forklen[fork] != 0) {
        return -1;
    }

    crc = htons(hqx.forkcrc[fork]);

    if (hqx_emit_data(&crc, sizeof(crc)) < 0) {
        return -1;
    }

    hqx.forkdone[fork] = 1;
    return 0;
}

/*!
 * @brief Open a BinHex file for reading or writing
 *
 * This must be called before other hqx operations. When opening for
 * reading, it skips the preamble and reads the BinHex header. When
 * opening for writing, it initializes output state and writes the
 * BinHex preamble and header.
 *
 * @param[in] hqxfile    input file path, or `-` for standard input
 * @param[in] flags      open flags; `O_RDONLY` selects read mode
 * @param[in,out] fh     file header to read or write
 * @param[in] options    output options, such as `OPTION_STDOUT`
 *
 * @returns 0 on success, -1 on error
 */
int hqx_open(char *hqxfile, int flags, struct FHeader *fh, int options)
{
#if DEBUG
    NAD_DEBUG("nad: entering hqx_open\n");
#endif /* DEBUG */
    hqx.filed = -1;
    hqx.owns_fd = 0;

    if (flags == O_RDONLY) {
        hqx.writing = 0;
        first_flag = 0;

        if (strcmp(hqxfile, STDIN) == 0) {
            hqx.filed = fileno(stdin);
        } else if ((hqx.filed = open(hqxfile, O_RDONLY)) < 0) {
            perror(hqxfile);
            return -1;
        } else {
            hqx.owns_fd = 1;
        }

        if (skip_junk(FIRST) == 0 && hqx_header_read(fh) == 0) {
#if DEBUG
            off_t	pos;
            pos = lseek(hqx.filed, 0, SEEK_CUR);
            NAD_DEBUG("nad: current position is %ld\n", pos);
#endif /* DEBUG */
            return 0;
        }

        hqx_close(KEEP);
        fprintf(stderr, "%s\n", hqxfile);
        return -1;
    } else {
        memset(hqx.forkcrc, 0, sizeof(hqx.forkcrc));
        memset(hqx.forkdone, 0, sizeof(hqx.forkdone));
        hqx.forklen[DATA] = ntohl(fh->forklen[DATA]);
        hqx.forklen[RESOURCE] = ntohl(fh->forklen[RESOURCE]);
        hqx.outcnt = 0;
        hqx.linecol = 0;
        hqx.writing = 1;

        if (options & OPTION_STDOUT) {
            hqx.filed = fileno(stdout);
            strlcpy(hqx.path, STDIN, sizeof(hqx.path));
        } else {
            if (strlcpy(hqx.path, fh->name, sizeof(hqx.path)) >= sizeof(hqx.path)
                    || strlcpy(hqx.path, mtoupath(hqx.path), sizeof(hqx.path))
                    >= sizeof(hqx.path)
                    || strlcat(hqx.path, ".hqx", sizeof(hqx.path))
                    >= sizeof(hqx.path)) {
                fprintf(stderr, "Output path is too long: %s.hqx\n", fh->name);
                return -1;
            }

            if ((hqx.filed = open(hqx.path, flags, 0666)) < 0) {
                perror(hqx.path);
                return -1;
            }

            hqx.owns_fd = 1;
        }

        static const char hqx_preamble[] =
            "(This file must be converted with BinHex 4.0)\n\n:";

        if (hqx_write_all(hqx_preamble, sizeof(hqx_preamble) - 1) < 0) {
            hqx_close(TRASH);
            return -1;
        }

        if (hqx_header_write(fh) != 0) {
            hqx_close(TRASH);
            fprintf(stderr, "%s\n", hqx.path);
            return -1;
        }

        return 0;
    }
}

/*!
 * @brief Close the active BinHex file
 *
 * This must be called before opening another file with hqx_open().
 * In write mode, `KEEP` finishes any remaining fork data, flushes the
 * encoder, and writes the BinHex terminator. `TRASH` closes the output
 * and removes the incomplete file.
 *
 * @param[in] keepflag    `KEEP` to keep the output, `TRASH` to discard it
 *
 * @returns 0 on success, -1 on error
 */
int hqx_close(int keepflag)
{
    if (keepflag == KEEP) {
        if (hqx.writing
                && (hqx_finish_fork(DATA) < 0
                    || hqx_finish_fork(RESOURCE) < 0
                    || hqx_flush_6bit() < 0
                    || hqx_write_all(":\n", 2) < 0)) {
            return -1;
        }

        if (hqx.owns_fd && hqx.filed >= 0) {
            int rc = close(hqx.filed);
            hqx.filed = -1;
            hqx.owns_fd = 0;
            return rc;
        }

        hqx.filed = -1;
        return 0;
    } else if (keepflag == TRASH) {
        if (hqx.owns_fd && hqx.filed >= 0) {
            close(hqx.filed);
        }

        hqx.filed = -1;
        hqx.owns_fd = 0;

        if ((strcmp(hqx.path, STDIN) != 0) && (unlink(hqx.path) < 0)) {
            perror(hqx.path);
        }

        return 0;
    } else {
        return -1;
    }
}

const char *hqx_path(void)
{
    return hqx.path;
}

/*!
 * @brief Read decoded data from a BinHex fork
 *
 * Call this until it returns zero for each fork. When no fork data
 * remains, the stored fork CRC is read and compared with the calculated
 * CRC before returning zero.
 *
 * @note hqx_read() must be called enough times to return zero, and no
 * more than that, for each fork.
 *
 * @param[in] fork       fork selector, `DATA` or `RESOURCE`
 * @param[out] buffer    destination buffer
 * @param[in] length     maximum number of bytes to read
 *
 * @returns number of bytes read, 0 when the fork is complete, -1 on error
 */
ssize_t hqx_read(int fork, char *buffer, size_t length)
{
    unsigned short		storedcrc;
    size_t		readlen;
    size_t		cc;
#if DEBUG >= 3
    {
        off_t	pos;
        pos = lseek(hqx.filed, 0, SEEK_CUR);
        NAD_DEBUG("hqx_read: current position is %ld\n", pos);
    }
    NAD_DEBUG("hqx_read: fork is %s\n", forkname[ fork ]);
    NAD_DEBUG("hqx_read: remaining length is %d\n", hqx.forklen[fork]);
#endif /* DEBUG >= 3 */

    if (hqx.forklen[fork] > 0x7FFFFFFF) {
        fprintf(stderr, "Invalid %s fork length: %u\n",
                forkname[fork],
                hqx.forklen[fork]);
        return -1;
    }

    if (hqx.forklen[ fork ] == 0) {
        cc = hqx_7tobin((char *)&storedcrc, sizeof(storedcrc));

        if (cc == sizeof(storedcrc)) {
            storedcrc = ntohs(storedcrc);
#if DEBUG >= 4
            NAD_DEBUG("hqx_read: storedcrc\t\t%x\n", storedcrc);
            NAD_DEBUG("hqx_read: observed crc\t\t%x\n\n", hqx.forkcrc[fork]);
#endif /* DEBUG >= 4 */

            if (storedcrc == hqx.forkcrc[ fork ]) {
                return 0;
            }

            fprintf(stderr, "hqx_read: bad %s fork CRC\n",
                    forkname[ fork ]);
        }

        return -1;
    }

    if (hqx.forklen[ fork ] < length) {
        readlen = hqx.forklen[ fork ];
    } else {
        readlen = length;
    }

#if DEBUG >= 3
    NAD_DEBUG("hqx_read: readlen is %d\n", readlen);
#endif /* DEBUG >= 3 */
    cc = hqx_7tobin(buffer, readlen);

    if (cc > 0) {
        hqx.forkcrc[ fork ] =
            crc16_xmodem_update(hqx.forkcrc[ fork ], buffer, cc);
        hqx.forklen[ fork ] -= cc;
    }

#if DEBUG >= 3
    NAD_DEBUG("hqx_read: chars read is %d\n", cc);
#endif /* DEBUG >= 3 */
    return cc;
}

/*!
 * @brief Read and validate a BinHex header
 *
 * This is called by hqx_open() before any fork data is read. It decodes
 * the header fields, initializes fork lengths and CRC state, and verifies
 * the header CRC.
 *
 * @param[out] fh    file header to populate
 *
 * @returns 0 on success, negative value on error
 */
int hqx_header_read(struct FHeader *fh)
{
    char		*headerbuf, *headerptr;
    uint32_t		time_seconds;
    unsigned short		mask;
    unsigned short		header_crc;
    char		namelen_byte;
    unsigned char	namelen;
    size_t              headerlen;
    mask = htons(0xfcee);
    hqx.headercrc = 0;

    if (hqx_7tobin(&namelen_byte, sizeof(namelen_byte))
            != sizeof(namelen_byte)) {
        fprintf(stderr, "Premature end of file :");
        return -2;
    }

    namelen = (unsigned char)namelen_byte;
    hqx.headercrc = crc16_xmodem_update(hqx.headercrc, &namelen_byte,
                                        sizeof(namelen_byte));
    headerlen = namelen + BHH_HEADSIZ;

    if ((headerbuf = (char *)malloc(headerlen)) == NULL) {
        return -1;
    }

    if (hqx_7tobin(headerbuf, headerlen) != headerlen) {
        free(headerbuf);
        fprintf(stderr, "Premature end of file :");
        return -2;
    }

    headerptr = headerbuf;
    hqx.headercrc = crc16_xmodem_update(hqx.headercrc,
                                        headerbuf, headerlen - BHH_CRCSIZ);

    /*
     * stuff from the hqx file header
     */
    if (namelen >= sizeof(fh->name)) {
        free(headerbuf);
        fprintf(stderr, "BinHex filename is too long :");
        return -2;
    }

    memcpy(fh->name, headerptr, (int)namelen);
    fh->name[namelen] = '\0';
    headerptr += namelen;
    headerptr += BHH_VERSION;
    memcpy(&fh->finder_info,  headerptr, BHH_TCSIZ);
    headerptr += BHH_TCSIZ;
    memcpy(&fh->finder_info.fdFlags,  headerptr, BHH_FLAGSIZ);
    fh->finder_info.fdFlags = fh->finder_info.fdFlags & mask;
    headerptr += BHH_FLAGSIZ;
    memcpy(&fh->forklen[ DATA ],  headerptr, BHH_DATASIZ);
    hqx.forklen[ DATA ] = ntohl(fh->forklen[ DATA ]);
    headerptr += BHH_DATASIZ;
    memcpy(&fh->forklen[ RESOURCE ], headerptr, BHH_RESSIZ);
    hqx.forklen[ RESOURCE ] = ntohl(fh->forklen[ RESOURCE ]);
    headerptr += BHH_RESSIZ;
    memcpy(&header_crc,  headerptr, BHH_CRCSIZ);
    header_crc = ntohs(header_crc);
    /*
     * stuff that should be zero'ed out
     */
    fh->comment[0] = '\0';
    fh->finder_info.fdLocation = 0;
    fh->finder_info.fdFldr = 0;
#if DEBUG >= 5
    {
        short		flags;
        NAD_DEBUG("Values read by hqx_header_read\n");
        NAD_DEBUG("name length\t\t%d\n", namelen);
        NAD_DEBUG("file name\t\t%s\n", fh->name);
        NAD_DEBUG("get info comment\t%s\n", fh->comment);
        NAD_DEBUG("type\t\t\t%.*s\n", sizeof(fh->finder_info.fdType),
                  &fh->finder_info.fdType);
        NAD_DEBUG("creator\t\t\t%.*s\n",
                  sizeof(fh->finder_info.fdCreator),
                  &fh->finder_info.fdCreator);
        memcpy(&flags, &fh->finder_info.fdFlags, sizeof(flags));
        flags = ntohs(flags);
        NAD_DEBUG("flags\t\t\t%x\n", flags);
        NAD_DEBUG("data fork length\t%ld\n", hqx.forklen[DATA]);
        NAD_DEBUG("resource fork length\t%ld\n", hqx.forklen[RESOURCE]);
        NAD_DEBUG("header_crc\t\t%x\n", header_crc);
        NAD_DEBUG("observed crc\t\t%x\n", hqx.headercrc);
        NAD_DEBUG("\n");
    }
#endif /* DEBUG >= 5 */
    /*
     * create and modify times are figured from right now
     */
    time_seconds = AD_DATE_FROM_UNIX(time(NULL));
    memcpy(&fh->create_date, &time_seconds,
           sizeof(fh->create_date));
    memcpy(&fh->mod_date, &time_seconds,
           sizeof(fh->mod_date));
    fh->backup_date = AD_DATE_START;
    fh->date_flags = 0;
    /*
     * stuff that should be zero'ed out
     */
    fh->comment[0] = '\0';
    memset(&fh->finder_info.fdLocation, 0,
           sizeof(fh->finder_info.fdLocation));
    memset(&fh->finder_info.fdFldr, 0, sizeof(fh->finder_info.fdFldr));
    hqx.forkcrc[ DATA ] = 0;
    hqx.forkcrc[ RESOURCE ] = 0;
    free(headerbuf);

    if (header_crc != hqx.headercrc) {
        fprintf(stderr, "Bad header CRC:");
        return -3;
    }

    return 0;
}

/*!
 * @brief Encode and write a BinHex header
 *
 * @param[in] fh    file header to write
 *
 * @returns 0 on success, -1 on error
 */
int hqx_header_write(struct FHeader *fh)
{
    unsigned char header[ADEDLEN_NAME + BHH_HEADSIZ + 1];
    unsigned char *p = header;
    size_t namelen;
    uint16_t header_crc;
    uint16_t flags;
    namelen = strnlen(fh->name, sizeof(fh->name));

    if (namelen > 63) {
        fprintf(stderr, "BinHex filename is too long: %s\n", fh->name);
        return -1;
    }

    *p++ = (unsigned char)namelen;
    memcpy(p, fh->name, namelen);
    p += namelen;
    *p++ = 0; /* BinHex version */
    memcpy(p, &fh->finder_info.fdType, sizeof(fh->finder_info.fdType));
    p += sizeof(fh->finder_info.fdType);
    memcpy(p, &fh->finder_info.fdCreator, sizeof(fh->finder_info.fdCreator));
    p += sizeof(fh->finder_info.fdCreator);
    flags = fh->finder_info.fdFlags & htons(0xfcee);
    memcpy(p, &flags, sizeof(flags));
    p += sizeof(flags);
    memcpy(p, &fh->forklen[DATA], sizeof(fh->forklen[DATA]));
    p += sizeof(fh->forklen[DATA]);
    memcpy(p, &fh->forklen[RESOURCE], sizeof(fh->forklen[RESOURCE]));
    p += sizeof(fh->forklen[RESOURCE]);
    header_crc = htons(crc16_xmodem_update(0, header, p - header));
    memcpy(p, &header_crc, sizeof(header_crc));
    p += sizeof(header_crc);

    if (hqx_emit_data(header, p - header) < 0) {
        return -1;
    }

    return 0;
}

ssize_t hqx_write(int fork, char *buffer, size_t length)
{
    if (fork < 0 || fork >= NUMFORKS) {
        return -1;
    }

    if (fork == RESOURCE && hqx_finish_fork(DATA) < 0) {
        return -1;
    }

    if (hqx.forkdone[fork] || length > hqx.forklen[fork]) {
        return -1;
    }

    hqx.forkcrc[fork] =
        crc16_xmodem_update(hqx.forkcrc[fork], buffer, length);

    if (hqx_emit_data(buffer, length) < 0) {
        return -1;
    }

    hqx.forklen[fork] -= length;

    if (hqx.forklen[fork] == 0 && hqx_finish_fork(fork) < 0) {
        return -1;
    }

    return length;
}

/*!
 * @brief Fill the BinHex text input buffer
 *
 * This is called from skip_junk() and hqx_7tobin(). It reads from the
 * BinHex file into the hqx7 buffer and updates hqx7_first and hqx7_last
 * to delimit valid data.
 *
 * @param[in] hqx7_ptr    pointer within hqx7_buf where reading should start
 *
 * @returns number of bytes read, 0 on end of file, -1 on error
 */
ssize_t hqx7_fill(unsigned char *hqx7_ptr)
{
    ssize_t		cc;
    size_t		cs;
    cs = hqx7_ptr - hqx7_buf;

    if (cs >= sizeof(hqx7_buf)) {
        return -1;
    }

    hqx7_first = hqx7_ptr;
    cc = read(hqx.filed, (char *)hqx7_first, (sizeof(hqx7_buf) - cs));

    if (cc < 0) {
        perror("");
        return cc;
    }

    hqx7_last = (hqx7_first + cc);
    return cc;
}

/*
char tr[] = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
	     0 123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
	     0                1               2               3
Input characters are translated to a number between 0 and 63 by direct
array lookup.  0xFF signals a bad character.  0xFE is signals a legal
character that should be skipped, namely '\n', '\r'.  0xFD signals ':'.
0xFC signals a whitespace character.
*/

static const unsigned char hqxlookup[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFE, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFC, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0xFF, 0xFF,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0xFF,
    0x14, 0x15, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0xFF,
    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0xFF,
    0x2C, 0x2D, 0x2E, 0x2F, 0xFF, 0xFF, 0xFF, 0xFF,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0xFF,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0xFF, 0xFF,
    0x3D, 0x3E, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/*!
 * @brief Skip non-BinHex text until encoded data is found
 *
 * This is called from hqx_open() to find the first valid BinHex line,
 * and from hqx_7tobin() to find subsequent encoded lines.
 *
 * @param[in] line    `FIRST` for the first encoded line, `OTHER` thereafter
 *
 * @returns 0 on success, -1 if valid encoded data is not found
 */
int skip_junk(int line)
{
    int			found = NOWAY;
    int			stopflag;
    int			nc = 0;
    unsigned char		c;
    unsigned char		prevchar;

    if (line == FIRST && hqx7_fill(hqx7_buf) <= 0) {
        fprintf(stderr, "Premature end of file :");
        return -1;
    }

    while (found == NOWAY) {
        if (line == FIRST) {
            if (*hqx7_first == ':') {
                nc = c = 0;
                stopflag = NOWAY;
                hqx7_first++;

                while ((stopflag == NOWAY) &&
                        (nc < (hqx7_last - hqx7_first))) {
                    switch (c = hqxlookup[ hqx7_first[ nc ]]) {
                    case 0xFC :
                    case 0xFF :
                    case 0xFE :
                    case 0xFD :
                        stopflag = SURETHANG;
                        break;

                    default :
                        nc++;
                        break;
                    }
                }

                if ((nc > 30) && (nc < 64) &&
                        ((c == 0xFE) || (c == 0xFD))) {
                    found = SURETHANG;
                }
            } else {
                hqx7_first++;
            }
        } else {
            if ((prevchar = hqxlookup[ *hqx7_first ]) == 0xFE) {
                nc = c = 0;
                stopflag = NOWAY;
                hqx7_first++;

                while ((stopflag == NOWAY) &&
                        (nc < (hqx7_last - hqx7_first))) {
                    switch (c = hqxlookup[ hqx7_first[ nc ]]) {
                    case 0xFC :
                    case 0xFE :
                        if ((prevchar == 0xFC) || (prevchar == 0xFE)) {
                            nc++;
                            break;
                        }

                    /* fall through */
                    case 0xFF :
                    case 0xFD :
                        stopflag = SURETHANG;
                        break;

                    default :
                        prevchar = c;
                        nc++;
                        break;
                    }
                }

                if (c == 0xFD) {
                    found = SURETHANG;
                } else if ((nc > 30) && (c == 0xFE)) {
                    found = SURETHANG;
                }
            } else {
                hqx7_first++;
            }
        }

        if ((hqx7_last - hqx7_first) == nc) {
            hqx7_buf[0] = (line == FIRST) ? ':' : '\n';
            memmove(&hqx7_buf[1], hqx7_first, nc);
            hqx7_first = &hqx7_buf[++nc];

            if (hqx7_fill(hqx7_first) <= 0) {
                fprintf(stderr, "Premature end of file :");
                return -1;
            }

            hqx7_first = hqx7_buf;
        }
    }

    return 0;
}

/*!
 * @brief Decode BinHex text data to binary
 *
 * This is called by hqx_header_read() for header data and by hqx_read()
 * for fork data and CRC fields. It buffers decoded data so callers get
 * the requested length unless end of file is reached.
 *
 * @param[out] outbuf    destination buffer for decoded bytes
 * @param[in] datalen    number of decoded bytes requested
 *
 * @returns number of decoded bytes written to @p outbuf
 */
size_t hqx_7tobin(char *outbuf, size_t datalen)
{
    static unsigned char	hqx8[3];
    static int		hqx8i;
    static unsigned char	prev_hqx8;
    static unsigned char	prev_out;
    static unsigned char	prev_hqx7;
    static int		eofflag;
    unsigned char		hqx7[4];
    int			hqx7i = 0;
    ssize_t		cc;
    char		*out_first;
    const char		*out_last;
#if DEBUG
    NAD_DEBUG("hqx_7tobin: datalen entering %d\n", datalen);
    NAD_DEBUG("hqx_7tobin: hqx8i entering %d\n", hqx8i);
#endif /* DEBUG */

    if (first_flag == 0) {
        prev_hqx8 = 0;
        prev_hqx7 = 0;
        prev_out = 0;
        hqx8i = 3;
        first_flag = 1;
        eofflag = 0;
    }

#if DEBUG
    NAD_DEBUG("hqx_7tobin: hqx8i entering %d\n", hqx8i);
#endif /* DEBUG */
    out_first = outbuf;
    out_last = out_first + datalen;

    while ((out_first < out_last) && (eofflag == 0)) {
        if (hqx7_first == hqx7_last) {
            cc = hqx7_fill(hqx7_buf);

            if (cc <= 0) {
                eofflag = 1;
                continue;
            }
        }

        if (hqx8i > 2) {
            while ((hqx7i < 4) && (hqx7_first < hqx7_last)) {
                hqx7[ hqx7i ] = hqxlookup[ *hqx7_first ];

                switch (hqx7[ hqx7i ]) {
                case 0xFC :
                    if ((prev_hqx7 == 0xFC) || (prev_hqx7 == 0xFE)) {
                        hqx7_first++;
                        break;
                    }

                /* fall through */
                case 0xFD :
                case 0xFF :
                    eofflag = 1;

                    while (hqx7i < 4) {
                        hqx7[ hqx7i++ ] = 0;
                    }

                    break;

                case 0xFE :
                    prev_hqx7 = hqx7[ hqx7i ];

                    if (skip_junk(OTHER) < 0) {
                        fprintf(stderr, "\n");
                        eofflag = 1;

                        while (hqx7i < 4) {
                            hqx7[ hqx7i++ ] = 0;
                        }
                    }

                    break;

                default :
                    prev_hqx7 = hqx7[ hqx7i++ ];
                    hqx7_first++;
                    break;
                }
            }

            if (hqx7i == 4) {
                hqx8[ 0 ] = (unsigned char)((hqx7[ 0 ] << 2) | (hqx7[ 1 ] >> 4));
                hqx8[ 1 ] = (unsigned char)((hqx7[ 1 ] << 4) | (hqx7[ 2 ] >> 2));
                hqx8[ 2 ] = (unsigned char)((hqx7[ 2 ] << 6) | (hqx7[ 3 ]));
                hqx7i = hqx8i = 0;
            }
        }

        while ((hqx8i < 3) && (out_first < out_last)) {
            if (prev_hqx8 == RUNCHAR) {
                if (hqx8[ hqx8i ] == 0) {
                    *out_first = prev_hqx8;
                    prev_out = prev_hqx8;
                    out_first++;
                }

                while ((out_first < out_last) && (hqx8[ hqx8i ] > 1)) {
                    *out_first = prev_out;
                    hqx8[ hqx8i ]--;
                    out_first++;
                }

                if (hqx8[ hqx8i ] < 2) {
                    prev_hqx8 = hqx8[ hqx8i ];
                    hqx8i++;
                }

                continue;
            }

            prev_hqx8 = hqx8[ hqx8i ];

            if (prev_hqx8 != RUNCHAR) {
                *out_first = prev_hqx8;
                prev_out = prev_hqx8;
                out_first++;
            }

            hqx8i++;
        }
    }

    return (out_first - outbuf);
}
