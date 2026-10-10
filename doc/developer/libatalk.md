# Using libatalk in Applications

libatalk provides Netatalk's shared routines for AppleDouble metadata,
resource forks, CNID databases, character conversion, and network protocols.
Netatalk's daemons and utilities use these routines, and other applications
can use them to work with Netatalk-managed files and metadata.

To use libatalk in your application, link against the shared library built
and installed by Netatalk.

The example below demonstrates linking against libatalk to copy metadata
and resource forks between existing files in a configured Netatalk volume.

## Example: Copying File Metadata and Resource Forks

This example links a small program with the libatalk shared library.
Assuming Netatalk has been installed to /usr/local/netatalk, compiling backup_demo.c is done like this:

```shell
c99 -I/usr/local/netatalk/include -o backup_demo backup_demo.c -L/usr/local/netatalk/lib -latalk -Wl,-rpath=/usr/local/netatalk/lib
```

The following *backup_demo.c* copies AppleDouble metadata, the resource fork, and
extended attributes to an existing file. It leaves the destination's data fork
unchanged. Both arguments must be distinct regular files in the same writable
Netatalk volume, and the source must have AppleDouble metadata. A missing resource
fork is treated as empty, replacing any existing destination resource fork.

Run the example with afpd stopped and without concurrent changes to either file.
The copy is not atomic: an error can leave partially updated destination metadata.
The program clears the copied CNID cache so afpd can resolve the destination's
identity from its own inode when it next accesses the file; it does not update the
CNID database itself.

```c
/*
 *  Copyright (c) 2012, Frank Lahm
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#define ERROR(...)                              \
    do {                                        \
        _log(__VA_ARGS__);                      \
        goto cleanup;                           \
    } while (0)

static void _log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

/* ad_copy_header() excludes comments; a file copy should preserve them. */
static int copy_comment(struct adouble *dst, const struct adouble *src)
{
    size_t len = ad_getentrylen(src, ADEID_COMMENT);
    const void *comment = ad_entry(src, ADEID_COMMENT);
    void *newcomment = ad_entry(dst, ADEID_COMMENT);

    if (len > ADEDLEN_COMMENT
            || (len != 0 && (comment == NULL || newcomment == NULL))
            || !ad_entry_fits(dst, ADEID_COMMENT, (uint32_t)len)) {
        errno = EIO;
        return -1;
    }

    if (len != 0) {
        memcpy(newcomment, comment, len);
    }

    ad_setentrylen(dst, ADEID_COMMENT, len);
    return 0;
}

int main(int argc, char **argv)
{
    char *file = NULL, *newfile = NULL;
    char macname[MAXPATHLEN + 2] = { 0 };
    const char empty_stamp[ADEDLEN_PRIVSYN] = { 0 };
    struct vol *vol, *newvol;
    struct adouble ad, adnew;
    struct stat st, newst;
    AFPObj obj = { 0 };
    int ad_initialized = 0, adnew_initialized = 0, config_started = 0;
    int status = EXIT_FAILURE;

    if (argc != 3) {
        ERROR("usage: backup_demo FILE NEWFILE");
    }

    if (lstat(argv[1], &st) != 0 || lstat(argv[2], &newst) != 0) {
        ERROR("Both files must exist: %s", strerror(errno));
    }

    if (!S_ISREG(st.st_mode) || !S_ISREG(newst.st_mode)) {
        ERROR("Both arguments must be regular files, not symlinks or directories");
    }

    if (st.st_dev == newst.st_dev && st.st_ino == newst.st_ino) {
        ERROR("Source and destination refer to the same file");
    }

    /* Canonical paths make the volume lookup independent of '..' components. */
    if ((file = realpath(argv[1], NULL)) == NULL
            || (newfile = realpath(argv[2], NULL)) == NULL) {
        ERROR("Couldn't resolve file paths: %s", strerror(errno));
    }

    /* LV_DEFAULT uses this identity for volume access checks and Homes shares. */
    obj.uid = getuid();
    obj.ngroups = getgroups(0, NULL);

    if (obj.ngroups < 0) {
        ERROR("Couldn't get groups: %s", strerror(errno));
    }

    obj.groups = malloc(((size_t)obj.ngroups + 1) * sizeof(*obj.groups));

    if (obj.groups == NULL
            || (obj.ngroups = getgroups(obj.ngroups, obj.groups)) < 0) {
        ERROR("Couldn't load groups: %s", strerror(errno));
    }

    /* getgroups() need not include the primary group. */
    int i;

    for (i = 0; i < obj.ngroups && obj.groups[i] != getgid(); i++) {
    }

    if (i == obj.ngroups) {
        obj.groups[obj.ngroups++] = getgid();
    }

    config_started = 1;

    /* This also configures logging using afp.conf. */
    if (afp_config_parse(&obj, "backup_demo") != 0) {
        ERROR("Couldn't parse Netatalk afp.conf");
    }

    if (load_afp_conf_vols(&obj, LV_DEFAULT) != 0) {
        ERROR("Couldn't load volumes");
    }

    if ((vol = getvolbypath(&obj, file)) == NULL) {
        ERROR("Not a Netatalk volume for file \"%s\"", file);
    }

    if ((newvol = getvolbypath(&obj, newfile)) == NULL || newvol != vol) {
        ERROR("Destination must be in the same Netatalk volume as the source");
    }

    if (vol->v_flags & AFPVOL_RO) {
        ERROR("Volume is read-only");
    }

    printf("Volume path \"%s\"\n", vol->v_path);

    if (vol->v_adouble == AD_VERSION2) {
        printf("Volume adouble version v2\n");
    } else if (vol->v_adouble == AD_VERSION_EA) {
        printf("Volume adouble version ea\n");
    } else {
        ERROR("Unknown adouble version");
    }

    ad_init(&ad, vol);
    ad_initialized = 1;

    /* The data fd also lets EA backends open the resource fork by fd. */
    if (ad_open(&ad, file, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RDONLY) != 0) {
        ERROR("Couldn't open metadata of \"%s\"", file);
    }

    if (ad_open(&ad, file, ADFLAGS_RF | ADFLAGS_RDONLY | ADFLAGS_NORF) != 0) {
        ERROR("Couldn't open resource of \"%s\"", file);
    }

    printf("File has resource fork of size: %jd, fd: %d\n", (intmax_t)ad.ad_rlen,
           ad_reso_fileno(&ad));

    size_t comment_len = ad_getentrylen(&ad, ADEID_COMMENT);

    if (comment_len > ADEDLEN_COMMENT
            || (comment_len != 0 && ad_entry(&ad, ADEID_COMMENT) == NULL)) {
        ERROR("Invalid source comment");
    }

    if (vol->v_adouble == AD_VERSION2) {
        const char *name;

        if (load_charset(vol) != 0
                || (name = convert_utf8_to_mac(vol, strrchr(newfile, '/') + 1)) == NULL) {
            ERROR("Couldn't convert destination filename");
        }

        snprintf(macname, sizeof(macname), "%s", name);
    }

    /* Ensure the destination's .AppleDouble directory exists for v2 copies. */
    ad_init(&adnew, vol);
    adnew_initialized = 1;

    if (ad_open(&adnew, newfile, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666)
            != 0) {
        ERROR("Couldn't create metadata/resource of \"%s\"", newfile);
    }

    if (ad_close(&adnew, ADFLAGS_HF) != 0) {
        ERROR("Couldn't close destination metadata: %s", strerror(errno));
    }

    /* VFS copies resource data and EAs; v2 also copies the complete header. */
    if (vol->vfs->vfs_copyfile(vol, -1, file, newfile) != 0) {
        ERROR("Couldn't copy resource/EAs to \"%s\": %s", newfile, strerror(errno));
    }

    /* Reopen after VFS has replaced the sidecar, so entry offsets are current. */
    ad_init(&adnew, vol);

    if (ad_open(&adnew, newfile,
                ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) != 0) {
        ERROR("Couldn't open destination metadata/resource: %s", strerror(errno));
    }

    if (ad_copy_header(&adnew, &ad) != 0 || copy_comment(&adnew, &ad) != 0) {
        ERROR("Couldn't copy metadata: %s", strerror(errno));
    }

    if (vol->v_adouble == AD_VERSION2 && ad_setname(&adnew, macname) < 0) {
        ERROR("Couldn't set destination name");
    }

    /* ad_setid() returns 1 on success and 0 on failure. */
    if (!ad_setid(&adnew, newst.st_dev, newst.st_ino,
                  CNID_INVALID, CNID_INVALID, empty_stamp)) {
        ERROR("Couldn't reset destination CNID cache");
    }

    /* In particular, clear an old destination fork when the source has none. */
    if (ad_rtruncate(&adnew, newfile, ad.ad_rlen) != 0) {
        ERROR("Couldn't set resource length: %s", strerror(errno));
    }

    /* Any optional ad_setdate() calls belong here, before flushing/closing. */
    if (ad_flush(&adnew) != 0) {
        ERROR("Couldn't save destination metadata: %s", strerror(errno));
    }

    status = EXIT_SUCCESS;

cleanup:
    if (ad_initialized && ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF) != 0) {
        _log("Couldn't close source forks: %s", strerror(errno));
        status = EXIT_FAILURE;
    }

    if (adnew_initialized && ad_close(&adnew, ADFLAGS_HF | ADFLAGS_RF) != 0) {
        _log("Couldn't close destination forks: %s", strerror(errno));
        status = EXIT_FAILURE;
    }

    if (config_started) {
        unload_volumes(&obj);
        afp_config_free(&obj);
    }

    free(obj.groups);
    free(file);
    free(newfile);
    log_close_all();
    return status;
}
```

---

Authored by Ralph Böhme
