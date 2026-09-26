/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

static unsigned count, failed;

static void check(int condition, const char *label)
{
    printf("%s %u - %s\n", condition ? "ok" : "not ok", ++count, label);
    failed += !condition;
}

static void create_file(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);

    if (fd == -1 || write(fd, "data\n", 5) != 5 || close(fd) != 0) {
        printf("Bail out! Cannot create %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

static int read_header(struct vol *vol, const char *path, struct adouble *ad)
{
    ad_init(ad, vol);
    errno = 0;
    return ad_open(ad, path, ADFLAGS_HF | ADFLAGS_RDONLY);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        return 1;
    }

    char bad[MAXPATHLEN], good[MAXPATHLEN], empty[MAXPATHLEN];
    char missing[MAXPATHLEN], notdir[MAXPATHLEN], target[MAXPATHLEN];
    char copied[MAXPATHLEN];
    snprintf(bad, sizeof(bad), "%s/metadata_bad", argv[1]);
    snprintf(good, sizeof(good), "%s/metadata_good", argv[1]);
    snprintf(empty, sizeof(empty), "%s/metadata_empty", argv[1]);
    snprintf(missing, sizeof(missing), "%s/metadata_missing", argv[1]);
    snprintf(notdir, sizeof(notdir), "%s/metadata_bad/child", argv[1]);
    snprintf(target, sizeof(target), "%s/metadata_target", argv[1]);
    snprintf(copied, sizeof(copied), "%s/metadata_target/metadata_good", argv[1]);
    create_file(bad);

    if (sys_setxattr(bad, AD_EA_META, "bad!", 4, 0) != 0) {
        if (errno == ENOTSUP || errno == ENOSYS) {
            puts("1..0 # SKIP filesystem does not support native EAs");
            return 0;
        }

        printf("Bail out! Cannot set test EA: %s\n", strerror(errno));
        return 1;
    }

    struct vol vol = {0};

    vol.v_path = argv[1];

    vol.v_adouble = AD_VERSION_EA;

    vol.v_vfs_ea = AFPVOL_EA_SYS;

    vol.v_volcodepage = "UTF8";

    vol.v_maccodepage = "MAC_ROMAN";

    initvol_vfs(&vol);

    AFPObj obj = {0};

    obj.cmdlineconfigfile = argv[2];

    if (afp_config_parse(&obj, "nad metadata tests") != 0
            || load_charset(&vol) != 0) {
        puts("Bail out! Cannot load test charsets");
        return 1;
    }

    struct adouble ad;

    int ret = read_header(&vol, missing, &ad);
    check(ret != 0 && errno == ENOENT, "missing metadata reports ENOENT");
    ret = read_header(&vol, notdir, &ad);
    check(ret != 0 && errno == ENOTDIR, "non-missing EA read errors retain errno");
    ret = read_header(&vol, bad, &ad);
    check(ret != 0 && errno == EINVAL, "corrupt metadata reports EINVAL");
    char buf[AD_DATASZ_EA];
    check(sys_getxattr(bad, AD_EA_META, buf, sizeof(buf)) == 4
          && memcmp(buf, "bad!", 4) == 0, "read failure leaves source metadata intact");
    create_file(empty);
    check(sys_setxattr(empty, AD_EA_META, "", 0, 0) == 0,
          "empty EA fixture created");
    ret = read_header(&vol, empty, &ad);
    check(ret != 0
          && errno == EINVAL, "empty metadata is corrupt rather than missing");
    check(sys_getxattr(empty, AD_EA_META, buf, sizeof(buf)) == 0,
          "read failure preserves an empty metadata EA");
    ad_init(&ad, &vol);
    ret = ad_open(&ad, empty, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666);
    check(ret == 0, "explicit creation can repair invalid metadata");

    if (ret == 0) {
        ad_close(&ad, ADFLAGS_HF);
    }

    check(sys_getxattr(empty, AD_EA_META, buf, sizeof(buf)) == AD_DATASZ_EA,
          "repair creates a complete metadata header");
    create_file(good);
    ad_init(&ad, &vol);
    ret = ad_open(&ad, good, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666);
    check(ret == 0, "valid source metadata created");

    if (ret != 0) {
        puts("Bail out! Cannot create source metadata");
        return 1;
    }

    memcpy(ad_entry(&ad, ADEID_COMMENT), "comment", 7);
    ad_setentrylen(&ad, ADEID_COMMENT, 7);
    ad_setdate(&ad, AD_DATE_CREATE | AD_DATE_UNIX, 1700000000);
    int saved = ad_flush(&ad);
    check(ad_close(&ad, ADFLAGS_HF) == 0
          && saved == 0, "source comment and creation date saved");
    check(mkdir(target, 0700) == 0, "multi-source target created");
    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
        int nullfd = open("/dev/null", O_WRONLY);

        if (nullfd == -1 || dup2(nullfd, STDOUT_FILENO) == -1
                || dup2(nullfd, STDERR_FILENO) == -1) {
            _exit(127);
        }

        close(nullfd);
        execl(argv[3], argv[3], "-F", argv[2], "cp", bad, good, target, (char *)NULL);
        _exit(127);
    }

    int status;

    if (pid < 0 || waitpid(pid, &status, 0) != pid) {
        puts("Bail out! Cannot run nad copy");
        return 1;
    }

    check(WIFEXITED(status) && WEXITSTATUS(status) == 1,
          "cp retains a metadata error when a later source succeeds");
    check(sys_getxattr(bad, AD_EA_META, buf, sizeof(buf)) == 4,
          "cp does not silently delete corrupt source metadata");
    ret = read_header(&vol, copied, &ad);
    check(ret == 0, "later source still receives usable metadata");

    if (ret == 0) {
        check(ad_getentrylen(&ad, ADEID_COMMENT) == 7
              && memcmp(ad_entry(&ad, ADEID_COMMENT), "comment", 7) == 0,
              "cp preserves the Finder comment");
        uint32_t date;
        check(ad_getdate(&ad, AD_DATE_CREATE | AD_DATE_UNIX, &date) == 0
              && date == 1700000000, "cp preserves the metadata creation date");
        ad_close(&ad, ADFLAGS_HF);
    }

    printf("1..%u\n", count);
    return failed != 0;
}
