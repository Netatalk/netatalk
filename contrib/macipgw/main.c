/*
 * AppleTalk MacIP Gateway
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
 * (c) 2025 Daniel Markstedt. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/globals.h>

#include <errno.h>
#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
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

int atsocket;
int tundev;
int debug = 0;
static char	*version = VERSION;

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
		"Usage:\tmacipgw [-d debug] [-f configfile] [-n nameserver] [-u unprivileged-user] [-z zone] [-V]\n"
		"\t\tmacip-net macip-netmask\n");
	exit(EX_USAGE);
}

struct passwd * get_user(const char *username) {
	struct passwd *pwd;
	pwd = getpwnam(username);
	if (pwd == NULL) {
		fprintf(stderr, "Unrecognized username: %s\n", username);
		exit (EX_USAGE);
	}
	return pwd;
}

macip_options * read_options(const char *conf)
{
    dictionary *config;
    macip_options *options = (macip_options *)malloc(sizeof(macip_options));

    if ((config = iniparser_load(conf)) == NULL)
        return NULL;
    options->network = INIPARSER_GETSTRDUP(config, INISEC_GLOBAL, "network", "");
    options->netmask = INIPARSER_GETSTRDUP(config, INISEC_GLOBAL, "netmask", "");
    options->nameserver = INIPARSER_GETSTRDUP(config, INISEC_GLOBAL, "nameserver", "");
    options->zone = INIPARSER_GETSTRDUP(config, INISEC_GLOBAL, "zone", "");
    options->unprivileged_user = INIPARSER_GETSTRDUP(config, INISEC_GLOBAL, "unprivileged user", "");

	iniparser_freedict(config);

    return options;
}


int main(int argc, char *argv[])
{
	struct sigaction sv;
	uint32_t net = 0;
	uint32_t mask = 0;
	uint32_t ns = 0;
	char *zone = "*";
	const char *conffile = _PATH_MACIPGWCONF;
	int opt;
	macip_options *mio;

	struct passwd *pwd = NULL;
	uid_t user;
	gid_t group;

	gDebug = 0;

    while ((opt = getopt(argc, argv, "Vvd:f:n:u:z:")) != -1) {
        switch (opt) {
		case 'd':
			gDebug = strtol(optarg, 0, 0);
			break;
		case 'f':
			conffile = optarg;
			if (strlen(conffile) > MAXPATHLEN)
				usage("not a valid config file path.");
			break;
		case 'n':
			ns = atoip(optarg);
			break;
		case 'z':
			zone = optarg;
            break;
        case 'V':
        case 'v':
            fprintf(stdout, "macipgw %s - Mac IP Gateway Daemon\n"
               "Copyright (c) 1997, 2013 Stefan Bethke. All rights reserved.\n"
               "Copyright (c) 1988, 1992, 1993\n"
               "\tThe Regents of the University of California.  All rights reserved.\n"
               "Copyright (c) 1990, 1996 Regents of The University of Michigan.\n"
               "\tAll Rights Reserved.\n"
               "See the file COPYRIGHT for further information.\n", version);
            exit(1);
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

	if (argc == 2) {
		net = atoip(argv[0]);
		mask = atoip(argv[1]);
	}

	mio = read_options(conffile);

	if (mio != NULL) {
		if ((mio->network[0] != '\0') && (net == 0)) {
			if (gDebug & DEBUG_MACIP)
				printf("set network to %s\n", mio->network);
			net = atoip(mio->network);
		}
		if ((mio->netmask[0] != '\0') && (mask == 0)) {
			if (gDebug & DEBUG_MACIP)
				printf("set netmask to %s\n", mio->netmask);
			mask = atoip(mio->netmask);
		}
		if ((mio->nameserver[0] != '\0') &&  (ns == 0)) {
			if (gDebug & DEBUG_MACIP)
				printf("set nameserver to %s\n", mio->nameserver);
			ns = atoip(mio->nameserver);
		}
		if ((mio->zone[0] != '\0') && (! strcmp(zone, "*"))) {
			if (gDebug & DEBUG_MACIP)
				printf("set zone to %s\n", mio->zone);
			zone = mio->zone;
		}
		if ((mio->unprivileged_user[0] != '\0') && (pwd == NULL)) {
			if (gDebug & DEBUG_MACIP)
				printf("set unprivileged_user to %s\n", mio->unprivileged_user);
			pwd = get_user(mio->unprivileged_user);
			user = pwd->pw_uid;
			group = pwd->pw_gid;
		}
	}

	net &= mask;
	if ((net & mask) == 0)
		usage("please supply a valid network address and netmask");

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

	if (mio != NULL) {
		if (mio->network) {
			CONFIG_ARG_FREE(mio->network)
		}
		if (mio->netmask) {
			CONFIG_ARG_FREE(mio->netmask)
		}
		if (mio->nameserver) {
			CONFIG_ARG_FREE(mio->nameserver)
		}
		if (mio->zone) {
			CONFIG_ARG_FREE(mio->zone)
		}
		if (mio->unprivileged_user) {
			CONFIG_ARG_FREE(mio->unprivileged_user)
		}
	}

	server();

	return 0;
}
