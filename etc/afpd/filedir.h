/*
 * $Id: filedir.h,v 1.10 2009-10-13 22:55:37 didg Exp $
 */

#ifndef AFPD_FILEDIR_H
#define AFPD_FILEDIR_H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include "globals.h"
#include "volume.h"

extern struct afp_options default_options;

extern char		*ctoupath (const struct vol *, struct dir *,
                                char *);
extern char		*absupath (const struct vol *, struct dir *,
                                char *);
extern int		veto_file (const char *veto_str, const char *path);
extern int 		check_name (const struct vol *vol, char *name);

/* FP functions */
extern int	matchfile2dirperms (char *, struct vol *, int);
extern int	afp_moveandrename (AFPObj *, char *, int, char *, int *);
extern int	afp_rename (AFPObj *, char *, int, char *, int *);
extern int	afp_delete (AFPObj *, char *, int, char *, int *);
extern int	afp_getfildirparams (AFPObj *, char *, int, char *,
                                        int *);
extern int	afp_setfildirparams (AFPObj *, char *, int, char *,
                                        int *);

#endif
