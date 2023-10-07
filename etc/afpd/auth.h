/*
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

#include <atalk/globals.h>

struct afp_versions {
    char	*av_name;
    int		av_number;
};

static const struct afp_versions  afp_versions[] = {
#ifndef NO_DDP
    { "AFPVersion 1.1", 11 },
    { "AFPVersion 2.0", 20 },
    { "AFPVersion 2.1", 21 },
#endif /* ! NO_DDP */
    { "AFP2.2", 22 },
    { "AFPX03", 30 },
    { "AFP3.1", 31 },
    { "AFP3.2", 32 },
    { "AFP3.3", 33 }
};

/* for GetUserInfo */
#define USERIBIT_USER  (1 << 0)
#define USERIBIT_GROUP (1 << 1)
#define USERIBIT_UUID  (1 << 2)
#define USERIBIT_ALL   (USERIBIT_USER | USERIBIT_GROUP | USERIBIT_UUID)

extern uid_t    uuid;
extern gid_t	*groups;
extern int	ngroups;

extern int set_groups(AFPObj *obj, struct passwd *pwd);
extern const char *print_groups(int ngroups, gid_t *groups);

/* FP functions */
int afp_login (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_login_ext (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_logincont (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_changepw (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_logout (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_getuserinfo (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_getsession (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_disconnect (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);
int afp_zzz (AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);

#endif /* auth.h */
