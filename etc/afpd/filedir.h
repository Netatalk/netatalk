/*
 * $Id: filedir.h,v 1.3 2001-06-20 18:33:04 rufustfirefly Exp $
 */

#ifndef AFPD_FILEDIR_H
#define AFPD_FILEDIR_H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include "globals.h"
#include "volume.h"

extern char		*ctoupath __P((const struct vol *, struct dir *, 
				       char *));

/* FP functions */
extern int	matchfile2dirperms __P((char *, struct vol *, int));
extern int	afp_moveandrename __P((AFPObj *, char *, int, char *, int *));
extern int	afp_rename __P((AFPObj *, char *, int, char *, int *));
extern int	afp_delete __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getfildirparams __P((AFPObj *, char *, int, char *, 
					 int *));
extern int	afp_setfildirparams __P((AFPObj *, char *, int, char *, 
					 int *));

#endif
