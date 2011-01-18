/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef AFPD_GLOBALS_H
#define AFPD_GLOBALS_H 1

#include <sys/param.h>

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
#include <atalk/unicode.h>
#include <atalk/uam.h>

#define MACFILELEN 31
#define MAXUSERLEN 256

#define OPTION_DEBUG         (1 << 0)
#define OPTION_USERVOLFIRST  (1 << 1)
#define OPTION_NOUSERVOL     (1 << 2)
#define OPTION_PROXY         (1 << 3)
#define OPTION_CUSTOMICON    (1 << 4)
#define OPTION_NOSLP         (1 << 5)
#define OPTION_ANNOUNCESSH   (1 << 6)
#define OPTION_UUID          (1 << 7)
#define OPTION_ACL2MACCESS   (1 << 8)
#define OPTION_NOZEROCONF    (1 << 9)

#ifdef FORCE_UIDGID
/* set up a structure for this */
typedef struct uidgidset_t {
    uid_t uid;
    gid_t gid;
} uidgidset;
#endif /* FORCE_UIDGID */

/* a couple of these options could get stuck in unions to save
 * space. */
struct afp_volume_name {
    time_t     mtime;
    char       *name;
    char       *full_name;
    int        loaded;
};

struct afp_options {
    int connections, transports, tickleval, timeout, server_notif, flags, dircachesize;
    unsigned char passwdbits, passwdminlen, loginmaxfail;
    u_int32_t server_quantum;
    char hostname[MAXHOSTNAMELEN + 1], *server, *ipaddr, *port, *configfile;
    struct at_addr ddpaddr;
    char *uampath, *fqdn;
    char *pidfile;
    char *sigconffile;
    char *uuidconf;
    struct afp_volume_name defaultvol, systemvol, uservol;
    int  closevol;

    char *guest, *loginmesg, *keyfile, *passwdfile;
    char *uamlist;
    char *authprintdir;
    char *signatureopt;
    unsigned char signature[16];
    char *k5service, *k5realm, *k5keytab;
    char *unixcodepage,*maccodepage;
    charset_t maccharset, unixcharset; 
    mode_t umask;
    mode_t save_mask;
    int    sleep;
#ifdef ADMIN_GRP
    gid_t admingid;
#endif /* ADMIN_GRP */
    int    volnamelen;

    /* default value for winbind authentication */
    char *ntdomain, *ntseparator;
    char *logconfig;
};

#define AFPOBJ_TMPSIZ (MAXPATHLEN)
typedef struct _AFPObj {
    int proto;
    unsigned long servernum;
    void *handle;               /* either (DSI *) or (ASP *) */
    void *config; 
    struct afp_options options;
    char *Obj, *Type, *Zone;
    char username[MAXUSERLEN];
    void (*logout)(void), (*exit)(int);
    int (*reply)(void *, int);
    int (*attention)(void *, AFPUserBytes);
    void (*sleep)(void);
    /* to prevent confusion, only use these in afp_* calls */
    char oldtmp[AFPOBJ_TMPSIZ + 1], newtmp[AFPOBJ_TMPSIZ + 1];
    void *uam_cookie; /* cookie for uams */
    struct session_info  sinfo;
    uid_t uid; 	/* client running user id */

#ifdef FORCE_UIDGID
    int                 force_uid;
    uidgidset		uidgid;
#endif
} AFPObj;

/* typedef for AFP functions handlers */
typedef int (*AFPCmd)(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,  size_t *rbuflen);

/* afp_dsi.c */
extern AFPObj *AFPobj;

extern int		afp_version;
extern int		afp_errno;
extern unsigned char	nologin;
extern struct dir	*curdir;
extern char		getwdbuf[];

/* FIXME CNID */
extern const char *Cnid_srv;
extern const char *Cnid_port;

extern int  get_afp_errno   (const int param);
extern void afp_options_init (struct afp_options *);
extern int afp_options_parse (int, char **, struct afp_options *);
extern int afp_options_parseline (char *, struct afp_options *);
extern void afp_options_free (struct afp_options *,
                                      const struct afp_options *);
extern void setmessage (const char *);
extern void readmessage (AFPObj *);

/* gettok.c */
extern void initline   (int, char *);
extern int  parseline  (int, char *);

/* afp_util.c */
extern const char *AfpNum2name (int );
extern const char *AfpErr2name(int err);

/* directory.c */
extern struct dir rootParent;

#ifndef NO_DDP
extern void afp_over_asp (AFPObj *);
#endif /* NO_DDP */
extern void afp_over_dsi (AFPObj *);

#endif /* globals.h */
