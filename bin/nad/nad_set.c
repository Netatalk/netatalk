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

/*!
 * @file
 * @brief The nad set command: change the Mac metadata of an entry
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "nad.h"

#define ADv2_DIRNAME ".AppleDouble"

#define DIR_DOT_OR_DOTDOT(a)                            \
    ((strcmp(a, ".") == 0) || (strcmp(a, "..") == 0))

static const char *labels[] = {
    "none",
    "grey",
    "green",
    "violet",
    "blue",
    "yellow",
    "red",
    "orange",
    NULL
};

static char *new_label;
static char *new_type;
static char *new_creator;
static char *new_flags;
static char *new_attributes;

/*!
 * @brief Print the usage of nad set and the meaning of its flag letters to
 * stdout
 */
static void usage_set(void)
{
    printf(
        "Usage: nad set [-t TYPE] [-c CREATOR] [-l label] [-f flags] [-a attributes] file|dir \n\n"
        "The set utility modifies AppleDouble metadata on files and\n"
        "directories in an AFP volume.\n\n"
        "     Color Label:\n"
        "       none | grey | green | violet | blue | yellow | red | orange\n\n"
        "     FinderFlags:\n"
        "       d = On Desktop (f/d)\n"
        "       e = Hidden extension (f/d)\n"
        "       m = Shared (can run multiple times) (f)\n"
        "       n = No INIT resources (f)\n"
        "       i = Inited (f/d)\n"
        "       c = Custom icon (f/d)\n"
        "       t = Stationery (f)\n"
        "       s = Name locked (f/d)\n"
        "       b = Bundle (f/d)\n"
        "       v = Invisible (f/d)\n"
        "       a = Alias file (f/d)\n\n"
        "     AFPAttributes:\n"
        "       y = System (f/d)\n"
        "       w = No write (f)\n"
        "       p = Needs backup (f/d)\n"
        "       r = No rename (f/d)\n"
        "       l = No delete (f/d)\n"
        "       o = No copy (f)\n\n"
        "     Uppercase letter sets the flag, lowercase removes the flag\n"
        "     f = valid for files\n"
        "     d = valid for directories\n"
    );
}

/*!
 * @brief Store a new file type in the Finder info of the metadata
 *
 * The call takes the first 4 bytes of @p p_new_type.
 *
 * @param[in]     path        entry path, unused
 * @param[in]     vol         volume of the entry, unused
 * @param[in]     st          stat result of the entry, unused
 * @param[in,out] ad          open metadata
 * @param[in]     p_new_type  type code of 4 characters
 */
static void change_type(char *path _U_, afpvol_t *vol _U_,
                        const struct stat *st _U_, struct adouble *ad, char *p_new_type)
{
    char *FinderInfo;

    if ((FinderInfo = ad_entry(ad, ADEID_FINDERI))) {
        memcpy(FinderInfo, p_new_type, 4);
    }
}

/*!
 * @brief Store a new creator in the Finder info of the metadata
 *
 * The call takes the first 4 bytes of @p p_new_creator.
 *
 * @param[in]     path           entry path, unused
 * @param[in]     vol            volume of the entry, unused
 * @param[in]     st             stat result of the entry, unused
 * @param[in,out] ad             open metadata
 * @param[in]     p_new_creator  creator code of 4 characters
 */
static void change_creator(char *path _U_, afpvol_t *vol _U_,
                           const struct stat *st _U_, struct adouble *ad, char *p_new_creator)
{
    char *FinderInfo;

    if ((FinderInfo = ad_entry(ad, ADEID_FINDERI))) {
        memcpy(FinderInfo + 4, p_new_creator, 4);
    }
}

/*!
 * @brief Set the color label in the Finder flags of the metadata
 *
 * The label is one of `none`, `grey`, `green`, `violet`, `blue`, `yellow`,
 * `red`, and `orange`, in any case, and another name changes nothing.
 *
 * @param[in]     path         entry path, unused
 * @param[in]     vol          volume of the entry, unused
 * @param[in]     st           stat result of the entry, unused
 * @param[in,out] ad           open metadata
 * @param[in]     p_new_label  label name
 */
static void change_label(char *path _U_, afpvol_t *vol _U_,
                         const struct stat *st _U_, struct adouble *ad, char *p_new_label)
{
    char *FinderInfo;
    const char **color = &labels[0];
    uint16_t FinderFlags;
    unsigned char color_count = 0;

    if ((FinderInfo = ad_entry(ad, ADEID_FINDERI)) == NULL) {
        return;
    }

    while (*color) {
        if (strcasecmp(*color, p_new_label) == 0) {
            /* get flags */
            memcpy(&FinderFlags, FinderInfo + 8, 2);
            FinderFlags = ntohs(FinderFlags);
            /* change value */
            FinderFlags &= ~FINDERINFO_COLOR;
            FinderFlags |= color_count << 1;
            /* copy it back */
            FinderFlags = ntohs(FinderFlags);
            memcpy(FinderInfo + 8, &FinderFlags, 2);
            break;
        }

        color++;
        color_count++;
    }
}

/*!
 * @brief Set or clear AFP attributes from a string of letters
 *
 * An uppercase letter sets an attribute, and a lowercase one clears it. Y,
 * P, R, and L stand for system, needs backup, no rename, and no delete.
 * Regular files also take W for no write and O for the ATTRBIT_NOCOPY bit.
 *
 * @param[in]     path              entry path, unused
 * @param[in]     vol               volume of the entry, unused
 * @param[in]     st                stat result of the entry
 * @param[in,out] ad                open metadata
 * @param[in]     p_new_attributes  attribute letters
 */
static void change_attributes(char *path _U_, afpvol_t *vol _U_,
                              const struct stat *st, struct adouble *ad, char *p_new_attributes)
{
    uint16_t AFPattributes;
    ad_getattr(ad, &AFPattributes);
    AFPattributes = ntohs(AFPattributes);

    if (S_ISREG(st->st_mode)) {
        if (strchr(p_new_attributes, 'W')) {
            AFPattributes |= ATTRBIT_NOWRITE;
        }

        if (strchr(new_attributes, 'w')) {
            AFPattributes &= ~ATTRBIT_NOWRITE;
        }

        if (strchr(new_attributes, 'O')) {
            AFPattributes |= ATTRBIT_NOCOPY;
        }

        if (strchr(new_attributes, 'o')) {
            AFPattributes &= ~ATTRBIT_NOCOPY;
        }
    }

    if (strchr(new_attributes, 'Y')) {
        AFPattributes |= ATTRBIT_SYSTEM;
    }

    if (strchr(new_attributes, 'y')) {
        AFPattributes &= ~ATTRBIT_SYSTEM;
    }

    if (strchr(new_attributes, 'P')) {
        AFPattributes |= ATTRBIT_BACKUP;
    }

    if (strchr(new_attributes, 'p')) {
        AFPattributes &= ~ATTRBIT_BACKUP;
    }

    if (strchr(new_attributes, 'R')) {
        AFPattributes |= ATTRBIT_NORENAME;
    }

    if (strchr(new_attributes, 'r')) {
        AFPattributes &= ~ATTRBIT_NORENAME;
    }

    if (strchr(new_attributes, 'L')) {
        AFPattributes |= ATTRBIT_NODELETE;
    }

    if (strchr(new_attributes, 'l')) {
        AFPattributes &= ~ATTRBIT_NODELETE;
    }

    AFPattributes = ntohs(AFPattributes);
    ad_setattr(ad, AFPattributes);
}

/*!
 * @brief Set or clear Finder flags from a string of letters
 *
 * An uppercase letter sets a flag, and a lowercase one clears it. D, E, I,
 * C, S, B, V, and A stand for on desktop, hidden extension, inited, custom
 * icon, name locked, bundle, invisible, and alias. Regular files also take M
 * for shared, N for no INIT resources, and T for stationery.
 *
 * @param[in]     path         entry path, unused
 * @param[in]     vol          volume of the entry, unused
 * @param[in]     st           stat result of the entry
 * @param[in,out] ad           open metadata
 * @param[in]     p_new_flags  flag letters
 */
static void change_flags(char *path _U_, afpvol_t *vol _U_,
                         const struct stat *st, struct adouble *ad, char *p_new_flags)
{
    char *FinderInfo;
    uint16_t FinderFlags;

    if ((FinderInfo = ad_entry(ad, ADEID_FINDERI)) == NULL) {
        return;
    }

    memcpy(&FinderFlags, FinderInfo + 8, 2);
    FinderFlags = ntohs(FinderFlags);

    if (S_ISREG(st->st_mode)) {
        if (strchr(p_new_flags, 'M')) {
            FinderFlags |= FINDERINFO_ISHARED;
        }

        if (strchr(new_flags, 'm')) {
            FinderFlags &= ~FINDERINFO_ISHARED;
        }

        if (strchr(new_flags, 'N')) {
            FinderFlags |= FINDERINFO_HASNOINITS;
        }

        if (strchr(new_flags, 'n')) {
            FinderFlags &= ~FINDERINFO_HASNOINITS;
        }

        if (strchr(new_flags, 'T')) {
            FinderFlags |= FINDERINFO_ISSTATIONNERY;
        }

        if (strchr(new_flags, 't')) {
            FinderFlags &= ~FINDERINFO_ISSTATIONNERY;
        }
    }

    if (strchr(new_flags, 'D')) {
        FinderFlags |= FINDERINFO_ISONDESK;
    }

    if (strchr(new_flags, 'd')) {
        FinderFlags &= ~FINDERINFO_ISONDESK;
    }

    if (strchr(new_flags, 'E')) {
        FinderFlags |= FINDERINFO_HIDEEXT;
    }

    if (strchr(new_flags, 'e')) {
        FinderFlags &= ~FINDERINFO_HIDEEXT;
    }

    if (strchr(new_flags, 'I')) {
        FinderFlags |= FINDERINFO_HASBEENINITED;
    }

    if (strchr(new_flags, 'i')) {
        FinderFlags &= ~FINDERINFO_HASBEENINITED;
    }

    if (strchr(new_flags, 'C')) {
        FinderFlags |= FINDERINFO_HASCUSTOMICON;
    }

    if (strchr(new_flags, 'c')) {
        FinderFlags &= ~FINDERINFO_HASCUSTOMICON;
    }

    if (strchr(new_flags, 'S')) {
        FinderFlags |= FINDERINFO_NAMELOCKED;
    }

    if (strchr(new_flags, 's')) {
        FinderFlags &= ~FINDERINFO_NAMELOCKED;
    }

    if (strchr(new_flags, 'B')) {
        FinderFlags |= FINDERINFO_HASBUNDLE;
    }

    if (strchr(new_flags, 'b')) {
        FinderFlags &= ~FINDERINFO_HASBUNDLE;
    }

    if (strchr(new_flags, 'V')) {
        FinderFlags |= FINDERINFO_INVISIBLE;
    }

    if (strchr(new_flags, 'v')) {
        FinderFlags &= ~FINDERINFO_INVISIBLE;
    }

    if (strchr(new_flags, 'A')) {
        FinderFlags |= FINDERINFO_ISALIAS;
    }

    if (strchr(new_flags, 'a')) {
        FinderFlags &= ~FINDERINFO_ISALIAS;
    }

    FinderFlags = ntohs(FinderFlags);
    memcpy(FinderInfo + 8, &FinderFlags, 2);
}

/*!
 * @brief Run the nad set command, which changes the Mac metadata of one entry
 *
 * The -t and -c options set the type and the creator, -l the color label,
 * -f the Finder flags, and -a the AFP attributes. The call creates missing
 * metadata. Metadata that does not open leaves the entry unchanged, and the
 * call still returns 0. A missing path or a failed stat exits with 1.
 *
 * @param[in] argc  argument count, from the set command on
 * @param[in] argv  argument vector, from the set command on
 * @param[in] obj   AFP configuration with the volumes
 * @returns 0, 1 when the volume does not open, or -1 for an unknown option
 */
int nad_set(int argc, char **argv, AFPObj *obj)
{
    int c;
    afpvol_t vol;
    struct stat st;
    int adflags = 0;
    struct adouble ad;

    while ((c = getopt(argc, argv, ":l:t:c:f:a:")) != -1) {
        switch (c) {
        case 'l':
            new_label = strdup(optarg);
            break;

        case 't':
            new_type = strdup(optarg);
            break;

        case 'c':
            new_creator = strdup(optarg);
            break;

        case 'f':
            new_flags = strdup(optarg);
            break;

        case 'a':
            new_attributes = strdup(optarg);
            break;

        default:
            usage_set();
            return -1;
        }
    }

    if (argc <= optind) {
        exit(1);
    }

    cnid_init();

    if (openvol_optional(obj, argv[optind], &vol) != 0) {
        return 1;
    }

    if (stat(argv[optind], &st) != 0) {
        perror("stat");
        exit(1);
    }

    if (S_ISDIR(st.st_mode)) {
        adflags = ADFLAGS_DIR;
    }

    ad_init(&ad, vol.vol);

    if (ad_open(&ad, argv[optind],
                adflags | ADFLAGS_HF | ADFLAGS_CREATE | ADFLAGS_RDWR, 0666) < 0) {
        goto exit;
    }

    if (new_label) {
        change_label(argv[optind], &vol, &st, &ad, new_label);
    }

    if (new_type) {
        change_type(argv[optind], &vol, &st, &ad, new_type);
    }

    if (new_creator) {
        change_creator(argv[optind], &vol, &st, &ad, new_creator);
    }

    if (new_flags) {
        change_flags(argv[optind], &vol, &st, &ad, new_flags);
    }

    if (new_attributes) {
        change_attributes(argv[optind], &vol, &st, &ad, new_attributes);
    }

    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
exit:
    closevol(&vol);
    return 0;
}
