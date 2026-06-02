/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * All Rights Reserved.  See COPYRIGHT.
 *
 * Macintosh file format transformer based on megatron by Charles Clark.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/volume.h>
#include <netatalk/endian.h>

#include "nad.h"
#include "hqx.h"
#include "macbin.h"
#include "megatron.h"
#include "nad_adouble.h"

char *forkname[] = { "data", "resource" };
static char forkbuf[8192];

struct archive_mode {
    const char *name;
    int module;
};

static const struct archive_mode archive_modes[] = {
    { "unhex", HEX2NAD },
    { "unbin", BIN2NAD },
    { "bin", NAD2BIN },
    { "hex", NAD2HEX },
};

static void usage_archive(FILE *out)
{
    fprintf(out,
            "Usage: nad [-F configfile] [--force] bin|hex|unbin|unhex [OPTIONS] [FILE...]\n");
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  bin       Encode file to MacBinary (.bin)\n");
    fprintf(out, "  hex       Encode file to BinHex (.hqx)\n");
    fprintf(out, "  unbin     Decode MacBinary (.bin) file\n");
    fprintf(out, "  unhex     Decode BinHex (.hqx) file\n");
    fprintf(out, "\nOptions:\n");
    fprintf(out,
            "  --header         print input header information only, no conversion\n");
    fprintf(out, "  --filename NAME  use NAME in the converted file header\n");
    fprintf(out,
            "  --stdout         write generated archive data to standard output\n");
    fprintf(out,
            "  --adouble        write AppleDouble sidecar metadata outside AFP volumes (default: EA)\n");
    fprintf(out, "  --verbose        print diagnostic conversion details\n");
    fprintf(out, "  -h, --help       show this help\n");
}

static int parse_archive_mode(const char *name)
{
    for (size_t i = 0; i < sizeof(archive_modes) / sizeof(archive_modes[0]); i++) {
        if (strcmp(archive_modes[i].name, name) == 0) {
            return archive_modes[i].module;
        }
    }

    return -1;
}

static int mode_writes_metadata(int module)
{
    return module == HEX2NAD || module == BIN2NAD;
}

static int mode_reads_metadata(int module)
{
    return module == NAD2BIN || module == NAD2HEX;
}

static int mode_creates_volume_file(int module, int flags)
{
    if (flags & OPTION_HEADERONLY) {
        return 0;
    }

    if (mode_writes_metadata(module)) {
        return 1;
    }

    return mode_reads_metadata(module) && !(flags & OPTION_STDOUT);
}

static int from_open(int un, char *file, struct FHeader *fh, int flags)
{
    switch (un) {
    case HEX2NAD:
        return hqx_open(file, O_RDONLY, fh, flags);

    case BIN2NAD:
        return bin_open(file, O_RDONLY, fh, flags);

    case NAD2BIN:
    case NAD2HEX:
        return nad_open(file, O_RDONLY, fh, flags);

    default:
        return -1;
    }
}

static ssize_t from_read(int un, int fork, char *buf, size_t len)
{
    switch (un) {
    case HEX2NAD:
        return hqx_read(fork, buf, len);

    case BIN2NAD:
        return bin_read(fork, buf, len);

    case NAD2BIN:
    case NAD2HEX:
        return nad_read(fork, buf, len);

    default:
        return -1;
    }
}

static int from_close(int un)
{
    switch (un) {
    case HEX2NAD:
        return hqx_close(KEEP);

    case BIN2NAD:
        return bin_close(KEEP);

    case NAD2BIN:
    case NAD2HEX:
        return nad_close(KEEP);

    default:
        return -1;
    }
}

static int to_open(int to, char *file, struct FHeader *fh, int flags)
{
    switch (to) {
    case HEX2NAD:
    case BIN2NAD:
        return nad_open(file, O_RDWR | O_CREAT | O_EXCL, fh, flags);

    case NAD2BIN:
        return bin_open(file, O_RDWR | O_CREAT | O_EXCL, fh, flags);

    case NAD2HEX:
        return hqx_open(file, O_RDWR | O_CREAT | O_EXCL, fh, flags);

    default:
        return -1;
    }
}

static ssize_t to_write(int to, int fork, size_t bufc)
{
    switch (to) {
    case HEX2NAD:
    case BIN2NAD:
        return nad_write(fork, forkbuf, bufc);

    case NAD2BIN:
        return bin_write(fork, forkbuf, bufc);

    case NAD2HEX:
        return hqx_write(fork, forkbuf, bufc);

    default:
        return -1;
    }
}

static int to_close(int to, int keepflag)
{
    switch (to) {
    case HEX2NAD:
    case BIN2NAD:
        return nad_close(keepflag);

    case NAD2BIN:
        return bin_close(keepflag);

    case NAD2HEX:
        return hqx_close(keepflag);

    default:
        return -1;
    }
}

static const char *created_file_path(int module)
{
    switch (module) {
    case NAD2BIN:
        return bin_path();

    case NAD2HEX:
        return hqx_path();

    default:
        return NULL;
    }
}

static int update_created_file_cnid(const struct nad_volume *volume,
                                    const char *path)
{
    struct adouble ad;
    struct stat st;
    struct vol *vol;
    cnid_t cnid;
    cnid_t did;
    int opened = 0;
    int rv = -1;

    if (path == NULL || strcmp(path, STDIN) == 0) {
        return 0;
    }

    if (volume == NULL || volume->fallback || volume->skip_cnid
            || volume->cnid_vol == NULL) {
        return 0;
    }

    if (volume->cnid_vol->v_cdb == NULL) {
        fprintf(stderr, "No CNID database selected for %s\n", path);
        errno = EINVAL;
        return -1;
    }

    vol = volume->cnid_vol;

    if (lstat(path, &st) != 0) {
        perror(path);
        return -1;
    }

    cnid = cnid_for_path(vol->v_cdb, vol->v_path, path, &did);

    if (cnid == CNID_INVALID) {
        fprintf(stderr, "Error resolving CNID for %s\n", path);
        errno = EIO;
        return -1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        fprintf(stderr, "Error opening metadata for %s\n", path);
        return -1;
    }

    opened = 1;

    if (ad_setid(&ad, st.st_dev, st.st_ino, cnid, did,
                 volume->db_stamp) <= 0) {
        fprintf(stderr, "Error storing CNID metadata for %s\n", path);
        errno = EIO;
        goto done;
    }

    if (ad_flush(&ad) < 0) {
        fprintf(stderr, "Error flushing CNID metadata for %s\n", path);
        goto done;
    }

    rv = 0;
done:

    if (opened && ad_close(&ad, ADFLAGS_HF) < 0) {
        return -1;
    }

    return rv;
}

static void print_header_date(const char *label, uint32_t ad_date,
                              int available)
{
    time_t t;
    struct tm tm;
    char buf[32];

    if (!available) {
        printf("%-20s%s\n", label, "(not available)");
        return;
    }

    if (ad_date == AD_DATE_START) {
        printf("%-20s%s\n", label, "(not set)");
        return;
    }

    t = AD_DATE_TO_UNIX(ad_date);

    if (localtime_r(&t, &tm) == NULL ||
            strftime(buf, sizeof(buf), "%a %b %e %H:%M:%S %Y", &tm) == 0) {
        printf("%-20s%s\n", label, "(invalid)");
        return;
    }

    printf("%-20s%s\n", label, buf);
}

static void print_header(const struct FHeader *fh)
{
    char buf[5] = "";
    printf("name:               %s\n", fh->name);
    printf("comment:            %s\n", fh->comment);
    memcpy(&buf, &fh->finder_info.fdCreator, sizeof(uint32_t));
    printf("creator:            '%4s'\n", buf);
    memcpy(&buf, &fh->finder_info.fdType, sizeof(uint32_t));
    printf("type:               '%4s'\n", buf);

    for (int i = 0; i < NUMFORKS; ++i) {
        printf("fork length[%d]:     %u\n", i, ntohl(fh->forklen[i]));
    }

    print_header_date("creation date:", fh->create_date,
                      fh->date_flags & FH_DATE_CREATE);
    print_header_date("modification date:", fh->mod_date,
                      fh->date_flags & FH_DATE_MODIFY);
    print_header_date("backup date:", fh->backup_date,
                      fh->date_flags & FH_DATE_BACKUP);
}

static int nad_archive_convert(char *path, int module, const char *newname,
                               int flags, const struct nad_volume *volume)
{
    struct stat st;
    struct FHeader fh;
    ssize_t bufc;
    size_t forkred;

    if (strcmp(path, STDIN) != 0) {
        if (stat(path, &st) < 0) {
            perror(path);
            return -1;
        }

        if (S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s is a directory.\n", path);
            return 0;
        }
    }

    nad_set_volume(volume);
    memset(&fh, 0, sizeof(fh));

    if (from_open(module, path, &fh, flags) < 0) {
        return -1;
    }

    if (flags & OPTION_HEADERONLY) {
        print_header(&fh);
        return from_close(module);
    }

    if (*newname && strlcpy(fh.name, newname, sizeof(fh.name)) >= sizeof(fh.name)) {
        fprintf(stderr, "Filename is too long: %s\n", newname);
        (void)from_close(module);
        return -1;
    }

    if (to_open(module, path, &fh, flags) < 0) {
        (void)from_close(module);
        return -1;
    }

    for (int fork = 0; fork < NUMFORKS; fork++) {
        forkred = 0;

        while ((bufc = from_read(module, fork, forkbuf, sizeof(forkbuf))) > 0) {
            if (to_write(module, fork, bufc) != bufc) {
                fprintf(stderr, "%s: probable write error\n", path);
                to_close(module, TRASH);
                (void)from_close(module);
                return -1;
            }

            forkred += bufc;
        }

        if ((bufc < 0) || (forkred != ntohl(fh.forklen[fork]))) {
            fprintf(stderr, "%s: input fork length mismatch\n", path);
            to_close(module, TRASH);
            (void)from_close(module);
            return -1;
        }
    }

    if (to_close(module, KEEP) < 0) {
        perror("nad");

        if (!mode_writes_metadata(module)) {
            (void)to_close(module, TRASH);
        }

        (void)from_close(module);
        return -1;
    }

    if (mode_reads_metadata(module) && !(flags & OPTION_STDOUT)
            && update_created_file_cnid(volume, created_file_path(module)) < 0) {
        (void)from_close(module);
        return -1;
    }

    return from_close(module);
}

static void close_metadata_volume(struct nad_volume *volume);

static int validate_volume_metadata(const char *volpath, const struct vol *vol)
{
    if (vol->v_adouble != AD_VERSION2 && vol->v_adouble != AD_VERSION_EA) {
        fprintf(stderr, "\"%s\" uses unsupported metadata format %u\n",
                volpath, vol->v_adouble);
        return -1;
    }

    return 0;
}

static struct vol fallback_volume;

static void set_fallback_volume(struct nad_volume *volume, int flags)
{
    memset(&fallback_volume, 0, sizeof(fallback_volume));
    fallback_volume.v_path = ".";
    fallback_volume.v_adouble = (flags & OPTION_ADOUBLE)
                                ? AD_VERSION2 : AD_VERSION_EA;
    fallback_volume.v_ad_options = 0;

    if (fallback_volume.v_adouble == AD_VERSION2) {
        fallback_volume.ad_path = ad_path;
    } else {
#if defined(HAVE_EAFD) && defined(SOLARIS)
        fallback_volume.ad_path = ad_path_ea;
#else
        fallback_volume.ad_path = ad_path_osx;
#endif
    }

    volume->vol = &fallback_volume;
    volume->fallback = true;
    volume->skip_cnid = true;
}

static int resolve_metadata_volume(AFPObj *obj, int module,
                                   const char *path, int flags,
                                   struct nad_volume *volume)
{
    const char *volpath = path;
    struct vol *vol;
    struct vol *cnid_vol;
    int cnid_flags = 0;
    int open_cnid = mode_creates_volume_file(module, flags);
    memset(volume, 0, sizeof(*volume));

    if (mode_writes_metadata(module) || strcmp(path, STDIN) == 0) {
        volpath = ".";
    }

    vol = getvolbypath(obj, volpath);

    if (vol == NULL || vol->v_path == NULL) {
        if (!forceflag) {
            nad_not_inside_volume(stderr, volpath, 1);
            return -1;
        }

        set_fallback_volume(volume, flags);
        return 0;
    }

    if (validate_volume_metadata(volpath, vol) < 0) {
        return -1;
    }

    volume->vol = vol;

    if (!open_cnid) {
        return 0;
    }

    cnid_vol = getvolbypath(obj, ".");

    if (cnid_vol == NULL || cnid_vol->v_path == NULL) {
        if (!forceflag) {
            nad_not_inside_volume(stderr, ".", 1);
            close_metadata_volume(volume);
            return -1;
        }

        volume->skip_cnid = true;
        return 0;
    }

    if (validate_volume_metadata(".", cnid_vol) < 0) {
        return -1;
    }

    volume->cnid_vol = cnid_vol;

    if (cnid_vol->v_cdb == NULL) {
        if (cnid_vol->v_flags & AFPVOL_NODEV) {
            cnid_flags |= CNID_FLAG_NODEV;
        }

        cnid_vol->v_cdb = cnid_open(cnid_vol, cnid_vol->v_cnidscheme,
                                    cnid_flags);

        if (cnid_vol->v_cdb == NULL) {
            fprintf(stderr, "Can't initialize CNID database connection for %s\n",
                    cnid_vol->v_path);
            volume->vol = NULL;
            volume->cnid_vol = NULL;
            return -1;
        }

        volume->owns_cdb = true;
    }

    if (cnid_getstamp(cnid_vol->v_cdb, volume->db_stamp,
                      sizeof(volume->db_stamp)) != 0) {
        fprintf(stderr, "Can't read CNID database stamp for %s\n",
                cnid_vol->v_path);
        close_metadata_volume(volume);
        return -1;
    }

    return 0;
}

static void close_metadata_volume(struct nad_volume *volume)
{
    if (volume->cnid_vol && volume->owns_cdb && volume->cnid_vol->v_cdb) {
        cnid_close(volume->cnid_vol->v_cdb);
        volume->cnid_vol->v_cdb = NULL;
    }

    memset(volume, 0, sizeof(*volume));
}

static int arg_is_option_with_value(const char *arg)
{
    return strcmp(arg, "--filename") == 0;
}

static int arg_is_flag_option(const char *arg)
{
    return strcmp(arg, "--header") == 0
           || strcmp(arg, "--stdout") == 0
           || strcmp(arg, "--adouble") == 0
           || strcmp(arg, "--verbose") == 0;
}

static int set_newname(char *newname, size_t newname_len, const char *name)
{
    if (strlcpy(newname, name, newname_len) >= newname_len) {
        fprintf(stderr, "nad: filename is too long: %s\n", name);
        return -1;
    }

    return 0;
}

static int option_after_file(const char *arg)
{
    fprintf(stderr, "nad: option '%s' appears after a FILE argument\n", arg);
    fprintf(stderr, "nad: place all options before FILE arguments\n");
    return 1;
}

int nad_archive(int argc, char **argv, AFPObj *obj)
{
    int rc;
    int rv = 0;
    int module;
    int flags = 0;
    int saw_file = 0;
    int saw_positional = 0;
    int argi = 1;
    char newname[ADEDLEN_NAME + 1];
    newname[0] = '\0';

    if (argc < 1) {
        usage_archive(stderr);
        return 1;
    }

    if (argc > 1 && (strcmp(argv[1], "--help") == 0
                     || strcmp(argv[1], "-h") == 0)) {
        usage_archive(stdout);
        return 0;
    }

    module = parse_archive_mode(argv[0]);

    if (module < 0) {
        usage_archive(stderr);
        return 1;
    }

    cnid_init();

    for (int i = argi; i < argc;) {
        const char *arg = argv[i];

        if (arg_is_flag_option(arg)) {
            if (saw_positional) {
                option_after_file(arg);
                usage_archive(stderr);
                return 1;
            }

            if (strcmp(arg, "--header") == 0) {
                flags |= OPTION_HEADERONLY;
            } else if (strcmp(arg, "--stdout") == 0) {
                flags |= OPTION_STDOUT;
            } else if (strcmp(arg, "--adouble") == 0) {
                flags |= OPTION_ADOUBLE;
            } else if (strcmp(arg, "--verbose") == 0) {
                flags |= OPTION_VERBOSE;
            }

            i++;
            continue;
        }

        if (arg_is_option_with_value(arg)) {
            if (saw_positional) {
                option_after_file(arg);
                usage_archive(stderr);
                return 1;
            }

            if (i + 1 >= argc) {
                fprintf(stderr, "nad: %s requires a name\n", arg);
                usage_archive(stderr);
                return 1;
            }

            i += 2;
            continue;
        }

        if ((arg[0] == '-') && (strcmp(arg, STDIN) != 0)) {
            fprintf(stderr, "nad: unknown option '%s'\n", arg);
            usage_archive(stderr);
            return 1;
        }

        saw_positional = 1;
        i++;
    }

    nad_log_verbose = (flags & OPTION_VERBOSE) != 0;

    while (argi < argc) {
        const char *arg = argv[argi];
        const char *effective_newname = newname;
        struct nad_volume volume;

        if (arg_is_flag_option(arg)) {
            argi++;
            continue;
        }

        if (strcmp(arg, "--filename") == 0) {
            argi++;

            if (argi >= argc) {
                fprintf(stderr, "nad: --filename requires a name\n");
                usage_archive(stderr);
                return 1;
            }

            if (set_newname(newname, sizeof(newname), argv[argi]) != 0) {
                return 1;
            }

            argi++;
            continue;
        }

        saw_file = 1;

        if (resolve_metadata_volume(obj, module, arg, flags, &volume) != 0) {
            rv = 1;
            argi++;
            continue;
        }

        rc = nad_archive_convert(argv[argi], module, effective_newname, flags,
                                 &volume);

        if (rc != 0) {
            rv = 1;
        }

        close_metadata_volume(&volume);
        newname[0] = '\0';
        argi++;
    }

    if (!saw_file) {
        struct nad_volume volume;

        if (mode_reads_metadata(module)) {
            fprintf(stderr, "nad: command requires at least one path\n");
            return 1;
        }

        if (resolve_metadata_volume(obj, module, STDIN, flags, &volume) != 0) {
            rv = 1;
        } else {
            if (nad_archive_convert(STDIN, module, newname, flags, &volume) != 0) {
                rv = 1;
            }

            close_metadata_volume(&volume);
        }
    }

    return rv;
}
