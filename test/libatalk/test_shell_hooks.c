/*
 * Regression test for shell_quote()
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <atalk/afp.h>
#include <atalk/globals.h>

/* Private test seam, not installed as part of the public API. */
extern char *conf_testutil_expand_hook(const AFPObj *, const char *,
                                       const struct passwd *, const char *,
                                       const char *);
extern int afprun(char *, int *);

static int tests_run;
static int tests_failed;

static void report(int passed, const char *name)
{
    printf("%s %d - %s\n", passed ? "ok" : "not ok", ++tests_run, name);
    tests_failed += !passed;
}

/* Exercise the actual shell execution sink, not just the generated spelling. */
static int runs_as(AFPObj *obj, struct passwd *pwd, const char *src,
                   const char *path, const char *name, const char *want)
{
    char *cmd = conf_testutil_expand_hook(obj, src, pwd, path, name);
    char output[16384];
    size_t used = 0;
    int fd = -1;

    if (!cmd) {
        return 0;
    }

    int status = afprun(cmd, &fd);
    free(cmd);

    if (status != 0 || fd < 0) {
        if (fd >= 0) {
            close(fd);
        }

        return 0;
    }

    ssize_t got;

    while (used < sizeof(output)
            && (got = read(fd, output + used, sizeof(output) - used)) != 0) {
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }

            close(fd);
            return 0;
        }

        if ((size_t)got > sizeof(output) - used) {
            close(fd);
            return 0;
        }

        used += (size_t)got;
    }

    close(fd);

    /* A full buffer has not reached EOF: matching its prefix is not enough.
     * Keep the comparison's bounds explicit for both input arrays. */
    if (used >= sizeof(output) || used != strlen(want)) {
        return 0;
    }

    return memcmp(output, want, used) == 0;
}

int main(void)
{
    AFPObj obj = {0};
    struct passwd pwd = {0};
    char payload[] = "O'Reilly \"; printf INJECTED; # $(printf INJECTED) "
                     "`printf INJECTED` \\ * ?\nsecond line";
    const char *commands[] = {
        "printf '%s' $f",
        "printf '%s' '$f'",
        "printf '%s' \"$f\"",
        "printf '%s' $u",
        "printf '%s' $v",
        "printf '%s' $d",
        "printf '%s' $b",
        "printf '%s' $h",
        "printf '%s' $s",
        "printf '%s' $f | cat",
    };
    pwd.pw_gecos = payload;
    snprintf(obj.username, sizeof(obj.username), "%s", payload);
    obj.options.hostname = payload;
    obj.options.servername = payload;
    printf("TAP version 13\n");

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        report(runs_as(&obj, &pwd, commands[i], payload, payload, payload),
               commands[i]);
    }

    char want[1024];
    snprintf(want, sizeof(want), "before%smiddle%safter", payload, payload);
    report(runs_as(&obj, &pwd, "printf '%s' \"before$f\"middle'$u'after",
                   payload, payload, want), "quoted and unquoted fragments concatenate");
    snprintf(want, sizeof(want), "$%s", payload);
    report(runs_as(&obj, &pwd, "printf '%s' $$$f", payload, payload, want),
           "a dollar before a substitution cannot form ANSI C quoting");
    report(runs_as(&obj, &pwd, "printf '%s' \\$$f", payload, payload, want),
           "an escaped dollar does not consume the following substitution");
    report(runs_as(&obj, &pwd, "printf '%s' \"\\$$f\"", payload, payload, want),
           "an escaped dollar before a quoted substitution stays literal");
    report(runs_as(&obj, &pwd, "printf '%s' \\$f", payload, payload, "$f"),
           "a shell-escaped dollar remains literal");
    report(runs_as(&obj, &pwd, "printf '%s' \"\\$f$(printf plain)\"",
                   payload, payload, "$fplain"),
           "an escaped dollar does not restrict a shell-only hook");
    snprintf(want, sizeof(want), "\\%s", payload);
    report(runs_as(&obj, &pwd, "printf '%s' '\\$f'", payload, payload, want),
           "a backslash inside single quotes does not escape a substitution");
    report(runs_as(&obj, &pwd, "printf '%s' \\\\$f", payload, payload, want),
           "an escaped backslash does not escape the following substitution");
    report(runs_as(&obj, &pwd, "printf '%s' \"$\\$f$(printf plain)\"",
                   payload, payload, "$$fplain"),
           "an unknown dollar does not consume a following escape");
    report(runs_as(&obj, &pwd, "printf '%s' '$$HOME'", "", "", "$HOME"),
           "shell-only hooks retain historical dollar-pair expansion");
    pwd.pw_gecos = "";
    report(runs_as(&obj, &pwd, "printf '<%s>' $f", "", "", "<>"),
           "an empty substitution remains an argument");
    pwd.pw_gecos = payload;
    const char *invalid[] = {
        "printf '%s' \"$(printf '%s' $f)\"",
        "printf '%s' `printf '%s' $f`",
        "cat <<EOF\n$f\nEOF",
        "printf '%s' ${value:-$f}",
        "printf '%s' $[$f]",
        "printf '%s' $'prefix$f'",
        "printf '%s' $\"prefix$f\"",
        "# '\nprintf '%s' $f",
        "printf '%s' \"$f",
        "printf '%s' $f \\",
        "printf '%s' $\\\n($f)",
    };

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char *cmd = conf_testutil_expand_hook(&obj, invalid[i], &pwd, "", "");
        report(cmd == NULL, "unsupported shell syntax fails closed");
        free(cmd);
    }

    report(runs_as(&obj, &pwd, "printf '%s' \"$(printf plain)\"", "", "", "plain"),
           "shell-only hooks retain complex syntax");
    char long_value[MAXPATHLEN + 2];
    memset(long_value, 'x', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    pwd.pw_gecos = long_value;
    char *cmd = conf_testutil_expand_hook(&obj, "printf '%s' $f", &pwd, "", "");
    report(cmd == NULL, "overlong values fail instead of truncating");
    free(cmd);
    pwd.pw_gecos = payload;
    /* Quote expansion can make a valid command much longer than MAXPATHLEN. */
    char long_command[MAXPATHLEN + 128];
    memset(long_command, ' ', MAXPATHLEN);
    strcpy(long_command + MAXPATHLEN, "printf '%s' $f");
    report(runs_as(&obj, &pwd, long_command, "", "", payload),
           "long commands retain the complete quoted argument");
    char full_output[16385];
    memset(full_output, '0', sizeof(full_output) - 1);
    full_output[sizeof(full_output) - 1] = '\0';
    report(!runs_as(&obj, &pwd, "printf '%016385d' 0", "", "", full_output),
           "output beyond the buffer cannot pass by matching its prefix");
    printf("1..%d\n", tests_run);
    return tests_failed ? 1 : 0;
}
