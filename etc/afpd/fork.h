/*
 * $Id: fork.h,v 1.12 2009-02-25 16:14:08 franklahm Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_FORK_H
#define AFPD_FORK_H 1

#include <stdio.h>
#include <sys/cdefs.h>

#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include "volume.h"
#include "directory.h"

struct file_key {
    dev_t		dev;
    ino_t		inode;
};

struct ofork {
    struct file_key     key;
    struct adouble	*of_ad;
    struct vol          *of_vol;
    struct dir		*of_dir;

    u_int16_t           of_refnum;
    int                 of_flags;

    struct ofork        **prevp, *next;
    struct ofork        *of_d_prev, *of_d_next;
};

#define OPENFORK_DATA	(0)
#define OPENFORK_RSCS	(1<<7)

#define OPENACC_RD	(1<<0)
#define OPENACC_WR	(1<<1)
#define OPENACC_DRD	(1<<4)
#define OPENACC_DWR	(1<<5)

/* ofork.of_flags bits */
#define AFPFORK_OPEN	(1<<0)
#define AFPFORK_RSRC	(1<<1)
#define AFPFORK_DATA	(1<<2)
#define AFPFORK_DIRTY   (1<<3)
#define AFPFORK_ACCRD   (1<<4)
#define AFPFORK_ACCWR   (1<<5)
#define AFPFORK_ACCMASK (AFPFORK_ACCRD | AFPFORK_ACCWR)

#define of_name(a) (a)->of_ad->ad_m_name
/* in ofork.c */
extern struct ofork *of_alloc    __P((struct vol *, struct dir *,
                                                      char *, u_int16_t *, const int,
                                                      struct adouble *,
                                                      struct stat *));
extern void         of_dealloc   __P((struct ofork *));
extern struct ofork *of_find     __P((const u_int16_t));
extern struct ofork *of_findname __P((struct path *));
extern int          of_rename    __P((const struct vol *,
                                          struct ofork *,
                                          struct dir *, const char *,
                                          struct dir *, const char *));
extern int          of_flush     __P((const struct vol *));
extern void         of_pforkdesc __P((FILE *));
extern int          of_stat      __P((struct path *));
extern int          of_statdir   __P((const struct vol *vol, struct path *));
extern int          of_closefork __P((struct ofork *ofork));
extern void         of_closevol  __P((const struct vol *vol));
extern struct adouble *of_ad     __P((const struct vol *, struct path *, struct adouble *));
/* in fork.c */
extern int          flushfork    __P((struct ofork *));
extern int          getforkmode  __P((struct adouble *, int , int ));

/* FP functions */
extern int	afp_openfork __P((AFPObj *, char *, int, char *, int *));
extern int	afp_bytelock __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getforkparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setforkparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_read __P((AFPObj *, char *, int, char *, int *));
extern int	afp_write __P((AFPObj *, char *, int, char *, int *));
extern int	afp_flushfork __P((AFPObj *, char *, int, char *, int *));
extern int	afp_flush __P((AFPObj *, char *, int, char *, int *));
extern int	afp_closefork __P((AFPObj *, char *, int, char *, int *));

extern int	afp_bytelock_ext __P((AFPObj *, char *, int, char *, int *));
extern int	afp_read_ext __P((AFPObj *, char *, int, char *, int *));
extern int	afp_write_ext __P((AFPObj *, char *, int, char *, int *));
extern int      afp_syncfork __P((AFPObj *, char *, int, char *, int *));
#endif
