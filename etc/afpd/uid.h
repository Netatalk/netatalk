/*
 * $Id: uid.h,v 1.3 2001-06-27 14:53:16 rufustfirefly Exp $
 * code: jeff@univrel.pr.uconn.edu
 */

#ifndef AFPD_UID_H
#define AFPD_UID_H 1

#ifdef FORCE_UIDGID

/* have to make sure struct vol is defined */
#include "volume.h"

/* set up a structure for this */
typedef struct uidgidset_t {
	int uid;
	int gid;
} uidgidset;

/* functions to save and restore uid/gid pairs */
extern void save_uidgid    ( uidgidset ** );
extern void restore_uidgid ( uidgidset ** );
extern void set_uidgid     ( const struct vol * );

/* internal functions to convert user and group names to ids */
extern int  user_to_uid  ( char * );
extern int  group_to_gid ( char * );

#endif /* FORCE_UIDGID */

#endif
