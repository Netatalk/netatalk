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
#include <libgen.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/volinfo.h>
#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/logger.h>
#include "ad.h"

#define ADv2_DIRNAME ".AppleDouble"

/* options */
static int cp_R;
static int cp_L;
static int cp_P;
static int cp_n;
static int cp_p;
static int cp_v;

static char           *netatalk_dirs[] = {
    ADv2_DIRNAME,
    ".AppleDB",
    ".AppleDesktop",
    NULL
};

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


static void usage_cp(void)
{
    printf(
        "Usage: ad cp [-R [-L | -P]] [-pv] <source_file> <target_file>\n"
        "Usage: ad cp [-R [-L | -P]] [-pv] <source_file [source_file ...]> <target_directory>\n"
        );
}

static int ad_cp_copy(const afpvol_t *srcvol,
                      const afpvol_t *dstvol,
                      char *srcfile,
                      char *dstfile,
                      struct stat *st)
{
    printf("copy: '%s' -> '%s'\n", srcfile, dstfile);
    return 0;
}

static int ad_cp_r(const afpvol_t *srcvol, const afpvol_t *dstvol, char *srcdir, char *dstdir)
{
    int ret = 0, dirprinted = 0, dirempty;
    static char srcpath[MAXPATHLEN+1];
    static char dstpath[MAXPATHLEN+1];
    char *tmp;
    DIR *dp = NULL;
    struct dirent *ep;
    static struct stat st;      /* Save some stack space */

    strlcat(srcpath, srcdir, sizeof(srcpath));
    strlcat(dstpath, dstdir, sizeof(dstpath));

    if ((dp = opendir (srcdir)) == NULL) {
        perror("Couldn't opendir .");
        return -1;
    }

    /* First run: copy files */
    while ((ep = readdir (dp))) {
        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name))
            continue;

        /* Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop" */
        if ((check_netatalk_dirs(ep->d_name)) != NULL)
            continue;

        if (lstat(ep->d_name, &st) < 0) {
            perror("Can't stat");
            return -1;
        }

        /* Build paths, copy, strip name */
        strlcat(srcpath, "/", sizeof(srcpath));
        strlcat(dstpath, "/", sizeof(dstpath));
        strlcat(srcpath, ep->d_name, sizeof(srcpath));
        strlcat(dstpath, ep->d_name, sizeof(dstpath));

        ret = ad_cp_copy(srcvol, dstvol, srcpath, dstpath, &st);

        if ((tmp = strrchr(srcpath, '/')))
            *tmp = 0;
        if ((tmp = strrchr(dstpath, '/')))
            *tmp = 0;

        if (ret != 0)
            goto exit;
    }

    /* Second run: recurse to dirs */
    rewinddir(dp);
    while ((ep = readdir (dp))) {
        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name))
            continue;
        
        /* Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop" */
        if (check_netatalk_dirs(ep->d_name) != NULL)
            continue;
        
        if (lstat(ep->d_name, &st) < 0) {
            perror("Can't stat");
            return -1;
        }
        
        /* Recursion */
        if (S_ISDIR(st.st_mode)) {
            strlcat(srcpath, "/", sizeof(srcpath));
            strlcat(dstpath, "/", sizeof(dstpath));
            ret = ad_cp_r(srcvol, dstvol, ep->d_name, ep->d_name);
        }
        if (ret != 0)
            goto exit;
    }

exit:
    if (dp)
        closedir(dp);
 
    if ((tmp = strrchr(srcpath, '/')))
        *tmp = 0;
    if ((tmp = strrchr(dstpath, '/')))
        *tmp = 0;

    return ret;
}

int ad_cp(int argc, char **argv)
{
    EC_INIT;
    int c, numpaths;
    afpvol_t srcvvol;
    struct stat sst;
    struct stat dst;
    afpvol_t srcvol;
    afpvol_t dstvol;
    char *srcfile = NULL;
    char *srcdir = NULL;    
    bstring dstfile;
    char *dstdir = NULL;
    char path[MAXPATHLEN+1];
    char *basenametmp;

    while ((c = getopt(argc, argv, ":npvLPR")) != -1) {
        switch(c) {
        case 'n':
            cp_n = 1;
            break;
        case 'p':
            cp_p = 1;
            break;
        case 'v':
            cp_v = 1;
            break;
        case 'L':
            cp_L = 1;
            break;
        case 'P':
            cp_P = 1;
            break;
        case 'R':
            cp_R = 1;
            break;
        case ':':
        case '?':
            usage_cp();
            return -1;
            break;
        }
    }

    /* How many pathnames do we have */
    numpaths = argc - optind;
    printf("Number of paths: %u\n", numpaths);
    if (numpaths < 2) {
        usage_cp();
        exit(EXIT_FAILURE);
    }

    while ( argv[argc-1][(strlen(argv[argc-1]) - 1)] == '/')
        argv[argc-1][(strlen(argv[argc-1]) - 1)] = 0;

    /* Create vol for destination */
    newvol(argv[argc-1], &dstvol);

    if (numpaths == 2) {
        /* Case 1: 2 paths */

        /* stat source */
        EC_ZERO(stat(argv[optind], &sst));

        if (S_ISREG(sst.st_mode)) {
            /* source is just a file, thats easy */
            /* Either file to file or file to dir copy */

            /* stat destination */
            if (stat(argv[argc-1], &dst) == 0) {
                if (S_ISDIR(dst.st_mode)) {
                    /* its a dir, build dest path: "dest" + "/" + basename("source") */
                    EC_NULL(dstfile = bfromcstr(argv[argc-1]));
                    EC_ZERO(bcatcstr(dstfile, "/"));
                    EC_ZERO(bcatcstr(dstfile, basename(argv[optind])));
                } else {
                    /* its an existing file, truncate */
                    EC_ZERO_LOG(truncate(argv[argc-1], 0));
                    EC_NULL(dstfile = bfromcstr(argv[argc-1]));
                }
            } else {
                EC_NULL(dstfile = bfromcstr(argv[argc-1]));
            }
            newvol(argv[optind], &srcvol);
            printf("Source: %s, Destination: %s\n", argv[optind], cfrombstring(dstfile));
            ad_cp_copy(&srcvol, &dstvol, argv[optind], cfrombstring(dstfile), &sst);
            freevol(&srcvol);
            freevol(&dstvol);
            
        } else if (S_ISDIR(sst.st_mode)) {
            /* dir to dir copy. Check if -R is requested */
            if (!cp_R) {
                usage_cp();
                exit(EXIT_FAILURE);
            }
        }

    } else {
        /* Case 2: >2 paths */
        while (optind < (argc-1)) {
            printf("Source is: %s\n", argv[optind]);
            newvol(argv[optind], &srcvol);
            if (stat(argv[optind], &sst) != 0)
                goto next;
            if (S_ISDIR(sst.st_mode)) {
                /* Source is a directory */
                if (!cp_R) {
                    printf("Source %s is a directory\n", argv[optind]);
                    goto next;
                }
                srcdir = argv[optind];
                dstdir = argv[argc-1];
                
                ad_cp_r(&srcvol, &dstvol, srcdir, dstdir);
                freevol(&srcvol);
            } else {
                /* Source is a file */
                srcfile = argv[optind];
                if (numpaths != 2 && !dstdir) {
                    usage_cp();
                    exit(EXIT_FAILURE);
                }
                path[0] = 0;
                strlcat(path, dstdir, sizeof(path));
                basenametmp = strdup(srcfile);
                strlcat(path, basename(basenametmp), sizeof(path));
                free(basenametmp);
                printf("%s %s\n", srcfile, path);
                if (ad_cp_copy(&srcvol, &dstvol, srcfile, path, &sst) != 0) {
                    freevol(&srcvol);
                    freevol(&dstvol);
                    exit(EXIT_FAILURE);
                }
            } /* else */
        next:
            optind++;
        } /* while */
    } /* else (numpath>2) */

EC_CLEANUP:
    freevol(&dstvol);

    EC_EXIT;
}
