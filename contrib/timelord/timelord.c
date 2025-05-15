/*
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * The "timelord protocol" was reverse engineered from Timelord,
 * distributed with CAP, Copyright (c) 1990, The University of
 * Melbourne.  The following copyright, supplied by The University
 * of Melbourne, may apply to this code:
 *
 *	This version of timelord.c is based on code distributed
 *	by the University of Melbourne as part of the CAP package.
 *
 *	The tardis/Timelord package for Macintosh/CAP is
 *	Copyright (c) 1990, The University of Melbourne.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <stdlib.h>

#include <unistd.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/atp.h>
#include <atalk/nbp.h>

#include <time.h>
#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif /* HAVE_SGTTY_H */
#include <errno.h>
#include <signal.h>
#include <atalk/logger.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#include <fcntl.h>

#include <termios.h>
#include <sys/ioctl.h>

#define	TL_GETTIME	0
#define	TL_OK		12
#define TL_BAD		10
#define EPOCH		0x7C25B080	/* 00:00:00 GMT Jan 1, 1970 for Mac */
#define SIZEOF_MAC_LONG 4

int	debug = 0;
char	*bad = "Bad request!";
char	buf[4624];
char	*server;
static char	*version = VERSION;

void usage(char *p) {
    char	*s;

    if ((s = rindex(p, '/')) == NULL) {
        s = p;
    } else {
        s++;
    }

    fprintf(stderr, "Usage:\t%s -d -l -n nbpname\n", s);
    exit(1);
}

/*
 * Unregister ourself on signal.
 */
void
goaway(int signal) {
    if (nbp_unrgstr(server, "TimeLord", "*", NULL) < 0) {
        LOG(log_error, logtype_default, "Can't unregister %s", server);
        exit(1);
    }

    LOG(log_info, logtype_default, "going down");
    exit(0);
}

int main(int ac, char **av) {
    ATP			atp;
    struct sockaddr_at	sat;
    struct atp_block	atpb;
    struct timeval	tv;
    struct iovec	iov;
    struct tm		tm;
    char		hostname[MAXHOSTNAMELEN];
    char		*p;
    int			c;
    int         lflag = 0;
    long		req, mtime, resp;
    extern char		*optarg;
    extern int		optind;

    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("gethostname");
        exit(1);
    }

    if ((server = index(hostname, '.')) != 0) {
        *server = '\0';
    }

    server = hostname;

    while ((c = getopt(ac, av, "dlVvn:")) != EOF) {
        switch (c) {
        case 'd' :
            debug++;
            break;

        case 'l':
            lflag = 1;
            break;

        case 'n' :
            server = optarg;
            break;

        case 'V' :
        case 'v' :
            fprintf(stdout, "timelord %s - Timelord Time Server Daemon\n"
                            "Copyright (c) 1990,1992 Regents of The University of Michigan.\n"
                            "\tAll Rights Reserved.\n"
                            "Copyright (c) 1990, The University of Melbourne.\n", version);
            exit(0);
            break;

        default :
            fprintf(stderr, "Unknown option -- '%c'\n", c);
            usage(*av);
        }
    }

    /*
     * Disassociate from controlling tty.
     */
    if (!debug) {
        int		i, dt;

        switch (fork()) {
        case 0 :
            dt = getdtablesize();

            for (i = 0; i < dt; i++) {
                (void)close(i);
            }

            if ((i = open("/dev/tty", O_RDWR)) >= 0) {
                (void)ioctl(i, TIOCNOTTY, 0);
                setpgid(0, getpid());
                (void)close(i);
            }

            break;

        case -1 :
            perror("fork");
            exit(1);

        default :
            exit(0);
        }
    }

    if ((p = rindex(*av, '/')) == NULL) {
        p = *av;
    } else {
        p++;
    }

    set_processname(p);
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid,
                 logfacility_daemon);
    /* allocate memory */
    memset(&sat.sat_addr, 0, sizeof(sat.sat_addr));

    if ((atp = atp_open(ATADDR_ANYPORT, &sat.sat_addr)) == NULL) {
        LOG(log_error, logtype_default, "main: atp_open: %s", strerror(errno));
        exit(1);
    }

    if (nbp_rgstr(atp_sockaddr(atp), server, "TimeLord", "*") < 0) {
        LOG(log_error, logtype_default, "Can't register %s", server);
        exit(1);
    }

    LOG(log_info, logtype_default, "%s:TimeLord started", server);

    if (lflag == 1) {
        LOG(log_info, logtype_default, "Time zone is localtime");
    } else {
        LOG(log_info, logtype_default, "Time zone is GMT");
    }

    signal(SIGHUP, goaway);
    signal(SIGTERM, goaway);

    for (;;) {
        /*
         * Something seriously wrong with atp, since these assigns must
         * be in the loop...
         */
        atpb.atp_saddr = &sat;
        atpb.atp_rreqdata = buf;
        bzero(&sat, sizeof(struct sockaddr_at));
        atpb.atp_rreqdlen = sizeof(buf);

        if (atp_rreq(atp, &atpb) < 0) {
            LOG(log_error, logtype_default, "main: atp_rreq: %s", strerror(errno));
            exit(1);
        }

        p = buf;
        bcopy(p, &req, SIZEOF_MAC_LONG);
        req = ntohl(req);
        p += SIZEOF_MAC_LONG;

        switch (req) {
        case TL_GETTIME :
            if (atpb.atp_rreqdlen > 5) {
                bcopy(p + 1, &mtime, SIZEOF_MAC_LONG);
                mtime = ntohl(mtime);
                LOG(log_info, logtype_default, "gettime from %s %s was %lu",
                    (*(p + 5) == '\0') ? "<unknown>" : p + 5,
                    (*p == 0) ? "at boot" : "in chooser",
                    mtime);
            } else {
                LOG(log_info, logtype_default, "gettime");
            }

            if (gettimeofday(&tv, NULL) < 0) {
                LOG(log_error, logtype_default, "main: gettimeofday: %s", strerror(errno));
                exit(1);
            }

            if (lflag == 1) {
                if (localtime_r(&tv.tv_sec, &tm) == NULL) {
                    LOG(log_error, logtype_default, "main: localtime_r: %s", strerror(errno));
                    exit(1);
                }

#if defined (HAVE_STRUCT_TM_TM_GMTOFF)
                mtime = tv.tv_sec + EPOCH + tm.tm_gmtoff;
#else /* HAVE_STRUCT_TM_TM_GMTOFF */
                mtime = tv.tv_sec + EPOCH - timezone;
#endif /* HAVE_STRUCT_TM_TM_GMTOFF */
            } else {
                if (gmtime_r(&tv.tv_sec, &tm) == NULL) {
                    LOG(log_error, logtype_default, "main: gmtime_r: %s", strerror(errno));
                    exit(1);
                }

                mtime = tv.tv_sec + EPOCH;
            }

            mtime = htonl(mtime);
            resp = htonl(TL_OK);
            bcopy(&resp, buf, SIZEOF_MAC_LONG);
            bcopy(&mtime, buf + SIZEOF_MAC_LONG, SIZEOF_MAC_LONG);
            iov.iov_len = SIZEOF_MAC_LONG + SIZEOF_MAC_LONG;
            break;

        default :
            LOG(log_error, logtype_default, bad);
            resp = htonl(TL_BAD);
            bcopy(&resp, buf, SIZEOF_MAC_LONG);
            *(buf + 4) = (unsigned char)strlen(bad);
            strcpy(buf + 5, bad);
            iov.iov_len = SIZEOF_MAC_LONG + 2 + strlen(bad);
            break;
        }

        iov.iov_base = buf;
        atpb.atp_sresiov = &iov;
        atpb.atp_sresiovcnt = 1;

        if (atp_sresp(atp, &atpb) < 0) {
            LOG(log_error, logtype_default, "main: atp_sresp: %s", strerror(errno));
            exit(1);
        }
    }
}
