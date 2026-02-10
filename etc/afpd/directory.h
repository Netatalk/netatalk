/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2025 Andy Lemin (andylemin)
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 *
 * You may also use it under the terms of the General Public License (GPL). See COPYING.
 */

#ifndef AFPD_DIRECTORY_H
#define AFPD_DIRECTORY_H 1

#include <arpa/inet.h>
#include <dirent.h>
#include <stdbool.h>

/* sys/types.h usually snarfs in major/minor macros. if they don't
 * try this file. */
#ifndef major
#include <sys/sysmacros.h>
#endif

#include <sys/types.h>

#include <atalk/adouble.h>
#include <atalk/directory.h>
#include <atalk/globals.h>

#include "volume.h"

/* Dircache modify flags — bitmask for selective field updates in dir_modify() */
#define DCMOD_PATH   (1 << 0)  /*!< Identity/navigation: d_pdid, names, d_fullpath */
#define DCMOD_STAT   (1 << 1)  /*!< Stat cache: dcache_ctime/ino/mode/mtime/uid/gid/size */
#define DCMOD_AD     (1 << 2)  /*!< AD cache: finderinfo/filedatesi/afpfilei/rlen (Phase 2) */
#define DCMOD_AD_INV (1 << 3)  /*!< AD invalidate: zero AD fields, set rlen = -1 (Phase 2) */

/*!
 * @brief Parameters for dir_modify() selective field updates
 *
 * Callers set flags to indicate which field groups to update.
 * Only fields corresponding to set DCMOD_* flags are read.
 */
struct dir_modify_args {
    unsigned int     flags;          /*!< DCMOD_* bitmask */
    cnid_t           new_pdid;       /*!< New parent DID (DCMOD_PATH) */
    const char
    *new_mname;      /*!< New mac name, NULL = use new_uname (DCMOD_PATH) */
    const char      *new_uname;      /*!< New unix name (DCMOD_PATH) */
    bstring
    new_pdir_path;  /*!< New parent directory fullpath (DCMOD_PATH) */
    struct stat     *st;             /*!< Fresh stat result (DCMOD_STAT) */
    struct adouble  *adp;            /*!< AD data to store (DCMOD_AD, Phase 2) */
};

/* directory bits */
#define DIRPBIT_ATTR	0
#define DIRPBIT_PDID	1
#define DIRPBIT_CDATE	2
#define DIRPBIT_MDATE	3
#define DIRPBIT_BDATE	4
#define DIRPBIT_FINFO	5
#define DIRPBIT_LNAME	6
#define DIRPBIT_SNAME	7
#define DIRPBIT_DID	8
#define DIRPBIT_OFFCNT	9
#define DIRPBIT_UID	10
#define DIRPBIT_GID	11
#define DIRPBIT_ACCESS	12
#define DIRPBIT_PDINFO  13         /*!< ProDOS Info */
#define DIRPBIT_UNIXPR  15

#define FILDIRBIT_ISDIR        (1 << 7) /*!< is a directory */
#define FILDIRBIT_ISFILE       (0)      /*!< is a file */

/* file/directory ids. what a mess. we scramble things in a vain attempt
 * to get something meaningful */

#define CNID(a,b)     ((a)->st_ino & 0xffffffff)

struct maccess {
    uint8_t	ma_user;
    uint8_t	ma_world;
    uint8_t	ma_group;
    uint8_t	ma_owner;
};

#define	AR_USEARCH	(1<<0)
#define	AR_UREAD	(1<<1)
#define	AR_UWRITE	(1<<2)
#define	AR_UOWN		(1<<7)

extern q_t *invalid_dircache_entries;

typedef int (*dir_loop)(struct dirent *, char *, void *);

extern void        dir_free_invalid_q(void);
extern struct dir  *dir_new(const char *mname, const char *uname,
                            const struct vol *,
                            cnid_t pdid, cnid_t did, bstring fullpath, struct stat *);
extern void        dir_free(struct dir *);
extern struct dir  *dir_add(struct vol *, const struct dir *, struct path *,
                            int);
extern int         dir_modify(const struct vol *vol, struct dir *dir,
                              const struct dir_modify_args *args);
extern int         dir_remove(const struct vol *vol, struct dir *dir,
                              int report_invalid);
extern struct dir  *dirlookup(const struct vol *, cnid_t);
extern struct dir  *dirlookup_strict(const struct vol *, cnid_t);
extern struct dir *dirlookup_bypath(const struct vol *vol, const char *path);

extern int         movecwd(const struct vol *, struct dir *);
extern struct path *cname(struct vol *, struct dir *, char **);

extern int         deletecurdir(struct vol *);
extern mode_t      mtoumode(struct maccess *);
extern int         getdirparams(const AFPObj *obj, const struct vol *, uint16_t,
                                struct path *,
                                struct dir *, char *, size_t *);

extern int         setdirparams(struct vol *, struct path *, uint16_t, char *);
extern int         renamedir(struct vol *, int, char *, char *, struct dir *,
                             char *);
extern int         path_error(struct path *, int error);
extern void        setdiroffcnt(struct dir *dir, struct stat *st,
                                uint32_t count);
extern int         dirreenumerate(struct dir *dir, struct stat *st);
extern int         for_each_dirent(const struct vol *, char *, dir_loop,
                                   void *);
extern int         check_access(const AFPObj *obj, struct vol *, char *name,
                                int mode);
extern int         file_access(const AFPObj *obj, struct vol *vol,
                               struct path *path, int mode);
extern int         netatalk_unlink(const char *name);

/* from enumerate.c */
extern char        *check_dirent(const struct vol *, char *);

/* FP functions */
int afp_createdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);
int afp_opendir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                size_t *rbuflen);
int afp_setdirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                     size_t *rbuflen);
int afp_closedir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen);
int afp_mapid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen);
int afp_mapname(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                size_t *rbuflen);
int afp_syncdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                size_t *rbuflen);

/* from enumerate.c */
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);
int afp_enumerate_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen);
int afp_enumerate_ext2(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                       size_t *rbuflen);

/* from catsearch.c */
int afp_catsearch(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);
int afp_catsearch_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen);

#endif
