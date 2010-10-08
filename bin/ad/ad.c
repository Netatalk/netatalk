/* 
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
   
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
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <atalk/cnid.h>
#include <atalk/volinfo.h>
#include <atalk/logger.h>
#include "ad.h"

static void usage_main(void)
{
/*
    printf("Usage: ad ls|rm|cp|mv|set [file|dir, ...]\n");
*/
    printf("Usage: ad ls [file|dir, ...]\n");
}

int main(int argc, char **argv)
{
    setuplog("default log_debug /dev/tty");

    if (argc < 2) {
        usage_main();
        return 1;
    }

    if (STRCMP(argv[1], ==, "ls"))
        return ad_ls(argc - 1, argv + 1);
    else if (STRCMP(argv[1], ==, "cp"))
        return ad_cp(argc - 1, argv + 1);
    else {
        usage_main();
        return 1;
    }

    return 0;
}
