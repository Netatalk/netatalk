/*
 * $Id: uid.h,v 1.4 2001-08-15 01:13:25 samnoble Exp $
 * code: jeff@univrel.pr.uconn.edu
 */

#ifndef AFPD_UID_H
#define AFPD_UID_H 1

#ifdef FORCE_UIDGID

/* have to make sure struct vol is defined */
#include "volume.h"

/* set up a structure for this */
typedef struct uidgidset_t {
	uid_t uid;
	gid_t gid;
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
