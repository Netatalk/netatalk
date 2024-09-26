/*
 * (c) 2013, 1997 Stefan Bethke. All rights reserved.
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
#include <sys/errno.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <stdio.h>
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

#define ROUTE_DEL 0
#define ROUTE_ADD 1

static int tunnel_ifconfig(void)
{
	char cmd[2048], addr[256], mask[256], net[256];

	bzero(cmd, sizeof(cmd));
	bzero(addr, sizeof(addr));
	bzero(mask, sizeof(mask));
	bzero(net, sizeof(net));
	strncpy(addr, iptoa(gTunnel.net + 1), sizeof(addr) - 1);
	strncpy(mask, iptoa(gTunnel.mask), sizeof(mask) - 1);
	strncpy(net, iptoa(gTunnel.net), sizeof(net) - 1);
	snprintf(cmd, sizeof(cmd),
		 "/sbin/ifconfig %s inet %s mtu %d up pointopoint",
		 gTunnel.name, addr, 586);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_ifconfig: %s\n", cmd);
	return system(cmd);
}


static int tunnel_route(int op, uint32_t net, uint32_t mask, uint32_t gw)
{
	char cmd[2048], saddr[256], smask[256], snet[256];

	bzero(cmd, sizeof(cmd));
	bzero(saddr, sizeof(saddr));
	bzero(smask, sizeof(smask));
	bzero(snet, sizeof(snet));
	strncpy(saddr, iptoa(gw), sizeof(saddr) - 1);
	strncpy(smask, iptoa(mask), sizeof(smask) - 1);
	strncpy(snet, iptoa(net), sizeof(snet) - 1);
	snprintf(cmd, sizeof(cmd),
		 "/sbin/route %s -net %s gw %s netmask %s",
		 op == ROUTE_ADD ? "add" : "delete", snet, saddr, smask);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_route: %s\n", cmd);
	return system(cmd);
}

int tunnel_open(uint32_t net, uint32_t mask, outputfunc_t o)
{
	struct ifreq ifr;
	int err;

	if ((gTunnel.dev = open("/dev/net/tun", O_RDWR)) < 0) {
		perror("open(/dev/net/tun)");
		return -1;
	}

	bzero(&ifr, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	if ((err = ioctl(gTunnel.dev, TUNSETIFF, (void *) &ifr)) < 0) {
		close(gTunnel.dev);
		perror("ioctl(tun, TUNSETIFF)");
		return -1;
	}

	strcpy(gTunnel.name, ifr.ifr_name);

	gTunnel.net = net;
	gTunnel.mask = mask;

	if (tunnel_ifconfig() < 0) {
		return -1;
	}

	tunnel_route(ROUTE_DEL, gTunnel.net, gTunnel.mask,
		     gTunnel.net + 1);
	if (tunnel_route
	    (ROUTE_ADD, gTunnel.net, gTunnel.mask, gTunnel.net + 1) < 0) {
		return -1;
	}

	gOutput = o;

	return gTunnel.dev;
}


void tunnel_close(void)
{
	close(gTunnel.dev);
	tunnel_route(ROUTE_DEL, gTunnel.net, gTunnel.mask,
		     gTunnel.net + 1);
}


void tunnel_input(void)
{
	char buffer[600];
	int i;

	i = read(gTunnel.dev, buffer, sizeof(buffer));
	if (i < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror("tunnel_input: read");
		return;
	}
	if (gDebug & DEBUG_TUNNEL)
		printf("read packet from tunnel.\n");
	gOutput(buffer, i);
}


void tunnel_output(char *buffer, int len)
{
	if (write(gTunnel.dev, buffer, len) < 0)
		fprintf(stderr, "tunnel_output() write failed\n");
	if (gDebug & DEBUG_TUNNEL)
		printf("sent packet into tunnel.\n");
}
#endif				/* __linux__ */
