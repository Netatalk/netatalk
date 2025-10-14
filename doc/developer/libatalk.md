# Using libatalk from third party apps

Third party developers wishing to add Netatalk support to their applications have two choices

1. Directly integrate libatalk from the Netatalk source tree
2. Link with libatalk which is a shared library in its own right
   starting with Netatalk 3

Please note that you need at least Netatalk 3.0.1 for approach 2 to work,
because in earlier Netatalk versions some necessary headers are not installed correctly.

## Example

This examples links a trivial example program with the libatalk shared library.
Assuming Netatalk has been installed to /usr/local/netatalk, compiling backup_demo.c is done like this:

```shell
c99 -I/usr/local/netatalk/include -o backup_demo backup_demo.c -L/usr/local/netatalk/lib -latalk -Wl,-rpath=/usr/local/netatalk/lib
```

Here follows the *backup_demo.c* code which copies metadata and resource from one file to another:

```c
/*
    Copyright (c) 2012, Frank Lahm 

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
#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/volume.h>

#define ERROR(...)                              \
    do {                                        \
        _log(__VA_ARGS__);                      \
        exit(1);                                \
    } while (0)

static void _log(char *fmt, ...)
{
    int len;
    static char logbuffer[1024];
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(logbuffer, 1023, fmt, args);
    va_end(args);
    logbuffer[1023] = 0;

    printf("%s\n", logbuffer);
}

int main(int argc, char **argv)
{
    const char *file, *newfile;
    struct vol *vol;
    struct adouble ad, adnew;
    struct stat st;

    if (argc != 3) {
        ERROR("usage: backup_demo FILE NEWFILE");
    }
    file = argv[1];
    newfile = argv[2];

    AFPObj obj = { 0 };

    if (afp_config_parse(&obj, "ad") != 0)
        ERROR("No Netatalk 3 afp.conf found", file);

    setuplog("default:note", "/dev/tty");

    if (load_volumes(&obj, NULL) != 0)
        ERROR("Couldn't load volumes");
    if ((vol = getvolbypath(&obj, file)) == NULL)
        ERROR("Not a Netatalk volume for file \"%s\"", file);

    printf("Volume path \"%s\"\n", vol->v_path);
    if (vol->v_adouble == AD_VERSION2) {
        printf("Volume adouble version v2\n");
    } else if (vol->v_adouble == AD_VERSION_EA) {
        printf("Volume adouble version ea\n");
    } else {
        ERROR("Unknown adouble version");
    }

    ad_init(&ad, vol);

    /* Open an existing file's metadata and ressource */
    if (ad_open(&ad, file, ADFLAGS_HF | ADFLAGS_RDONLY) != 0)
        ERROR("Couldn't open metadata of \"%s\"", file);

    int ad_size = vol->v_adouble == AD_VERSION2 ? AD_DATASZ2 : AD_DATASZ_EA;
    printf("%d bytes in adouble metadata buffer ad->ad_data\n", ad_size);

    if (ad_open(&ad, file, ADFLAGS_RF | ADFLAGS_RDONLY) != 0)
        ERROR("Couldn't open ressource of \"%s\"", file);

    printf("File has ressource fork of size: %d, fd: %d\n", ad.ad_rlen, ad_reso_fileno(&ad));

    /* Copy over metadata and ressource to new file */
    if (lstat(newfile, &st) != 0)
        ERROR("Can't stat \"%s\", must exist, create with eg touch", newfile);

    ad_init(&adnew, vol);
    if (ad_open(&ad, file, ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR | ADFLAGS_CREATE) != 0)
        ERROR("Couldn't create metadata/ressource of \"%s\"", newfile);

    memcpy(adnew.ad_data, ad.ad_data, ad_size);
    ad_flush(&adnew);
    if (vol->vfs->vfs_copyfile(vol, -1, file, newfile) != 0)
        ERROR("Couln't copy ressource to \"%s\"", newfile);

    ad_close(&ad, ADFLAGS_HF | ADFLAGS_RF);
    ad_close(&adnew, ADFLAGS_HF | ADFLAGS_RF);

#if 0
    /* examples setting some stuff */
    ad_setdate(&ad, AD_DATE_CREATE | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(&ad, AD_DATE_MODIFY | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(&ad, AD_DATE_ACCESS | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(&ad, AD_DATE_BACKUP, AD_DATE_START);
    ad_flush(&ad);
#endif

    return 0;
}
```

---

Authored by Ralph Böhme
