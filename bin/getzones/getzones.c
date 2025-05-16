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
#include <atalk/ddp.h>
#include <atalk/util.h>
#include <atalk/unicode.h>
#include <atalk/zip.h>

#define MACCHARSET "MAC_ROMAN"
#define ZIPOP_DEFAULT ZIPOP_GETZONELIST

static void print_zones(short n, char *buf, charset_t charset);
void do_atp_lookup(struct sockaddr_at *saddr, uint8_t lookup_type, charset_t charset);

static void usage(char *s)
{
    fprintf(stderr, "usage:\t%s [-m | -l] [-c Mac charset] [address]\n", s);
    exit(1);
}

int main(int argc, char *argv[])
{
    struct sockaddr_at saddr;
    struct servent *se;
    int c, errflg = 0;
    extern int optind;
    charset_t chMac = CH_MAC;
    uint8_t lookup_type = ZIPOP_DEFAULT;

    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, MACCHARSET);

    while ((c = getopt(argc, argv, "mlc:")) != EOF) {
        switch (c) {
        case 'm':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            lookup_type = ZIPOP_GETMYZONE;
            break;

        case 'l':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            lookup_type = ZIPOP_GETLOCALZONES;
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

    do_atp_lookup(&saddr, lookup_type, chMac);
}

void do_atp_lookup(struct sockaddr_at *saddr, uint8_t lookup_type, charset_t charset) {
    struct atp_handle *ah;
    struct atp_block atpb;
    char reqdata[4], buf[ATP_MAXDATA];
    struct iovec iov;
    uint16_t start_index = 0;

    if ((ah = atp_open(ATADDR_ANYPORT, NULL)) == NULL) {
        perror("atp_open");
        exit(1);
    }

    reqdata[0] = lookup_type;
    reqdata[1] = 0;

    /* We need to set the "starting index" in the query we're going to send out.
       This allows for a kind of pagination; you ask for zones starting at zone 1 and
       you get a reply's worth.  Say you get three zones back.  You then add three to
       your starting index and ask for the list of zones starting at 4.  Repeat until
       you get a reply with the "last reply" flag set.
       Yes, this is racy, and will not give the correct results in a network where
       the zone state has not converged.  We can't do much about that now... */
    start_index = 1;
    if (lookup_type == ZIPOP_GETMYZONE) {
        /* However, for some reason best known to Apple circa 1992, GetMyZone requires
           the index set to 0 (see Inside Appletalk, p. 8-15). */
        start_index = 0;
    }

    do {
        atpb.atp_saddr = saddr;

        /* Put the start index into the packet */
        uint16_t start_idx_for_packet = htons(start_index);
        memcpy(reqdata + 2, &start_idx_for_packet, 2);

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

        /* Print the zones out */
        uint16_t zone_count_returned;
        memcpy(&zone_count_returned, (char *) iov.iov_base + 2, 2);
        zone_count_returned = ntohs(zone_count_returned);
        print_zones(zone_count_returned, (char *) iov.iov_base + 4, charset);

        /* If we're doing a GetMyZone request, we can just bail out now; there will
           never be more than one zone returned, so we don't need to ask for the next
           "page". */
        if (lookup_type == ZIPOP_GETMYZONE) {
            break;
        }

        /* Otherwise, we need to add the number of zones returned to the start index
           and loop.  The loop condition here is checking the LastFlag field of the
           reply, which is 0 if there are more zones to come, and 1 otherwise. */
        start_index += zone_count_returned; 
    } while (!((char *)iov.iov_base)[0]);

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
