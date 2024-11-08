/*
 * AppleTalk MacIP Gateway
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sysexits.h>
#include <pwd.h>

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
		"Usage:\tmacipgw [-d debug] [-z zone] [-n nameserver] [-u unprivileged-user] [-V]\n"
		"\t\tmacip-net macip-netmask\n");
	exit(EX_USAGE);
}

struct passwd * get_user(const char *username) {
	struct passwd *pwd;
	pwd = getpwnam(username);
	if (pwd == NULL) {
		fprintf(stderr, "Unrecognized username: %s", username);
		exit (EX_USAGE);
	}
	return pwd;
}


int main(int argc, char *argv[])
{
	struct sigaction sv;
	uint32_t net = 0, mask = 0, ns = 0;
	char *zone = "*";
    int opt;

	struct passwd *pwd = NULL;
	uid_t user;
	gid_t group;

	gDebug = 0;

    while ((opt = getopt(argc, argv, "Vd:n:u:z:")) != -1) {
        switch (opt) {
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
		case 'u':
			pwd = get_user(optarg);
			user = pwd->pw_uid;
			group = pwd->pw_gid;
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

	if (pwd) {
		if (setgid(user) == -1) {
			printf("macipgw: could not drop root privileges (group)\n");
			die(EX_OSERR);
		}
		if (setuid(group) == -1) {
			printf("macipgw: could not drop root privileges (user)\n");
			die(EX_OSERR);
		}
	}

	server();

	return 0;
}
