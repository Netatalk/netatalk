/*
 * $Id: globals.h,v 1.8 2001-12-15 06:25:44 jmarcus Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_GLOBALS_H
#define AFPD_GLOBALS_H 1

#include <sys/param.h>
#include <sys/cdefs.h>

#ifdef ADMIN_GRP
#include <grp.h>
#include <sys/types.h>
#endif /* ADMIN_GRP */

#ifdef HAVE_NETDB_H
#include <netdb.h>  /* this isn't header-protected under ultrix */
#endif /* HAVE_NETDB_H */

#include <netatalk/at.h>
#include <atalk/afp.h>
#include <atalk/compat.h>

/* test for inline */
#ifndef __inline__
#define __inline__
#endif

#define MACFILELEN 31

#define OPTION_DEBUG         (1 << 0)
#define OPTION_USERVOLFIRST  (1 << 1)
#define OPTION_NOUSERVOL     (1 << 2)
#define OPTION_PROXY         (1 << 3)
#define OPTION_CUSTOMICON    (1 << 4)

/* a couple of these options could get stuck in unions to save
 * space. */
struct afp_options {
    int connections, port, transports, tickleval, timeout, flags;
    unsigned char passwdbits, passwdminlen, loginmaxfail;
    u_int32_t server_quantum;
    char hostname[MAXHOSTNAMELEN + 1], *server, *ipaddr, *configfile;
    struct at_addr ddpaddr;
    char *uampath, *nlspath, *fqdn;
    char *pidfile, *defaultvol, *systemvol;
    char *guest, *loginmesg, *keyfile, *passwdfile;
    char *uamlist;
    char *authprintdir;
    mode_t umask;
#ifdef ADMIN_GRP
    gid_t admingid;
#endif /* ADMIN_GRP */
};

#define AFPOBJ_TMPSIZ (MAXPATHLEN)
typedef struct AFPObj {
    int proto;
    unsigned long servernum;
    void *handle, *config;
    struct afp_options options;
    char *Obj, *Type, *Zone;
    char username[MACFILELEN + 1];
    void (*logout)(void), (*exit)(int);
    int (*reply)(void *, int);
    int (*attention)(void *, AFPUserBytes);
    /* to prevent confusion, only use these in afp_* calls */
    char oldtmp[AFPOBJ_TMPSIZ + 1], newtmp[AFPOBJ_TMPSIZ + 1];
    void *uam_cookie; /* cookie for uams */
} AFPObj;

extern int		afp_version;
extern unsigned char	nologin;
extern struct vol	*volumes;
extern struct dir	*curdir;
extern char		getwdbuf[];

extern void afp_options_init __P((struct afp_options *));
extern int afp_options_parse __P((int, char **, struct afp_options *));
extern int afp_options_parseline __P((char *, struct afp_options *));
extern void afp_options_free __P((struct afp_options *,
                                      const struct afp_options *));
extern void setmessage __P((const char *));
extern void readmessage __P((void));

/* gettok.c */
extern void initline   __P((int, char *));
extern int  parseline  __P((int, char *));

#ifndef NO_DDP
extern void afp_over_asp __P((AFPObj *));
#endif /* NO_DDP */
extern void afp_over_dsi __P((AFPObj *));

#endif /* globals.h */
