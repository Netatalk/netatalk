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

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>

#include "nad.h"

/* Workaround for mismatched gid_t type and getgrouplist() prototype on macOS */
#ifdef __APPLE__
#define my_gid_type int
#else
#define my_gid_type gid_t
#endif

typedef enum {
    CMD_UNKNOWN = -1,
    CMD_LS,
    CMD_CP,
    CMD_RM,
    CMD_MV,
    CMD_SET,
    CMD_FIND,
    CMD_VERSION
} nad_command_t;

static nad_command_t get_command(const char *cmd)
{
    if (STRCMP(cmd, ==, "ls")) {
        return CMD_LS;
    }

    if (STRCMP(cmd, ==, "cp")) {
        return CMD_CP;
    }

    if (STRCMP(cmd, ==, "rm")) {
        return CMD_RM;
    }

    if (STRCMP(cmd, ==, "mv")) {
        return CMD_MV;
    }

    if (STRCMP(cmd, ==, "set")) {
        return CMD_SET;
    }

    if (STRCMP(cmd, ==, "find")) {
        return CMD_FIND;
    }

    if (STRCMP(cmd, ==, "-v") || STRCMP(cmd, ==, "--version")) {
        return CMD_VERSION;
    }

    return CMD_UNKNOWN;
}

static void usage_main(void)
{
    printf("Usage: nad ls|cp|rm|mv|set|find [file|dir, ...]\n");
    printf("       nad -v|--version\n");
}

static void show_version(void)
{
    printf("nad (Netatalk %s)\n", VERSION);
}

int main(int argc, char **argv)
{
    AFPObj obj = { 0 };

    if (argc < 2) {
        usage_main();
        return 1;
    }

    if (afp_config_parse(&obj, "nad") != 0) {
        return 1;
    }

    obj.uid = getuid();
    const struct passwd *pwd = getpwuid(obj.uid);

    if (pwd) {
        int ngroups = 0;
        my_gid_type *groups = NULL;
        getgrouplist(pwd->pw_name, pwd->pw_gid, NULL, &ngroups);
        groups = malloc(sizeof(my_gid_type) * ngroups);

        if (groups) {
            if (getgrouplist(pwd->pw_name, pwd->pw_gid, groups, &ngroups) >= 0) {
                obj.groups = malloc(sizeof(my_gid_type) * ngroups);

                if (obj.groups) {
                    memcpy(obj.groups, groups, sizeof(my_gid_type) * ngroups);
                    obj.ngroups = ngroups;
                }
            }

            free(groups);
        }
    }

    setuplog(obj.options.logconfig, obj.options.logfile,
             obj.options.log_us_timestamp);

    if (load_volumes(&obj, LV_DEFAULT) != 0) {
        return 1;
    }

    nad_command_t cmd = get_command(argv[1]);

    switch (cmd) {
    case CMD_LS:
        return ad_ls(argc - 1, argv + 1, &obj);

    case CMD_CP:
        return ad_cp(argc - 1, argv + 1, &obj);

    case CMD_RM:
        return ad_rm(argc - 1, argv + 1, &obj);

    case CMD_MV:
        return ad_mv(argc, argv, &obj);

    case CMD_SET:
        return ad_set(argc - 1, argv + 1, &obj);

    case CMD_FIND:
        return ad_find(argc, argv, &obj);

    case CMD_VERSION:
        show_version();
        return 1;

    case CMD_UNKNOWN:
    default:
        usage_main();
        return 1;
    }

    return 0;
}
