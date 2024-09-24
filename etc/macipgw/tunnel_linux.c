/*
 * (c) 2013, 1997 Stefan Bethke. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
	int		dev;
	char	name[32];
	uint32_t	net;
	uint32_t	mask;
};

static struct tunnel gTunnel;

static outputfunc_t	gOutput;

#define ROUTE_DEL 0
#define ROUTE_ADD 1

static int
tunnel_ifconfig (void) {
	char cmd[2048], addr[256], mask[256], net[256];

	bzero(cmd, sizeof(cmd));
	bzero(addr, sizeof(addr));
	bzero(mask, sizeof(mask));
	bzero(net, sizeof(net));
	strncpy(addr, iptoa(gTunnel.net+1), sizeof(addr)-1);
	strncpy(mask, iptoa(gTunnel.mask), sizeof(mask)-1);
	strncpy(net, iptoa(gTunnel.net), sizeof(net)-1);
	snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s inet %s mtu %d up pointopoint",
		gTunnel.name, addr, 586);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_ifconfig: %s\n", cmd);
	return system(cmd);
}


static int
tunnel_route (int op, uint32_t net, uint32_t mask, uint32_t gw) {
	char cmd[2048], saddr[256], smask[256], snet[256];

	bzero(cmd, sizeof(cmd));	
	bzero(saddr, sizeof(saddr));	
	bzero(smask, sizeof(smask));	
	bzero(snet, sizeof(snet));	
	strncpy(saddr, iptoa(gw), sizeof(saddr)-1);
	strncpy(smask, iptoa(mask), sizeof(smask)-1);
	strncpy(snet, iptoa(net), sizeof(snet)-1);
	snprintf(cmd, sizeof(cmd), "/sbin/route %s -net %s gw %s netmask %s",
		op == ROUTE_ADD ? "add" : "delete", snet, saddr, smask);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_route: %s\n", cmd);
	return system(cmd);
}

int
tunnel_open (uint32_t net, uint32_t mask, outputfunc_t o) {
	struct ifreq ifr;
	int err;

	if( (gTunnel.dev = open("/dev/net/tun", O_RDWR)) < 0 ) {
		perror("open(/dev/net/tun)");
		return -1;
	}

	bzero(&ifr, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	if( (err = ioctl(gTunnel.dev, TUNSETIFF, (void *) &ifr)) < 0 ) {
		close(gTunnel.dev);
		perror("ioctl(tun, TUNSETIFF)");
		return -1;
	}

	strcpy (gTunnel.name, ifr.ifr_name);
	
	gTunnel.net   = net;
	gTunnel.mask  = mask;

	if (tunnel_ifconfig () < 0) {
		return -1;
	}
	
	tunnel_route (ROUTE_DEL, gTunnel.net, gTunnel.mask, gTunnel.net+1);
	if (tunnel_route (ROUTE_ADD, gTunnel.net, gTunnel.mask, gTunnel.net+1) < 0) {
		return -1;
	}

	gOutput = o;

	return gTunnel.dev;
}


void
tunnel_close (void) {
	char cmd[2048];
	
	close(gTunnel.dev);
	tunnel_route (ROUTE_DEL, gTunnel.net, gTunnel.mask, gTunnel.net+1);
}


void tunnel_input (void) {
	char	buffer[600];
	int		i;
	
	i = read (gTunnel.dev, buffer, sizeof (buffer));
	if (i < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_input: read");
		return;
	}
	if (gDebug & DEBUG_TUNNEL)
		printf ("read packet from tunnel.\n");
	gOutput (buffer, i);
}


void tunnel_output (char *buffer, int len) {
	write (gTunnel.dev, buffer, len);
	if (gDebug & DEBUG_TUNNEL)
		printf ("sent packet into tunnel.\n");
}
