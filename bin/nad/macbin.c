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
#include <sys/param.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

#include <atalk/adouble.h>
#include <atalk/util.h>
#include <netatalk/endian.h>
#include "nad.h"
#include "megatron.h"
#include "macbin.h"
#include "crc16.h"

/* This allows nad to generate .bin files that won't choke other
   well-known converter apps. It also makes sure that checksums
   always match. (RLB) */
#define MACBINARY_PLAY_NICE_WITH_OTHERS

/*	String used to indicate standard input instead of a disk
	file.  Should be a string not normally used for a file
 */
#ifndef	STDIN
#	define	STDIN	"-"
#endif /* STDIN */

/*	Yes and no
 */
#define NOWAY		0
#define SURETHANG	1

/*	Size of a macbinary file header
 */
#define HEADBUFSIZ	128
#define MACBIN_NAMELEN	63

/*	Both input and output routines use this struct and the
	following globals; therefore this module can only be used
	for one of the two functions at a time.
 */
static struct bin_file_data {
    uint32_t		forklen[ NUMFORKS ];
    char		path[ MAXPATHLEN + 1];
    int			filed;
    int                 owns_fd;
    off_t               filepos;
    unsigned short		headercrc;
} 		bin;

extern char	*forkname[];
static unsigned char	head_buf[HEADBUFSIZ];

static uint16_t macbin_u16_from_bytes(unsigned char high, unsigned char low)
{
    unsigned int value = ((unsigned int)high << 8) | (unsigned int)low;
    return (uint16_t)value;
}

static uint32_t macbin_u32_from_header(const unsigned char *header,
                                       size_t offset)
{
    return ((uint32_t)header[offset] << 24)
           | ((uint32_t)header[offset + 1] << 16)
           | ((uint32_t)header[offset + 2] << 8)
           | (uint32_t)header[offset + 3];
}

static uint16_t macbin_fdflags_from_header(const unsigned char *header,
        int revision)
{
    unsigned char flags_low = 0;

    if (revision >= 2) {
        flags_low = header[101];
#ifdef MACBINARY_PLAY_NICE_WITH_OTHERS

        /* Some compatibility-oriented writers mirror the low byte at 74. */
        if (flags_low == 0) {
            flags_low = header[74];
        }

#endif /* MACBINARY_PLAY_NICE_WITH_OTHERS */
    }

    return htons(macbin_u16_from_bytes(header[73], flags_low));
}

static void macbin_fdflags_to_header(unsigned char *header, uint16_t fdflags)
{
    uint16_t flags = ntohs(fdflags);
    header[73] = (unsigned char)(flags >> 8);
#ifdef MACBINARY_PLAY_NICE_WITH_OTHERS
    header[74] = (unsigned char)(flags & 0xff);
#endif /* MACBINARY_PLAY_NICE_WITH_OTHERS */
    header[101] = (unsigned char)(flags & 0xff);
}

static int macbin_ad_date_from_header(uint32_t mac_date, uint32_t *ad_date)
{
    *ad_date = AD_DATE_FROM_MAC(mac_date);
    return set_utc_offset(ad_date, TO_UTC);
}

static int macbin_ad_date_to_header(uint32_t ad_date, uint32_t *mac_date)
{
    if (set_utc_offset(&ad_date, TO_LOCALTIME) != 0) {
        return -1;
    }

    *mac_date = AD_DATE_TO_MAC(ad_date);
    return 0;
}

static int macbin_discard_bytes(size_t length)
{
    unsigned char discard[HEADBUFSIZ];

    while (length > 0) {
        size_t readlen = length < sizeof(discard) ? length : sizeof(discard);
        ssize_t cc = read(bin.filed, discard, readlen);

        if (cc < 0) {
            perror("Couldn't skip MacBinary padding:");
            return -1;
        }

        if (cc == 0) {
            fprintf(stderr,
                    "Premature end of file while skipping MacBinary padding.\n");
            return -1;
        }

        length -= (size_t)cc;
        bin.filepos += cc;
    }

    return 0;
}

static int macbin_write_repeat(unsigned char value, size_t length)
{
    unsigned char buffer[HEADBUFSIZ];
    memset(buffer, value, sizeof(buffer));

    while (length > 0) {
        size_t writelen = length < sizeof(buffer) ? length : sizeof(buffer);
        ssize_t cc = write(bin.filed, buffer, writelen);

        if (cc < 0) {
            perror("Couldn't write to macbinary file:");
            return -1;
        }

        if (cc == 0) {
            fprintf(stderr, "Couldn't write to macbinary file: short write\n");
            return -1;
        }

        length -= (size_t)cc;
        bin.filepos += cc;
    }

    return 0;
}

static int macbin_skip_padding(void)
{
    off_t pos = lseek(bin.filed, 0, SEEK_CUR);
    int seekable = 0;
    size_t padding;

    if (pos >= 0) {
        seekable = 1;
        bin.filepos = pos;
    } else {
        pos = bin.filepos;
    }

#if DEBUG
    NAD_DEBUG("current position is %ld\n", pos);
#endif /* DEBUG */
    pos %= HEADBUFSIZ;

    if (pos == 0) {
        return 0;
    }

    padding = (size_t)(HEADBUFSIZ - pos);

    if (seekable) {
        pos = lseek(bin.filed, padding, SEEK_CUR);

        if (pos >= 0) {
            bin.filepos = pos;
#if DEBUG
            NAD_DEBUG("current position is %ld\n", pos);
#endif /* DEBUG */
            return 0;
        }
    }

    if (macbin_discard_bytes(padding) < 0) {
        return -1;
    }

#if DEBUG
    NAD_DEBUG("current position is %ld\n", bin.filepos);
#endif /* DEBUG */
    return 0;
}

static int macbin_write_padding(void)
{
    unsigned char padchar = 0x7f;
    /* Not sure why, but it seems this must be 0x7f to match
       other converters, not 0. (RLB) */
    off_t pos = lseek(bin.filed, 0, SEEK_CUR);
    size_t padding;

    if (pos >= 0) {
        bin.filepos = pos;
    } else {
        pos = bin.filepos;
    }

#if DEBUG
    NAD_DEBUG("current position is %ld\n", pos);
#endif /* DEBUG */
    pos %= HEADBUFSIZ;

    if (pos == 0) {
        return 0;
    }

    padding = (size_t)(HEADBUFSIZ - pos);

    if (macbin_write_repeat(padchar, padding) < 0) {
        return -1;
    }

#if DEBUG
    NAD_DEBUG("current position is %ld\n", bin.filepos);
#endif /* DEBUG */
    return 0;
}

/*!
 * @brief Open a MacBinary file for reading or writing
 *
 * This must be called before other MacBinary operations. When opening
 * for reading, it validates and reads the MacBinary header. When opening
 * for writing, it initializes output state and writes the header.
 *
 * @param[in] binfile    input file path, or `-` for standard input
 * @param[in] flags      open flags; `O_RDONLY` selects read mode
 * @param[in,out] fh     file header to read or write
 * @param[in] options    output options, such as `OPTION_STDOUT`
 *
 * @returns 0 on success, -1 on error
 */
int bin_open(char *binfile, int flags, struct FHeader *fh, int options)
{
    int			rc;
#if DEBUG
    NAD_DEBUG("entering bin_open\n");
#endif /* DEBUG */
    bin.filed = -1;
    bin.owns_fd = 0;

    if (flags == O_RDONLY) {   /* input */
        bin.filepos = 0;

        if (strcmp(binfile, STDIN) == 0) {
            bin.filed = fileno(stdin);
        } else if ((bin.filed = open(binfile, flags)) < 0) {
            perror(binfile);
            return -1;
        } else {
            bin.owns_fd = 1;
        }

#if DEBUG
        NAD_DEBUG("opened %s for read\n", binfile);
#endif /* DEBUG */

        if (((rc = test_header()) > 0) &&
                (bin_header_read(fh, rc) == 0)) {
            return 0;
        }

        fprintf(stderr, "%s is not a macbinary file.\n", binfile);

        if (bin.owns_fd && bin.filed >= 0) {
            close(bin.filed);
        }

        bin.filed = -1;
        bin.owns_fd = 0;
        return -1;
    } else { /* output */
        bin.filepos = 0;

        if (options & OPTION_STDOUT) {
            bin.filed = fileno(stdout);
            strlcpy(bin.path, STDIN, sizeof(bin.path));
        } else {
#if DEBUG
            NAD_DEBUG("sizeof bin.path\t\t\t%d\n", sizeof(bin.path));
#endif /* DEBUG */

            if (strlcpy(bin.path, fh->name, sizeof(bin.path)) >= sizeof(bin.path)
                    || strlcpy(bin.path, mtoupath(bin.path), sizeof(bin.path))
                    >= sizeof(bin.path)
                    || strlcat(bin.path, ".bin", sizeof(bin.path))
                    >= sizeof(bin.path)) {
                fprintf(stderr, "Output path is too long: %s.bin\n", fh->name);
                return -1;
            }

            if ((bin.filed = open(bin.path, flags, 0666)) < 0) {
                perror(bin.path);
                return -1;
            }

            bin.owns_fd = 1;
#if DEBUG
            NAD_DEBUG("opened %s for write\n",
                      (options & OPTION_STDOUT) ? "(stdout)" : bin.path);
#endif /* DEBUG */
        }

        if (bin_header_write(fh) != 0) {
            bin_close(TRASH);
            fprintf(stderr, "%s\n", bin.path);
            return -1;
        }

        return 0;
    }
}

/*!
 * @brief Close the active MacBinary file
 *
 * This must be called before opening another file with bin_open().
 * `KEEP` closes the file and preserves it. `TRASH` closes the file and
 * removes incomplete output.
 *
 * @param[in] keepflag    `KEEP` to keep the output, `TRASH` to discard it
 *
 * @returns 0 on success, -1 on error
 */
int bin_close(int keepflag)
{
#if DEBUG
    NAD_DEBUG("entering bin_close\n");
#endif /* DEBUG */

    if (keepflag == KEEP) {
        if (bin.owns_fd && bin.filed >= 0) {
            int rc = close(bin.filed);
            bin.filed = -1;
            bin.owns_fd = 0;
            return rc;
        }

        bin.filed = -1;
        return 0;
    } else if (keepflag == TRASH) {
        if (bin.owns_fd && bin.filed >= 0) {
            close(bin.filed);
        }

        bin.filed = -1;
        bin.owns_fd = 0;

        if ((strcmp(bin.path, STDIN) != 0) &&
                (unlink(bin.path) < 0)) {
            perror(bin.path);
        }

        return 0;
    } else {
        return -1;
    }
}

const char *bin_path(void)
{
    return bin.path;
}

/*!
 * @brief Read data from a MacBinary fork
 *
 * Call this until it returns zero for each fork. When the data fork is
 * complete, it skips MacBinary padding before the resource fork begins.
 *
 * @note bin_read() must be called enough times to return zero, and no
 * more than that, for each fork.
 *
 * @param[in] fork       fork selector, `DATA` or `RESOURCE`
 * @param[out] buffer    destination buffer
 * @param[in] length     maximum number of bytes to read
 *
 * @returns number of bytes read, 0 when the fork is complete, -1 on error
 */
ssize_t bin_read(int fork, char *buffer, size_t length)
{
    char		*buf_ptr;
    size_t		readlen;
    ssize_t		cc = 1;
#if DEBUG >= 3
    NAD_DEBUG("bin_read: fork is %s\n", forkname[ fork ]);
    NAD_DEBUG("bin_read: remaining length is %d\n", bin.forklen[fork]);
#endif /* DEBUG >= 3 */

    if (bin.forklen[fork] > 0x7FFFFFFF) {
        fprintf(stderr, "Invalid %s fork length: %u\n",
                forkname[fork],
                bin.forklen[fork]);
        return -1;
    }

    if (bin.forklen[ fork ] == 0) {
        if (fork == DATA && macbin_skip_padding() < 0) {
            return -1;
        }

        return 0;
    }

    if (bin.forklen[ fork ] < length) {
        readlen = bin.forklen[ fork ];
    } else {
        readlen = length;
    }

#if DEBUG >= 3
    NAD_DEBUG("bin_read: readlen is %d\n", readlen);
    NAD_DEBUG("bin_read: cc is %d\n", cc);
#endif /* DEBUG >= 3 */
    buf_ptr = buffer;

    while ((readlen > 0) && (cc > 0)) {
        if ((cc = read(bin.filed, buf_ptr, readlen)) > 0) {
#if DEBUG >= 3
            NAD_DEBUG("bin_read: cc is %d\n", cc);
#endif /* DEBUG >= 3 */
            readlen -= cc;
            buf_ptr += cc;
            bin.filepos += cc;
        }
    }

    if (cc >= 0) {
        cc = buf_ptr - buffer;
        bin.forklen[ fork ] -= cc;
    }

#if DEBUG >= 3
    NAD_DEBUG("bin_read: chars read is %d\n", cc);
#endif /* DEBUG >= 3 */
    return cc;
}

/*!
 * @brief Write data to a MacBinary fork
 *
 * Data must be written in fork order. The resource fork cannot be
 * written until the data fork is complete. Padding is written when each
 * fork is finished.
 *
 * @param[in] fork       fork selector, `DATA` or `RESOURCE`
 * @param[in] buffer     source buffer
 * @param[in] length     number of bytes to write
 *
 * @returns number of bytes written, -1 on error
 */
ssize_t bin_write(int fork, char *buffer, size_t length)
{
    const char		*buf_ptr;
    ssize_t		writelen;
    ssize_t		cc = 0;
#if DEBUG >= 3
    NAD_DEBUG("bin_write: fork is %s\n", forkname[ fork ]);
    NAD_DEBUG("bin_write: remaining length is %d\n", bin.forklen[fork]);
#endif /* DEBUG >= 3 */

    if ((fork == RESOURCE) && (bin.forklen[ DATA ] != 0)) {
        fprintf(stderr, "Forklength error.\n");
        return -1;
    }

    buf_ptr = buffer;

    if (bin.forklen[ fork ] >= length) {
        writelen = length;
    } else {
        fprintf(stderr, "Forklength error.\n");
        return -1;
    }

#if DEBUG >= 3
    NAD_DEBUG("bin_write: write length is %d\n", writelen);
#endif /* DEBUG >= 3 */

    while (writelen > 0) {
        cc = write(bin.filed, buf_ptr, writelen);

        if (cc <= 0) {
            break;
        }

        buf_ptr += cc;
        writelen -= cc;
        bin.filepos += cc;
    }

    if (writelen > 0) {
        if (cc < 0) {
            perror("Couldn't write to macbinary file:");
            return cc;
        }

        fprintf(stderr, "Couldn't write to macbinary file: short write\n");
        return -1;
    }

    bin.forklen[fork] -= length;

    /*
     * add the padding at end of data and resource forks
     */

    if (bin.forklen[ fork ] == 0 && macbin_write_padding() < 0) {
        return -1;
    }

#if DEBUG
    NAD_DEBUG("\n");
#endif /* DEBUG */
    return length;
}

/*!
 * @brief Read a MacBinary header into a file header
 *
 * This is called by bin_open() before any fork data is read. It validates
 * the MacBinary revision, decodes header fields, and initializes fork
 * lengths.
 *
 * @param[out] fh         file header to populate
 * @param[in] revision    MacBinary revision returned by test_header()
 *
 * @returns 0 on success, -1 on error
 */
int bin_header_read(struct FHeader *fh, int revision)
{
    /*
     * Check the MacBinary revision before reading fields from the header.
     * If it is not a macbinary file revision of I, II, or III, then return
     * negative.
     */
    switch (revision) {
    case 3:
    case 2:
    case 1:
        break;

    default:
        return -1;
    }

    /*
     * Go through and copy all the stuff you can get from the
     * MacBinary header into the fh struct.  What fun!
     */
    if (head_buf[1] >= sizeof(fh->name)) {
        return -1;
    }

    memcpy(fh->name, head_buf +  2, head_buf[ 1 ]);
    fh->name[head_buf[1]] = '\0';
    memcpy(&fh->create_date, head_buf +  91, 4);

    if (macbin_ad_date_from_header(fh->create_date, &fh->create_date) != 0) {
        return -1;
    }

    memcpy(&fh->mod_date, head_buf +  95, 4);

    if (macbin_ad_date_from_header(fh->mod_date, &fh->mod_date) != 0) {
        return -1;
    }

    fh->backup_date = AD_DATE_START;
    fh->date_flags = FH_DATE_CREATE | FH_DATE_MODIFY;
    memcpy(&fh->finder_info, head_buf +  65, 8);
    fh->finder_info.fdFlags = macbin_fdflags_from_header(head_buf, revision);
    memcpy(&fh->finder_info.fdLocation, head_buf + 75, 4);
    memcpy(&fh->finder_info.fdFldr, head_buf +  79, 2);
    memcpy(&fh->forklen[ DATA ],  head_buf + 83, 4);
    bin.forklen[ DATA ] = ntohl(fh->forklen[ DATA ]);
    memcpy(&fh->forklen[ RESOURCE ],  head_buf +  87, 4);
    bin.forklen[ RESOURCE ] = ntohl(fh->forklen[ RESOURCE ]);
    fh->comment[0] = '\0';

    if (revision == 3) {
        fh->finder_xinfo.fdScript = *(head_buf + 106);
        fh->finder_xinfo.fdXFlags = *(head_buf + 107);
    }

#if DEBUG >= 5
    {
        short		flags;
        long		flags_long;
        NAD_DEBUG("Values read by bin_header_read\n");
        NAD_DEBUG("name length\t\t%d\n", head_buf[ 1 ]);
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
        /* Show fdLocation too (RLB) */
        memcpy(&flags_long, &fh->finder_info.fdLocation,
               sizeof(flags_long));
        flags_long = ntohl(flags_long);
        NAD_DEBUG("location flags\t\t%lx\n", flags_long);
        NAD_DEBUG("data fork length\t%ld\n", bin.forklen[DATA]);
        NAD_DEBUG("resource fork length\t%ld\n", bin.forklen[RESOURCE]);
        NAD_DEBUG("\n");
    }
#endif /* DEBUG >= 5 */
    return 0;
}

/*!
 * @brief Write a file header as a MacBinary header
 *
 * This is called by bin_open() before any fork data is written. It
 * encodes the file header as a MacBinary III header and initializes fork
 * lengths.
 *
 * @param[in] fh    file header to write
 *
 * @returns 0 on success, -1 on error
 */
int bin_header_write(struct FHeader *fh)
{
    const char		*write_ptr;
    uint32_t           t;
    size_t             namelen;
    size_t             wc;
    ssize_t		wr;
    memset(head_buf, 0, sizeof(head_buf));
    namelen = strnlen(fh->name, sizeof(fh->name));

    if (namelen > MACBIN_NAMELEN) {
        fprintf(stderr, "MacBinary filename is too long: %s\n", fh->name);
        return -1;
    }

    head_buf[ 1 ] = (unsigned char)namelen;
    memcpy(head_buf + 2, fh->name, head_buf[ 1 ]);
    memcpy(head_buf + 65, &fh->finder_info, 8);
    macbin_fdflags_to_header(head_buf, fh->finder_info.fdFlags);
    memcpy(head_buf + 75, &fh->finder_info.fdLocation, 4);
    memcpy(head_buf + 79, &fh->finder_info.fdFldr, 2);
    memcpy(head_buf + 83, &fh->forklen[ DATA ], 4);
    memcpy(head_buf + 87, &fh->forklen[ RESOURCE ], 4);

    if (macbin_ad_date_to_header(fh->create_date, &t) != 0) {
        return -1;
    }

    memcpy(head_buf + 91, &t, sizeof(t));

    if (macbin_ad_date_to_header(fh->mod_date, &t) != 0) {
        return -1;
    }

    memcpy(head_buf + 95, &t, sizeof(t));
    /* macbinary III */
    memcpy(head_buf + 102, "mBIN", 4);
    *(head_buf + 106) = fh->finder_xinfo.fdScript;
    *(head_buf + 107) = fh->finder_xinfo.fdXFlags;
    head_buf[ 122 ] = 130;
    head_buf[ 123 ] = 129;
    bin.headercrc = htons(crc16_xmodem_update(0, head_buf, 124));
    memcpy(head_buf + 124, &bin.headercrc, sizeof(bin.headercrc));
    bin.forklen[ DATA ] = ntohl(fh->forklen[ DATA ]);
    bin.forklen[ RESOURCE ] = ntohl(fh->forklen[ RESOURCE ]);
#if DEBUG >= 5
    {
        short	flags;
        long	flags_long;
        NAD_DEBUG("Values written by bin_header_write\n");
        NAD_DEBUG("name length\t\t%d\n", head_buf[ 1 ]);
        NAD_DEBUG("file name\t\t%s\n", (char *)&head_buf[ 2 ]);
        NAD_DEBUG("type\t\t\t%.4s\n", (char *)&head_buf[ 65 ]);
        NAD_DEBUG("creator\t\t\t%.4s\n", (char *)&head_buf[ 69 ]);
        memcpy(&flags, &fh->finder_info.fdFlags, sizeof(flags));
        flags = ntohs(flags);
        NAD_DEBUG("flags\t\t\t%x\n", flags);
        /* Show fdLocation too (RLB) */
        memcpy(&flags_long, &fh->finder_info.fdLocation,
               sizeof(flags_long));
        flags_long = ntohl(flags_long);
        NAD_DEBUG("location flags\t\t%ldx\n", flags_long);
        NAD_DEBUG("data fork length\t%ld\n", bin.forklen[DATA]);
        NAD_DEBUG("resource fork length\t%ld\n", bin.forklen[RESOURCE]);
        NAD_DEBUG("\n");
    }
#endif /* DEBUG >= 5 */
    write_ptr = (char *)head_buf;
    wc = sizeof(head_buf);
    wr = 0;

    while (wc > 0) {
        wr = write(bin.filed, write_ptr, wc);

        if (wr <= 0) {
            break;
        }

        write_ptr += (size_t)wr;
        wc -= (size_t)wr;
        bin.filepos += wr;
    }

    if (wr < 0) {
        perror("Couldn't write macbinary header:");
        return -1;
    }

    if (wc > 0) {
        fprintf(stderr, "Couldn't write complete macbinary header.\n");
        return -1;
    }

    return 0;
}

/*!
 * @brief Test the input header for a supported MacBinary revision
 *
 * Reads the first 128 bytes and determines whether the file is
 * MacBinary, MacBinary II, MacBinary III, or not a MacBinary file.
 *
 * @note Apple's MacBinary II files can have a non-zero value at byte 74,
 * so the byte 74 check is not very useful.
 *
 * @returns 1 for MacBinary, 2 for MacBinary II, 3 for MacBinary III,
 * -1 if the header is invalid
 */
int test_header(void)
{
    const char          zeros[25] = "";
    ssize_t		cc;
    uint32_t            forklen;
    unsigned short		header_crc;
    unsigned char		namelen;
#if DEBUG
    NAD_DEBUG("entering test_header\n");
#endif /* DEBUG */
    cc = read(bin.filed, (char *)head_buf, sizeof(head_buf));

    if (cc < 0) {
        perror("Couldn't read MacBinary header:");
        return -1;
    }

    if ((size_t)cc < sizeof(head_buf)) {
        fprintf(stderr,
                "Premature end of file while reading MacBinary header.\n");
        return -1;
    }

    bin.filepos = cc;
#if DEBUG
    NAD_DEBUG("was able to read HEADBUFSIZ bytes\n");
#endif /* DEBUG */

    /* check for macbinary III header */
    if (memcmp(head_buf + 102, "mBIN", 4) == 0) {
        return 3;
    }

    /* check for macbinary II even if only one of the bytes is zero */
    if ((head_buf[ 0 ] == 0) || (head_buf[ 74 ] == 0)) {
#if DEBUG
        NAD_DEBUG("byte 0 and 74 are both zero\n");
#endif /* DEBUG */
        bin.headercrc = crc16_xmodem_update(0, head_buf, 124);
        memcpy(&header_crc, head_buf + 124, sizeof(header_crc));
        header_crc = ntohs(header_crc);

        if (header_crc == bin.headercrc) {
            return 2;
        }

#if DEBUG
        NAD_DEBUG("header crc didn't pan out\n");
#endif /* DEBUG */
    }

    /* now see if we have a macbinary file. */
    if (head_buf[ 82 ] != 0) {
        return -1;
    }

    memcpy(&namelen, head_buf + 1, sizeof(namelen));
#if DEBUG
    NAD_DEBUG("name length is %d\n", namelen);
#endif /* DEBUG */

    if ((namelen < 1) || (namelen > MACBIN_NAMELEN)) {
        return -1;
    }

    /* bytes 101 - 125 should be zero */
    if (memcmp(head_buf + 101, zeros, sizeof(zeros)) != 0) {
        return -1;
    }

    /* macbinary forks aren't larger than 0x7FFFFF */
    /* we allow forks to be larger, breaking the specs */
    forklen = macbin_u32_from_header(head_buf, 83);

    if (forklen > 0x7FFFFFFFU) {
        return -1;
    }

    forklen = macbin_u32_from_header(head_buf, 87);

    if (forklen > 0x7FFFFFFFU) {
        return -1;
    }

#if DEBUG
    NAD_DEBUG("byte 82 is zero and name length is valid\n");
#endif /* DEBUG */
    return 1;
}
