/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Open fork state, byte range locking and AFP fork command handlers
 */

#ifndef AFPD_FORK_H
#define AFPD_FORK_H 1

#include <arpa/inet.h>
#include <stdio.h>

#include <atalk/adouble.h>

#include "directory.h"
#include "volume.h"

struct file_key {
    dev_t       dev;
    ino_t       inode;
};

struct ofork {
    struct file_key     key;
    struct adouble      *of_ad;
    struct vol          *of_vol;
    char                *of_uname;  /*!< the file's name on disk */
    cnid_t              of_did;
    uint16_t            of_refnum;
    int                 of_flags;
    int                 of_adflags; /*!< ADFLAGS_DF/HF/RF this fork holds */
    const unsigned char *of_virtual_data;
    off_t               of_virtual_len;
    struct ofork        **prevp, *next;
};

#define OPENFORK_DATA   (0)
#define OPENFORK_RSCS   (1<<7)

#define OPENACC_RD  (1<<0)
#define OPENACC_WR  (1<<1)
#define OPENACC_DRD (1<<4)
#define OPENACC_DWR (1<<5)

/* ofork.of_flags bits */
#define AFPFORK_DATA    (1<<0)  /*!< open datafork */
#define AFPFORK_RSRC    (1<<1)  /*!< open rsrcfork */
#define AFPFORK_META    (1<<2)  /*!< open metadata */
#define AFPFORK_DIRTY   (1<<3)
#define AFPFORK_ACCRD   (1<<4)
#define AFPFORK_ACCWR   (1<<5)
#define AFPFORK_ACCMASK (AFPFORK_ACCRD | AFPFORK_ACCWR)
#define AFPFORK_MODIFIED (1<<6) /*!< written or its length set since open */
#define AFPFORK_ERROR   (1<<7)  /*!< used to indicate an error in opening the fork */
#define AFPFORK_VIRTUAL (1<<8)  /*!< virtual file fork */
#define AFPFORK_RFHEADER (1<<9) /*!< its resource fork held a header at open */
#define AFPFORK_DENYRD  (1<<10) /*!< a virtual fork opened denying reads */
#define AFPFORK_DENYWR  (1<<11) /*!< a virtual fork opened denying writes */

#define of_name(a) (a)->of_ad->ad_name

/*! @brief Check that [offset, offset + length) fits in size without overflow */
static inline int fork_range_within(off_t offset, off_t length, off_t size)
{
    return offset >= 0 && length >= 0 && size >= 0
           && offset <= size && length <= size - offset;
}

/* in ofork.c */
extern struct ofork *of_alloc(struct vol *, struct dir *,
                              char *, const char *, uint16_t *, const int,
                              struct adouble *,
                              struct stat *);
extern void         of_dealloc(struct ofork *);
extern struct ofork *of_find(const uint16_t);
extern struct ofork *of_findname(const struct vol *vol, struct path *);
extern void         of_rekey(struct ofork *of, const struct stat *st);
extern int          of_rename(const struct vol *,
                              struct ofork *,
                              struct dir *, const char *,
                              struct dir *, const char *, const char *);
extern int          of_flush(const struct vol *);
extern void         of_pforkdesc(FILE *);
extern void         of_log_usage(int level, const char *event);
extern void         of_log_nfile(const char *name, int err);
extern void         of_note_open(int newfd);
extern const char   *of_breakdown(char *buf, size_t len);
extern int          of_stat(const struct vol *vol, struct path *);
extern int          of_statdir(struct vol *vol, struct path *);
extern int          of_closefork(const AFPObj *obj, struct ofork *ofork);
extern struct dir   *of_dircache_entry(const struct ofork *ofork);
extern void         of_rfork_changed(const struct ofork *ofork);
extern int          of_rfork_held(const struct ofork *ofork,
                                  const char *forkpath);
extern int          of_sidecar_fd(const struct vol *vol, const char *name);
extern void         of_hint_siblings(const AFPObj *obj,
                                     const struct ofork *ofork,
                                     const struct dir *cached);
extern void         of_closevol(const AFPObj *obj, const struct vol *vol);
extern void         of_close_all_forks(const AFPObj *obj);
extern struct adouble *of_ad(const struct vol *, struct path *,
                             struct adouble *);
extern struct ofork *of_findnameat(const struct vol *vol, int dirfd,
                                   struct path *path);
extern int of_delete_blocked(const struct ofork *of, uint16_t *band,
                             int *content);
extern void of_close_inode_forks(const AFPObj *obj, const struct vol *vol,
                                 struct path *path);

enum of_locks_status { OF_LOCKS_ERROR = -1, OF_LOCKS_OK = 0, OF_LOCKS_NOENT = 1 };

extern int of_get_locks(const struct vol *vol, int dirfd, struct path *path,
                        int df_get, off_t df_off, off_t df_len,
                        int rf_get, off_t rf_off, off_t rf_len,
                        uint16_t band_request, int try_root,
                        int *df_locked, int *rf_locked, uint16_t *band_held);


/* in fork.c */
extern int          flushfork(struct ofork *);
extern int          fork_setmode_deny(int access, int f_rddny, int f_wrdny,
                                      int f_nodny);

/* FP functions */
int afp_openfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen);
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen);
int afp_getforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen);
int afp_setforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen);
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
             size_t *rbuflen);
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen);
int afp_flushfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);
int afp_flush(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen);
int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);

int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                     size_t *rbuflen);
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen);
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen);
int afp_syncfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen);
#endif
