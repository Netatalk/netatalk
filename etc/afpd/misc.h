#ifndef AFPD_MISC_H
#define AFPD_MISC_H 1

#include <sys/cdefs.h>
#include "globals.h"

/* FP functions */
/* messages.c */
extern int	afp_getsrvrmesg __P((AFPObj *, char *, int, char *, int *));

/* afs.c */
# ifdef AFS
extern int	afp_getdiracl __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setdiracl __P((AFPObj *, char *, int, char *, int *));
# else AFS
#define afp_getdiracl	NULL
#define afp_setdiracl	NULL
# endif AFS

# if defined( AFS ) && defined( UAM_AFSKRB )
extern int	afp_afschangepw __P((AFPObj *, char *, int, char *, int *));
# else AFS UAM_AFSKRB
#define afp_afschangepw	NULL
# endif AFS UAM_AFSKRB

#endif
