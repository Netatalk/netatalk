/*
 * (c) 2013, 1997 Stefan Bethke. All rights reserved.
 * (c) 2015 Jason King. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if defined(__linux__)
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/route.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "util.h"
#include "tunnel.h"


struct tunnel {
    int dev;
    char name[32];
    uint32_t net;
    uint32_t mask;
};

static struct tunnel gTunnel;

static outputfunc_t gOutput;

static void set_sin(struct sockaddr *s, uint32_t ip) {
    struct sockaddr_in *sin = (struct sockaddr_in *)s;
    bzero(sin, sizeof(*sin));
    sin->sin_family      = AF_INET;
    sin->sin_addr.s_addr = htonl(ip);
}

static int tunnel_ifconfig(void) {
    int sk;
    struct ifreq ifrq;
    printf("device = %s\n", gTunnel.name);
    sk = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sk < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: socket");
        }

        return -1;
    }

    bzero(&ifrq, sizeof(ifrq));
    strncpy(ifrq.ifr_name,  gTunnel.name, IFNAMSIZ);

    if (ioctl(sk, SIOCGIFFLAGS, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: SIOCGIFFLAGS");
        }

        close(sk);
        return -1;
    }

    ifrq.ifr_flags |= IFF_UP;

    if (ioctl(sk, SIOCSIFFLAGS, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: SIOCSIFFLAGS");
        }

        close(sk);
        return -1;
    }

    ifrq.ifr_mtu = 586;

    if (ioctl(sk, SIOCSIFMTU, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: SIOCSIFMTU");
        }

        close(sk);
        return -1;
    }

    set_sin(&ifrq.ifr_addr,      gTunnel.net + 1);

    if (ioctl(sk, SIOCSIFADDR, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: SIOCSIFADDR");
        }

        close(sk);
        return -1;
    }

    set_sin(&ifrq.ifr_addr,      gTunnel.mask);

    if (ioctl(sk, SIOCSIFNETMASK, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_ifconfig: SIOCSIFNETMASK");
        }

        close(sk);
        return -1;
    }

    if (gDebug & DEBUG_TUNNEL) {
        printf("tunnel_ifconfig: setting address %s -> ", iptoa(gTunnel.net + 1));
        printf("%s netmask ", iptoa(gTunnel.net));
        printf("%s\n", iptoa(gTunnel.mask));
    }

    close(sk);
    return 0;
}

static int tunnel_create(char *dev, int flags) {
    struct ifreq ifr;
    int fd;
    char *tundev = "/dev/net/tun";

    if ((fd = open(tundev, O_RDWR)) < 0) {
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_alloc: TUNSETIFF");
        }

        close(fd);
        return -1;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}


int tunnel_open(uint32_t net, uint32_t mask, outputfunc_t o) {
    int i;
    char s[32];
    gTunnel.dev = 0;

    for (i = 0; i <= 9; i++) {
        sprintf(s, "tun%d", i);
        gTunnel.dev = tunnel_create(s, IFF_TUN | IFF_NO_PI);

        if (gTunnel.dev > 0) {
            break;
        }
    }

    if (gTunnel.dev < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_open: open");
        }

        return -1;
    }

    strcpy(gTunnel.name, s);

    if (gDebug & DEBUG_TUNNEL) {
        printf("tunnel_open: %s:%s opened.\n", s, gTunnel.name);
    }

    gTunnel.net   = net;
    gTunnel.mask  = mask;

    if (tunnel_ifconfig() < 0) {
        return -1;
    }

    gOutput = o;
    return gTunnel.dev;
}


void tunnel_close(void) {
    int sk;
    struct ifreq ifrq;
    sk = socket(PF_INET, SOCK_DGRAM, 0);

    if (sk < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_close: socket");
        }

        return;
    }

    bzero(&ifrq, sizeof(ifrq));
    strncpy(ifrq.ifr_name,  gTunnel.name, IFNAMSIZ);

    if (ioctl(sk, SIOCGIFFLAGS, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_close: SIOCGIFFLAGS");
        }

        close(sk);
        return;
    }

    ifrq.ifr_flags &= ~IFF_UP;

    if (ioctl(sk, SIOCSIFFLAGS, &ifrq) < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_close: SIOCSIFFLAGS");
        }

        close(sk);
        return;
    }

    close(gTunnel.dev);
}


void tunnel_input(void) {
    char buffer[600];
    int i;
    i = read(gTunnel.dev, buffer, sizeof(buffer));

    if (i < 0) {
        if (gDebug & DEBUG_TUNNEL) {
            perror("tunnel_input: read");
        }

        return;
    }

    if (gDebug & DEBUG_TUNNEL) {
        printf("read packet from tunnel.\n");
    }

    gOutput(buffer, i);
}


void tunnel_output(char *buffer, int len) {
    if (write(gTunnel.dev, buffer, len) < 0) {
        fprintf(stderr, "tunnel_output() write failed\n");
    }

    if (gDebug & DEBUG_TUNNEL) {
        printf("sent packet into tunnel.\n");
    }
}
#endif				/* __linux__ */
