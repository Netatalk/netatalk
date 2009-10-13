/*
 * $Id: misc.h,v 1.4 2009-10-13 22:55:37 didg Exp $
 */

#ifndef AFPD_MISC_H
#define AFPD_MISC_H 1

#include <sys/cdefs.h>
#include "globals.h"

/* FP functions */
/* messages.c */
extern int	afp_getsrvrmesg (AFPObj *, char *, int, char *, int *);

/* afs.c */
#ifdef AFS
extern int	afp_getdiracl (AFPObj *, char *, int, char *, int *);
extern int	afp_setdiracl (AFPObj *, char *, int, char *, int *);
#else /* AFS */
#define afp_getdiracl	NULL
#define afp_setdiracl	NULL
#endif /* AFS */

#if defined( AFS ) && defined( UAM_AFSKRB )
extern int	afp_afschangepw (AFPObj *, char *, int, char *, int *);
#else /* AFS && UAM_AFSKRB */
#define afp_afschangepw	NULL
#endif /* AFS && UAM_AFSKRB */

#endif
