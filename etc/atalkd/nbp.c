/*
 * $Id: nbp.c,v 1.13 2009-10-14 02:24:05 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <netatalk/at.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/nbp.h>
#include <atalk/util.h>

#ifdef __svr4__
#include <sys/sockio.h>
#endif /* __svr4__ */

#include "atserv.h"
#include "interface.h"
#include "list.h"
#include "rtmp.h"
#include "gate.h"
#include "zip.h"
#include "nbp.h"
#include "multicast.h"

extern int  transition;

struct nbptab   *nbptab = NULL;

static
void nbp_ack( int fd, int nh_op, int nh_id, struct sockaddr_at *to)
{
    struct nbphdr   nh;
    char        *data, packet[ SZ_NBPHDR + 1 ];

    nh.nh_op = nh_op;
    nh.nh_cnt = 0;
    nh.nh_id = nh_id;
    data = packet;
    *data++ = DDPTYPE_NBP;
    memcpy( data, &nh, SZ_NBPHDR );
    data += SZ_NBPHDR;
    if ( sendto( fd, packet, data - packet, 0, (struct sockaddr *)to,
                 sizeof( struct sockaddr_at )) < 0 ) {
        LOG(log_error, logtype_atalkd, "sendto: %s", strerror(errno) );
    }
}

int nbp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
{
    struct nbphdr   nh;
    struct nbptuple nt;
    struct nbpnve   nn;
    struct sockaddr_at  sat;
    struct nbptab   *ntab;
    struct ziptab   *zt=NULL;
    struct interface    *iface;
    struct list     *l;
    struct rtmptab  *rtmp;
    char        *end, *nbpop, *zonep, packet[ ATP_BUFSIZ ];
    int         n, i, cc, locallkup;
    u_char      tmplen;

    end = data + len;
    if ( data >= end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet malformed packet" );
        return 1;
    }
    if ( *data++ != DDPTYPE_NBP ) {
        LOG(log_info, logtype_atalkd, "nbp_packet bad ddp type" );
        return 1;
    }

    if ( data + SZ_NBPHDR + SZ_NBPTUPLE > end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: malformed packet" );
        return 1;
    }
    memcpy( &nh, data, SZ_NBPHDR );
    nbpop = data;           /* remember for fwd and brrq */
    data += SZ_NBPHDR;
    if ( nh.nh_cnt != 1 ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: bad tuple count (%d/%d)", nh.nh_cnt,
            nh.nh_op );
        return 1;
    }

    memcpy( &nt, data, SZ_NBPTUPLE );
    data += SZ_NBPTUPLE;

    memset( &nn.nn_sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    nn.nn_sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    nn.nn_sat.sat_family = AF_APPLETALK;
    nn.nn_sat.sat_addr.s_net = nt.nt_net;
    nn.nn_sat.sat_addr.s_node = nt.nt_node;
    nn.nn_sat.sat_port = nt.nt_port;

    /* object */
    tmplen = (u_char) *data;
    if ( data >= end || tmplen > 32 || data + tmplen > end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: malformed packet" );
        return 1;
    }
    nn.nn_objlen = tmplen;
    data++;
    memcpy( nn.nn_obj, data, nn.nn_objlen );
    data += nn.nn_objlen;

    /* type */
    tmplen = (u_char) *data;
    if ( data >= end || tmplen > 32 || data + tmplen > end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: malformed packet" );
        return 1;
    }
    nn.nn_typelen = tmplen;
    data++;
    memcpy( nn.nn_type, data, nn.nn_typelen );
    data += nn.nn_typelen;

    /* zone */
    tmplen = (u_char) *data;
    if ( data >= end || tmplen > 32 || data + tmplen > end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: malformed packet" );
        return 1;
    }
    zonep = data;           /* remember for fwd */
    nn.nn_zonelen = tmplen;
    data++;
    memcpy( nn.nn_zone, data, nn.nn_zonelen );
    data += nn.nn_zonelen;

    if ( data != end ) {
        LOG(log_info, logtype_atalkd, "nbp_packet: malformed packet" );
        return 1;
    }

    locallkup = 0;
    switch ( nh.nh_op ) {

    case NBPOP_RGSTR :
        /*
         * Find the ziptab entry for the zone we're trying to register in.
         */
        if ( nn.nn_zonelen == 0 ||
             ( nn.nn_zonelen == 1 && *nn.nn_zone == '*' )) {
            if ( interfaces->i_next->i_rt->rt_zt ) {
                zt = (struct ziptab *)interfaces->i_next->i_rt->rt_zt->l_data;
            } else {
                zt = NULL;
            }
        } else {
            for ( zt = ziptab; zt; zt = zt->zt_next ) {
                if ( zt->zt_len == nn.nn_zonelen && strndiacasecmp( zt->zt_name,
                                                                    nn.nn_zone, zt->zt_len ) == 0 ) {
                    break;
                }
            }
            if ( zt == NULL ) {
                nbp_ack( ap->ap_fd, NBPOP_ERROR, (int)nh.nh_id, from );
                return 0;
            }
        }

        /*
         * Observe that we don't have to do any local-zone verification
         * if the zone aleady has a multicast address set.
         */
        if ( zt != NULL && zt->zt_bcast == NULL ) {
            /*
             * Check if zone is associated with any of our local interfaces.
             */
            for ( iface = interfaces; iface; iface = iface->i_next ) {
                for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
                    if ( zt == (struct ziptab *)l->l_data ) {
                        break;
                    }
                }
                if ( l != NULL ) {
                    break;
                }
            }
            if ( iface == NULL ) {
                nbp_ack( ap->ap_fd, NBPOP_ERROR, (int)nh.nh_id, from );
                return 0;
            }

            /* calculate and save multicast address */
            if (zone_bcast(zt) < 0) {
                LOG(log_error, logtype_atalkd, "nbp_packet: zone_bcast");
                return -1;
            }

            for ( iface = interfaces; iface; iface = iface->i_next ) {
                if (( iface->i_flags & IFACE_PHASE2 ) == 0 ) {
                    continue;
                }
                for ( l = iface->i_rt->rt_zt; l; l = l->l_next ) {
                    if ( zt == (struct ziptab *)l->l_data ) {
                        /* add multicast */
                        if (addmulti(iface->i_name, zt->zt_bcast) < 0) {
                            LOG(log_error, logtype_atalkd, "nbp_packet: addmulti: %s",
                                strerror(errno) );
                            return -1;
                        }
                    }
                }
            }
        }

        if (( ntab = (struct nbptab *)malloc( sizeof( struct nbptab )))
            == NULL ) {
            LOG(log_error, logtype_atalkd, "nbp_packet: malloc: %s", strerror(errno) );
            return -1;
        }
        memcpy( &ntab->nt_nve, &nn, sizeof( struct nbpnve ));
        ntab->nt_iface = ap->ap_iface;
        ntab->nt_next = nbptab;
        ntab->nt_prev = NULL;
        if ( nbptab ) {
            nbptab->nt_prev = ntab;
        }
        nbptab = ntab;

        nbp_ack( ap->ap_fd, NBPOP_OK, (int)nh.nh_id, from );
        break;

    case NBPOP_UNRGSTR :
        /* deal with local zone info */
        if (( nn.nn_zonelen == 1 && *nn.nn_zone == '*' ) ||
            ( nn.nn_zonelen == 0 )) {
            locallkup = 1;
            if ( interfaces->i_next->i_rt->rt_zt ) {
                zt = (struct ziptab *)interfaces->i_next->i_rt->rt_zt->l_data;
            } else {
                zt = NULL;
            }
        }

        /* remove from our data, perhaps removing a multicast address */
        for ( ntab = nbptab; ntab; ntab = ntab->nt_next ) {
            if ( ntab->nt_nve.nn_objlen != nn.nn_objlen ||
                 strndiacasecmp( ntab->nt_nve.nn_obj, nn.nn_obj,
                                 nn.nn_objlen )) {
                continue;
            }
            if ( ntab->nt_nve.nn_typelen != nn.nn_typelen ||
                 strndiacasecmp( ntab->nt_nve.nn_type, nn.nn_type,
                                 nn.nn_typelen )) {
                continue;
            }
            /*
             * I *think* we really do check the zone, here.
             *
             * i changed it to better handle local zone cases as well.
             * -- asun
             */

            /* match local zones */
            if (locallkup) {
                /* ntab is also local zone */
                if (( ntab->nt_nve.nn_zonelen == 1 &&
                      *ntab->nt_nve.nn_zone == '*' ) ||
                    (ntab->nt_nve.nn_zonelen == 0))
                    break;

                /* ntab is default zone */
                if (zt && (zt->zt_len == ntab->nt_nve.nn_zonelen) &&
                    (strndiacasecmp(ntab->nt_nve.nn_zone, zt->zt_name,
                                    zt->zt_len) == 0)) {
                    break;
                }
            }

            /* match particular zone */
            if ((ntab->nt_nve.nn_zonelen == nn.nn_zonelen) &&
                (strndiacasecmp( ntab->nt_nve.nn_zone, nn.nn_zone,
                                 nn.nn_zonelen ) == 0)) {
                break;
            }
        }
        if ( ntab == NULL ) {
            nbp_ack( ap->ap_fd, NBPOP_ERROR, (int)nh.nh_id, from );
            return 0;
        }

        if ( ntab->nt_next != NULL ) {
            ntab->nt_next->nt_prev = ntab->nt_prev;
        }
        if ( ntab->nt_prev != NULL ) {
            ntab->nt_prev->nt_next = ntab->nt_next;
        }
        if ( ntab == nbptab ) {
            nbptab = ntab->nt_next;
        }

        /*
         * Check for another nbptab entry with the same zone.  If
         * there isn't one, find the ziptab entry for the zone and
         * remove the multicast address from the appropriate interfaces.
         * XXX
         */

        nbp_ack( ap->ap_fd, NBPOP_OK, (int)nh.nh_id, from );
        break;

    case NBPOP_BRRQ :
        /*
         * Couple of things:  1. Unless we have the -t flag (which is sort
         * of a misnomer, since you need it if you're doing any phase 1
         * work), always send NBPOP_FWD.  2. If we get a zone of '*',
         * and we know what the sender meant by '*', we copy the real
         * zone into the packet.
         */
        if ( nn.nn_zonelen == 0 ||
             ( nn.nn_zonelen == 1 && *nn.nn_zone == '*' )) {
            iface = ap->ap_iface;
            if ( iface && iface->i_rt->rt_zt ) {
                zt = (struct ziptab *)iface->i_rt->rt_zt->l_data;
            } else if ( interfaces->i_next->i_rt->rt_zt ) {
                zt = (struct ziptab *)interfaces->i_next->i_rt->rt_zt->l_data;
            } else {
                zt = NULL;
            }

            /*
             * Copy zone into packet.  Note that we're changing len, data, and
             * nbpop.  Later, we'll use ( data - len ) to mean the beginning
             * of this packet.
             */
            if ( zt ) {
                memcpy( packet, data - len, len );
                nbpop = packet + ( len - ( data - nbpop ));
                data = packet + ( len - ( data - zonep ));
                *data++ = zt->zt_len;
                memcpy( data, zt->zt_name, zt->zt_len );
                data += zt->zt_len;
                len = data - packet;
            }
        } else {
            for ( zt = ziptab; zt; zt = zt->zt_next ) {
                if ( zt->zt_len == nn.nn_zonelen && strndiacasecmp( zt->zt_name,
                                                                    nn.nn_zone, zt->zt_len ) == 0 ) {
                    break;
                }
            }
            if ( zt == NULL ) {
                nbp_ack( ap->ap_fd, NBPOP_ERROR, (int)nh.nh_id, from );
                return 0;
            }
        }

        /*
         * If we've got no zones, send out LKUP on the local net.
         * Otherwise, look through the zone table.
         */
        if ( zt == NULL ) {
#ifdef BSD4_4
            sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
            sat.sat_family = AF_APPLETALK;
            sat.sat_port = ap->ap_port;

            nh.nh_op = NBPOP_LKUP;
            memcpy( nbpop, &nh, SZ_NBPHDR );
            sat.sat_addr.s_net = 0;         /* XXX */
            sat.sat_addr.s_node = ATADDR_BCAST;

            /* Find the first non-loopback ap */
            for ( iface = interfaces; iface; iface = iface->i_next ) {
                if ((( iface->i_flags & IFACE_LOOPBACK ) == 0) &&
                    (iface == ap->ap_iface ||
                     (iface->i_flags & IFACE_ISROUTER))) {
                    break;
                }
            }
            if ( iface == NULL ) {
                return 0;
            }
            for ( ap = iface->i_ports; ap; ap = ap->ap_next ) {
                if ( ap->ap_packet == nbp_packet ) {
                    break;
                }
            }
            if (ap == NULL)
                return 0;

            if ( sendto( ap->ap_fd, data - len, len, 0, (struct sockaddr *)&sat,
                         sizeof( struct sockaddr_at )) < 0 ) {
                LOG(log_error, logtype_atalkd, "nbp brrq sendto: %s", strerror(errno) );
            }

            locallkup = 1;
        } else {
#ifdef BSD4_4
            sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
            sat.sat_family = AF_APPLETALK;
            sat.sat_port = ap->ap_port;
            for ( l = zt->zt_rt; l; l = l->l_next ) {
                rtmp = (struct rtmptab *)l->l_data;

                if ( rtmp->rt_gate == NULL ) {
                    for ( iface = interfaces; iface;
                          iface = iface->i_next ) {
                        if ( iface->i_rt == rtmp ) {
                            break;
                        }
                    }
                    if ( !iface ) {
                        LOG(log_error, logtype_atalkd, "nbp_packet: \
Can't find route's interface!" );
                        return -1;
                    }
                    ap = iface->i_ports;
                } else {
                    ap = rtmp->rt_gate->g_iface->i_ports;
                }
                for ( ; ap; ap = ap->ap_next ) {
                    if ( ap->ap_packet == nbp_packet ) {
                        break;
                    }
                }
                if ( !ap ) {
                    LOG(log_error, logtype_atalkd, "nbp_packet: Can't find port!" );
                    return -1;
                }

                if ( transition &&
                     ( rtmp->rt_flags & RTMPTAB_EXTENDED ) == 0 ) {
                    if ( rtmp->rt_gate == NULL ) {
                        locallkup = 1;
                    }
                    nh.nh_op = NBPOP_LKUP;
                    memcpy( nbpop, &nh, SZ_NBPHDR );
                    sat.sat_addr.s_net = rtmp->rt_firstnet;
                    sat.sat_addr.s_node = ATADDR_BCAST;
                } else {
                    if ( rtmp->rt_gate == NULL ) {
                        nh.nh_op = NBPOP_LKUP;
                        memcpy( nbpop, &nh, SZ_NBPHDR );
                        sat.sat_addr.s_net = 0;
                        sat.sat_addr.s_node = ATADDR_BCAST;
                        locallkup = 1;
                    } else {
                        nh.nh_op = NBPOP_FWD;
                        memcpy( nbpop, &nh, SZ_NBPHDR );
                        sat.sat_addr.s_net = rtmp->rt_firstnet;
                        sat.sat_addr.s_node = 0;
                    }
                }

                if ( sendto( ap->ap_fd, data - len, len, 0,
                             (struct sockaddr *)&sat,
                             sizeof( struct sockaddr_at )) < 0 ) {
                    LOG(log_error, logtype_atalkd, "nbp brrq sendto %u.%u: %s",
                        ntohs( sat.sat_addr.s_net ), sat.sat_addr.s_node,
                        strerror(errno) );
                    continue;
                }
            }
        }

        if ( !locallkup ) {
            break;
        }
        /*FALL THROUGH*/

    case NBPOP_FWD :
        /* send lkup on net. we need to make sure we're a router. */
        if ( !locallkup && (ap->ap_iface->i_flags & IFACE_ISROUTER)) {
            nh.nh_op = NBPOP_LKUP;
            memcpy( nbpop, &nh, SZ_NBPHDR );
            from->sat_addr.s_net = 0;
            from->sat_addr.s_node = ATADDR_BCAST;
            if ( sendto( ap->ap_fd, data - len, len, 0, (struct sockaddr *)from,
                         sizeof( struct sockaddr_at )) < 0 ) {
                LOG(log_error, logtype_atalkd, "nbp fwd sendto %u.%u: %s",
                    ntohs( from->sat_addr.s_net ), from->sat_addr.s_node,
                    strerror(errno) );
                return 0;
            }
        }
        /*FALL THROUGH*/

    case NBPOP_LKUP :
        /* search our data */
        n = i = 0;
        data = packet + 1 + SZ_NBPHDR;
        end = packet + sizeof( packet );

        for ( ntab = nbptab; ntab; ntab = ntab->nt_next ) {
            /* don't send out entries if we don't want to route. */
            if ((ap->ap_iface != ntab->nt_iface) &&
                (ntab->nt_iface->i_flags & IFACE_ISROUTER) == 0) {
                continue;
            }

            if ( nn.nn_objlen != 1 || *nn.nn_obj != '=' ) {
                if ( ntab->nt_nve.nn_objlen != nn.nn_objlen ||
                     strndiacasecmp( ntab->nt_nve.nn_obj, nn.nn_obj,
                                     nn.nn_objlen )) {
                    continue;
                }
            }

            if ( nn.nn_typelen != 1 || *nn.nn_type != '=' ) {
                if ( ntab->nt_nve.nn_typelen != nn.nn_typelen ||
                     strndiacasecmp( ntab->nt_nve.nn_type, nn.nn_type,
                                     nn.nn_typelen )) {
                    continue;
                }
            }

            if ( nn.nn_zonelen != 0 &&
                 ( nn.nn_zonelen != 1 || *nn.nn_zone != '*' )) {
                if ( ntab->nt_nve.nn_zonelen == 0 ||
                     ( ntab->nt_nve.nn_zonelen == 1 &&
                       *ntab->nt_nve.nn_zone == '*' )) {
                    if ( interfaces->i_next->i_rt->rt_zt ) {
                        zt = (struct ziptab *)interfaces->i_next->i_rt->
                            rt_zt->l_data;
                        if ( zt->zt_len != nn.nn_zonelen ||
                             strndiacasecmp( zt->zt_name, nn.nn_zone,
                                             zt->zt_len )) {
                            continue;
                        }
                    }
                } else {
                    if ( ntab->nt_nve.nn_zonelen != nn.nn_zonelen ||
                         strndiacasecmp( ntab->nt_nve.nn_zone, nn.nn_zone,
                                         nn.nn_zonelen )) {
                        continue;
                    }
                }
            }

            /*
             * Another tuple won't fit. Send what we've already
             * got, and start the next packet.
             */
            if ( n > 14 || data + SZ_NBPTUPLE + 3 + ntab->nt_nve.nn_objlen +
                 ntab->nt_nve.nn_typelen + ntab->nt_nve.nn_zonelen > end ) {
                nh.nh_op = NBPOP_LKUPREPLY;
                nh.nh_cnt = n;
                cc = data - packet;
                data = packet;
                *data++ = DDPTYPE_NBP;
                memcpy( data, &nh, SZ_NBPHDR );

                if ( sendto( ap->ap_fd, packet, cc, 0,
                             (struct sockaddr *)&nn.nn_sat,
                             sizeof( struct sockaddr_at )) < 0 ) {
                    LOG(log_error, logtype_atalkd, "nbp lkup sendto %u.%u: %s",
                        ntohs( nn.nn_sat.sat_addr.s_net ),
                        nn.nn_sat.sat_addr.s_node,
                        strerror(errno) );
                    return 0;
                }

                n = 0;
                data = packet + 1 + SZ_NBPHDR;
                end = packet + sizeof( packet );
            }

            nt.nt_net = ntab->nt_nve.nn_sat.sat_addr.s_net;
            nt.nt_node = ntab->nt_nve.nn_sat.sat_addr.s_node;
            nt.nt_port = ntab->nt_nve.nn_sat.sat_port;
            /*
             * Right now, we'll just give each name a unique enum.  In
             * the future, we might need to actually assign and save
             * an enum, based on the associated address.  For the moment,
             * the enums will be unique and constant, since the order
             * is fixed.
             */
            nt.nt_enum = i++;

            memcpy( data, &nt, SZ_NBPTUPLE );
            data += SZ_NBPTUPLE;

            *data++ = ntab->nt_nve.nn_objlen;
            memcpy( data, ntab->nt_nve.nn_obj, ntab->nt_nve.nn_objlen );
            data += ntab->nt_nve.nn_objlen;

            *data++ = ntab->nt_nve.nn_typelen;
            memcpy(data, ntab->nt_nve.nn_type, ntab->nt_nve.nn_typelen );
            data += ntab->nt_nve.nn_typelen;

            /*
             * Macs won't see something with a zone of 0 length.  We
             * will always return '*' instead.  Perhaps we should
             * unconditionally return the real zone?
             */
            if ( ntab->nt_nve.nn_zonelen ) {
                *data++ = ntab->nt_nve.nn_zonelen;
                memcpy( data, ntab->nt_nve.nn_zone, ntab->nt_nve.nn_zonelen );
                data += ntab->nt_nve.nn_zonelen;
            } else {
                *data++ = 1;
                *data++ = '*';
            }

            n++;
        }

        if ( n != 0 ) {
            nh.nh_op = NBPOP_LKUPREPLY;
            nh.nh_cnt = n;
            cc = data - packet;
            data = packet;
            *data++ = DDPTYPE_NBP;
            memcpy( data, &nh, SZ_NBPHDR );

            if ( sendto( ap->ap_fd, packet, cc, 0,
                         (struct sockaddr *)&nn.nn_sat,
                         sizeof( struct sockaddr_at )) < 0 ) {
                LOG(log_error, logtype_atalkd, "nbp lkup sendto %u.%u: %s",
                    ntohs( nn.nn_sat.sat_addr.s_net ),
                    nn.nn_sat.sat_addr.s_node,
                    strerror(errno) );
                return 0;
            }
        }
        break;

    default :
        LOG(log_info, logtype_atalkd, "nbp_packet: bad op (%d)", nh.nh_op );
        return 1;
    }

    return 0;
}
