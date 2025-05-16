#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/util.h>
#include <atalk/unicode.h>
#include <atalk/zip.h>

#define MACCHARSET "MAC_ROMAN"


static void print_zones(short n, char *buf, charset_t charset);

static void usage(char *s)
{
    fprintf(stderr, "usage:\t%s [-m | -l] [-c Mac charset] [address]\n", s);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct atp_handle *ah;
    struct atp_block atpb;
    struct sockaddr_at saddr;
    struct servent *se;
    char reqdata[4], buf[ATP_MAXDATA];
    struct iovec iov;
    short temp, index = 0;
    int c, myzoneflg = 0, localzonesflg = 0, errflg = 0;
    extern int optind;
    charset_t chMac = CH_MAC;
    
    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, MACCHARSET);
    
    reqdata[0] = ZIPOP_GETZONELIST;

    while ((c = getopt(argc, argv, "mlc:")) != EOF) {
        switch (c) {
        case 'm':
            if (localzonesflg) {
                ++errflg;
            }

            ++myzoneflg;
            reqdata[0] = ZIPOP_GETMYZONE;
            break;

        case 'l':
            if (myzoneflg) {
                ++errflg;
            }

            ++localzonesflg;
            reqdata[0] = ZIPOP_GETLOCALZONES;
            break;

        case 'c':
            chMac = add_charset(optarg);
            if ((charset_t) -1 == chMac) {
                fprintf(stderr, "Invalid Mac charset.\n");
                exit(1);
            }
            break;

        default:
            ++errflg;
        }
    }


    if (errflg || argc - optind > 1) {
        usage(argv[0]);
    }

    memset(&saddr, 0, sizeof(struct sockaddr_at));
#ifdef BSD4_4
    saddr.sat_len = sizeof(struct sockaddr_at);
#endif /* BSD4_4 */
    saddr.sat_family = AF_APPLETALK;

    if ((se = getservbyname("zip", "ddp")) == NULL) {
        saddr.sat_port = 6;
    } else {
        saddr.sat_port = ntohs(se->s_port);
    }

    if (argc == optind) {
        saddr.sat_addr.s_net = ATADDR_ANYNET;
        saddr.sat_addr.s_node = ATADDR_ANYNODE;
    } else {
        if (!atalk_aton(argv[optind], &saddr.sat_addr)) {
            fprintf(stderr, "Bad address.\n");
            exit(1);
        }
    }

    if ((ah = atp_open(ATADDR_ANYPORT, NULL)) == NULL) {
        perror("atp_open");
        exit(1);
    }

    index = (myzoneflg ? 0 : 1);
    reqdata[1] = 0;

    do {
        atpb.atp_saddr = &saddr;
        temp = htons(index);
        memcpy(reqdata + 2, &temp, 2);
        atpb.atp_sreqdata = reqdata;
        atpb.atp_sreqdlen = 4;
        atpb.atp_sreqto = 2;
        atpb.atp_sreqtries = 5;

        /* send getzone request zones (or get my zone)
        */
        if (atp_sreq(ah, &atpb, 1, 0) < 0) {
            perror("atp_sreq");
            exit(1);
        }

        iov.iov_base = buf;
        iov.iov_len = ATP_MAXDATA;
        atpb.atp_rresiov = &iov;
        atpb.atp_rresiovcnt = 1;

        if (atp_rresp(ah, &atpb) < 0) {
            perror("atp_rresp");
            exit(1);
        }

        memcpy(&temp, (char *) iov.iov_base + 2, 2);
        temp = ntohs(temp);
        print_zones(temp, (char *) iov.iov_base + 4, chMac);
        index += temp;
    } while (!myzoneflg && !((char *)iov.iov_base)[0]);

    if (atp_close(ah) != 0) {
        perror("atp_close");
        exit(1);
    }

    exit(0);
}


/*
 * n:   number of zones in this packet
 * buf: zone length/name pairs
 */
static void print_zones(short n, char *buf, charset_t charset)
{
    size_t zone_len;
    char *zone;

    for (; n--; buf += (*buf) + 1) {
        if ((size_t)(-1) == (zone_len = convert_string_allocate(charset,
                                        CH_UNIX, buf + 1, *buf, &zone))) {
            zone_len = *buf;

            if ((zone = strdup(buf + 1)) == NULL) {
                perror("strdup");
                exit(1);
            }
        }

        printf("%.*s\n", (int)zone_len, zone);
        free(zone);
    }
}
