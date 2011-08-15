/*
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
    dev_t       dev;
    ino_t       inode;
};

struct ofork {
    struct file_key     key;
    struct adouble      *of_ad;
    struct vol          *of_vol;
    cnid_t              of_did;
    uint16_t            of_refnum;
    int                 of_flags;
    struct ofork        **prevp, *next;
//    struct ofork        *of_d_prev, *of_d_next;
};

#define OPENFORK_DATA   (0)
#define OPENFORK_RSCS   (1<<7)

#define OPENACC_RD  (1<<0)
#define OPENACC_WR  (1<<1)
#define OPENACC_DRD (1<<4)
#define OPENACC_DWR (1<<5)

/* ofork.of_flags bits */
#define AFPFORK_OPEN    (1<<0)
#define AFPFORK_RSRC    (1<<1)
#define AFPFORK_DATA    (1<<2)
#define AFPFORK_DIRTY   (1<<3)
#define AFPFORK_ACCRD   (1<<4)
#define AFPFORK_ACCWR   (1<<5)
#define AFPFORK_ACCMASK (AFPFORK_ACCRD | AFPFORK_ACCWR)
#define AFPFORK_MODIFIED (1<<6) /* used in FCE for modified files */

#ifdef AFS
extern struct ofork *writtenfork;
#endif

#define of_name(a) (a)->of_ad->ad_m_name
/* in ofork.c */
extern struct ofork *of_alloc    (struct vol *, struct dir *,
                                                      char *, u_int16_t *, const int,
                                                      struct adouble *,
                                                      struct stat *);
extern void         of_dealloc   (struct ofork *);
extern struct ofork *of_find     (const u_int16_t);
extern struct ofork *of_findname (struct path *);
extern int          of_rename    (const struct vol *,
                                          struct ofork *,
                                          struct dir *, const char *,
                                          struct dir *, const char *);
extern int          of_flush     (const struct vol *);
extern void         of_pforkdesc (FILE *);
extern int          of_stat      (struct path *);
extern int          of_statdir   (struct vol *vol, struct path *);
extern int          of_closefork (struct ofork *ofork);
extern void         of_closevol  (const struct vol *vol);
extern void         of_close_all_forks(void);
extern struct adouble *of_ad     (const struct vol *, struct path *, struct adouble *);

#ifdef HAVE_ATFUNCS
extern struct ofork *of_findnameat(int dirfd, struct path *path);
extern int of_fstatat(int dirfd, struct path *path);
#endif  /* HAVE_ATFUNCS */


/* in fork.c */
extern int          flushfork    (struct ofork *);
extern int          getforkmode  (struct adouble *, int , off_t );

/* FP functions */
int afp_openfork (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_bytelock (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_getforkparams (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_setforkparams (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_read (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_write (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_flushfork (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_flush (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_closefork (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);

int afp_bytelock_ext (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_read_ext (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_write_ext (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen); 
int afp_syncfork (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
#endif
