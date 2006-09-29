/*
 * $Id: volume.c,v 1.68 2006-09-29 09:39:16 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB*/

#include "globals.h"
#include "directory.h"
#include "file.h"
#include "volume.h"
#include "unix.h"
#include "fork.h"

extern int afprun(int root, char *cmd, int *outfd);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

#ifndef NO_LARGE_VOL_SUPPORT
#if BYTE_ORDER == BIG_ENDIAN
#define hton64(x)       (x)
#define ntoh64(x)       (x)
#else /* BYTE_ORDER == BIG_ENDIAN */
#define hton64(x)       ((u_int64_t) (htonl(((x) >> 32) & 0xffffffffLL)) | \
                         (u_int64_t) ((htonl(x) & 0xffffffffLL) << 32))
#define ntoh64(x)       (hton64(x))
#endif /* BYTE_ORDER == BIG_ENDIAN */
#endif /* ! NO_LARGE_VOL_SUPPORT */

static struct vol *Volumes = NULL;
static u_int16_t	lastvid = 0;
static char		*Trash = "\02\024Network Trash Folder";

static struct extmap	*Extmap = NULL, *Defextmap = NULL;
static int              Extmap_cnt;
static void             free_extmap(void);

#define VOLOPT_ALLOW      0  /* user allow list */
#define VOLOPT_DENY       1  /* user deny list */
#define VOLOPT_RWLIST     2  /* user rw list */
#define VOLOPT_ROLIST     3  /* user ro list */
#define VOLOPT_PASSWORD   4  /* volume password */
#define VOLOPT_CASEFOLD   5  /* character case mangling */
#define VOLOPT_FLAGS      6  /* various flags */
#define VOLOPT_DBPATH     7  /* path to database */
#define VOLOPT_MAPCHARS   8  /* does mtou and utom mappings. syntax:
m and u can be double-byte hex
strings if necessary.
m=u -> map both ways
  m>u -> map m to u
  m<u -> map u to m
  !u  -> make u illegal always
  ~u  -> make u illegal only as the first
  part of a double-byte character.
  */
#define VOLOPT_VETO          10  /* list of veto filespec */
#define VOLOPT_PREEXEC       11  /* preexec command */
#define VOLOPT_ROOTPREEXEC   12  /* root preexec command */

#define VOLOPT_POSTEXEC      13  /* postexec command */
#define VOLOPT_ROOTPOSTEXEC  14  /* root postexec command */

#define VOLOPT_ENCODING      15  /* mac encoding (pre OSX)*/
#define VOLOPT_MACCHARSET    16
#define VOLOPT_CNIDSCHEME    17
#define VOLOPT_ADOUBLE       18  /* adouble version */
#ifdef FORCE_UIDGID
#warning UIDGID
#include "uid.h"

#define VOLOPT_FORCEUID  19  /* force uid for username x */
#define VOLOPT_FORCEGID  20  /* force gid for group x */
#define VOLOPT_UMASK     21
#define VOLOPT_DFLTPERM  22
#else 
#define VOLOPT_UMASK     19
#define VOLOPT_DFLTPERM  20
#endif /* FORCE_UIDGID */

#define VOLOPT_MAX       (VOLOPT_DFLTPERM +1)

#define VOLOPT_NUM        (VOLOPT_MAX + 1)

#define VOLPASSLEN  8
#define VOLOPT_DEFAULT     ":DEFAULT:"
#define VOLOPT_DEFAULT_LEN 9
  struct vol_option {
      char *c_value;
      int i_value;
  };

typedef struct _special_folder {
        const char *name;
        int precreate;
        mode_t mode;
        int hide;
} _special_folder;

static const _special_folder special_folders[] = {
  {"Network Trash Folder",     1,  0777,  1},
  {"Temporary Items",          1,  0777,  1},
  {".AppleDesktop",            1,  0777,  0},
#if 0
  {"TheFindByContentFolder",   0,     0,  1},
  {"TheVolumeSettingsFolder",  0,     0,  1},
#endif
  {NULL, 0, 0, 0}};

typedef struct _volopt_name {
	const u_int32_t option;
	const char 	*name;
} _vol_opt_name;

static const _vol_opt_name vol_opt_names[] = {
    {AFPVOL_A2VOL,      "PRODOS"},      /* prodos volume */
    {AFPVOL_CRLF,       "CRLF"},        /* cr/lf translation */
    {AFPVOL_NOADOUBLE,  "NOADOUBLE"},   /* don't create .AppleDouble by default */
    {AFPVOL_RO,         "READONLY"},    /* read-only volume */
    {AFPVOL_MSWINDOWS,  "MSWINDOWS"},   /* deal with ms-windows yuckiness. this is going away. */
    {AFPVOL_NOHEX,      "NOHEX"},       /* don't do :hex translation */
    {AFPVOL_USEDOTS,    "USEDOTS"},     /* use real dots */
    {AFPVOL_LIMITSIZE,  "LIMITSIZE"},   /* limit size for older macs */
    {AFPVOL_MAPASCII,   "MAPASCII"},    /* map the ascii range as well */
    {AFPVOL_DROPBOX,    "DROPBOX"},     /* dropkludge dropbox support */
    {AFPVOL_NOFILEID,   "NOFILEID"},    /* don't advertise createid resolveid and deleteid calls */
    {AFPVOL_NOSTAT,     "NOSTAT"},      /* advertise the volume even if we can't stat() it
                                         * maybe because it will be mounted later in preexec */
    {AFPVOL_UNIX_PRIV,  "UNIXPRIV"},    /* support unix privileges */
    {AFPVOL_NODEV,      "NODEV"},       /* always use 0 for device number in cnid calls */
    {AFPVOL_CASEINSEN,  "CASEINSENSITIVE"}, /* volume is case insensitive */
    {AFPVOL_EILSEQ,     "ILLEGALSEQ"},     /* encode illegal sequence */
    {AFPVOL_CACHE,      "CACHEID"},     /* Use adouble v2 CNID caching, default don't use it */
    {0, NULL}
};

static const _vol_opt_name vol_opt_casefold[] = {
    {AFPVOL_MTOUUPPER,	"MTOULOWER"},
    {AFPVOL_MTOULOWER,  "MTOULOWER"},
    {AFPVOL_UTOMUPPER,  "UTOMUPPER"},
    {AFPVOL_UTOMLOWER,  "UTOMLOWER"},
    {0, NULL}
};

static void handle_special_folders (const struct vol *);
static int savevoloptions (const struct vol *);
static void deletevol(struct vol *vol);

static __inline__ void volfree(struct vol_option *options,
                               const struct vol_option *save)
{
    int i;

    if (save) {
        for (i = 0; i < VOLOPT_MAX; i++) {
            if (options[i].c_value && (options[i].c_value != save[i].c_value))
                free(options[i].c_value);
        }
    } else {
        for (i = 0; i < VOLOPT_MAX; i++) {
            if (options[i].c_value)
                free(options[i].c_value);
        }
    }
}


/* handle variable substitutions. here's what we understand:
 * $b   -> basename of path
 * $c   -> client ip/appletalk address
 * $d   -> volume pathname on server
 * $f   -> full name (whatever's in the gecos field)
 * $g   -> group
 * $h   -> hostname 
 * $i   -> client ip/appletalk address without port
 * $s   -> server name (hostname if it doesn't exist)
 * $u   -> username (guest is usually nobody)
 * $v   -> volume name or basename if null
 * $z   -> zone (may not exist)
 * $$   -> $
 *
 *
 */
#define is_var(a, b) (strncmp((a), (b), 2) == 0)

static char *volxlate(AFPObj *obj, char *dest, size_t destlen,
                     char *src, struct passwd *pwd, char *path, char *volname)
{
    char *p, *q;
    int len;
    char *ret;
    
    if (!src) {
        return NULL;
    }
    if (!dest) {
        dest = calloc(destlen +1, 1);
    }
    ret = dest;
    if (!ret) {
        return NULL;
    }
    strlcpy(dest, src, destlen +1);
    if ((p = strchr(src, '$')) == NULL) /* nothing to do */
        return ret;

    /* first part of the path. just forward to the next variable. */
    len = MIN((size_t)(p - src), destlen);
    if (len > 0) {
        destlen -= len;
        dest += len;
    }

    while (p && destlen > 0) {
        /* now figure out what the variable is */
        q = NULL;
        if (is_var(p, "$b")) {
            if (path) {
                if ((q = strrchr(path, '/')) == NULL)
                    q = path;
                else if (*(q + 1) != '\0')
                    q++;
            }
        } else if (is_var(p, "$c")) {
            if (obj->proto == AFPPROTO_ASP) {
                ASP asp = obj->handle;

                len = sprintf(dest, "%u.%u", ntohs(asp->asp_sat.sat_addr.s_net),
                              asp->asp_sat.sat_addr.s_node);
                dest += len;
                destlen -= len;

            } else if (obj->proto == AFPPROTO_DSI) {
                DSI *dsi = obj->handle;

                len = sprintf(dest, "%s:%u", inet_ntoa(dsi->client.sin_addr),
                              ntohs(dsi->client.sin_port));
                dest += len;
                destlen -= len;
            }
        } else if (is_var(p, "$d")) {
             q = path;
        } else if (is_var(p, "$f")) {
            if ((q = strchr(pwd->pw_gecos, ',')))
                *q = '\0';
            q = pwd->pw_gecos;
        } else if (is_var(p, "$g")) {
            struct group *grp = getgrgid(pwd->pw_gid);
            if (grp)
                q = grp->gr_name;
        } else if (is_var(p, "$h")) {
            q = obj->options.hostname;
        } else if (is_var(p, "$i")) {
            if (obj->proto == AFPPROTO_ASP) {
                ASP asp = obj->handle;
 
                len = sprintf(dest, "%u", ntohs(asp->asp_sat.sat_addr.s_net));
                dest += len;
                destlen -= len;
 
            } else if (obj->proto == AFPPROTO_DSI) {
                DSI *dsi = obj->handle;
 
                q = inet_ntoa(dsi->client.sin_addr);
            }
        } else if (is_var(p, "$s")) {
            if (obj->Obj)
                q = obj->Obj;
            else if (obj->options.server) {
                q = obj->options.server;
            } else
                q = obj->options.hostname;
        } else if (is_var(p, "$u")) {
            q = obj->username;
        } else if (is_var(p, "$v")) {
            if (volname) {
                q = volname;
            }
            else if (path) {
                if ((q = strrchr(path, '/')) == NULL)
                    q = path;
                else if (*(q + 1) != '\0')
                    q++;
            }
        } else if (is_var(p, "$z")) {
            q = obj->Zone;
        } else if (is_var(p, "$$")) {
            q = "$";
        } else
            q = p;

        /* copy the stuff over. if we don't understand something that we
         * should, just skip it over. */
        if (q) {
            len = MIN(p == q ? 2 : strlen(q), destlen);
            strncpy(dest, q, len);
            dest += len;
            destlen -= len;
        }

        /* stuff up to next $ */
        src = p + 2;
        p = strchr(src, '$');
        len = p ? MIN((size_t)(p - src), destlen) : destlen;
        if (len > 0) {
            strncpy(dest, src, len);
            dest += len;
            destlen -= len;
        }
    }
    return ret;
}

/* to make sure that val is valid, make sure to select an opt that
   includes val */
static int optionok(const char *buf, const char *opt, const char *val) 
{
    if (!strstr(buf,opt))
        return 0;
    if (!val[1])
        return 0;
    return 1;    
}


/* -------------------- */
static void setoption(struct vol_option *options, struct vol_option *save, int opt, const char *val)
{
    if (options[opt].c_value && (!save || options[opt].c_value != save[opt].c_value))
        free(options[opt].c_value);
    options[opt].c_value = strdup(val + 1);
}

/* ------------------------------------------
   handle all the options. tmp can't be NULL. */
static void volset(struct vol_option *options, struct vol_option *save, 
                   char *volname, int vlen,
                   const char *tmp)
{
    char *val;

    val = strchr(tmp, ':');
    if (!val) {
        /* we'll assume it's a volume name. */
        strncpy(volname, tmp, vlen);
        volname[vlen] = 0;
        return;
    }
#if 0
    LOG(log_debug, logtype_afpd, "Parsing volset %s", val);
#endif
    if (optionok(tmp, "allow:", val)) {
        setoption(options, save, VOLOPT_ALLOW, val);

    } else if (optionok(tmp, "deny:", val)) {
        setoption(options, save, VOLOPT_DENY, val);

    } else if (optionok(tmp, "rwlist:", val)) {
        setoption(options, save, VOLOPT_RWLIST, val);

    } else if (optionok(tmp, "rolist:", val)) {
        setoption(options, save, VOLOPT_ROLIST, val);

    } else if (optionok(tmp, "codepage:", val)) {
	LOG (log_error, logtype_afpd, "The old codepage system has been removed. Please make sure to read the documentation !!!!");
	/* Make sure we don't screw anything */
	exit (EXITERR_CONF);
    } else if (optionok(tmp, "volcharset:", val)) {
        setoption(options, save, VOLOPT_ENCODING, val);
    } else if (optionok(tmp, "maccharset:", val)) {
        setoption(options, save, VOLOPT_MACCHARSET, val);
    } else if (optionok(tmp, "veto:", val)) {
        setoption(options, save, VOLOPT_VETO, val);
    } else if (optionok(tmp, "cnidscheme:", val)) {
        setoption(options, save, VOLOPT_CNIDSCHEME, val);
    } else if (optionok(tmp, "casefold:", val)) {
        if (strcasecmp(val + 1, "tolower") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UMLOWER;
        else if (strcasecmp(val + 1, "toupper") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UMUPPER;
        else if (strcasecmp(val + 1, "xlatelower") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UUPPERMLOWER;
        else if (strcasecmp(val + 1, "xlateupper") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_ULOWERMUPPER;
    } else if (optionok(tmp, "adouble:", val)) {
        if (strcasecmp(val + 1, "v1") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION1;
#if AD_VERSION == AD_VERSION2            
        else if (strcasecmp(val + 1, "v2") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION2;
        else if (strcasecmp(val + 1, "osx") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION2_OSX;
        else if (strcasecmp(val + 1, "ads") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION1_ADS;
        else if (strcasecmp(val + 1, "sfm") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION1_SFM;
#endif
    } else if (optionok(tmp, "options:", val)) {
        char *p;

        if ((p = strtok(val + 1, ",")) == NULL) /* nothing */
            return;

        while (p) {
            if (strcasecmp(p, "prodos") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_A2VOL;
            else if (strcasecmp(p, "mswindows") == 0) {
                options[VOLOPT_FLAGS].i_value |= AFPVOL_MSWINDOWS | AFPVOL_USEDOTS;
            } else if (strcasecmp(p, "crlf") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_CRLF;
            else if (strcasecmp(p, "noadouble") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NOADOUBLE;
            else if (strcasecmp(p, "ro") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_RO;
            else if (strcasecmp(p, "nohex") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NOHEX;
            else if (strcasecmp(p, "usedots") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_USEDOTS;
            else if (strcasecmp(p, "invisibledots") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_USEDOTS | AFPVOL_INV_DOTS;
            else if (strcasecmp(p, "limitsize") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_LIMITSIZE;
            /* support for either "dropbox" or "dropkludge" */
            else if (strcasecmp(p, "dropbox") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_DROPBOX;
            else if (strcasecmp(p, "dropkludge") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_DROPBOX;
            else if (strcasecmp(p, "nofileid") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NOFILEID;
            else if (strcasecmp(p, "nostat") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NOSTAT;
            else if (strcasecmp(p, "preexec_close") == 0)
		options[VOLOPT_PREEXEC].i_value = 1;
            else if (strcasecmp(p, "root_preexec_close") == 0)
		options[VOLOPT_ROOTPREEXEC].i_value = 1;
            else if (strcasecmp(p, "upriv") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_UNIX_PRIV;
            else if (strcasecmp(p, "nodev") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NODEV;
            else if (strcasecmp(p, "caseinsensitive") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_CASEINSEN;
            else if (strcasecmp(p, "illegalseq") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_EILSEQ;
            else if (strcasecmp(p, "cachecnid") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_CACHE;

            p = strtok(NULL, ",");
        }

    } else if (optionok(tmp, "dbpath:", val)) {
        setoption(options, save, VOLOPT_DBPATH, val);

    } else if (optionok(tmp, "umask:", val)) {
	options[VOLOPT_UMASK].i_value = (int)strtol(val +1, NULL, 8);
    } else if (optionok(tmp, "perm:", val)) {
        options[VOLOPT_DFLTPERM].i_value = (int)strtol(val+1, NULL, 8);
    } else if (optionok(tmp, "mapchars:",val)) {
        setoption(options, save, VOLOPT_MAPCHARS, val);

    } else if (optionok(tmp, "password:", val)) {
        setoption(options, save, VOLOPT_PASSWORD, val);

#ifdef FORCE_UIDGID

        /* this code allows forced uid/gid per volume settings */
    } else if (optionok(tmp, "forceuid:", val)) {
        setoption(options, save, VOLOPT_FORCEUID, val);
    } else if (optionok(tmp, "forcegid:", val)) {
        setoption(options, save, VOLOPT_FORCEGID, val);

#endif /* FORCE_UIDGID */
    } else if (optionok(tmp, "root_preexec:", val)) {
        setoption(options, save, VOLOPT_ROOTPREEXEC, val);

    } else if (optionok(tmp, "preexec:", val)) {
        setoption(options, save, VOLOPT_PREEXEC, val);

    } else if (optionok(tmp, "root_postexec:", val)) {
        setoption(options, save, VOLOPT_ROOTPOSTEXEC, val);

    } else if (optionok(tmp, "postexec:", val)) {
        setoption(options, save, VOLOPT_POSTEXEC, val);

    } else {
        /* ignore unknown options */
        LOG(log_debug, logtype_afpd, "ignoring unknown volume option: %s", tmp);

    } 
}

/* ----------------- */
static void showvol(const ucs2_t *name)
{
    struct vol	*volume;
    for ( volume = Volumes; volume; volume = volume->v_next ) {
        if (volume->v_hide && !strcasecmp_w( volume->v_name, name ) ) {
            volume->v_hide = 0;
            return;
        }
    }
}

/* ------------------------------- */
static int creatvol(AFPObj *obj, struct passwd *pwd, 
                    char *path, char *name, 
                    struct vol_option *options, 
                    const int user /* user defined volume */
                    )
{
    struct vol	*volume;
    int		vlen;
    int         hide = 0;
    ucs2_t	tmpname[512];

    if ( name == NULL || *name == '\0' ) {
        if ((name = strrchr( path, '/' )) == NULL) {
            return -1;	/* Obviously not a fully qualified path */
        }

        /* if you wish to share /, you need to specify a name. */
        if (*++name == '\0')
            return -1;
    }

    vlen = strlen( name );
    if ( vlen > AFPVOL_NAMELEN ) {
        vlen = AFPVOL_NAMELEN;
        name[AFPVOL_NAMELEN] = '\0';
    }

    /* convert name to UCS2 first */
    if ( 0 >= ( vlen = convert_string(obj->options.unixcharset, CH_UCS2, name, vlen, tmpname, 512)) )
        return -1;

    for ( volume = Volumes; volume; volume = volume->v_next ) {
        if ( strcasecmp_w( volume->v_name, tmpname ) == 0 ) {
           if (volume->v_deleted) {
               volume->v_new = hide = 1;
           }
           else {
               return -1;	/* Won't be able to access it, anyway... */
           }
        }
    }


    if (!( volume = (struct vol *)calloc(1, sizeof( struct vol ))) ) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        return -1;
    }
    if ( NULL == ( volume->v_name = strdup_w(tmpname))) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        free(volume);
        return -1;
    }
    if (!( volume->v_path = (char *)malloc( strlen( path ) + 1 )) ) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        free(volume->v_name);
        free(volume);
        return -1;
    }
    volume->v_hide = hide;
    strcpy( volume->v_path, path );

#ifdef __svr4__
    volume->v_qfd = -1;
#endif /* __svr4__ */
    /* os X start at 1 and use network order ie. 1 2 3 */
    volume->v_vid = ++lastvid;
    volume->v_vid = htons(volume->v_vid);

    /* handle options */
    if (options) {
        /* should we casefold? */
        volume->v_casefold = options[VOLOPT_CASEFOLD].i_value;

        /* shift in some flags */
        volume->v_flags = options[VOLOPT_FLAGS].i_value;

        volume->v_ad_options = 0;
        if ((volume->v_flags & AFPVOL_NODEV))
            volume->v_ad_options |= ADVOL_NODEV;
        if ((volume->v_flags & AFPVOL_CACHE))
            volume->v_ad_options |= ADVOL_CACHE;
        if ((volume->v_flags & AFPVOL_UNIX_PRIV))
            volume->v_ad_options |= ADVOL_UNIXPRIV;
        if ((volume->v_flags & AFPVOL_INV_DOTS))
            volume->v_ad_options |= ADVOL_INVDOTS;

        if (options[VOLOPT_PASSWORD].c_value)
            volume->v_password = strdup(options[VOLOPT_PASSWORD].c_value);

        if (options[VOLOPT_VETO].c_value)
            volume->v_veto = strdup(options[VOLOPT_VETO].c_value);

        if (options[VOLOPT_ENCODING].c_value)
            volume->v_volcodepage = strdup(options[VOLOPT_ENCODING].c_value);

        if (options[VOLOPT_MACCHARSET].c_value)
            volume->v_maccodepage = strdup(options[VOLOPT_MACCHARSET].c_value);

        if (options[VOLOPT_DBPATH].c_value)
            volume->v_dbpath = volxlate(obj, NULL, MAXPATHLEN, options[VOLOPT_DBPATH].c_value, pwd, path, name);

       if (options[VOLOPT_CNIDSCHEME].c_value)
           volume->v_cnidscheme = strdup(options[VOLOPT_CNIDSCHEME].c_value);

	if (options[VOLOPT_UMASK].i_value)
	    volume->v_umask = (mode_t)options[VOLOPT_UMASK].i_value;

	if (options[VOLOPT_DFLTPERM].i_value)
	    volume->v_perm = (mode_t)options[VOLOPT_DFLTPERM].i_value;

	if (options[VOLOPT_ADOUBLE].i_value)
	    volume->v_adouble = options[VOLOPT_ADOUBLE].i_value;
	else 
	    volume->v_adouble = AD_VERSION;

	initvol_vfs(volume);
#ifdef FORCE_UIDGID
        if (options[VOLOPT_FORCEUID].c_value) {
            volume->v_forceuid = strdup(options[VOLOPT_FORCEUID].c_value);
        } else {
            volume->v_forceuid = NULL; /* set as null so as to return 0 later on */
        }

        if (options[VOLOPT_FORCEGID].c_value) {
            volume->v_forcegid = strdup(options[VOLOPT_FORCEGID].c_value);
        } else {
            volume->v_forcegid = NULL; /* set as null so as to return 0 later on */
        }
#endif
        if (!user) {
            if (options[VOLOPT_PREEXEC].c_value)
                volume->v_preexec = volxlate(obj, NULL, MAXPATHLEN, options[VOLOPT_PREEXEC].c_value, pwd, path, name);
            volume->v_preexec_close = options[VOLOPT_PREEXEC].i_value;

            if (options[VOLOPT_POSTEXEC].c_value)
                volume->v_postexec = volxlate(obj, NULL, MAXPATHLEN, options[VOLOPT_POSTEXEC].c_value, pwd, path, name);

            if (options[VOLOPT_ROOTPREEXEC].c_value)
                volume->v_root_preexec = volxlate(obj, NULL, MAXPATHLEN, options[VOLOPT_ROOTPREEXEC].c_value, pwd, path,  name);
            volume->v_root_preexec_close = options[VOLOPT_ROOTPREEXEC].i_value;

            if (options[VOLOPT_ROOTPOSTEXEC].c_value)
                volume->v_root_postexec = volxlate(obj, NULL, MAXPATHLEN, options[VOLOPT_ROOTPOSTEXEC].c_value, pwd, path,  name);
        }
    }

    volume->v_next = Volumes;
    Volumes = volume;
    return 0;
}

/* ---------------- */
static char *myfgets( buf, size, fp )
char	*buf;
int		size;
FILE	*fp;
{
    char	*p;
    int		c;

    p = buf;
    while ((EOF != ( c = getc( fp )) ) && ( size > 0 )) {
        if ( c == '\n' || c == '\r' ) {
            *p++ = '\n';
            break;
        } else {
            *p++ = c;
        }
        size--;
    }

    if ( p == buf ) {
        return( NULL );
    } else {
        *p = '\0';
        return( buf );
    }
}


/* check access list. this function wants something of the following
 * form:
 *        @group,name,name2,@group2,name3
 *
 * a NULL argument allows everybody to have access.
 * we return three things:
 *     -1: no list
 *      0: list exists, but name isn't in it
 *      1: in list
 */

#ifndef NO_REAL_USER_NAME
/* authentication is case insensitive 
 * FIXME should we do the same with group name?
*/
#define access_strcmp strcasecmp

#else
#define access_strcmp strcmp

#endif

static int accessvol(args, name)
const char *args;
const char *name;
{
    char buf[MAXPATHLEN + 1], *p;
    struct group *gr;

    if (!args)
        return -1;

    strlcpy(buf, args, sizeof(buf));
    if ((p = strtok(buf, ",")) == NULL) /* nothing, return okay */
        return -1;

    while (p) {
        if (*p == '@') { /* it's a group */
            if ((gr = getgrnam(p + 1)) && gmem(gr->gr_gid))
                return 1;
        } else if (access_strcmp(p, name) == 0) /* it's a user name */
            return 1;
        p = strtok(NULL, ",");
    }

    return 0;
}

static void setextmap( ext, type, creator, user)
char		*ext, *type, *creator;
int			user;
{
    struct extmap	*em;
    int                 cnt;

    if (Extmap == NULL) {
        if (( Extmap = calloc(1, sizeof( struct extmap ))) == NULL ) {
            LOG(log_error, logtype_afpd, "setextmap: calloc: %s", strerror(errno) );
            return;
        }
    }
    ext++;
    for ( em = Extmap, cnt = 0; em->em_ext; em++, cnt++) {
        if ( (strdiacasecmp( em->em_ext, ext )) == 0 ) {
            break;
        }
    }

    if ( em->em_ext == NULL ) {
        if (!(Extmap  = realloc( Extmap, sizeof( struct extmap ) * (cnt +2))) ) {
            LOG(log_error, logtype_afpd, "setextmap: realloc: %s", strerror(errno) );
            return;
        }
        (Extmap +cnt +1)->em_ext = NULL;
        em = Extmap +cnt;
    } else if ( !user ) {
        return;
    }
    if (em->em_ext)
    	free(em->em_ext);

    if (!(em->em_ext = strdup(  ext))) {
        LOG(log_error, logtype_afpd, "setextmap: strdup: %s", strerror(errno) );
        return;
    }

    if ( *type == '\0' ) {
        memcpy(em->em_type, "????", sizeof( em->em_type ));
    } else {
        memcpy(em->em_type, type, sizeof( em->em_type ));
    }
    if ( *creator == '\0' ) {
        memcpy(em->em_creator, "UNIX", sizeof( em->em_creator ));
    } else {
        memcpy(em->em_creator, creator, sizeof( em->em_creator ));
    }
}

/* -------------------------- */
static int extmap_cmp(const void *map1, const void *map2)
{
    const struct extmap *em1 = map1;
    const struct extmap *em2 = map2;
    return strdiacasecmp(em1->em_ext, em2->em_ext);
}

static void sortextmap( void)
{
    struct extmap	*em;

    Extmap_cnt = 0;
    if ((em = Extmap) == NULL) {
        return;
    }
    while (em->em_ext) {
        em++;
        Extmap_cnt++;
    }
    if (Extmap_cnt) {
        qsort(Extmap, Extmap_cnt, sizeof(struct extmap), extmap_cmp);
        if (*Extmap->em_ext == 0) {
            /* the first line is really "." the default entry, 
             * we remove the leading '.' in setextmap
            */
            Defextmap = Extmap;
        }
    }
}

/* ----------------------
*/
static void free_extmap( void)
{
    struct extmap	*em;

    if (Extmap) {
        for ( em = Extmap; em->em_ext; em++) {
             free (em->em_ext);
        }
        free(Extmap);
        Extmap = NULL;
        Defextmap = Extmap;
        Extmap_cnt = 0;
    }
}

/* ----------------------
*/
static int volfile_changed(struct afp_volume_name *p) 
{
    struct stat      st;
    char *name;
    
    if (p->full_name) 
    	name = p->full_name;
    else
        name = p->name;
        
    if (!stat( name, &st) && st.st_mtime > p->mtime) {
        p->mtime = st.st_mtime;
        return 1;
    }
    return 0;
}

/* ----------------------
 * Read a volume configuration file and add the volumes contained within to
 * the global volume list.  If p2 is non-NULL, the file that is opened is
 * p1/p2
 * 
 * Lines that begin with # and blank lines are ignored.
 * Volume lines are of the form:
 *		<unix path> [<volume name>] [allow:<user>,<@group>,...] \
 *                           [codepage:<file>] [casefold:<num>]
 *		<extension> TYPE [CREATOR]
 */
static int readvolfile(obj, p1, p2, user, pwent)
AFPObj      *obj;
struct afp_volume_name 	*p1;
char        *p2;
int		user;
struct passwd *pwent;
{
    FILE		*fp;
    char		path[ MAXPATHLEN + 1], tmp[ MAXPATHLEN + 1],
    volname[ AFPVOL_NAMELEN + 1 ], buf[ BUFSIZ ],
    type[ 5 ], creator[ 5 ];
    char		*u, *p;
    struct passwd	*pw;
    struct vol_option   options[VOLOPT_NUM], save_options[VOLOPT_NUM];
    int                 i;
    struct stat         st;
    int                 fd;

    if (!p1->name)
        return -1;
    p1->mtime = 0;
    strcpy( path, p1->name );
    if ( p2 != NULL ) {
        strcat( path, "/" );
        strcat( path, p2 );
        if (p1->full_name) {
            free(p1->full_name);
        }
        p1->full_name = strdup(path);
    }

    if (NULL == ( fp = fopen( path, "r" )) ) {
        return( -1 );
    }
    fd = fileno(fp);
    if (fd != -1 && !fstat( fd, &st) ) {
        p1->mtime = st.st_mtime;
    }

    memset(save_options, 0, sizeof(save_options));
    while ( myfgets( buf, sizeof( buf ), fp ) != NULL ) {
        initline( strlen( buf ), buf );
        parseline( sizeof( path ) - 1, path );
        switch ( *path ) {
        case '\0' :
        case '#' :
            continue;

        case ':':
            /* change the default options for this file */
            if (strncmp(path, VOLOPT_DEFAULT, VOLOPT_DEFAULT_LEN) == 0) {
                *tmp = '\0';
                for (i = 0; i < VOLOPT_NUM; i++) {
                    if (parseline( sizeof( path ) - VOLOPT_DEFAULT_LEN - 1,
                                   path + VOLOPT_DEFAULT_LEN) < 0)
                        break;
                    volset(save_options, NULL, tmp, sizeof(tmp) - 1,
                           path + VOLOPT_DEFAULT_LEN);
                }
            }
            break;

        case '~' :
            if (( p = strchr( path, '/' )) != NULL ) {
                *p++ = '\0';
            }
            u = path;
            u++;
            if ( *u == '\0' ) {
                u = obj->username;
            }
            if ( u == NULL || *u == '\0' || ( pw = getpwnam( u )) == NULL ) {
                continue;
            }
            strcpy( tmp, pw->pw_dir );
            if ( p != NULL && *p != '\0' ) {
                strcat( tmp, "/" );
                strcat( tmp, p );
            }
	    /* Tag a user's home directory with their umask.  Note, this will
	     * be overwritten if the user actually specifies a umask: option
	     * for a '~' volume. */
	    save_options[VOLOPT_UMASK].i_value = obj->options.save_mask;
            /* fall through */

        case '/' :
            /* send path through variable substitution */
            if (*path != '~') /* need to copy path to tmp */
                strcpy(tmp, path);
            if (!pwent)
                pwent = getpwnam(obj->username);
            volxlate(obj, path, sizeof(path) - 1, tmp, pwent, NULL, NULL);

            /* this is sort of braindead. basically, i want to be
             * able to specify things in any order, but i don't want to 
             * re-write everything. 
             *
             * currently we have options: 
             *   volname
             *   codepage:x
             *   casefold:x
             *   allow:x,y,@z
             *   deny:x,y,@z
             *   rwlist:x,y,@z
             *   rolist:x,y,@z
             *   options:prodos,crlf,noadouble,ro...
             *   dbpath:x
             *   password:x
             *   preexec:x
             *
             *   namemask:x,y,!z  (not implemented yet)
             */
            memcpy(options, save_options, sizeof(options));
            *volname = '\0';

            /* read in up to VOLOP_NUM possible options */
            for (i = 0; i < VOLOPT_NUM; i++) {
                if (parseline( sizeof( tmp ) - 1, tmp ) < 0)
                    break;

                volset(options, save_options, volname, sizeof(volname) - 1, tmp);
            }

            /* check allow/deny lists:
               allow -> either no list (-1), or in list (1)
               deny -> either no list (-1), or not in list (0) */
            if (accessvol(options[VOLOPT_ALLOW].c_value, obj->username) &&
                    (accessvol(options[VOLOPT_DENY].c_value, obj->username) < 1)) {

                /* handle read-only behaviour. semantics:
                 * 1) neither the rolist nor the rwlist exist -> rw
                 * 2) rolist exists -> ro if user is in it.
                 * 3) rwlist exists -> ro unless user is in it. */
                if (((options[VOLOPT_FLAGS].i_value & AFPVOL_RO) == 0) &&
                        ((accessvol(options[VOLOPT_ROLIST].c_value,
                                    obj->username) == 1) ||
                         !accessvol(options[VOLOPT_RWLIST].c_value,
                                    obj->username)))
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_RO;

                /* do variable substitution for volname */
                volxlate(obj, tmp, sizeof(tmp) - 1, volname, pwent, path, NULL);
                creatvol(obj, pwent, path, tmp, options, p2 != NULL);
            }
            volfree(options, save_options);
            break;

        case '.' :
            parseline( sizeof( type ) - 1, type );
            parseline( sizeof( creator ) - 1, creator );
            setextmap( path, type, creator, user);
            break;

        default :
            break;
        }
    }
    volfree(save_options, NULL);
    sortextmap();
    if ( fclose( fp ) != 0 ) {
        LOG(log_error, logtype_afpd, "readvolfile: fclose: %s", strerror(errno) );
    }
    p1->loaded = 1;
    return( 0 );
}

/* ------------------------------- */
static void volume_free(struct vol *vol)
{
    free(vol->v_name);
    vol->v_name = NULL;
    free(vol->v_path);
    free(vol->v_password);
    free(vol->v_veto);
    free(vol->v_volcodepage);
    free(vol->v_maccodepage);
    free(vol->v_cnidscheme);
    free(vol->v_dbpath);
    free(vol->v_gvs);
#ifdef FORCE_UIDGID
    free(vol->v_forceuid);
    free(vol->v_forcegid);
#endif /* FORCE_UIDGID */
}

/* ------------------------------- */
static void free_volumes(void )
{
    struct vol	*vol;
    struct vol  *nvol, *ovol;

    for ( vol = Volumes; vol; vol = vol->v_next ) {
        if (( vol->v_flags & AFPVOL_OPEN ) ) {
            vol->v_deleted = 1;
            continue;
        }
        volume_free(vol);
    }

    for ( vol = Volumes, ovol = NULL; vol; vol = nvol) {
        nvol = vol->v_next;

        if (vol->v_name == NULL) {
           if (Volumes == vol) {
               Volumes = nvol;
               ovol = Volumes;
           }
           else {
              ovol->v_next = nvol;
           }
           free(vol);
        }
        else {
           ovol = vol;
        }
    }
}

/* ------------------------------- */
static void volume_unlink(struct vol *volume)
{
struct vol *vol, *ovol, *nvol;

    if (volume == Volumes) {
        Volumes = Volumes->v_next;
        return;
    }
    for ( vol = Volumes->v_next, ovol = Volumes; vol; vol = nvol) {
        nvol = vol->v_next;

        if (vol == volume) {
            ovol->v_next = nvol;
            break;
        }
        else {
           ovol = vol;
        }
    }
}

static int getvolspace( vol, bfree, btotal, xbfree, xbtotal, bsize )
struct vol	*vol;
u_int32_t	*bfree, *btotal, *bsize;
VolSpace    *xbfree, *xbtotal;
{
    int	        spaceflag, rc;
    u_int32_t   maxsize;
#ifndef NO_QUOTA_SUPPORT
    VolSpace	qfree, qtotal;
#endif

    spaceflag = AFPVOL_GVSMASK & vol->v_flags;
    /* report up to 2GB if afp version is < 2.2 (4GB if not) */
    maxsize = (vol->v_flags & AFPVOL_A2VOL) ? 0x01fffe00 :
              (((afp_version < 22) || (vol->v_flags & AFPVOL_LIMITSIZE))
               ? 0x7fffffffL : 0xffffffffL);

#ifdef AFS
    if ( spaceflag == AFPVOL_NONE || spaceflag == AFPVOL_AFSGVS ) {
        if ( afs_getvolspace( vol, xbfree, xbtotal, bsize ) == AFP_OK ) {
            vol->v_flags = ( ~AFPVOL_GVSMASK & vol->v_flags ) | AFPVOL_AFSGVS;
            goto getvolspace_done;
        }
    }
#endif

    if (( rc = ustatfs_getvolspace( vol, xbfree, xbtotal,
                                    bsize)) != AFP_OK ) {
        return( rc );
    }

#define min(a,b)	((a)<(b)?(a):(b))
#ifndef NO_QUOTA_SUPPORT
    if ( spaceflag == AFPVOL_NONE || spaceflag == AFPVOL_UQUOTA ) {
        if ( uquota_getvolspace( vol, &qfree, &qtotal, *bsize ) == AFP_OK ) {
            vol->v_flags = ( ~AFPVOL_GVSMASK & vol->v_flags ) | AFPVOL_UQUOTA;
            *xbfree = min(*xbfree, qfree);
            *xbtotal = min( *xbtotal, qtotal);
            goto getvolspace_done;
        }
    }
#endif
    vol->v_flags = ( ~AFPVOL_GVSMASK & vol->v_flags ) | AFPVOL_USTATFS;

getvolspace_done:
    *bfree = min( *xbfree, maxsize);
    *btotal = min( *xbtotal, maxsize);
    return( AFP_OK );
}

/* ----------------------- 
 * set volume creation date
 * avoid duplicate, well at least it tries
*/
static void vol_setdate(u_int16_t id, struct adouble *adp, time_t date)
{
    struct vol	*volume;
    struct vol	*vol = Volumes;

    for ( volume = Volumes; volume; volume = volume->v_next ) {
        if (volume->v_vid == id) {
            vol = volume;
        }
        else if ((time_t)(AD_DATE_FROM_UNIX(date)) == volume->v_ctime) {
            date = date+1;
            volume = Volumes; /* restart */
        }
    }
    vol->v_ctime = AD_DATE_FROM_UNIX(date);
    ad_setdate(adp, AD_DATE_CREATE | AD_DATE_UNIX, date);
}

/* ----------------------- */
static int getvolparams( bitmap, vol, st, buf, buflen )
u_int16_t	bitmap;
struct vol	*vol;
struct stat	*st;
char	*buf;
int		*buflen;
{
    struct adouble	ad;
    int			bit = 0, isad = 1;
    u_int32_t		aint;
    u_short		ashort;
    u_int32_t		bfree, btotal, bsize;
    VolSpace            xbfree, xbtotal; /* extended bytes */
    char		*data, *nameoff = NULL;
    char                *slash;

    /* courtesy of jallison@whistle.com:
     * For MacOS8.x support we need to create the
     * .Parent file here if it doesn't exist. */

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    if ( ad_open_metadata( vol->v_path, vol_noadouble(vol) | ADFLAGS_DIR, O_CREAT, &ad) < 0 ) {
        isad = 0;
        vol->v_ctime = AD_DATE_FROM_UNIX(st->st_mtime);

    } else if (ad_get_MD_flags( &ad ) & O_CREAT) {
        slash = strrchr( vol->v_path, '/' );
        if(slash)
            slash++;
        else
            slash = vol->v_path;
        if (ad_getentryoff(&ad, ADEID_NAME)) {
            ad_setentrylen( &ad, ADEID_NAME, strlen( slash ));
            memcpy(ad_entry( &ad, ADEID_NAME ), slash,
               ad_getentrylen( &ad, ADEID_NAME ));
        }
        vol_setdate(vol->v_vid, &ad, st->st_mtime);
        ad_flush(&ad);
    }
    else {
        if (ad_getdate(&ad, AD_DATE_CREATE, &aint) < 0)
            vol->v_ctime = AD_DATE_FROM_UNIX(st->st_mtime);
        else 
            vol->v_ctime = aint;
    }

    if (( bitmap & ( (1<<VOLPBIT_BFREE)|(1<<VOLPBIT_BTOTAL) |
                     (1<<VOLPBIT_XBFREE)|(1<<VOLPBIT_XBTOTAL) |
                     (1<<VOLPBIT_BSIZE)) ) != 0 ) {
        if ( getvolspace( vol, &bfree, &btotal, &xbfree, &xbtotal,
                          &bsize) < 0 ) {
            if ( isad ) {
                ad_close( &ad, ADFLAGS_HF );
            }
            return( AFPERR_PARAM );
        }
    }

    data = buf;
    while ( bitmap != 0 ) {
        while (( bitmap & 1 ) == 0 ) {
            bitmap = bitmap>>1;
            bit++;
        }

        switch ( bit ) {
        case VOLPBIT_ATTR :
            ashort = 0;
            if (0 == (vol->v_flags & AFPVOL_NOFILEID) && vol->v_cdb != NULL &&
                           (vol->v_cdb->flags & CNID_FLAG_PERSISTENT)) {
                ashort = VOLPBIT_ATTR_FILEID;
            }
            /* check for read-only.
             * NOTE: we don't actually set the read-only flag unless
             *       it's passed in that way as it's possible to mount
             *       a read-write filesystem under a read-only one. */
            if ((vol->v_flags & AFPVOL_RO) ||
                    ((utime(vol->v_path, NULL) < 0) && (errno == EROFS))) {
                ashort |= VOLPBIT_ATTR_RO;
            }
            ashort |= VOLPBIT_ATTR_CATSEARCH;
            if (afp_version >= 30) {
                ashort |= VOLPBIT_ATTR_UTF8;
	        if (vol->v_flags & AFPVOL_UNIX_PRIV)
		    ashort |= VOLPBIT_ATTR_UNIXPRIV;
            }
            ashort = htons(ashort);
            memcpy(data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case VOLPBIT_SIG :
            ashort = htons( AFPVOLSIG_DEFAULT );
            memcpy(data, &ashort, sizeof( ashort ));
            data += sizeof( ashort );
            break;

        case VOLPBIT_CDATE :
            aint = vol->v_ctime;
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case VOLPBIT_MDATE :
            if ( st->st_mtime > vol->v_mtime ) {
                vol->v_mtime = st->st_mtime;
            }
            aint = AD_DATE_FROM_UNIX(vol->v_mtime);
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case VOLPBIT_BDATE :
            if (!isad ||  (ad_getdate(&ad, AD_DATE_BACKUP, &aint) < 0))
                aint = AD_DATE_START;
            memcpy(data, &aint, sizeof( aint ));
            data += sizeof( aint );
            break;

        case VOLPBIT_VID :
            memcpy(data, &vol->v_vid, sizeof( vol->v_vid ));
            data += sizeof( vol->v_vid );
            break;

        case VOLPBIT_BFREE :
            bfree = htonl( bfree );
            memcpy(data, &bfree, sizeof( bfree ));
            data += sizeof( bfree );
            break;

        case VOLPBIT_BTOTAL :
            btotal = htonl( btotal );
            memcpy(data, &btotal, sizeof( btotal ));
            data += sizeof( btotal );
            break;

#ifndef NO_LARGE_VOL_SUPPORT
        case VOLPBIT_XBFREE :
            xbfree = hton64( xbfree );
#if defined(__GNUC__) && defined(HAVE_GCC_MEMCPY_BUG)
            bcopy(&xbfree, data, sizeof(xbfree));
#else /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
            memcpy(data, &xbfree, sizeof( xbfree ));
#endif /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
            data += sizeof( xbfree );
            break;

        case VOLPBIT_XBTOTAL :
            xbtotal = hton64( xbtotal );
#if defined(__GNUC__) && defined(HAVE_GCC_MEMCPY_BUG)
            bcopy(&xbtotal, data, sizeof(xbtotal));
#else /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
            memcpy(data, &xbtotal, sizeof( xbtotal ));
#endif /* __GNUC__ && HAVE_GCC_MEMCPY_BUG */
            data += sizeof( xbfree );
            break;
#endif /* ! NO_LARGE_VOL_SUPPORT */

        case VOLPBIT_NAME :
            nameoff = data;
            data += sizeof( u_int16_t );
            break;

        case VOLPBIT_BSIZE:  /* block size */
            bsize = htonl(bsize);
            memcpy(data, &bsize, sizeof(bsize));
            data += sizeof(bsize);
            break;

        default :
            if ( isad ) {
                ad_close( &ad, ADFLAGS_HF );
            }
            return( AFPERR_BITMAP );
        }
        bitmap = bitmap>>1;
        bit++;
    }
    if ( nameoff ) {
        ashort = htons( data - buf );
        memcpy(nameoff, &ashort, sizeof( ashort ));
        /* name is always in mac charset, FIXME mangle if length > 27 char */
	aint = ucs2_to_charset( vol->v_maccharset, vol->v_name, data+1, 255);
	if ( aint <= 0 ) {
	    *buflen = 0;
            return AFPERR_MISC;
        }
		
        *data++ = aint;
        data += aint;
    }
    if ( isad ) {
        ad_close_metadata( &ad);
    }
    *buflen = data - buf;
    return( AFP_OK );
}

/* ------------------------- */
static int stat_vol(u_int16_t bitmap, struct vol *vol, char *rbuf, int *rbuflen)
{
    struct stat	st;
    int		buflen, ret;

    if ( stat( vol->v_path, &st ) < 0 ) {
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }
    /* save the volume device number */
    vol->v_dev = st.st_dev;

    buflen = *rbuflen - sizeof( bitmap );
    if (( ret = getvolparams( bitmap, vol, &st,
                              rbuf + sizeof( bitmap ), &buflen )) != AFP_OK ) {
        *rbuflen = 0;
        return( ret );
    }
    *rbuflen = buflen + sizeof( bitmap );
    bitmap = htons( bitmap );
    memcpy(rbuf, &bitmap, sizeof( bitmap ));
    return( AFP_OK );

}

/* ------------------------------- */
void load_volumes(AFPObj *obj)
{
    struct passwd	*pwent;

    if (Volumes) {
        int changed = 0;
        
        /* check files date */
        if (obj->options.defaultvol.loaded) {
            changed = volfile_changed(&obj->options.defaultvol);
        }
        if (obj->options.systemvol.loaded) {
            changed |= volfile_changed(&obj->options.systemvol);
        }
        if (obj->options.uservol.loaded) {
            changed |= volfile_changed(&obj->options.uservol);
        }
        if (!changed)
            return;
        
        free_extmap();
        free_volumes();
    }
    
    pwent = getpwnam(obj->username);
    if ( (obj->options.flags & OPTION_USERVOLFIRST) == 0 ) {
        readvolfile(obj, &obj->options.systemvol, NULL, 0, pwent);
    }

    if ((*obj->username == '\0') || (obj->options.flags & OPTION_NOUSERVOL)) {
        readvolfile(obj, &obj->options.defaultvol, NULL, 1, pwent);
    } else if (pwent) {
        /*
        * Read user's AppleVolumes or .AppleVolumes file
        * If neither are readable, read the default volumes file. if
        * that doesn't work, create a user share.
        */
        if (obj->options.uservol.name) {
            free(obj->options.uservol.name);
        }
        obj->options.uservol.name = strdup(pwent->pw_dir);
        if ( readvolfile(obj, &obj->options.uservol,    "AppleVolumes", 1, pwent) < 0 &&
                readvolfile(obj, &obj->options.uservol, ".AppleVolumes", 1, pwent) < 0 &&
                readvolfile(obj, &obj->options.uservol, "applevolumes", 1, pwent) < 0 &&
                readvolfile(obj, &obj->options.uservol, ".applevolumes", 1, pwent) < 0 &&
                obj->options.defaultvol.name != NULL ) {
            if (readvolfile(obj, &obj->options.defaultvol, NULL, 1, pwent) < 0)
                creatvol(obj, pwent, pwent->pw_dir, NULL, NULL, 1);
        }
    }
    if ( obj->options.flags & OPTION_USERVOLFIRST ) {
        readvolfile(obj, &obj->options.systemvol, NULL, 0, pwent );
    }
    
    if ( obj->options.closevol ) {
        struct vol *vol;
        
        for ( vol = Volumes; vol; vol = vol->v_next ) {
            if (vol->v_deleted && !vol->v_new ) {
                of_closevol(vol);
                deletevol(vol);
                vol = Volumes;
            }
        }
    }
}

/* ------------------------------- */
int afp_getsrvrparms(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj;
char	*ibuf _U_, *rbuf;
int 	ibuflen _U_, *rbuflen;
{
    struct timeval	tv;
    struct stat		st;
    struct vol		*volume;
    char	*data;
    char		*namebuf;
    int			vcnt;
    size_t		len;

    load_volumes(obj);

    data = rbuf + 5;
    for ( vcnt = 0, volume = Volumes; volume; volume = volume->v_next ) {
        if (!(volume->v_flags & AFPVOL_NOSTAT)) {
            struct maccess ma;

            if ( stat( volume->v_path, &st ) < 0 ) {
                LOG(log_info, logtype_afpd, "afp_getsrvrparms(%s): stat: %s",
                        volume->v_path, strerror(errno) );
                continue;		/* can't access directory */
            }
            if (!S_ISDIR(st.st_mode)) {
                continue;		/* not a dir */
            }
            accessmode(volume->v_path, &ma, NULL, &st);
            if ((ma.ma_user & (AR_UREAD | AR_USEARCH)) != (AR_UREAD | AR_USEARCH)) {
                continue;   /* no r-x access */
            }
        }
        if (volume->v_hide) {
            continue;		/* config file changed but the volume was mounted */
        }
	len = ucs2_to_charset_allocate((utf8_encoding()?CH_UTF8_MAC:obj->options.maccharset),
					&namebuf, volume->v_name);
	if (len == (size_t)-1)
		continue;

        /* set password bit if there's a volume password */
        *data = (volume->v_password) ? AFPSRVR_PASSWD : 0;

        /* Apple 2 clients running ProDOS-8 expect one volume to have
           bit 0 of this byte set.  They will not recognize anything
           on the server unless this is the case.  I have not
           completely worked this out, but it's related to booting
           from the server.  Support for that function is a ways
           off.. <shirsch@ibm.net> */
        *data |= (volume->v_flags & AFPVOL_A2VOL) ? AFPSRVR_CONFIGINFO : 0;
        *data++ |= 0; /* UNIX PRIVS BIT ..., OSX doesn't seem to use it, so we don't either */
        *data++ = len;
        memcpy(data, namebuf, len );
        data += len;
	free(namebuf);
        vcnt++;
    }

    *rbuflen = data - rbuf;
    data = rbuf;
    if ( gettimeofday( &tv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_getsrvrparms(%s): gettimeofday: %s", volume->v_path, strerror(errno) );
        *rbuflen = 0;
        return AFPERR_PARAM;
    }
    tv.tv_sec = AD_DATE_FROM_UNIX(tv.tv_sec);
    memcpy(data, &tv.tv_sec, sizeof( u_int32_t));
    data += sizeof( u_int32_t);
    *data = vcnt;
    return( AFP_OK );
}

/* ------------------------- 
 * we are the user here
*/
int afp_openvol(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj;
char	*ibuf, *rbuf;
int	ibuflen _U_, *rbuflen;
{
    struct stat	st;
    char	*volname;
    char        *p;
    struct vol	*volume;
    struct dir	*dir;
    int		len, ret;
    size_t	namelen;
    u_int16_t	bitmap;
    char        path[ MAXPATHLEN + 1];
    char        *vol_uname;
    char        *vol_mname;

    ibuf += 2;
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );
    if (( bitmap & (1<<VOLPBIT_VID)) == 0 ) {
        *rbuflen = 0;
        return AFPERR_BITMAP;
    }

    len = (unsigned char)*ibuf++;
    volname = obj->oldtmp;
    namelen = convert_string( (utf8_encoding()?CH_UTF8_MAC:obj->options.maccharset), CH_UCS2,
                              ibuf, len, volname, sizeof(obj->oldtmp));
    if ( namelen <= 0){
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    ibuf += len;
    if ((len + 1) & 1) /* pad to an even boundary */
        ibuf++;

    load_volumes(obj);

    for ( volume = Volumes; volume; volume = volume->v_next ) {
        if ( strcasecmp_w( (ucs2_t*) volname, volume->v_name ) == 0 ) {
            break;
        }
    }

    if ( volume == NULL ) {
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    /* check for a volume password */
    if (volume->v_password && strncmp(ibuf, volume->v_password, VOLPASSLEN)) {
        *rbuflen = 0;
        return AFPERR_ACCESS;
    }

    if (( volume->v_flags & AFPVOL_OPEN  ) ) {
        /* the volume is already open */
#ifdef FORCE_UIDGID
        set_uidgid ( volume );
#endif
        return stat_vol(bitmap, volume, rbuf, rbuflen);
    }

    /* initialize volume variables
     * FIXME file size
    */
    if (afp_version >= 30) {
        volume->max_filename = 255;
    }
    else {
        volume->max_filename = MACFILELEN;
    }

    volume->v_dir = volume->v_root = NULL;
    volume->v_hash = NULL;

    volume->v_flags |= AFPVOL_OPEN;
    volume->v_cdb = NULL;  

    if (volume->v_root_preexec) {
	if ((ret = afprun(1, volume->v_root_preexec, NULL)) && volume->v_root_preexec_close) {
            LOG(log_error, logtype_afpd, "afp_openvol(%s): root preexec : %d", volume->v_path, ret );
            ret = AFPERR_MISC;
            goto openvol_err;
	}
    }

#ifdef FORCE_UIDGID
    set_uidgid ( volume );
#endif

    if (volume->v_preexec) {
	if ((ret = afprun(0, volume->v_preexec, NULL)) && volume->v_preexec_close) {
            LOG(log_error, logtype_afpd, "afp_openvol(%s): preexec : %d", volume->v_path, ret );
            ret = AFPERR_MISC;
            goto openvol_err;
	}
    }

    if ( stat( volume->v_path, &st ) < 0 ) {
        ret = AFPERR_PARAM;
        goto openvol_err;
    }

    if ( chdir( volume->v_path ) < 0 ) {
        ret = AFPERR_PARAM;
        goto openvol_err;
    }

    len = convert_string_allocate( CH_UCS2, (utf8_encoding()?CH_UTF8_MAC:obj->options.maccharset),
                              	       volume->v_name, namelen, &vol_mname);
    if ( !vol_mname || len <= 0) {
        ret = AFPERR_MISC;
        goto openvol_err;
    }
    
    if ( NULL == getcwd(path, MAXPATHLEN)) {
        /* shouldn't be fatal but it will fail later */
        LOG(log_error, logtype_afpd, "afp_openvol(%s): volume pathlen too long", volume->v_path);
        ret = AFPERR_MISC;
        goto openvol_err;
    }        
    
    if ((vol_uname = strrchr(path, '/')) == NULL)
         vol_uname = path;
    else if (*(vol_uname + 1) != '\0')
         vol_uname++;
	
    if ((dir = dirnew(vol_mname, vol_uname) ) == NULL) {
	free(vol_mname);
        LOG(log_error, logtype_afpd, "afp_openvol(%s): malloc: %s", volume->v_path, strerror(errno) );
        ret = AFPERR_MISC;
        goto openvol_err;
    }
    free(vol_mname);

    dir->d_did = DIRDID_ROOT;
    dir->d_color = DIRTREE_COLOR_BLACK; /* root node is black */
    dir->d_m_name_ucs2 = strdup_w(volume->v_name);
    volume->v_dir = volume->v_root = dir;
    volume->v_hash = dirhash();

    curdir = volume->v_dir;
    if (volume->v_cnidscheme == NULL) {
        volume->v_cnidscheme = strdup(DEFAULT_CNID_SCHEME);
        LOG(log_warning, logtype_afpd, "Warning: No CNID scheme for volume %s. Using default.",
               volume->v_path);
    }
    if (volume->v_dbpath)
        volume->v_cdb = cnid_open (volume->v_dbpath, volume->v_umask, volume->v_cnidscheme, (volume->v_flags & AFPVOL_NODEV));
    else
        volume->v_cdb = cnid_open (volume->v_path, volume->v_umask, volume->v_cnidscheme, (volume->v_flags & AFPVOL_NODEV));
    if (volume->v_cdb == NULL) {
        LOG(log_error, logtype_afpd, "Fatal error: cannot open CNID or invalid CNID backend for %s: %s", 
	    volume->v_path, volume->v_cnidscheme);
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    /* Codepages */

    if (!volume->v_volcodepage)
	volume->v_volcodepage = strdup("UTF8");

    if ( (charset_t) -1 == ( volume->v_volcharset = add_charset(volume->v_volcodepage)) ) {
	LOG (log_error, logtype_afpd, "Setting codepage %s as volume codepage failed", volume->v_volcodepage);
	ret = AFPERR_MISC;
	goto openvol_err;
    }

    if ( NULL == ( volume->v_vol = find_charset_functions(volume->v_volcodepage)) || volume->v_vol->flags & CHARSET_ICONV ) {
	LOG (log_warning, logtype_afpd, "WARNING: volume encoding %s is *not* supported by netatalk, expect problems !!!!", volume->v_volcodepage);
    }	

    if (!volume->v_maccodepage)
	volume->v_maccodepage = strdup(obj->options.maccodepage);

    if ( (charset_t) -1 == ( volume->v_maccharset = add_charset(volume->v_maccodepage)) ) {
	LOG (log_error, logtype_afpd, "Setting codepage %s as mac codepage failed", volume->v_maccodepage);
	ret = AFPERR_MISC;
	goto openvol_err;
    }

    if ( NULL == ( volume->v_mac = find_charset_functions(volume->v_maccodepage)) || ! (volume->v_mac->flags & CHARSET_CLIENT) ) {
	LOG (log_error, logtype_afpd, "Fatal error: mac charset %s not supported", volume->v_maccodepage);
	ret = AFPERR_MISC;
	goto openvol_err;
    }	

    ret  = stat_vol(bitmap, volume, rbuf, rbuflen);
    if (ret == AFP_OK) {

        if (!(volume->v_flags & AFPVOL_RO)) {
            handle_special_folders( volume );
            savevoloptions( volume);
        }

        /*
         * If you mount a volume twice, the second time the trash appears on
         * the desk-top.  That's because the Mac remembers the DID for the
         * trash (even for volumes in different zones, on different servers).
         * Just so this works better, we prime the DID cache with the trash,
         * fixing the trash at DID 17.
         * FIXME (RL): should it be done inside a CNID backend ? (always returning Trash DID when asked) ?
         */
        if ((volume->v_cdb->flags & CNID_FLAG_PERSISTENT)) {

            /* FIXME find db time stamp */
            if (cnid_getstamp(volume->v_cdb, volume->v_stamp, sizeof(volume->v_stamp)) < 0) {
		LOG (log_error, logtype_afpd, 
		      "afp_openvol(%s): Fatal error: Unable to get stamp value from CNID backend",
		      volume->v_path);
		ret = AFPERR_MISC;
		goto openvol_err;
	    }
	}
	else {
            p = Trash;
            cname( volume, volume->v_dir, &p );
        }
        return( AFP_OK );
    }

openvol_err:
    if (volume->v_dir) {
    	hash_free( volume->v_hash);
        dirfree( volume->v_dir );
        volume->v_dir = volume->v_root = NULL;
    }

    volume->v_flags &= ~AFPVOL_OPEN;
    if (volume->v_cdb != NULL) {
        cnid_close(volume->v_cdb);
        volume->v_cdb = NULL;
    }
    *rbuflen = 0;
    return ret;
}

/* ------------------------- */
static void closevol(struct vol	*vol)
{
    if (!vol)
        return;

    hash_free( vol->v_hash);
    dirfree( vol->v_root );
    vol->v_dir = NULL;
    if (vol->v_cdb != NULL) {
        cnid_close(vol->v_cdb);
        vol->v_cdb = NULL;
    }

    if (vol->v_postexec) {
	afprun(0, vol->v_postexec, NULL);
    }
    if (vol->v_root_postexec) {
	afprun(1, vol->v_root_postexec, NULL);
    }
}

/* ------------------------- */
void close_all_vol(void)
{
    struct vol	*ovol;
    curdir = NULL;
    for ( ovol = Volumes; ovol; ovol = ovol->v_next ) {
        if ( (ovol->v_flags & AFPVOL_OPEN) ) {
            ovol->v_flags &= ~AFPVOL_OPEN;
            closevol(ovol);
        }
    }
}

/* ------------------------- */
static void deletevol(struct vol *vol)
{
    struct vol	*ovol;

    vol->v_flags &= ~AFPVOL_OPEN;
    for ( ovol = Volumes; ovol; ovol = ovol->v_next ) {
        if ( (ovol->v_flags & AFPVOL_OPEN) ) {
            break;
        }
    }
    if ( ovol != NULL ) {
        /* Even if chdir fails, we can't say afp_closevol fails. */
        if ( chdir( ovol->v_path ) == 0 ) {
            curdir = ovol->v_dir;
        }
    }

    closevol(vol);
    if (vol->v_deleted) {
	showvol(vol->v_name);
	volume_free(vol);
	volume_unlink(vol);
	free(vol);
    }
}

/* ------------------------- */
int afp_closevol(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf, *rbuf _U_;
int	ibuflen _U_, *rbuflen;
{
    struct vol	*vol;
    u_int16_t	vid;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof( vid ));
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    deletevol(vol);

    return( AFP_OK );
}

/* ------------------------- */
struct vol *getvolbyvid(const u_int16_t vid )
{
    struct vol	*vol;

    for ( vol = Volumes; vol; vol = vol->v_next ) {
        if ( vid == vol->v_vid ) {
            break;
        }
    }
    if ( vol == NULL || ( vol->v_flags & AFPVOL_OPEN ) == 0 ) {
        return( NULL );
    }

#ifdef FORCE_UIDGID
    set_uidgid ( vol );
#endif /* FORCE_UIDGID */

    return( vol );
}

/* ------------------------ */
static int ext_cmp_key(const void *key, const void *obj)
{
    const char          *p = key;
    const struct extmap *em = obj;
    return strdiacasecmp(p, em->em_ext);
}
struct extmap *getextmap(const char *path)
{
    char	  *p;
    struct extmap *em;

    if (NULL == ( p = strrchr( path, '.' )) ) {
        return( Defextmap );
    }
    p++;
    if (!*p || !Extmap_cnt) {
        return( Defextmap );
    }
    em = bsearch(p, Extmap, Extmap_cnt, sizeof(struct extmap), ext_cmp_key);
    if (em) {
        return( em );
    } else {
        return( Defextmap );
    }
}

/* ------------------------- */
struct extmap *getdefextmap(void)
{
    return( Defextmap );
}

/* --------------------------
   poll if a volume is changed by other processes.
*/
int  pollvoltime(obj)
AFPObj *obj;
{
    struct vol	     *vol;
    struct timeval   tv;
    struct stat      st;
    
    if (!(afp_version > 21 && obj->options.server_notif)) 
         return 0;

    if ( gettimeofday( &tv, 0 ) < 0 ) 
         return 0;

    for ( vol = Volumes; vol; vol = vol->v_next ) {
        if ( (vol->v_flags & AFPVOL_OPEN)  && vol->v_mtime + 30 < tv.tv_sec) {
            if ( !stat( vol->v_path, &st ) && vol->v_mtime != st.st_mtime ) {
                vol->v_mtime = st.st_mtime;
                if (!obj->attention(obj->handle, AFPATTN_NOTIFY | AFPATTN_VOLCHANGED))
                    return -1;
                return 1;
            }
        }
    }
    return 0;
}

/* ------------------------- */
void setvoltime(obj, vol )
AFPObj *obj;
struct vol	*vol;
{
    struct timeval	tv;

    /* just looking at vol->v_mtime is broken seriously since updates
     * from other users afpd processes never are seen.
     * This is not the most elegant solution (a shared memory between
     * the afpd processes would come closer)
     * [RS] */

    if ( gettimeofday( &tv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "setvoltime(%s): gettimeofday: %s", vol->v_path, strerror(errno) );
        return;
    }
    if( utime( vol->v_path, NULL ) < 0 ) {
        /* write of time failed ... probably a read only filesys,
         * where no other users can interfere, so there's no issue here
         */
    }

    /* a little granularity */
    if (vol->v_mtime < tv.tv_sec) {
        vol->v_mtime = tv.tv_sec;
        /* or finder doesn't update free space */
        if (afp_version > 21 && obj->options.server_notif) {
            obj->attention(obj->handle, AFPATTN_NOTIFY | AFPATTN_VOLCHANGED);
        }
    }
}

/* ------------------------- */
int afp_getvolparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf, *rbuf;
int	ibuflen _U_, *rbuflen;
{
    struct vol	*vol;
    u_int16_t	vid, bitmap;

    ibuf += 2;
    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        *rbuflen = 0;
        return( AFPERR_PARAM );
    }

    return stat_vol(bitmap, vol, rbuf, rbuflen);
}

/* ------------------------- */
int afp_setvolparams(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj  *obj _U_;
char	*ibuf, *rbuf _U_;
int	ibuflen _U_, *rbuflen;
{
    struct adouble ad;
    struct vol	*vol;
    u_int16_t	vid, bitmap;
    u_int32_t   aint;

    ibuf += 2;
    *rbuflen = 0;

    memcpy(&vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof(bitmap);

    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    if ((vol->v_flags & AFPVOL_RO))
        return AFPERR_VLOCK;

    /* we can only set the backup date. */
    if (bitmap != (1 << VOLPBIT_BDATE))
        return AFPERR_BITMAP;

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    if ( ad_open( vol->v_path, ADFLAGS_HF|ADFLAGS_DIR, O_RDWR,
                  0666, &ad) < 0 ) {
        if (errno == EROFS)
            return AFPERR_VLOCK;

        return AFPERR_ACCESS;
    }

    memcpy(&aint, ibuf, sizeof(aint));
    ad_setdate(&ad, AD_DATE_BACKUP, aint);
    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
    return( AFP_OK );
}

/* ------------------------- */
int wincheck(const struct vol *vol, const char *path)
{
    int len;

    if (!(vol->v_flags & AFPVOL_MSWINDOWS))
        return 1;

    /* empty paths are not allowed */
    if ((len = strlen(path)) == 0)
        return 0;

    /* leading or trailing whitespaces are not allowed, carriage returns
     * and probably other whitespace is okay, tabs are not allowed
     */
    if ((path[0] == ' ') || (path[len-1] == ' '))
        return 0;

    /* certain characters are not allowed */
    if (strpbrk(path, MSWINDOWS_BADCHARS))
        return 0;

    /* everything else is okay */
    return 1;
}


/*
 * precreate a folder 
 * this is only intended for folders in the volume root 
 * It will *not* work if the folder name contains extended characters 
 */
static int create_special_folder (const struct vol *vol, const struct _special_folder *folder)
{
	char 		*p,*q,*r;
    	struct adouble	ad;
	u_int16_t	attr;
	struct stat	st;
	int		ret;


	p = (char *) malloc ( strlen(vol->v_path)+strlen(folder->name)+2);
	if ( p == NULL) {
		LOG(log_error, logtype_afpd,"malloc failed");
		exit (EXITERR_SYS);
	}

	q=strdup(folder->name);
	if ( q == NULL) {
		LOG(log_error, logtype_afpd,"malloc failed");
		exit (EXITERR_SYS);
	}

	strcpy(p, vol->v_path);
	strcat(p, "/");

	r=q;
	while (*r) {
    		if ((vol->v_casefold & AFPVOL_MTOUUPPER))
			*r=toupper(*r);
    		else if ((vol->v_casefold & AFPVOL_MTOULOWER))
			*r=tolower(*r);
		r++;
	}
	strcat(p, q);

    	if ( (ret = stat( p, &st )) < 0 ) {
		if (folder->precreate) {
		    if (ad_mkdir(p, folder->mode)) {
			LOG(log_debug, logtype_afpd,"Creating '%s' failed in %s: %s", p, vol->v_path, strerror(errno));
			free(p);
			free(q);
			return -1;
                    }
		    ret = 0;
		}
	}

	if ( !ret && folder->hide) {
		/* Hide it */
		ad_init(&ad, vol->v_adouble, vol->v_ad_options);
		if (ad_open( p, vol_noadouble(vol) | ADFLAGS_HF|ADFLAGS_DIR,
                 	O_RDWR|O_CREAT, 0666, &ad) < 0) {
			free (p);
			free(q);
			return (-1);
		}
    		if ((ad_get_HF_flags( &ad ) & O_CREAT) ) {
    		    if (ad_getentryoff(&ad, ADEID_NAME)) {
    		        ad_setentrylen( &ad, ADEID_NAME, strlen(folder->name));
    		        memcpy(ad_entry( &ad, ADEID_NAME ), folder->name,
    		               ad_getentrylen( &ad, ADEID_NAME ));
    		    }
		}
 
		ad_getattr(&ad, &attr);
		attr |= htons( ntohs( attr ) | ATTRBIT_INVISIBLE );
		ad_setattr(&ad, attr);

		/* do the same with the finder info */
		if (ad_entry(&ad, ADEID_FINDERI)) {
			memcpy(&attr, ad_entry(&ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF, sizeof(attr));
			attr   |= htons(FINDERINFO_INVISIBLE);
			memcpy(ad_entry(&ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF,&attr, sizeof(attr));
		}

        	ad_flush( &ad );
        	ad_close( &ad, ADFLAGS_HF );
	}
	free(p);
	free(q);
	return 0;
}

static void handle_special_folders (const struct vol * vol)
{
	const _special_folder *p = &special_folders[0];

    	if ((vol->v_flags & AFPVOL_RO))
		return;

        for (; p->name != NULL; p++) {
		create_special_folder (vol, p);
	}
}

/*
 * Save the volume options to a file, used by
 * shell utilities.
 * Writing the file everytime a volume is opened is
 * unnecessary, but it shouldn't hurt much.
 */
static int savevoloptions (const struct vol *vol)
{
    char buf[16348];
    char item[MAXPATHLEN];
    int fd;
    int ret = 0;
    struct flock lock;
    const _vol_opt_name *op = &vol_opt_names[0];
    const _vol_opt_name *cf = &vol_opt_casefold[0];

    strlcpy (item, vol->v_path, sizeof(item));
    strlcat (item, "/.AppleDesktop/", sizeof(item));
    strlcat (item, VOLINFOFILE, sizeof(item));

    if ((fd = open( item, O_RDWR | O_CREAT , 0666)) <0 ) {
        LOG(log_debug, logtype_afpd,"Error opening %s: %s", item, strerror(errno));
        return (-1);
    }

    /* try to get a lock */
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type   = F_WRLCK;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            /* ignore, other process already writing the file */
            return 0;
        } else {
            LOG(log_error, logtype_cnid, "savevoloptions: cannot get lock: %s", strerror(errno));
            return (-1);
        }
    }

    /* write volume options */
    snprintf(buf, sizeof(buf), "MAC_CHARSET:%s\n", vol->v_maccodepage);
    snprintf(item, sizeof(item), "VOL_CHARSET:%s\n", vol->v_volcodepage);
    strlcat(buf, item, sizeof(buf));

    switch (vol->v_adouble) {
        case AD_VERSION1:
            strlcat(buf, "ADOUBLE_VER:v1\n", sizeof(buf));
            break;
        case AD_VERSION2:
            strlcat(buf, "ADOUBLE_VER:v2\n", sizeof(buf));
            break;
        case AD_VERSION2_OSX:
            strlcat(buf, "ADOUBLE_VER:osx\n", sizeof(buf));
            break;
        case AD_VERSION1_ADS:
            strlcat(buf, "ADOUBLE_VER:ads\n", sizeof(buf));
            break;
        case AD_VERSION1_SFM:
            strlcat(buf, "ADOUBLE_VER:sfm\n", sizeof(buf));
            break;
    }

    strlcat(buf, "CNIDBACKEND:", sizeof(buf));
    strlcat(buf, vol->v_cnidscheme, sizeof(buf));
    strlcat(buf, "\n", sizeof(buf));

    strlcat(buf, "CNIDDBDHOST:", sizeof(buf));
    strlcat(buf, Cnid_srv, sizeof(buf));
    strlcat(buf, "\n", sizeof(buf));

    snprintf(item, sizeof(item), "CNIDDBDPORT:%u\n", Cnid_port);
    strlcat(buf, item, sizeof(buf));

    strcpy(item, "CNID_DBPATH:");
    if (vol->v_dbpath)
        strlcat(item, vol->v_dbpath, sizeof(item));
    else
        strlcat(item, vol->v_path, sizeof(item));
    strlcat(item, "\n", sizeof(item));
    strlcat(buf, item, sizeof(buf));

    /* volume flags */
    strcpy(item, "VOLUME_OPTS:");
    for (;op->name; op++) {
	if ( (vol->v_flags & op->option) ) {
            strlcat(item, op->name, sizeof(item));
            strlcat(item, " ", sizeof(item));
        }
    }
    strlcat(item, "\n", sizeof(item));
    strlcat(buf, item, sizeof(buf));

    /* casefold flags */
    strcpy(item, "VOLCASEFOLD:");
    for (;cf->name; cf++) {
        if ( (vol->v_casefold & cf->option) ) {
            strlcat(item, cf->name, sizeof(item));
            strlcat(item, " ", sizeof(item));
        }
    }
    strlcat(item, "\n", sizeof(item));
    strlcat(buf, item, sizeof(buf));

    if (strlen(buf) >= sizeof(buf)-1)
        LOG(log_debug, logtype_afpd,"Error writing .volinfo file: buffer too small, %s", buf);


   if (write( fd, buf, strlen(buf)) < 0) {
       LOG(log_debug, logtype_afpd,"Error writing .volinfo file: %s", strerror(errno));
       goto done;
   }
   ftruncate(fd, strlen(buf));

done:
   lock.l_type = F_UNLCK;
   fcntl(fd, F_SETLK, &lock);
   close (fd);
   return ret;
}
