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
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include "ad.h"

#define ADv2_DIRNAME ".AppleDouble"

#define DIR_DOT_OR_DOTDOT(a) \
        ((strcmp(a, ".") == 0) || (strcmp(a, "..") == 0))

static volatile sig_atomic_t alarmed;

/* ls options */
static int ls_a;
static int ls_l;
static int ls_R;
static int ls_d;
static int ls_u;

/* Used for pretty printing */
static int first = 1;
static int recursion;

static char           *netatalk_dirs[] = {
    ADv2_DIRNAME,
    ".AppleDB",
    ".AppleDesktop",
    NULL
};

static char *labels[] = {
    "---",
    "gry",
    "gre",
    "vio",
    "blu",
    "yel",
    "red",
    "ora"
};

/*
  SIGNAL handling:
  catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.
*/

static void sig_handler(int signo)
{
    alarmed = 1;
    return;
}

static void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGTERM, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGTERM): %s", strerror(errno));

    if (sigaction(SIGINT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGINT): %s", strerror(errno));

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGABRT): %s", strerror(errno));

    if (sigaction(SIGHUP, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGHUP): %s", strerror(errno));

    if (sigaction(SIGQUIT, &sv, NULL) < 0)
        ERROR("error in sigaction(SIGQUIT): %s", strerror(errno));
}

/*
  Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop"
  Returns pointer to name or NULL.
*/
static const char *check_netatalk_dirs(const char *name)
{
    int c;

    for (c=0; netatalk_dirs[c]; c++) {
        if ((strcmp(name, netatalk_dirs[c])) == 0)
            return netatalk_dirs[c];
    }
    return NULL;
}


static void usage_set(void)
{
    printf(
        "Usage: ad set file|dir [-t TYPE] [-c CREATOR] [-l label] [-f flags]\n\n"
        "     FinderFlags (valid for (f)ile and/or (d)irectory):\n"
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
        "     Note: any letter appearing in uppercase means the flag is set\n"
        "           but it's a directory for which the flag is not allowed.\n"
        );
}

static void print_flags(char *path, afpvol_t *vol, const struct stat *st)
{
    int adflags = 0;
    struct adouble ad;
    char *FinderInfo;
    uint16_t FinderFlags;
    uint16_t AFPattributes;
    char type[5] = "----";
    char creator[5] = "----";
    int i;
    uint32_t cnid;

    if (S_ISDIR(st->st_mode))
        adflags = ADFLAGS_DIR;

    if (vol->vol->v_path == NULL)
        return;

    ad_init(&ad, vol->vol);

    if ( ad_metadata(path, adflags, &ad) < 0 )
        return;

    FinderInfo = ad_entry(&ad, ADEID_FINDERI);

    memcpy(&FinderFlags, FinderInfo + 8, 2);
    FinderFlags = ntohs(FinderFlags);

    memcpy(type, FinderInfo, 4);
    memcpy(creator, FinderInfo + 4, 4);

    ad_getattr(&ad, &AFPattributes);
    AFPattributes = ntohs(AFPattributes);

    /*
      Finder flags. Lowercase means valid, uppercase means invalid because
      object is a dir and flag is only valid for files.
    */
    putchar(' ');
    if (FinderFlags & FINDERINFO_ISONDESK)
        putchar('d');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_HIDEEXT)
        putchar('e');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_ISHARED) {
        if (adflags & ADFLAGS_DIR)
            putchar('M');
        else
            putchar('m');
    } else
        putchar('-');

    if (FinderFlags & FINDERINFO_HASNOINITS) {
        if (adflags & ADFLAGS_DIR)
            putchar('N');
        else
            putchar('n');
    } else
        putchar('-');

    if (FinderFlags & FINDERINFO_HASBEENINITED)
        putchar('i');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_HASCUSTOMICON)
        putchar('c');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_ISSTATIONNERY) {
        if (adflags & ADFLAGS_DIR)
            putchar('T');
        else
            putchar('t');
    } else
        putchar('-');

    if (FinderFlags & FINDERINFO_NAMELOCKED)
        putchar('s');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_HASBUNDLE)
        putchar('b');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_INVISIBLE)
        putchar('v');
    else
        putchar('-');

    if (FinderFlags & FINDERINFO_ISALIAS)
        putchar('a');
    else
        putchar('-');

    putchar(' ');

    /* AFP attributes */
    if (AFPattributes & ATTRBIT_SYSTEM)
        putchar('y');
    else
        putchar('-');

    if (AFPattributes & ATTRBIT_NOWRITE) {
        if (adflags & ADFLAGS_DIR)
            putchar('W');
        else
            putchar('w');
    } else
        putchar('-');

    if (AFPattributes & ATTRBIT_BACKUP)
        putchar('p');
    else
        putchar('-');

    if (AFPattributes & ATTRBIT_NORENAME)
        putchar('r');
    else
        putchar('-');

    if (AFPattributes & ATTRBIT_NODELETE)
        putchar('l');
    else
        putchar('-');

    if (AFPattributes & ATTRBIT_NOCOPY) {
        if (adflags & ADFLAGS_DIR)
            putchar('O');
        else
            putchar('o');                
    } else
        putchar('-');

    /* Color */
    printf(" %s ", labels[(FinderFlags & FINDERINFO_COLOR) >> 1]);

    /* Type & Creator */
    for(i=0; i<4; i++) {
        if (isalnum(type[i]))
            putchar(type[i]);
        else
            putchar('-');
    }
    putchar(' '); 
    for(i=0; i<4; i++) {
        if (isalnum(creator[i]))
            putchar(creator[i]);
        else
            putchar('-');
    }
    putchar(' '); 

    /* CNID */
    cnid = ad_forcegetid(&ad);
    if (cnid)
        printf(" %10u ", ntohl(cnid));
    else
        printf(" !ADVOL_CACHE ");

    ad_close(&ad, ADFLAGS_HF);
}

int ad_ls(int argc, char **argv, AFPObj *obj)
{
    int c, firstarg;
    afpvol_t vol;
    struct stat st;

    while ((c = getopt(argc, argv, ":adlRu")) != -1) {
        switch(c) {
        case 'a':
            ls_a = 1;
            break;
        case 'd':
            ls_d = 1;
            break;
        case 'l':
            ls_l = 1;
            break;
        case 'R':
            ls_R = 1;
            break;
        case 'u':
            ls_u = 1;
            break;
        case ':':
        case '?':
            usage_ls();
            return -1;
            break;
        }

    }

    set_signal();
    cnid_init();

    if ((argc - optind) == 0) {
        openvol(obj, ".", &vol);
        ad_ls_r(".", &vol);
        closevol(&vol);
    }
    else {
        int havefile = 0;

        firstarg = optind;

        /* First run: only print files from argv paths */
        while(optind < argc) {
            if (stat(argv[optind], &st) != 0)
                goto next;
            if (S_ISDIR(st.st_mode))
                goto next;

            havefile = 1;
            first = 1;
            recursion = 0;

            openvol(obj, argv[optind], &vol);
            ad_ls_r(argv[optind], &vol);
            closevol(&vol);
        next:
            optind++;
        }
        if (havefile && (! ls_l))
            printf("\n");

        /* Second run: print dirs */
        optind = firstarg;
        while(optind < argc) {
            if (stat(argv[optind], &st) != 0)
                goto next2;
            if ( ! S_ISDIR(st.st_mode))
                goto next2;
            if ((optind > firstarg) || havefile)
                printf("\n%s:\n", argv[optind]);

            first = 1;
            recursion = 0;

            openvol(obj, argv[optind], &vol);
            ad_ls_r(argv[optind], &vol);
            closevol(&vol);

        next2:
            optind++;
        }


    }

    return 0;
}
