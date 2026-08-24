/*
  Copyright (c) 2026 Andy Lemin (andylemin)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  Config-resolution unit tests: the strict boolean parser, the ea (G)/(V)
  fallback, the strict locking rename, and the ea = samba coherency
  defaults.  Each test is self-contained: it writes its own afp.conf to a
  temp file, parses it into a local AFPObj, asserts, and tears down.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>

#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "dircache.h"
#include "peer_lock.h"
#include "subtests_conf.h"
#include "test.h"

/* Fixture files live in the working directory (the meson build dir, like
 * the harness's own test.conf), not /tmp: no world-writable-dir exposure,
 * and the build dir's filesystem reliably supports user xattrs where a
 * tmpfs /tmp may not.  The conf tests run before the harness chdirs. */

/* Write ini text to a fresh temp file; returns malloc'd path or NULL. */
static char *conf_write_tmp(const char *ini_text)
{
    static char path_template[] = "utest_conf_XXXXXX";
    char *path = strdup(path_template);

    if (path == NULL) {
        return NULL;
    }

    int fd = mkstemp(path);

    if (fd == -1) {
        free(path);
        return NULL;
    }

    size_t len = strlen(ini_text);

    if (write(fd, ini_text, len) != (ssize_t)len) {
        close(fd);
        unlink(path);
        free(path);
        return NULL;
    }

    close(fd);
    return path;
}

/* Read a whole (small) log file into a malloc'd NUL-terminated buffer.
 * Returns NULL if the file exceeds the buffer: a truncated log would make
 * negative assertions (needle must be absent) silently pass. */
#define CONF_SLURP_MAX 65536

static char *conf_slurp(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL) {
        return NULL;
    }

    char *buf = calloc(1, CONF_SLURP_MAX);

    if (buf) {
        size_t got = fread(buf, 1, CONF_SLURP_MAX - 1, fp);
        buf[got] = '\0';

        if (got == CONF_SLURP_MAX - 1 && fgetc(fp) != EOF) {
            fprintf(test_stream(),
                    "# conf_slurp: log %s exceeds %d bytes\n", path,
                    CONF_SLURP_MAX);
            free(buf);
            buf = NULL;
        }
    }

    fclose(fp);
    return buf;
}

/* Parse a config built from ini_body (appended below a [Global] header that
 * routes logging to logpath) into obj.  Returns 0 on success.  Failures
 * here are deliberately RED, not skips: pre-test env probes (conf_mklog,
 * conf_mkvoldir) skip, but a failure after setup succeeded once is
 * either product behaviour or a mid-run host problem worth surfacing. */
static int conf_parse_fixture(AFPObj *obj, const char *ini_body,
                              const char *logpath)
{
    char ini[8192];
    int inilen = snprintf(ini, sizeof(ini),
                          "[Global]\n"
                          "log level = default:note\n"
                          "log file = %s\n"
                          "%s",
                          logpath, ini_body);

    if (inilen < 0 || (size_t)inilen >= sizeof(ini)) {
        fprintf(test_stream(), "# conf_parse_fixture: ini truncated\n");
        return -1;
    }

    char *conf = conf_write_tmp(ini);

    if (conf == NULL) {
        return -1;
    }

    memset(obj, 0, sizeof(*obj));
    obj->cmdlineconfigfile = conf;
    obj->uid = getuid();
    /* A previous test that failed mid-body may not have reached teardown:
     * reset the volume-list statics defensively before parsing. */
    unload_volumes(obj);

    if (afp_config_parse(obj, NULL) != 0) {
        unlink(conf);
        free(conf);
        obj->cmdlineconfigfile = NULL;
        return -1;
    }

    return 0;
}

static void conf_teardown(AFPObj *obj, const char *logpath)
{
    unload_volumes(obj);
    afp_config_free(obj);

    if (obj->cmdlineconfigfile) {
        unlink(obj->cmdlineconfigfile);
        free(obj->cmdlineconfigfile);
        obj->cmdlineconfigfile = NULL;
    }

    if (logpath) {
        unlink(logpath);
    }

    /* Restore the harness's log routing (the fixture's afp_config_parse()
     * re-routed process logging to the temp log file). */
    setuplog("default:note", "/dev/stderr", true);
}

/* Truncate the log file between sub-cases so greps see only the current
 * case's messages. */
static void conf_log_truncate(const char *logpath)
{
    int fd = open(logpath, O_WRONLY | O_TRUNC);

    if (fd != -1) {
        close(fd);
    }
}

/* 1 = needle present, 0 = definitely absent, -1 = log unreadable or
 * overflowed.  Callers compare against the exact value they assert so a
 * bad log can never satisfy either a positive or a negative assertion. */
static int conf_log_contains(const char *logpath, const char *needle)
{
    char *log = conf_slurp(logpath);
    int found;

    if (log == NULL) {
        return -1;
    }

    found = strstr(log, needle) != NULL ? 1 : 0;
    free(log);
    return found;
}

/* Create a scratch volume directory; returns a malloc'd ABSOLUTE path or
 * NULL (the fixture inis carry it as the volume's "path" value, and the
 * harness chdirs into volumes later). */
static char *conf_mkvoldir(void)
{
    char dir[] = "utest_conf_vol_XXXXXX";
    char abspath[MAXPATHLEN + 1];

    if (mkdtemp(dir) == NULL) {
        return NULL;
    }

    if (realpath(dir, abspath) == NULL) {
        rmdir(dir);
        return NULL;
    }

    return strdup(abspath);
}

/* Whether dir's filesystem accepts user xattrs (the ea = sys/samba tests
 * assert the post-probe EA backend, which needs a capable fixture). */
static int conf_dir_xattr_ok(const char *dir)
{
    return sys_setxattr(dir, "org.netatalk.utest-xattr-probe", "1", 1, 0) == 0;
}

static struct vol *conf_vol_by_name(const char *name)
{
    for (struct vol *vol = getvolumes(); vol; vol = vol->v_next) {
        if (vol->v_localname && strcmp(vol->v_localname, name) == 0) {
            return vol;
        }
    }

    return NULL;
}

/* Parse a config AND load its volumes (LV_ALL: no session context). */
static int conf_parse_fixture_vols(AFPObj *obj, const char *ini_body,
                                   const char *logpath)
{
    if (conf_parse_fixture(obj, ini_body, logpath) != 0) {
        return -1;
    }

    if (load_afp_conf_vols(obj, LV_ALL) != 0) {
        conf_teardown(obj, NULL);
        return -1;
    }

    return 0;
}

/* Make a mkstemp'd log path; returns 0 and fills buf, or -1. */
static int conf_mklog(char *buf, size_t buflen)
{
    int fd;
    snprintf(buf, buflen, "utest_conf_log_XXXXXX");
    fd = mkstemp(buf);

    if (fd == -1) {
        return -1;
    }

    close(fd);
    return 0;
}

/* utest_conf_parse_bool: the strict boolean parser, table-driven through a
 * real boolean option ("advertise ssh" -> OPTION_ANNOUNCESSH, default 0).
 * Valid spellings parse with no warning; invalid values warn and keep the
 * default; empty is unset with no warning. */
int utest_conf_parse_bool(void)
{
    static const struct {
        const char *val;
        int expect_flag;    /* OPTION_ANNOUNCESSH set? */
        int expect_warning;
    } cases[] = {
        {"yes", 1, 0}, {"YES", 1, 0}, {"Yes", 1, 0},
        {"true", 1, 0}, {"True", 1, 0},
        {"on", 1, 0}, {"enabled", 1, 0}, {"1", 1, 0},
        {"y", 1, 0}, {"Y", 1, 0}, {"t", 1, 0}, {"T", 1, 0},
        {"no", 0, 0}, {"false", 0, 0}, {"off", 0, 0},
        {"disabled", 0, 0}, {"0", 0, 0},
        {"n", 0, 0}, {"N", 0, 0}, {"f", 0, 0}, {"F", 0, 0},
        /* invalid: warned, default (off) applies; whole-word matching, so
         * prefix accidents that iniparser accepted ('ture' -> true,
         * 'never' -> false) are rejected */
        {"maybe", 0, 1}, {"ture", 0, 1}, {"enable", 0, 1},
        {"never", 0, 1}, {"2", 0, 1}, {"10", 0, 1},
        /* empty: unset, no warning */
        {"", 0, 0},
    };
    AFPObj obj;
    char logpath[64];
    int failed = -1;

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        return TEST_SKIP;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char body[256];
        snprintf(body, sizeof(body), "advertise ssh = %s\n", cases[i].val);
        conf_log_truncate(logpath);

        if (conf_parse_fixture(&obj, body, logpath) != 0) {
            goto cleanup;
        }

        int flag = (obj.options.flags & OPTION_ANNOUNCESSH) ? 1 : 0;
        int warned = conf_log_contains(logpath, "is not a valid boolean");
        conf_teardown(&obj, NULL);

        if (flag != cases[i].expect_flag) {
            fprintf(test_stream(),
                    "# utest_conf_parse_bool: '%s' -> flag %d, expected %d\n",
                    cases[i].val, flag, cases[i].expect_flag);
            goto cleanup;
        }

        if (warned != cases[i].expect_warning) {
            fprintf(test_stream(),
                    "# utest_conf_parse_bool: '%s' -> warning %d, expected %d\n",
                    cases[i].val, warned, cases[i].expect_warning);
            goto cleanup;
        }
    }

    failed = 0;
cleanup:
    unlink(logpath);
    return failed;
}

/* utest_conf_permission_options_require_unix_priv: file perm, directory
 * perm, and umask are meaningful only on UNIX-privilege volumes. A
 * no-UNIX-privilege volume retains the configured values, but the live
 * accessors suppress all three options (including explicitly configured zero
 * masks). */
int utest_conf_permission_options_require_unix_priv(void)
{
    static const struct {
        const char *unix_priv;
        const char *umask;
        const char *file_perm;
        const char *directory_perm;
        mode_t expected_umask;
        mode_t expected_file_perm;
        mode_t expected_directory_perm;
        int expect_unix_priv;
    } cases[] = {
        {"no",  "0077", "0222", "0444", 0077, 0222, 0444, 0},
        {"no",  "0000", "0000", "0000", 0,    0,    0,    0},
        {"yes", "0077", "0222", "0444", 0077, 0222, 0444, 1},
        {"no",  NULL,   NULL,   NULL,   0,    0,    0,    0},
    };
    AFPObj obj;
    char logpath[64];
    char body[1024];
    char *voldir = conf_mkvoldir();
    int failed = -1;

    if (voldir == NULL || conf_mklog(logpath, sizeof(logpath)) != 0) {
        free(voldir);
        return TEST_SKIP;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char umask_line[32] = "";
        char file_perm_line[32] = "";
        char directory_perm_line[32] = "";

        if (cases[i].umask) {
            snprintf(umask_line, sizeof(umask_line), "umask = %s\n",
                     cases[i].umask);
        }

        if (cases[i].file_perm) {
            snprintf(file_perm_line, sizeof(file_perm_line), "file perm = %s\n",
                     cases[i].file_perm);
        }

        if (cases[i].directory_perm) {
            snprintf(directory_perm_line, sizeof(directory_perm_line),
                     "directory perm = %s\n", cases[i].directory_perm);
        }

        int n = snprintf(body, sizeof(body),
                         "[utestvol]\n"
                         "path = %s\n"
                         "ea = none\n"
                         "unix priv = %s\n"
                         "%s"
                         "%s"
                         "%s",
                         voldir, cases[i].unix_priv,
                         umask_line, file_perm_line, directory_perm_line);

        if (n < 0 || (size_t)n >= sizeof(body)) {
            goto cleanup;
        }

        conf_log_truncate(logpath);

        if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
            goto cleanup;
        }

        const struct vol *vol = conf_vol_by_name("utestvol");
        int unix_priv = vol && (vol->v_flags & AFPVOL_UNIX_PRIV) ? 1 : 0;
        mode_t effective_umask = vol ? vol_umask(vol) : 0;
        mode_t effective_file_perm = vol ? vol_fperm(vol) : 0;
        mode_t effective_directory_perm = vol ? vol_dperm(vol) : 0;
        mode_t expected_effective_umask = cases[i].expect_unix_priv
                                          ? cases[i].expected_umask : 0;
        mode_t expected_effective_file_perm = cases[i].expect_unix_priv
                                              ? cases[i].expected_file_perm : 0;
        mode_t expected_effective_directory_perm = cases[i].expect_unix_priv
            ? cases[i].expected_directory_perm : 0;

        if (vol == NULL || unix_priv != cases[i].expect_unix_priv
                || vol->v_umask != cases[i].expected_umask
                || vol->v_fperm != cases[i].expected_file_perm
                || vol->v_dperm != cases[i].expected_directory_perm
                || effective_umask != expected_effective_umask
                || effective_file_perm != expected_effective_file_perm
                || effective_directory_perm != expected_effective_directory_perm) {
            fprintf(test_stream(),
                    "# utest_conf_permission_options_require_unix_priv: case %zu: "
                    "unix priv %d/%d umask %04o/%04o (%04o) file %04o/%04o (%04o) "
                    "directory %04o/%04o (%04o)\n",
                    i, unix_priv, cases[i].expect_unix_priv,
                    vol ? vol->v_umask : 0, cases[i].expected_umask,
                    effective_umask,
                    vol ? vol->v_fperm : 0, cases[i].expected_file_perm,
                    effective_file_perm,
                    vol ? vol->v_dperm : 0, cases[i].expected_directory_perm,
                    effective_directory_perm);
            conf_teardown(&obj, NULL);
            goto cleanup;
        }

        conf_teardown(&obj, NULL);
    }

    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}


/* utest_conf_ea_fallback: the ea (G)/(V) resolution chain, one loaded
 * volume per case.
 *   1. [Global] ea = ad, volume silent            -> ad        (global)
 *   2. [Global] ea = ad, volume ea = sys          -> sys       (volume wins)
 *   3. preset ea = none beats [Global] ea = ad    -> none      (preset)
 *   4. preset + volume ea = sys                   -> sys       (volume beats preset)
 *   5. [Global] ea = samba                        -> sys + AFPVOL_EA_SAMBA flag
 *   6. nothing set                                -> auto-detect, no samba flag
 *   7. [Global] ea = bogus                        -> warning + auto-detect
 * Cases needing the post-probe EA outcome require a user-xattr-capable
 * volume dir; the whole test skips without one. */
int utest_conf_ea_fallback(void)
{
    AFPObj obj;
    char logpath[64];
    int failed = -1;
    char body[1024];
    const struct vol *vol;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (!conf_dir_xattr_ok(voldir)) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    static const struct {
        const char *global_ea;   /* NULL = omit */
        const char *preset;      /* NULL = no preset section/selection */
        const char *vol_ea;      /* NULL = omit */
        int vfs_ea;
        int adouble;
        int samba_flag;
        const char *expect_log;  /* NULL = no assertion */
    } cases[] = {
        {"ad",    NULL,   NULL,  AFPVOL_EA_AD,   AD_VERSION2,   0, NULL},
        {"ad",    NULL,   "sys", AFPVOL_EA_SYS,  AD_VERSION_EA, 0, NULL},
        {"ad",    "none", NULL,  AFPVOL_EA_NONE, AD_VERSION,    0, NULL},
        {"ad",    "none", "sys", AFPVOL_EA_SYS,  AD_VERSION_EA, 0, NULL},
        {"samba", NULL,   NULL,  AFPVOL_EA_SYS,  AD_VERSION_EA, 1, NULL},
        /* absent everywhere: auto-detect probes the (xattr-capable)
         * fixture dir and resolves to sys, samba flag clear */
        {NULL,    NULL,   NULL,  AFPVOL_EA_SYS,  AD_VERSION_EA, 0, NULL},
        {
            "bogus", NULL,   NULL,  AFPVOL_EA_SYS,  AD_VERSION_EA, 0,
            "unknown ea mode \"bogus\"; using auto-detect"
        },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char global_line[64] = "";
        char preset_block[128] = "";
        char preset_line[64] = "";
        char vol_line[64] = "";

        if (cases[i].global_ea) {
            snprintf(global_line, sizeof(global_line), "ea = %s\n",
                     cases[i].global_ea);
        }

        if (cases[i].preset) {
            snprintf(preset_block, sizeof(preset_block),
                     "[utestpreset]\nea = %s\n", cases[i].preset);
            snprintf(preset_line, sizeof(preset_line),
                     "vol preset = utestpreset\n");
        }

        if (cases[i].vol_ea) {
            snprintf(vol_line, sizeof(vol_line), "ea = %s\n",
                     cases[i].vol_ea);
        }

        snprintf(body, sizeof(body),
                 "%s"
                 "%s"
                 "[utestvol]\n"
                 "path = %s\n"
                 "%s"
                 "%s",
                 global_line, preset_block, voldir, preset_line, vol_line);
        conf_log_truncate(logpath);

        if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
            goto cleanup;
        }

        vol = conf_vol_by_name("utestvol");

        if (vol == NULL) {
            fprintf(test_stream(),
                    "# utest_conf_ea_fallback: case %zu: volume did not load\n", i);
            conf_teardown(&obj, NULL);
            goto cleanup;
        }

        int samba_flag = (vol->v_flags & AFPVOL_EA_SAMBA) ? 1 : 0;

        if (vol->v_vfs_ea != cases[i].vfs_ea
                || vol->v_adouble != cases[i].adouble
                || samba_flag != cases[i].samba_flag) {
            fprintf(test_stream(),
                    "# utest_conf_ea_fallback: case %zu: vfs_ea %d/%d adouble 0x%x/0x%x samba %d/%d\n",
                    i, vol->v_vfs_ea, cases[i].vfs_ea,
                    vol->v_adouble, cases[i].adouble,
                    samba_flag, cases[i].samba_flag);
            conf_teardown(&obj, NULL);
            goto cleanup;
        }

        if (cases[i].expect_log
                && conf_log_contains(logpath, cases[i].expect_log) != 1) {
            fprintf(test_stream(),
                    "# utest_conf_ea_fallback: case %zu: expected log '%s' missing\n",
                    i, cases[i].expect_log);
            conf_teardown(&obj, NULL);
            goto cleanup;
        }

        conf_teardown(&obj, NULL);
    }

    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_strict_locking_keys: the canonical key, the deprecated alias,
 * their precedence, and the explicitness marker.  Global-only parse, no
 * volumes. */
int utest_conf_strict_locking_keys(void)
{
    AFPObj obj;
    char logpath[64];
    int failed = -1;
    static const struct {
        const char *body;
        int flag;             /* OPTION_STRICT_LOCKING expected? */
        int explicit_marker;
        int expect_deprecation;
        int expect_invalid;
    } cases[] = {
        {"strict locking = yes\n",                          1, 1, 0, 0},
        {"afp read locks = yes\n",                          1, 1, 1, 0},
        {"strict locking = no\nafp read locks = yes\n",     0, 1, 1, 0},
        {"strict locking = no\n",                           0, 1, 0, 0},
        {"strict locking = maybe\n",                        0, 0, 0, 1},
        /* invalid new key + valid alias: alias backfills, both warnings */
        {"strict locking = maybe\nafp read locks = yes\n",  1, 1, 1, 1},
        /* 'enabled' is a valid spelling under the strict parser */
        {"afp read locks = enabled\n",                      1, 1, 1, 0},
        {"afp read locks = garbled\n",                      0, 0, 1, 1},
    };

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        return TEST_SKIP;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        conf_log_truncate(logpath);

        if (conf_parse_fixture(&obj, cases[i].body, logpath) != 0) {
            goto cleanup;
        }

        int flag = (obj.options.flags & OPTION_STRICT_LOCKING) ? 1 : 0;
        int deprecated = conf_log_contains(logpath,
                                           "deprecated 'afp read locks'");
        int invalid = conf_log_contains(logpath, "is not a valid boolean");

        if (flag != cases[i].flag
                || obj.options.strict_locking_explicit != cases[i].explicit_marker
                || deprecated != cases[i].expect_deprecation
                || invalid != cases[i].expect_invalid) {
            fprintf(test_stream(),
                    "# utest_conf_strict_locking_keys: case %zu: flag %d/%d explicit %d/%d depr %d/%d invalid %d/%d\n",
                    i, flag, cases[i].flag,
                    obj.options.strict_locking_explicit, cases[i].explicit_marker,
                    deprecated, cases[i].expect_deprecation,
                    invalid, cases[i].expect_invalid);
            conf_teardown(&obj, NULL);
            goto cleanup;
        }

        conf_teardown(&obj, NULL);
    }

    failed = 0;
cleanup:
    unlink(logpath);
    return failed;
}

/* utest_conf_samba_defaults: ea = samba with no explicit settings applies
 * the coherency defaults process-wide and logs the note; a garbled
 * strict locking value is ignored (warned) and the samba default still
 * applies; a mixed samba+sys config applies them too and both volumes
 * load. */
int utest_conf_samba_defaults(void)
{
    AFPObj obj;
    char logpath[64];
    char body[1024];
    int failed = -1;
    char *voldir = conf_mkvoldir();
    char *voldir2 = NULL;

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (!conf_dir_xattr_ok(voldir)) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    voldir2 = conf_mkvoldir();

    if (voldir2 == NULL || conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);

        if (voldir2) {
            rmdir(voldir2);
            free(voldir2);
        }

        return TEST_SKIP;
    }

    /* Case 1: bare ea = samba -> defaults applied + notes logged */
    snprintf(body, sizeof(body), "[utestvol]\npath = %s\nea = samba\n",
             voldir);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (!(obj.options.flags & OPTION_STRICT_LOCKING)
            || obj.options.dircache_validation_freq != 1
            || conf_log_contains(logpath,
                                 "defaulting 'strict locking' to yes") != 1) {
        fprintf(test_stream(),
                "# utest_conf_samba_defaults: defaults not applied (flags 0x%x freq %d)\n",
                obj.options.flags, obj.options.dircache_validation_freq);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    /* Case 2: garbled strict locking is ignored; samba default applies */
    conf_log_truncate(logpath);
    snprintf(body, sizeof(body),
             "strict locking = garbled\n[utestvol]\npath = %s\nea = samba\n",
             voldir);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (!(obj.options.flags & OPTION_STRICT_LOCKING)
            || obj.options.strict_locking_explicit
            || conf_log_contains(logpath, "is not a valid boolean") != 1
            || conf_log_contains(logpath,
                                 "defaulting 'strict locking' to yes") != 1) {
        fprintf(test_stream(),
                "# utest_conf_samba_defaults: garbled value blocked the default\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    /* Case 3: mixed samba + sys volumes; both load, defaults process-wide */
    conf_log_truncate(logpath);
    snprintf(body, sizeof(body),
             "[utestvol]\npath = %s\nea = samba\n[utestvol2]\npath = %s\nea = sys\n",
             voldir, voldir2);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (!(obj.options.flags & OPTION_STRICT_LOCKING)
            || obj.options.dircache_validation_freq != 1
            || conf_vol_by_name("utestvol") == NULL
            || conf_vol_by_name("utestvol2") == NULL) {
        fprintf(test_stream(),
                "# utest_conf_samba_defaults: mixed config failed\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    rmdir(voldir2);
    free(voldir2);
    return failed;
}

/* utest_conf_samba_explicit_wins: the core defaults-not-force test —
 * explicit weaker settings SURVIVE ea = samba, each with its verbose
 * warning. */
int utest_conf_samba_explicit_wins(void)
{
    AFPObj obj;
    char logpath[64];
    char body[1024];
    int failed = -1;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (!conf_dir_xattr_ok(voldir)) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body),
             "strict locking = no\n"
             "dircache validation freq = 100\n"
             "dircache rfork budget = 1024\n"
             "[utestvol]\npath = %s\nea = samba\n",
             voldir);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if ((obj.options.flags & OPTION_STRICT_LOCKING)
            || obj.options.dircache_validation_freq != 100
            || obj.options.dircache_rfork_budget != 1024
            || !obj.options.strict_locking_explicit
            || !obj.options.dircache_validation_freq_explicit
            || !obj.options.dircache_rfork_budget_explicit) {
        fprintf(test_stream(),
                "# utest_conf_samba_explicit_wins: explicit settings overridden "
                "(flags 0x%x freq %d budget %d)\n",
                obj.options.flags, obj.options.dircache_validation_freq,
                obj.options.dircache_rfork_budget);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    if (conf_log_contains(logpath,
                          "'strict locking' is explicitly disabled") != 1
            || conf_log_contains(logpath,
                                 "'dircache validation freq' is explicitly 100") != 1
            || conf_log_contains(logpath,
                                 "rfork caching is explicitly enabled") != 1) {
        fprintf(test_stream(),
                "# utest_conf_samba_explicit_wins: missing coherency warnings\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_no_samba_regression: without ea = samba nothing changes —
 * strict locking stays off, an explicit freq keeps its value, and no
 * samba log lines appear. */
int utest_conf_no_samba_regression(void)
{
    AFPObj obj;
    char logpath[64];
    char body[512];
    int failed = -1;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body),
             "dircache validation freq = 7\n[utestvol]\npath = %s\n", voldir);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if ((obj.options.flags & OPTION_STRICT_LOCKING)
            || obj.options.dircache_validation_freq != 7
            || conf_log_contains(logpath, "ea = samba") != 0) {
        fprintf(test_stream(),
                "# utest_conf_no_samba_regression: state changed without samba "
                "(flags 0x%x freq %d)\n",
                obj.options.flags, obj.options.dircache_validation_freq);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_samba_requires_ea: an ea = samba volume on a filesystem
 * without user-xattr support must NOT load (hard-fail), other volumes in
 * the same config still load, and the defaults are not applied.  Needs a
 * genuinely xattr-rejecting mount (probe must fail ENOTSUP); skips when
 * the host offers none. */
int utest_conf_samba_requires_ea(void)
{
    static const char *candidates[] = {"/proc", "/sys"};
    const char *noxattr = NULL;
    AFPObj obj;
    char logpath[64];
    char body[1024];
    int failed = -1;
    char *voldir;

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (sys_setxattr(candidates[i], "org.netatalk.utest-xattr-probe",
                         "1", 1, 0) == -1 && errno == ENOTSUP) {
            noxattr = candidates[i];
            break;
        }
    }

    if (noxattr == NULL) {
        return TEST_SKIP;
    }

    voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body),
             "[utestsamba]\npath = %s\nea = samba\n[utestok]\npath = %s\n",
             noxattr, voldir);

    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (conf_vol_by_name("utestsamba") != NULL
            || conf_vol_by_name("utestok") == NULL
            || (obj.options.flags & OPTION_STRICT_LOCKING)
            || conf_log_contains(logpath,
                                 "requires filesystem Extended Attribute support") != 1) {
        fprintf(test_stream(),
                "# utest_conf_samba_requires_ea: hard-fail contract broken "
                "(samba vol %p, ok vol %p, flags 0x%x)\n",
                (void *)conf_vol_by_name("utestsamba"),
                (void *)conf_vol_by_name("utestok"), obj.options.flags);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_samba_ea_failure_keeps_vid: a volume refused by the ea = samba
 * EA-support check must return its volume id.  The refusal happens after
 * creatvol() allocated a vid, and a misconfigured volume is retried on every
 * config reload, so a leak here walks lastvid to overflow and then refuses
 * valid volumes too.  Saturate the counter one short of overflow: the failed
 * samba volume must not consume the last vid, so the sibling volume still
 * loads. */
int utest_conf_samba_ea_failure_keeps_vid(void)
{
    static const char *candidates[] = {"/proc", "/sys"};
    const char *noxattr = NULL;
    AFPObj obj;
    char logpath[64];
    char body[1024];
    int failed = -1;
    char *voldir;

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (sys_setxattr(candidates[i], "org.netatalk.utest-xattr-probe",
                         "1", 1, 0) == -1 && errno == ENOTSUP) {
            noxattr = candidates[i];
            break;
        }
    }

    if (noxattr == NULL) {
        return TEST_SKIP;
    }

    voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    /* samba volume first so it is the one that consumes a vid and fails */
    snprintf(body, sizeof(body),
             "[utestsamba]\npath = %s\nea = samba\n[utestok]\npath = %s\n",
             noxattr, voldir);

    if (conf_parse_fixture(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    /* Two vids left: the refused samba volume must give its one back, or
     * the sibling hits the overflow check. */
    conf_testutil_set_lastvid(UINT16_MAX - 2);

    if (load_afp_conf_vols(&obj, LV_ALL) != 0) {
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    if (conf_vol_by_name("utestsamba") != NULL
            || conf_vol_by_name("utestok") == NULL
            || conf_log_contains(logpath, "vid overflow") != 0) {
        fprintf(test_stream(),
                "# utest_conf_samba_ea_failure_keeps_vid: refused volume "
                "consumed a vid (ok vol %p)\n",
                (void *)conf_vol_by_name("utestok"));
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    conf_testutil_set_lastvid(0);
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_samba_defaults_not_leaked_on_failed_volume: a samba volume
 * whose creatvol() fails AFTER the ea parse (vid saturation seam) must not
 * change process defaults nor log that it did — proves the defaults call
 * sits in the commit block, not at the parse. */
int utest_conf_samba_defaults_not_leaked_on_failed_volume(void)
{
    AFPObj obj;
    char logpath[64];
    char body[512];
    int failed = -1;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (!conf_dir_xattr_ok(voldir)) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body), "[utestvol]\npath = %s\nea = samba\n",
             voldir);

    if (conf_parse_fixture(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    /* Saturate the vid counter (after the fixture's defensive
     * unload_volumes(), which resets it): the single fresh load below then
     * hits creatvol()'s vid-overflow EC_FAIL after the ea parse and before
     * the commit block. */
    conf_testutil_set_lastvid(UINT16_MAX);

    if (load_afp_conf_vols(&obj, LV_ALL) != 0) {
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    if (conf_vol_by_name("utestvol") != NULL
            || (obj.options.flags & OPTION_STRICT_LOCKING)
            || conf_log_contains(logpath, "ea = samba") != 0) {
        fprintf(test_stream(),
                "# utest_conf_samba_defaults_not_leaked: defaults leaked from a "
                "failed volume (flags 0x%x)\n", obj.options.flags);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    /* conf_teardown()'s unload_volumes() resets lastvid; reset again in
     * case we bailed before it ran. */
    conf_testutil_set_lastvid(0);
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_samba_future_defaults: ea = samba reverts
 * performance-oriented compiled defaults (dircache validation freq
 * 100, rfork cache on).  Overriding the parsed values between
 * afp_config_parse() and the volume load produces exactly the state a
 * changed built-in default would: values set, explicitness markers
 * clear, keys absent from the config. */
int utest_conf_samba_future_defaults(void)
{
    AFPObj obj;
    char logpath[64];
    char body[512];
    int failed = -1;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (!conf_dir_xattr_ok(voldir)) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body), "[utestvol]\npath = %s\nea = samba\n",
             voldir);

    if (conf_parse_fixture(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    obj.options.dircache_validation_freq = 100;
    obj.options.dircache_rfork_budget = 1024;

    if (load_afp_conf_vols(&obj, LV_ALL) != 0) {
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    /* freq reverts to 1 with the note; the budget stays (it is
     * process-global -- non-samba volumes keep the cache; the samba
     * volume is excluded per-volume at the fork.c gate) and the
     * exclusion note fires. */
    if (obj.options.dircache_validation_freq != 1
            || obj.options.dircache_rfork_budget != 1024
            || conf_log_contains(logpath,
                                 "defaulting 'dircache validation freq' to 1") != 1
            || conf_log_contains(logpath,
                                 "excluded from the rfork cache by default") != 1) {
        fprintf(test_stream(),
                "# utest_conf_samba_future_defaults: reversion failed "
                "(freq %d budget %d)\n",
                obj.options.dircache_validation_freq,
                obj.options.dircache_rfork_budget);
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:
    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_load_afp_conf_vols_locked: the config loader fails closed
 * under lock contention and the failure is state-neutral.  A forked child
 * holds F_WRLCK on the config: load must fail after its one retry with no
 * volume loaded.  After release, the SAME AFPObj must load the volume on
 * the next call (proves the mtime was not consumed by the failed attempt).
 * Regression half: an uncontended load succeeds first try. */
int utest_conf_load_afp_conf_vols_locked(void)
{
    AFPObj obj;
    char logpath[64];
    char body[512];
    int failed = -1;
    struct peer peer;
    int peer_held = 0;
    char *voldir = conf_mkvoldir();

    if (voldir == NULL) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        rmdir(voldir);
        free(voldir);
        return TEST_SKIP;
    }

    snprintf(body, sizeof(body), "[utestvol]\npath = %s\n", voldir);

    /* Regression half: uncontended load succeeds. */
    if (conf_parse_fixture_vols(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (conf_vol_by_name("utestvol") == NULL) {
        fprintf(test_stream(),
                "# utest_conf_load_afp_conf_vols_locked: uncontended load failed\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);

    /* Contended half: parse only, then have a forked peer hold F_WRLCK on
     * the config before the volume load.  POSIX locks are per-process, so
     * the peer's lock genuinely contends with the loader's read_lock. */
    if (conf_parse_fixture(&obj, body, logpath) != 0) {
        goto cleanup;
    }

    if (peer_hold_lock(&peer, obj.options.configfile, F_WRLCK, 0, 0) != 0) {
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    peer_held = 1;

    if (load_afp_conf_vols(&obj, LV_ALL) == 0
            || conf_vol_by_name("utestvol") != NULL
            || conf_log_contains(logpath, "can't lock configfile") != 1) {
        fprintf(test_stream(),
                "# utest_conf_load_afp_conf_vols_locked: locked config did "
                "not fail closed\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    /* Release the peer and prove state-neutrality: the same AFPObj now
     * loads the volume. */
    peer_release(&peer);
    peer_held = 0;

    if (load_afp_conf_vols(&obj, LV_ALL) != 0
            || conf_vol_by_name("utestvol") == NULL) {
        fprintf(test_stream(),
                "# utest_conf_load_afp_conf_vols_locked: load after release "
                "failed - failed attempt was not state-neutral\n");
        conf_teardown(&obj, NULL);
        goto cleanup;
    }

    conf_teardown(&obj, NULL);
    failed = 0;
cleanup:

    if (peer_held) {
        peer_release(&peer);
    }

    unlink(logpath);
    rmdir(voldir);
    free(voldir);
    return failed;
}

/* utest_conf_dircache_resolve_size: bounds behaviour of the pure size
 * resolution helper; touches no dircache state (the binary's live
 * dircache from test.c stays intact). */
int utest_conf_dircache_resolve_size(void)
{
    static const struct {
        int reqsize;
        unsigned int expect;
    } cases[] = {
        {-1, 65536},        /* unset: default */
        {512, 65536},       /* below minimum: default, with warning */
        {1024, 1024},       /* minimum accepted verbatim */
        {100000, 131072},   /* in range: next power of two */
        {2000000, 1048576}, /* above maximum: clamp, with warning */
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        unsigned int got = dircache_resolve_size(cases[i].reqsize);

        if (got != cases[i].expect) {
            fprintf(test_stream(),
                    "# utest_conf_dircache_resolve_size: %d -> %u, want %u\n",
                    cases[i].reqsize, got, cases[i].expect);
            return -1;
        }
    }

    return 0;
}
