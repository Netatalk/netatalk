/* 
   Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/volinfo.h>
#include "ad.h"

static volatile sig_atomic_t alarmed;

/*
  SIGNAL handling:
  catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.
*/

static void sig_handler(int signo)
{
    alarmed = 1;
    return;
}

static void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGTERM, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGTERM): %s", strerror(errno));

    if (sigaction(SIGINT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGINT): %s", strerror(errno));

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGABRT): %s", strerror(errno));

    if (sigaction(SIGHUP, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGHUP): %s", strerror(errno));

    if (sigaction(SIGQUIT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGQUIT): %s", strerror(errno));
}

static void usage_find(void)
{
    printf(
        "Usage: ad find VOLUME_PATH NAME\n"
        );
}

int ad_find(int argc, char **argv)
{
    int c;
    afpvol_t vol;

    printf("argc: %d, argv0: %s\n", argc, argv[0]);

    if (argc != 4) {
        usage_find();
        exit(1);
    }

    set_signal();
    cnid_init();

    openvol(argv[2], &vol);
    cnid_find(vol.volume.v_cdb, argv[3], strlen(argv[3]) + 1);
    closevol(&vol);

    return 0;
}
