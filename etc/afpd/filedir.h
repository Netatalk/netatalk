#ifndef AFPD_FILEDIR_H
#define AFPD_FILEDIR_H 1

#include <sys/cdefs.h>
#include "globals.h"
#include "volume.h"

extern char		*ctoupath __P((const struct vol *, struct dir *, 
				       char *));

/* FP functions */
extern int	afp_moveandrename __P((AFPObj *, char *, int, char *, int *));
extern int	afp_rename __P((AFPObj *, char *, int, char *, int *));
extern int	afp_delete __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getfildirparams __P((AFPObj *, char *, int, char *, 
					 int *));
extern int	afp_setfildirparams __P((AFPObj *, char *, int, char *, 
					 int *));
#endif
