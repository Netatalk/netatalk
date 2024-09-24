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

#if defined(BSD)
#include <net/if.h>
#include <net/if_tun.h>
#endif

#if defined(__linux__)
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

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

	strlcpy(addr, iptoa(gTunnel.net+1), sizeof(addr));
	strlcpy(mask, iptoa(gTunnel.mask), sizeof(mask));
	strlcpy(net, iptoa(gTunnel.net), sizeof(net));
	snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s inet %s %s up",
		gTunnel.name, addr, net);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_ifconfig: %s\n", cmd);
	return system(cmd);
}


static int
tunnel_route (int op, uint32_t net, uint32_t mask, uint32_t gw) {
	char cmd[2048], saddr[256], smask[256], snet[256];

	strlcpy(saddr, iptoa(gw), sizeof(saddr));
	strlcpy(smask, iptoa(mask), sizeof(smask));
	strlcpy(snet, iptoa(net), sizeof(snet));
	snprintf(cmd, sizeof(cmd), "/sbin/route %s -net %s %s %s",
		op == ROUTE_ADD ? "add" : "delete", snet, saddr, smask);
	if (gDebug & DEBUG_TUNNEL)
		fprintf(stderr, "tunnel_route: %s\n", cmd);
	return system(cmd);
}

int
tunnel_open (uint32_t net, uint32_t mask, outputfunc_t o) {
	int					i;
	char				s[32], *q;
#if !defined(__NetBSD__)
	struct tuninfo		ti;
#endif
	
	gTunnel.dev = 0;
	for (i=0; i<=9; i++) {
		sprintf (s, "/dev/tun%d", i);
		gTunnel.dev = open (s, O_RDWR);
		if (gTunnel.dev)
			break;
	}
	if (gTunnel.dev < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_open: open");
		return -1;
	}

	q = rindex (s, '/');
	if (!q)
		return -1;
	q++;
	strcpy (gTunnel.name, q);
	if (gDebug & DEBUG_TUNNEL) {
		printf ("tunnel_open: %s:%s opened.\n", s, gTunnel.name);
	}

#if !defined(__NetBSD__)	
	ti.type = 23;
	ti.mtu  = 586;		/*	DDP_MAXSZ - 1	*/
	ti.baudrate = 38400;
	if (ioctl (gTunnel.dev, TUNSIFINFO, &ti) < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_open: TUNSIFINFO");
		return -1;
	}
#endif

	i = gDebug & DEBUG_TUNDEV;
	if (ioctl (gTunnel.dev, TUNSDEBUG, &i) < 0) {
		if (gDebug & DEBUG_TUNDEV)
			perror ("tunnel_open: TUNSDEBUG");
		return -1;
	}
	
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
	int		i, sk;
	struct ifreq		ifrq;
	struct ifaliasreq	ifra;
	struct sockaddr_in	*sin;
	
	i = 0;
	if (ioctl (gTunnel.dev, TUNSDEBUG, &i) < 0)
	{	if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_close: TUNSIFINFO");
	}

	sk = socket (PF_INET, SOCK_DGRAM, 0);
	if (sk < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_close: socket");
		return;
	}
	bzero (&ifrq, sizeof (ifrq));
	strncpy (ifrq.ifr_name,  gTunnel.name, IFNAMSIZ);
	strncpy (ifra.ifra_name, gTunnel.name, IFNAMSIZ);

	if (ioctl (sk, SIOCGIFFLAGS, &ifrq) < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_close: SIOCGIFFLAGS");
		close (sk);
		return;
	}
	ifrq.ifr_flags &= ~IFF_UP;
	if (ioctl (sk, SIOCSIFFLAGS, &ifrq) < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_close: SIOCSIFFLAGS");
		close (sk);
		return;
	}

	bzero (&ifra.ifra_addr,      sizeof (*sin));
	bzero (&ifra.ifra_broadaddr, sizeof (*sin));
	bzero (&ifra.ifra_mask,      sizeof (*sin));
	if (ioctl (sk, SIOCDIFADDR, &ifra) < 0) {
		if (gDebug & DEBUG_TUNNEL)
			perror ("tunnel_close: SIOCDIFADDR");
		close (sk);
		return;
	}
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
