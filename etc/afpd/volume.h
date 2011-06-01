/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_VOLUME_H
#define AFPD_VOLUME_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>

#include <atalk/volume.h>
#include <atalk/cnid.h>
#include <atalk/unicode.h>
#include <atalk/globals.h>

extern struct vol       *getvolbyvid (const u_int16_t);
extern int              ustatfs_getvolspace (const struct vol *,
            VolSpace *, VolSpace *,
            u_int32_t *);
extern void             setvoltime (AFPObj *, struct vol *);
extern int              pollvoltime (AFPObj *);
extern void             load_volumes (AFPObj *obj);
extern const struct vol *getvolumes(void);
extern void             unload_volumes_and_extmap(void);
extern void             vol_fce_tm_event(void);

/* FP functions */
int afp_openvol      (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_getvolparams (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_setvolparams (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_getsrvrparms (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_closevol     (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);

/* netatalk functions */
extern void     close_all_vol   (void);

struct vol *current_vol;        /* last volume from getvolbyvid() */

#endif
