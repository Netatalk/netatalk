/*
 * AppleTalk MacIP Gateway
 *
 * $Id: main.c,v 1.1.1.1 2001/10/28 15:01:49 stefanbethke Exp $
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
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
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/uio.h>

#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sysexits.h>

#include "common.h"
#include "macip.h"
#include "tunnel.h"
#include "util.h"

static char *version = "macipgw 1.1\n"
    "Copyright (c) 1997 Stefan Bethke. All rights reserved.\n"
    "Copyright (c) 1988, 1992, 1993\n"
    "\tThe Regents of the University of California.  All rights reserved.\n"
    "Copyright (c) 1990,1996 Regents of The University of Michigan.\n"
    "\tAll Rights Reserved.\n"
    "See the file COPYRIGHT for further information.\n";

int atsocket;
int tundev;
int debug = 0;

static void die(int n)
{
	syslog(LOG_INFO, "going down on signal");
	if (gDebug)
		fprintf(stderr, "Stopping\n");
	macip_close();
	tunnel_close();
	if (gDebug)
		fprintf(stderr, "\nstopped.\n");
	exit(n);
}


static void server(void)
{
	fd_set fds;
	int maxfd = 0;
	struct timeval tv;
	int i;

	maxfd = atsocket;
	maxfd = MAX(maxfd, tundev);
	maxfd++;
	while (1) {
		FD_ZERO(&fds);
		FD_SET(atsocket, &fds);
		FD_SET(tundev, &fds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if (gDebug) {
			printf("waiting for packet: ");
			fflush(stdout);
		}
		if ((i = select(maxfd, &fds, 0, 0, &tv)) > 0) {
			if (FD_ISSET(atsocket, &fds)) {
				if (gDebug)
					printf("got AT packet.\n");
				macip_input();
			} else if (FD_ISSET(tundev, &fds)) {
				if (gDebug)
					printf
					    ("got IP packet from tunnel.\n");
				tunnel_input();
			}
		} else {
			printf("\r");
			if (i == 0) {
				macip_idle();
			} else {
				if (gDebug)
					perror("select");
			}
		}
	}
}


void disassociate(void)
{
	int i, dt;

	switch (fork()) {
	case 0:
		dt = getdtablesize();
		for (i = 0; i < dt; i++) {
			(void) close(i);
		}
		if ((i = open("/dev/tty", O_RDWR)) >= 0) {
			(void) ioctl(i, TIOCNOTTY, 0);
			setpgid(0, getpid());
			(void) close(i);
		}
		if ((i = open("/", O_RDONLY)) >= 0) {
			dup2(i, 1);
			dup2(i, 2);
		}
		break;
	case -1:
		perror("fork");
		die(EX_OSERR);
	default:
		exit(0);
	}
}


void usage(char *c)
{
	if (c)
		fprintf(stderr, "%s\n", c);
	fprintf(stderr,
		"Usage:\tmacipgw [-d debug] [-z zone] [-n nameserver] [-V]\n"
		"\t\tmacip-net macip-netmask\n");
	exit(EX_USAGE);
}


int main(int argc, char *argv[])
{
	struct sigaction sv;
	uint32_t net = 0, mask = 0, ns = 0;
	char *zone = "*";
	char c;

	gDebug = 0;

	while ((c = getopt(argc, argv, "d:n:z:V")) != EOF) {
		switch (c) {
		case 'd':
			gDebug = strtol(optarg, 0, 0);
			break;

		case 'n':
			ns = atoip(optarg);
			break;

		case 'z':
			zone = optarg;
			break;
		case 'V':
			usage(version);
			break;

		default:
			usage("unknown option.");
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 2)
		usage("wrong number of parameters.");
	net = atoip(argv[0]);
	mask = atoip(argv[1]);
	net &= mask;
	if ((net & mask) == 0)
		usage("invalid ip address.");

	openlog("macipgw", LOG_PID | gDebug ? LOG_PERROR : 0, LOG_DAEMON);

	sv.sa_handler = die;
	sigemptyset(&sv.sa_mask);
	sv.sa_flags = 0;
	if (sigaction(SIGTERM, &sv, 0) < 0) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(EX_OSERR);
	}
	if (sigaction(SIGQUIT, &sv, 0) < 0) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(EX_OSERR);
	}
	if (sigaction(SIGINT, &sv, 0) < 0) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(EX_OSERR);
	}

	if (!gDebug)
		disassociate();

	tundev = tunnel_open(net, mask, macip_output);
	if (tundev < 0) {
		syslog(LOG_ERR, "could not open tunnel.\n");
		die(EX_OSERR);
	}

	atsocket = macip_open(zone, net, mask, ns, tunnel_output);
	if (atsocket < 0) {
		syslog(LOG_ERR, "could not initialise MacIP\n");
		die(EX_OSERR);
	}

	server();

	return 0;
}
