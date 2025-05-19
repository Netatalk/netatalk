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
#include <atalk/netddp.h>
#include <atalk/util.h>
#include <atalk/unicode.h>
#include <atalk/zip.h>

#define MACCHARSET "MAC_ROMAN"
#define ZIPOP_DEFAULT ZIPOP_GETZONELIST

static void print_zones(short n, char *buf, charset_t charset);
void do_atp_lookup(struct sockaddr_at *saddr, uint8_t lookup_type, charset_t charset);
static void print_gnireply(size_t len, uint8_t *buf, charset_t charset);
void do_getnetinfo(struct sockaddr_at *dest, char *zone_to_confirm, charset_t charset);
static int print_and_count_zones_in_reply(char *buf, size_t len, charset_t charset);
void do_query(struct sockaddr_at *dest, uint16_t network, charset_t charset);

static void usage(char *s)
{
    fprintf(stderr, "usage:\t%s [-g | -l | -m | -q network | -z zone ] [-c Mac charset] [address]\n", s);
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
    char* requested_zone = NULL;
    long requested_network = 0;
    char *end_of_digits;

    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, MACCHARSET);

    while ((c = getopt(argc, argv, "glmc:q:z:")) != EOF) {
        switch (c) {
        case 'g':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            lookup_type = ZIPOP_GNI;
            break;

        case 'l':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            lookup_type = ZIPOP_GETLOCALZONES;
            break;
            
        case 'm':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            lookup_type = ZIPOP_GETMYZONE;
            break;

        case 'c':
            chMac = add_charset(optarg);
            if ((charset_t) -1 == chMac) {
                fprintf(stderr, "Invalid Mac charset.\n");
                exit(1);
            }
            break;
            
        case 'q':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            requested_network = strtol(optarg, &end_of_digits, 10);
            if (*end_of_digits != '\0') {
                fprintf(stderr, "Network must be a number between 0 and 65535.\n");
                exit(1);
            }
            if (requested_network < 0 || requested_network > 65535) {
                fprintf(stderr, "Network must be a number between 0 and 65535.\n");
                exit(1);
            }
            lookup_type = ZIPOP_QUERY;
            break;
            
        case 'z':
            if (lookup_type != ZIPOP_DEFAULT) {
                ++errflg;
            }
            requested_zone = strdup(optarg);
            lookup_type = ZIPOP_GNI;
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

    switch(lookup_type) {
    case ZIPOP_GETZONELIST:
    case ZIPOP_GETMYZONE:
    case ZIPOP_GETLOCALZONES:
        do_atp_lookup(&saddr, lookup_type, chMac);
        break;
        
    case ZIPOP_GNI:
        do_getnetinfo(&saddr, requested_zone, chMac);
        break;
        
    case ZIPOP_QUERY:
        do_query(&saddr, (uint16_t)requested_network, chMac);
        break;
    }
    
    if (requested_zone != NULL) {
        free(requested_zone);
    }
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
            
            /* If the string conversion fails, just copy the MacRoman string
               and hope it makes sense as whatever our UNIX charset is */
            zone_len = *buf;
            zone = malloc(zone_len+1);
            if (zone == NULL) {
                perror("malloc");
                exit(1);
            }
            memcpy(zone, buf+1, zone_len);
            zone[zone_len] = 0;
        }

        printf("%.*s\n", (int)zone_len, zone);
        free(zone);
    }
}

void do_getnetinfo(struct sockaddr_at *dest, char *zone_to_confirm, charset_t charset) {
    uint8_t buf[600];
    uint8_t *cursor;
    int i;
    struct sockaddr_at laddr = { 0 };
    
    cursor = buf;
    
    /* Construct packet. Layout is in IA, 2nd ed. p. 8-17 */
    *cursor++ = DDPTYPE_ZIP;
    *cursor++ = ZIPOP_GNI;
    for (i = 0; i < 5; i++) {
        *cursor++ = 0;
    }
    
    if (zone_to_confirm != NULL) {
        size_t zonelen = strnlen(zone_to_confirm, 32);
        *cursor++ = (uint8_t)zonelen;
        memcpy(cursor, zone_to_confirm, zonelen);
        cursor += zonelen;
    } else {
        /* If we aren't trying to find out about a specific zone, give an empty name */
        *cursor++ = 0;
    }
    
    /* Send the thing */
    int sockfd = netddp_open(&laddr, NULL);
    if (sockfd < 0) {
        perror("netddp_open");
        exit(1);
    }
    
    int ret = netddp_sendto(sockfd, buf, cursor - buf, 0, (struct sockaddr*)dest,
        sizeof(struct sockaddr_at));
    if (ret < 0) {
        perror("netddp_sendto");
        exit(1);
    }
    
    /* Wait for a reply */
    for(;;) {
        ret = netddp_recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
        if (ret < 0) {
            perror("netddp_recvfrom");
            exit(1);
        }
        
        /* Is this actually a ZIP GNI reply or some random packet that's just wandered
           into our socket? */
        if (ret < 2) {
            continue;
        }
        if (buf[0] != DDPTYPE_ZIP || buf[1] != ZIPOP_GNIREPLY) {
            continue;
        }
                
        break;
    }
    
    print_gnireply(ret, buf, charset);   
}

static void print_gnireply(size_t len, uint8_t *buf, charset_t charset) {
    int ret;
    uint8_t *cursor;
    
    /* Is the packet long enough for our flags and stuff? */
    if (len < 9) {
        fprintf(stderr, "getnetinfo reply too short, bailing out\n");
        exit(1);
    }

    /* Fish out the fixed-length metadata. */
    cursor = buf;
    /* skip over DDP type and ZIP operation, which we checked above */
    cursor += 2; 
    
    uint8_t flags = *cursor++;
    uint16_t net_start = ntohs(*((uint16_t*)cursor));
    cursor += 2;
    uint16_t net_end = ntohs(*((uint16_t*)cursor));
    cursor += 2;
    
    printf("Network range: %"PRIu16"-%"PRIu16"\n", net_start, net_end);
        
    /* Decode the flags */
    printf("Flags (0x%"PRIx8"):", flags);
    if (flags & ZIPGNI_INVALID) {
        printf(" requested-zone-invalid");
    }
    if (flags & ZIPGNI_USEBROADCAST) {
        printf(" use-broadcast");
    }
    if (flags & ZIPGNI_ONEZONE) {
        printf(" only-one-zone");
    }
    printf("\n");
    
    /* Print out the zone requested */
    uint8_t requested_zone_length = *cursor++;
    if ((cursor - buf) + requested_zone_length >= len) {
        fprintf(stderr, "getnetinfo reply too short (missing requested zone), bailing out\n");
        exit(1);
    }
    char* requested_zone;
    ret = convert_string_allocate(charset, CH_UNIX, cursor, requested_zone_length,
                                  &requested_zone);
    if (ret == -1) {
        fprintf(stderr, "malformed requested zone name, bailing out\n");
        exit(1);
    }
    printf("Requested zone: %s\n", requested_zone);
    cursor += ret;
    free(requested_zone);
    
    /* Print out multicast address */
    uint8_t multicast_length = *cursor++;
    if ((cursor - buf) + multicast_length > len) {
        fprintf(stderr, "getnetinfo reply too short (missing multicast address), bailing out\n");
        exit(1);
    }
    
    printf("Zone multicast address: ");
    bool first_octet = true;
    for (int i = 0; i < multicast_length; i++) {
        if (!first_octet) {
            printf(":");
        }
        printf("%02"PRIx8, *cursor++);
        first_octet = false;
    }
    printf("\n");
    
    /* Do we have a default zone? */
    uint8_t default_zone_len = 0;
    if ((cursor - buf) <= len) {
        default_zone_len = *cursor++;
    }
    
    if (default_zone_len > 0 && (cursor - buf) + default_zone_len > len) {
        fprintf(stderr, "getnetinfo reply too short (insufficient room for default zone), bailing out\n");
        exit(1);
    }
    
    if (default_zone_len > 0) {
        /* Yes! (We will never have a legit zone name with len = 0, it's not allowed). */
        char* default_zone_name;
        ret = convert_string_allocate(charset, CH_UNIX, cursor, default_zone_len,
                                      &default_zone_name);
        if (ret == -1) {
            fprintf(stderr, "malformed default zone name, bailing out\n");
            exit(1);
        }
        printf("Default zone: %s\n", default_zone_name);
        free(default_zone_name);
    }
    
    /* If we're trying to verify a specific zone, then bail out with exit status 2 if
       the zone is invalid. */
    if (requested_zone != NULL && (flags & ZIPGNI_INVALID)) {
        exit(2);
    }
}

void do_query(struct sockaddr_at *dest, uint16_t network, charset_t charset) {
    uint8_t buf[600];
    uint8_t *cursor;
    int i;
    struct sockaddr_at laddr = { 0 };
    uint8_t reply_type;
    int replied_zone_count = 0;
    int expected_zone_count = 0;
    
    cursor = buf;
    
    /* Construct packet. Layout is in IA, 2nd ed. p. 8-12 */
    *cursor++ = DDPTYPE_ZIP;
    *cursor++ = ZIPOP_QUERY;
    /* Network count = 1; we only support querying one network at a time */
    *cursor++ = 1; 
    *((uint16_t*)cursor) = htons(network);
    cursor += 2;
    
    /* Send the thing */
    int sockfd = netddp_open(&laddr, NULL);
    if (sockfd < 0) {
        perror("netddp_open");
        exit(1);
    }
    
    int length = netddp_sendto(sockfd, buf, cursor - buf, 0, (struct sockaddr*)dest,
        sizeof(struct sockaddr_at));
    if (length < 0) {
        perror("netddp_sendto");
        exit(1);
    }
    
    for(;;) {
        length = netddp_recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
        if (length < 0) {
            perror("netddp_recvfrom");
            exit(1);
        }
        
        /* Is this a ZIP reply or extended reply?  If not, it's just some random
           packet that's fallen into our socket; ignore it.
           The 3 bytes here are: DDP type, ZIP Op, Num of networks */
        if (length < 3) {
            continue;
        }
        if (buf[0] != DDPTYPE_ZIP) {
            continue;
        }
        reply_type = buf[1];
        if (reply_type != ZIPOP_REPLY && reply_type != ZIPOP_EREPLY) {
            continue;
        }
                
        if (reply_type == ZIPOP_REPLY) {
            /* A reply contains all the zones in one packet; we can bail out.
               Ignore the network count; we only asked for one network. */
            print_and_count_zones_in_reply(buf + 3, length - 3, charset);
            break;
        } else {
            /* An extended reply may be spread over more than one packet; we
               need to keep track of how many we've seen and only exit once we've
               seen enough zones. */
            
            if (expected_zone_count == 0) {
                expected_zone_count = buf[2];
            }
            replied_zone_count += print_and_count_zones_in_reply(buf + 3, length - 3, charset);
            if (replied_zone_count >= expected_zone_count) {
                break;
            }
        }
    }
}

static int print_and_count_zones_in_reply(char *buf, size_t len, charset_t charset) {
    char *cursor = buf;
    int count = 0;
    
    while (cursor - buf < len) {
        uint16_t network_number = 0;
        uint8_t zone_len = 0;
        
        /* Proceed carefully; we don't know how many zones are in this packet, so we
           just have to iterate until we run out of packet to iterate over. */
        
        /* Can we read a network number without falling off the end of the packet? */
        if ((cursor + 2) - buf <= len) {
            network_number = ntohs(*(uint16_t*)cursor);
            cursor += 2;
        } else {
            break;
        }
        
        /* What about a string length? */
        if ((cursor + 1) - buf <= len) {
            zone_len = *cursor++;
        } else {
            break;
        }
        
        /* How about the zone name? */
        if ((cursor + zone_len) - buf <= len) {
            char* unix_zone_name;
            int ret = convert_string_allocate(charset, CH_UNIX, cursor, zone_len,
                                              &unix_zone_name);
            if (ret == -1) {
                fprintf(stderr, "malformed default zone name, bailing out\n");
                exit(1);
            }
            
            /* Hooray!  We got a whole tuple.  Print it, quick, before it gets away. */
            printf("%"PRIu16" %s\n", network_number, unix_zone_name);
            free(unix_zone_name);
            
        } else {
            break;
        }
        
        count++;
    }
    
    return count;
}
