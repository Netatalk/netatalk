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
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

#include <stuffit_ffi.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "nad.h"
#include "megatron.h"
#include "nad_adouble.h"

#define SIT_DEFAULT_METHOD 13

static void usage_stuffit(FILE *out)
{
    fprintf(out,
            "Usage: nad [-F configfile] [--force] sit [OPTIONS] FILE|DIR [FILE|DIR ...]\n");
    fprintf(out,
            "       nad [-F configfile] [--force] unsit [OPTIONS] ARCHIVE.sit [ARCHIVE.sit ...]\n");
    fprintf(out, "\nCommands:\n");
    fprintf(out, "  sit       Encode files and directories to StuffIt (.sit)\n");
    fprintf(out, "  unsit     Decode StuffIt (.sit) archive entries\n");
    fprintf(out, "\nOptions:\n");
    fprintf(out, "  -o, --output FILE  write sit output to FILE\n");
    fprintf(out,
            "  --method METHOD    compression method for sit output (default: 13)\n");
    fprintf(out,
            "  --verbose          print diagnostic conversion details\n");
    fprintf(out, "  -h, --help         show this help\n");
}

static int set_nad_volume_from_path(AFPObj *obj, const char *path,
                                    afpvol_t *afpvol,
                                    struct nad_volume *volume)
{
    memset(volume, 0, sizeof(*volume));

    if (openvol_optional(obj, path, afpvol) != 0) {
        return -1;
    }

    volume->vol = afpvol->vol;
    volume->cnid_vol = afpvol->vol;
    volume->fallback = afpvol->vol->v_path == NULL;
    volume->skip_cnid = afpvol->vol->v_path == NULL || afpvol->vol->v_cdb == NULL;

    if (!volume->skip_cnid) {
        memcpy(volume->db_stamp, afpvol->db_stamp, sizeof(volume->db_stamp));
    }

    nad_set_volume(volume);
    return 0;
}

static int entry_name_is_safe(const char *name)
{
    const char *p = name;
    size_t comp_len = 0;

    if (name == NULL || *name == '\0' || *name == '/') {
        return 0;
    }

    while (1) {
        if (*p == '/' || *p == '\0') {
            if (comp_len == 0) {
                return 0;
            }

            if ((comp_len == 1 && p[-1] == '.')
                    || (comp_len == 2 && p[-2] == '.' && p[-1] == '.')) {
                return 0;
            }

            comp_len = 0;

            if (*p == '\0') {
                return 1;
            }
        } else {
            comp_len++;
        }

        p++;
    }
}

static int make_parent_dirs(const char *path)
{
    char buf[MAXPATHLEN + 1];
    char *slash;

    if (strlcpy(buf, path, sizeof(buf)) >= sizeof(buf)) {
        fprintf(stderr, "Path is too long: %s\n", path);
        return -1;
    }

    for (slash = strchr(buf, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
        *slash = '\0';

        if (mkdir(buf, 0777) < 0 && errno != EEXIST) {
            perror(buf);
            return -1;
        }

        *slash = '/';
    }

    return 0;
}

static int make_dir_path(const char *path)
{
    char buf[MAXPATHLEN + 1];
    char *slash;

    if (strlcpy(buf, path, sizeof(buf)) >= sizeof(buf)) {
        fprintf(stderr, "Path is too long: %s\n", path);
        return -1;
    }

    for (slash = buf; *slash != '\0'; slash++) {
        if (*slash != '/') {
            continue;
        }

        *slash = '\0';

        if (*buf != '\0' && mkdir(buf, 0777) < 0 && errno != EEXIST) {
            perror(buf);
            return -1;
        }

        *slash = '/';
    }

    if (mkdir(buf, 0777) < 0 && errno != EEXIST) {
        perror(buf);
        return -1;
    }

    return 0;
}

static const char *path_basename(const char *path)
{
    const char *base = strrchr(path, '/');
    return base == NULL ? path : base + 1;
}

static int split_parent_base(const char *path, char *parent, size_t parent_len,
                             char *base, size_t base_len)
{
    const char *slash = strrchr(path, '/');

    if (slash == NULL) {
        if (strlcpy(parent, ".", parent_len) >= parent_len
                || strlcpy(base, path, base_len) >= base_len) {
            fprintf(stderr, "Path is too long: %s\n", path);
            return -1;
        }

        return 0;
    }

    if ((size_t)(slash - path) >= parent_len
            || strlcpy(base, slash + 1, base_len) >= base_len) {
        fprintf(stderr, "Path is too long: %s\n", path);
        return -1;
    }

    memcpy(parent, path, (size_t)(slash - path));
    parent[slash - path] = '\0';

    if (parent[0] == '\0') {
        strlcpy(parent, ".", parent_len);
    }

    return 0;
}

static int fill_header_from_entry(const StuffitEntryInfo *info,
                                  struct FHeader *fh)
{
    uint32_t ad_time = AD_DATE_FROM_UNIX(time(NULL));
    const char *base = path_basename(info->name);
    memset(fh, 0, sizeof(*fh));

    if (strlcpy(fh->name, base, sizeof(fh->name)) >= sizeof(fh->name)) {
        fprintf(stderr, "Mac filename is too long: %s\n", base);
        return -1;
    }

    fh->forklen[DATA] = htonl((uint32_t)info->data_len);
    fh->forklen[RESOURCE] = htonl((uint32_t)info->resource_len);

    if (info->creation_date != 0) {
        fh->create_date = AD_DATE_FROM_MAC(htonl((uint32_t)info->creation_date));
    } else {
        fh->create_date = ad_time;
    }

    if (info->modification_date != 0) {
        fh->mod_date = AD_DATE_FROM_MAC(htonl((uint32_t)info->modification_date));
    } else {
        fh->mod_date = ad_time;
    }

    fh->backup_date = AD_DATE_START;
    fh->date_flags = FH_DATE_CREATE | FH_DATE_MODIFY;
    memcpy(&fh->finder_info.fdType, info->file_type,
           sizeof(fh->finder_info.fdType));
    memcpy(&fh->finder_info.fdCreator, info->creator,
           sizeof(fh->finder_info.fdCreator));
    fh->finder_info.fdFlags = htons(info->finder_flags);
    return 0;
}

static int write_fork_bytes(int fork, const uint8_t *ptr, size_t len)
{
    size_t written = 0;

    while (written < len) {
        size_t chunk = len - written;

        if (chunk > 8192) {
            chunk = 8192;
        }

        if (nad_write(fork, (char *)(ptr + written), chunk) != (ssize_t)chunk) {
            return -1;
        }

        written += chunk;
    }

    return 0;
}

static int extract_file_entry(const StuffitEntryInfo *info,
                              const StuffitBytes *data,
                              const StuffitBytes *rsrc)
{
    struct FHeader fh;
    struct utimbuf filetimestamp;
    char parent[MAXPATHLEN + 1];
    char base[ADEDLEN_NAME + 1];
    int cwd = -1;
    int opened = 0;
    int rc = -1;

    if (make_parent_dirs(info->name) < 0
            || split_parent_base(info->name, parent, sizeof(parent),
                                 base, sizeof(base)) < 0
            || fill_header_from_entry(info, &fh) < 0) {
        return -1;
    }

    if (strlcpy(fh.name, base, sizeof(fh.name)) >= sizeof(fh.name)) {
        fprintf(stderr, "Mac filename is too long: %s\n", base);
        return -1;
    }

    cwd = open(".", O_RDONLY);

    if (cwd < 0) {
        perror("open of . failed");
        return -1;
    }

    if (chdir(parent) < 0) {
        perror(parent);
        goto done;
    }

    if (nad_open(base, O_RDWR | O_CREAT | O_EXCL, &fh, OPTION_NONE) < 0) {
        goto done;
    }

    opened = 1;

    if (write_fork_bytes(DATA, data->ptr, data->len) < 0
            || write_fork_bytes(RESOURCE, rsrc->ptr, rsrc->len) < 0) {
        goto done;
    }

    filetimestamp.modtime = AD_DATE_TO_UNIX(fh.mod_date);
    filetimestamp.actime = time(NULL);

    if (utime(base, &filetimestamp) < 0) {
        perror("unable to change modification date");
        goto done;
    }

    if (nad_close(KEEP) < 0) {
        opened = 0;
        goto done;
    }

    opened = 0;
    rc = 0;
done:

    if (opened) {
        (void)nad_close(TRASH);
    }

    if (fchdir(cwd) < 0) {
        perror("restoring working directory failed");
        rc = -1;
    }

    close(cwd);
    return rc;
}

static int unsit_archive(const char *path, AFPObj *obj)
{
    StuffitArchive *archive;
    afpvol_t afpvol;
    struct nad_volume volume;
    size_t count;
    int rv = 0;

    if (set_nad_volume_from_path(obj, ".", &afpvol, &volume) < 0) {
        return -1;
    }

    archive = stuffit_archive_parse_file(path);

    if (archive == NULL) {
        fprintf(stderr, "%s: %s\n", path, stuffit_last_error());
        closevol(&afpvol);
        return -1;
    }

    count = stuffit_archive_entry_count(archive);
    NAD_DEBUG("%s: %zu StuffIt entries\n", path, count);

    for (size_t i = 0; i < count; i++) {
        StuffitEntryInfo info;
        StuffitBytes data;
        StuffitBytes rsrc;

        if (stuffit_archive_entry_info(archive, i, &info) != 0
                || stuffit_archive_entry_fork(archive, i, STUFFIT_FORK_DATA,
                                              &data) != 0
                || stuffit_archive_entry_fork(archive, i,
                                              STUFFIT_FORK_RESOURCE, &rsrc) != 0) {
            fprintf(stderr, "%s: %s\n", path, stuffit_last_error());
            rv = -1;
            break;
        }

        if (!entry_name_is_safe(info.name)) {
            fprintf(stderr, "%s: unsafe StuffIt entry name: %s\n", path, info.name);
            rv = -1;
            break;
        }

        if (info.is_folder) {
            if (make_dir_path(info.name) < 0) {
                rv = -1;
                break;
            }

            continue;
        }

        if (extract_file_entry(&info, &data, &rsrc) < 0) {
            rv = -1;
            break;
        }
    }

    stuffit_archive_free(archive);
    closevol(&afpvol);
    return rv;
}

static int read_fork_bytes(int fork, size_t len, uint8_t **out)
{
    uint8_t *buf;
    size_t done = 0;
    *out = NULL;

    if (len == 0) {
        return 0;
    }

    buf = malloc(len);

    if (buf == NULL) {
        perror("malloc");
        return -1;
    }

    while (done < len) {
        size_t chunk = len - done;
        ssize_t cc;

        if (chunk > 8192) {
            chunk = 8192;
        }

        cc = nad_read(fork, (char *)(buf + done), chunk);

        if (cc <= 0) {
            fprintf(stderr, "Unexpected short read from %s fork\n", forkname[fork]);
            free(buf);
            return -1;
        }

        done += (size_t)cc;
    }

    *out = buf;
    return 0;
}

static int add_file_to_writer(StuffitWriter *writer, const char *path,
                              const char *entry_name, AFPObj *obj)
{
    struct FHeader fh;
    StuffitNewEntry entry;
    afpvol_t afpvol;
    struct nad_volume volume;
    uint8_t *data = NULL;
    uint8_t *rsrc = NULL;
    size_t data_len;
    size_t rsrc_len;
    int rv = -1;

    if (set_nad_volume_from_path(obj, path, &afpvol, &volume) < 0) {
        return -1;
    }

    if (nad_open((char *)path, O_RDONLY, &fh, OPTION_NONE) < 0) {
        closevol(&afpvol);
        return -1;
    }

    data_len = ntohl(fh.forklen[DATA]);
    rsrc_len = ntohl(fh.forklen[RESOURCE]);

    if (read_fork_bytes(DATA, data_len, &data) < 0
            || read_fork_bytes(RESOURCE, rsrc_len, &rsrc) < 0) {
        goto done;
    }

    memset(&entry, 0, sizeof(entry));
    entry.name = entry_name;
    entry.creation_date = htonl(AD_DATE_TO_MAC(fh.create_date));
    entry.modification_date = htonl(AD_DATE_TO_MAC(fh.mod_date));
    entry.data_fork.ptr = data;
    entry.data_fork.len = data_len;
    entry.resource_fork.ptr = rsrc;
    entry.resource_fork.len = rsrc_len;
    memcpy(entry.file_type, &fh.finder_info.fdType, sizeof(entry.file_type));
    memcpy(entry.creator, &fh.finder_info.fdCreator, sizeof(entry.creator));
    entry.finder_flags = ntohs(fh.finder_info.fdFlags);

    if (stuffit_writer_add_entry(writer, &entry) != 0) {
        fprintf(stderr, "%s: %s\n", path, stuffit_last_error());
        goto done;
    }

    rv = 0;
done:
    free(data);
    free(rsrc);
    (void)nad_close(KEEP);
    closevol(&afpvol);
    return rv;
}

static int add_folder_to_writer(StuffitWriter *writer, const char *entry_name)
{
    StuffitNewEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.name = entry_name;
    entry.is_folder = 1;

    if (stuffit_writer_add_entry(writer, &entry) != 0) {
        fprintf(stderr, "%s: %s\n", entry_name, stuffit_last_error());
        return -1;
    }

    return 0;
}

static int join_path(char *out, size_t out_len, const char *dir,
                     const char *name)
{
    if (snprintf(out, out_len, "%s/%s", dir, name) >= (int)out_len) {
        fprintf(stderr, "Path is too long: %s/%s\n", dir, name);
        return -1;
    }

    return 0;
}

static int is_ad_metadata_entry(const char *name)
{
    return strncmp(name, "._", 2) == 0
           || strcmp(name, ".AppleDouble") == 0
           || strcmp(name, ".AppleDB") == 0
           || strcmp(name, ".AppleDesktop") == 0;
}

static int add_path_to_writer(StuffitWriter *writer, const char *path,
                              const char *entry_name, AFPObj *obj)
{
    struct stat st;

    if (lstat(path, &st) < 0) {
        perror(path);
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir;
        struct dirent *de;

        if (add_folder_to_writer(writer, entry_name) < 0) {
            return -1;
        }

        dir = opendir(path);

        if (dir == NULL) {
            perror(path);
            return -1;
        }

        while ((de = readdir(dir)) != NULL) {
            char child_path[MAXPATHLEN + 1];
            char child_entry[MAXPATHLEN + 1];

            if (DIR_DOT_OR_DOTDOT(de->d_name)
                    || is_ad_metadata_entry(de->d_name)) {
                continue;
            }

            if (join_path(child_path, sizeof(child_path), path, de->d_name) < 0
                    || join_path(child_entry, sizeof(child_entry), entry_name,
                                 de->d_name) < 0
                    || add_path_to_writer(writer, child_path, child_entry, obj) < 0) {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);
        return 0;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "%s is not a regular file or directory.\n", path);
        return -1;
    }

    return add_file_to_writer(writer, path, entry_name, obj);
}

static char *default_sit_output(const char *path)
{
    size_t len = strlen(path) + 5;
    char *out = malloc(len);

    if (out == NULL) {
        perror("malloc");
        return NULL;
    }

    snprintf(out, len, "%s.sit", path);
    return out;
}

static int write_owned_bytes(const char *path, StuffitOwnedBytes *bytes)
{
    FILE *out = fopen(path, "wb");

    if (out == NULL) {
        perror(path);
        return -1;
    }

    if (fwrite(bytes->ptr, 1, bytes->len, out) != bytes->len) {
        perror(path);
        fclose(out);
        return -1;
    }

    if (fclose(out) != 0) {
        perror(path);
        return -1;
    }

    return 0;
}

static int sit_paths(int argc, char **argv, AFPObj *obj, const char *output,
                     uint8_t method)
{
    StuffitWriter *writer;
    StuffitOwnedBytes bytes;
    const char *outpath = output;
    char *default_out = NULL;
    int rv = -1;

    if (argc <= 0) {
        fprintf(stderr, "nad: sit requires at least one file or directory\n");
        return -1;
    }

    if (outpath == NULL) {
        if (argc != 1) {
            fprintf(stderr, "nad: sit with multiple inputs requires -o FILE\n");
            return -1;
        }

        default_out = default_sit_output(argv[0]);

        if (default_out == NULL) {
            return -1;
        }

        outpath = default_out;
    }

    writer = stuffit_writer_new();

    if (writer == NULL) {
        fprintf(stderr, "nad: %s\n", stuffit_last_error());
        free(default_out);
        return -1;
    }

    for (int i = 0; i < argc; i++) {
        if (add_path_to_writer(writer, argv[i], path_basename(argv[i]), obj) < 0) {
            goto done;
        }
    }

    if (stuffit_writer_finish(writer, method, &bytes) != 0) {
        fprintf(stderr, "nad: %s\n", stuffit_last_error());
        goto done;
    }

    if (write_owned_bytes(outpath, &bytes) == 0) {
        rv = 0;
    }

    stuffit_owned_bytes_free(bytes);
done:
    stuffit_writer_free(writer);
    free(default_out);
    return rv;
}

int nad_stuffit(int argc, char **argv, AFPObj *obj)
{
    int rv = 0;
    int argi = 1;
    uint8_t method = SIT_DEFAULT_METHOD;
    const char *output = NULL;
    int extract = 0;

    if (argc < 1) {
        usage_stuffit(stderr);
        return 1;
    }

    if (strcmp(argv[0], "unsit") == 0) {
        extract = 1;
    } else if (strcmp(argv[0], "sit") != 0) {
        usage_stuffit(stderr);
        return 1;
    }

    if (argc > 1 && (strcmp(argv[1], "--help") == 0
                     || strcmp(argv[1], "-h") == 0)) {
        usage_stuffit(stdout);
        return 0;
    }

    cnid_init();

    while (argi < argc) {
        const char *arg = argv[argi];

        if (strcmp(arg, "--verbose") == 0) {
            nad_log_verbose = 1;
            argi++;
            continue;
        }

        if (!extract && (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0)) {
            if (++argi >= argc) {
                fprintf(stderr, "nad: %s requires a file name\n", arg);
                usage_stuffit(stderr);
                return 1;
            }

            output = argv[argi++];
            continue;
        }

        if (!extract && strcmp(arg, "--method") == 0) {
            unsigned long parsed;
            char *end = NULL;

            if (++argi >= argc) {
                fprintf(stderr, "nad: --method requires a number\n");
                usage_stuffit(stderr);
                return 1;
            }

            parsed = strtoul(argv[argi], &end, 10);

            if (*argv[argi] == '\0' || *end != '\0' || parsed > UINT8_MAX) {
                fprintf(stderr, "nad: invalid StuffIt method: %s\n", argv[argi]);
                return 1;
            }

            method = (uint8_t)parsed;
            argi++;
            continue;
        }

        if (arg[0] == '-' && strcmp(arg, STDIN) != 0) {
            fprintf(stderr, "nad: unknown option '%s'\n", arg);
            usage_stuffit(stderr);
            return 1;
        }

        break;
    }

    if (extract) {
        if (argi >= argc) {
            fprintf(stderr, "nad: unsit requires at least one archive\n");
            return 1;
        }

        for (; argi < argc; argi++) {
            if (unsit_archive(argv[argi], obj) < 0) {
                rv = 1;
            }
        }

        return rv;
    }

    return sit_paths(argc - argi, argv + argi, obj, output, method) == 0 ? 0 : 1;
}
