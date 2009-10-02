/*
 * $Id: volume.h,v 1.34 2009-10-02 09:32:40 franklahm Exp $
 *
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

#include "globals.h"
#if 0
#include "hash.h"
#endif

extern struct vol	*getvolbyvid __P((const u_int16_t));
extern int              ustatfs_getvolspace __P((const struct vol *,
            VolSpace *, VolSpace *,
            u_int32_t *));
extern void             setvoltime __P((AFPObj *, struct vol *));
extern int              pollvoltime __P((AFPObj *));
extern void             load_volumes __P((AFPObj *obj));

/* FP functions */
extern int	afp_openvol      __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getvolparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setvolparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getsrvrparms __P((AFPObj *, char *, int, char *, int *));
extern int	afp_closevol     __P((AFPObj *, char *, int, char *, int *));

/* netatalk functions */
extern void     close_all_vol   __P((void));

#endif
