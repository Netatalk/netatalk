/*
 * $Id: auth.h,v 1.6 2005-04-28 20:49:40 bfernhomberg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_AUTH_H
#define AFPD_AUTH_H 1

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif /* HAVE_SYS_CDEFS_H */

#include "globals.h"

struct afp_versions {
    char	*av_name;
    int		av_number;
};

/* for GetUserInfo */
#define USERIBIT_USER  (1 << 0)
#define USERIBIT_GROUP (1 << 1)
#define USERIBIT_ALL   (USERIBIT_USER | USERIBIT_GROUP)

extern uid_t    uuid;
#if defined( sun ) && !defined( __svr4__ ) || defined( ultrix )
extern int	*groups;
#else /*sun __svr4__ ultrix*/
extern gid_t	*groups;
#endif /*sun __svr4__ ultrix*/
extern int	ngroups;

/* FP functions */
extern int	afp_login __P((AFPObj *, char *, int, char *, int *));
extern int	afp_login_ext __P((AFPObj *, char *, unsigned int, char *, unsigned int *));
extern int	afp_logincont __P((AFPObj *, char *, int, char *, int *));
extern int	afp_changepw __P((AFPObj *, char *, int, char *, int *));
extern int	afp_logout __P((AFPObj *, char *, int, char *, int *));
extern int      afp_getuserinfo __P((AFPObj *, char *, int, char *, int *));
extern int      afp_getsession __P((AFPObj *, char *, unsigned int, char *, unsigned int *));
extern int      afp_disconnect __P((AFPObj *, char *, int, char *, int *));
extern int      afp_zzz __P((AFPObj *, char *, unsigned int, char *, unsigned int *));

#endif /* auth.h */
