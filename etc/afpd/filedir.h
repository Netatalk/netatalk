/*
 * $Id: filedir.h,v 1.5 2001-10-23 13:44:37 rufustfirefly Exp $
 */

#ifndef AFPD_FILEDIR_H
#define AFPD_FILEDIR_H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include "globals.h"
#include "volume.h"

extern struct afp_options default_options;

extern char		*ctoupath __P((const struct vol *, struct dir *, 
				       char *));
extern int		veto_file __P((const char *veto_str, const char *path));

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
