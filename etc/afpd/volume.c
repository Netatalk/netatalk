/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <time.h>

#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/volinfo.h>
#include <atalk/logger.h>
#include <atalk/vfs.h>
#include <atalk/uuid.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/ftw.h>
#include <atalk/globals.h>
#include <atalk/fce_api.h>
#include <atalk/errchk.h>

#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB*/

#include "directory.h"
#include "file.h"
#include "volume.h"
#include "unix.h"
#include "mangle.h"
#include "fork.h"
#include "hash.h"
#include "acls.h"
#include "auth.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

#ifndef UUID_PRINTABLE_STRING_LENGTH
#define UUID_PRINTABLE_STRING_LENGTH 37
#endif

/* Globals */
struct vol *current_vol;        /* last volume from getvolbyvid() */

static struct vol *Volumes = NULL;
static u_int16_t    lastvid = 0;
static char     *Trash = "\02\024Network Trash Folder";

static struct extmap    *Extmap = NULL, *Defextmap = NULL;
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
#define VOLOPT_LIMITSIZE  8  /* Limit the size of the volume */
/* Usable slot: 9 */
#define VOLOPT_VETO          10  /* list of veto filespec */
#define VOLOPT_ENCODING      15  /* mac encoding (pre OSX)*/
#define VOLOPT_MACCHARSET    16
#define VOLOPT_CNIDSCHEME    17
#define VOLOPT_ADOUBLE       18  /* adouble version */

#define VOLOPT_UMASK         21
#define VOLOPT_ALLOWED_HOSTS 22
#define VOLOPT_DENIED_HOSTS  23
#define VOLOPT_DPERM         24  /* dperm default directories perms */
#define VOLOPT_FPERM         25  /* fperm default files perms */
#define VOLOPT_DFLTPERM      26  /* perm */
#define VOLOPT_EA_VFS        27  /* Extended Attributes vfs indirection */
#define VOLOPT_CNIDSERVER    28  /* CNID Server ip address*/
#define VOLOPT_CNIDPORT      30  /* CNID server tcp port */

#define VOLOPT_MAX           31  /* <== IMPORTANT !!!!!! */
#define VOLOPT_NUM           (VOLOPT_MAX + 1)

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

/* Forward declarations */
static void handle_special_folders (const struct vol *);
static void deletevol(struct vol *vol);
static void volume_free(struct vol *vol);
static void check_ea_sys_support(struct vol *vol);
static char *get_vol_uuid(const AFPObj *obj, const char *volname);
static int readvolfile(AFPObj *obj, struct afp_volume_name *p1,char *p2, int user, struct passwd *pwent);

static void volfree(struct vol_option *options, const struct vol_option *save)
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


#define is_var(a, b) (strncmp((a), (b), 2) == 0)

/*
 * Handle variable substitutions. here's what we understand:
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
 * This get's called from readvolfile with
 * path = NULL, volname = NULL for xlating the volumes path
 * path = path, volname = NULL for xlating the volumes name
 * ... and from volumes options parsing code when xlating eg dbpath with
 * path = path, volname = volname
 *
 * Using this information we can reject xlation of any variable depeninding on a login
 * context which is not given in the afp master, where we must evaluate this whole stuff
 * too for the Zeroconf announcements.
 */
static char *volxlate(AFPObj *obj,
                      char *dest,
                      size_t destlen,
                      char *src,
                      struct passwd *pwd,
                      char *path,
                      char *volname)
{
    char *p, *r;
    const char *q;
    int len;
    char *ret;
    int afpmaster = 0;
    int xlatevolname = 0;

    if (parent_or_child == 0)
        afpmaster = 1;

    if (path && !volname)
        /* cf above */
        xlatevolname = 1;

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
            if (afpmaster && xlatevolname)
                return NULL;
            if (path) {
                if ((q = strrchr(path, '/')) == NULL)
                    q = path;
                else if (*(q + 1) != '\0')
                    q++;
            }
        } else if (is_var(p, "$c")) {
            if (afpmaster && xlatevolname)
                return NULL;
#ifndef NO_DDP
            if (obj->proto == AFPPROTO_ASP) {
                ASP asp = obj->handle;

                len = sprintf(dest, "%u.%u", ntohs(asp->asp_sat.sat_addr.s_net),
                              asp->asp_sat.sat_addr.s_node);
                dest += len;
                destlen -= len;

            }
#endif
            if (obj->proto == AFPPROTO_DSI) {
                DSI *dsi = obj->handle;
                len = sprintf(dest, "%s:%u",
                              getip_string((struct sockaddr *)&dsi->client),
                              getip_port((struct sockaddr *)&dsi->client));
                dest += len;
                destlen -= len;
            }
        } else if (is_var(p, "$d")) {
            if (afpmaster && xlatevolname)
                return NULL;
            q = path;
        } else if (pwd && is_var(p, "$f")) {
            if (afpmaster && xlatevolname)
                return NULL;
            if ((r = strchr(pwd->pw_gecos, ',')))
                *r = '\0';
            q = pwd->pw_gecos;
        } else if (pwd && is_var(p, "$g")) {
            if (afpmaster && xlatevolname)
                return NULL;
            struct group *grp = getgrgid(pwd->pw_gid);
            if (grp)
                q = grp->gr_name;
        } else if (is_var(p, "$h")) {
            q = obj->options.hostname;
        } else if (is_var(p, "$i")) {
            if (afpmaster && xlatevolname)
                return NULL;
#ifndef NO_DDP
            if (obj->proto == AFPPROTO_ASP) {
                ASP asp = obj->handle;
                len = sprintf(dest, "%u", ntohs(asp->asp_sat.sat_addr.s_net));
                dest += len;
                destlen -= len;

            }
#endif
            if (obj->proto == AFPPROTO_DSI) {
                DSI *dsi = obj->handle;
                q = getip_string((struct sockaddr *)&dsi->client);
            }
        } else if (is_var(p, "$s")) {
            if (obj->Obj)
                q = obj->Obj;
            else if (obj->options.server) {
                q = obj->options.server;
            } else
                q = obj->options.hostname;
        } else if ((strlen(obj->username) > 0) && is_var(p, "$u")) {
            if (afpmaster && xlatevolname)
                return NULL;
            char* sep = NULL;
            if ( obj->options.ntseparator && (sep = strchr(obj->username, obj->options.ntseparator[0])) != NULL)
                q = sep+1;
            else
                q = obj->username;
        } else if (is_var(p, "$v")) {
            if (afpmaster && xlatevolname)
                return NULL;
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
    LOG(log_debug, logtype_afpd, "Parsing volset %s", val);
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
            else if (strcasecmp(p, "nofileid") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NOFILEID;
            else if (strcasecmp(p, "upriv") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_UNIX_PRIV;
            else if (strcasecmp(p, "nodev") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NODEV;
            else if (strcasecmp(p, "caseinsensitive") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_CASEINSEN;
            else if (strcasecmp(p, "illegalseq") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_EILSEQ;
            else if (strcasecmp(p, "nocnidcache") == 0)
                options[VOLOPT_FLAGS].i_value &= ~AFPVOL_CACHE;
            else if (strcasecmp(p, "tm") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_TM;
            else if (strcasecmp(p, "searchdb") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_SEARCHDB;
            else if (strcasecmp(p, "nonetids") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_NONETIDS;
            else if (strcasecmp(p, "noacls") == 0)
                options[VOLOPT_FLAGS].i_value &= ~AFPVOL_ACLS;
            else if (strcasecmp(p, "followsymlinks") == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_FOLLOWSYM;
            p = strtok(NULL, ",");
        }

    } else if (optionok(tmp, "cnidserver:", val)) {
        setoption(options, save, VOLOPT_CNIDSERVER, val);

        char *p = strrchr(val + 1, ':');
        if (p) {
            *p = 0;
            setoption(options, save, VOLOPT_CNIDPORT, p);
        }

        LOG(log_debug, logtype_afpd, "CNID Server for volume '%s': %s:%s",
            volname, val + 1, p ? p + 1 : Cnid_port);

    } else if (optionok(tmp, "dbpath:", val)) {
        setoption(options, save, VOLOPT_DBPATH, val);

    } else if (optionok(tmp, "umask:", val)) {
        options[VOLOPT_UMASK].i_value = (int)strtol(val +1, NULL, 8);
    } else if (optionok(tmp, "dperm:", val)) {
        options[VOLOPT_DPERM].i_value = (int)strtol(val+1, NULL, 8);
    } else if (optionok(tmp, "fperm:", val)) {
        options[VOLOPT_FPERM].i_value = (int)strtol(val+1, NULL, 8);
    } else if (optionok(tmp, "perm:", val)) {
        options[VOLOPT_DFLTPERM].i_value = (int)strtol(val+1, NULL, 8);
    } else if (optionok(tmp, "password:", val)) {
        setoption(options, save, VOLOPT_PASSWORD, val);

    } else if (optionok(tmp, "allowed_hosts:", val)) {
        setoption(options, save, VOLOPT_ALLOWED_HOSTS, val);

    } else if (optionok(tmp, "denied_hosts:", val)) {
        setoption(options, save, VOLOPT_DENIED_HOSTS, val);

    } else if (optionok(tmp, "ea:", val)) {
        if (strcasecmp(val + 1, "ad") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_AD;
        else if (strcasecmp(val + 1, "sys") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_SYS;
        else if (strcasecmp(val + 1, "none") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_NONE;

    } else if (optionok(tmp, "volsizelimit:", val)) {
        options[VOLOPT_LIMITSIZE].i_value = (uint32_t)strtoul(val + 1, NULL, 10);

    } else {
        /* ignore unknown options */
        LOG(log_debug, logtype_afpd, "ignoring unknown volume option: %s", tmp);

    }
}

/* ----------------- */
static void showvol(const ucs2_t *name)
{
    struct vol  *volume;
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
                    struct vol_option *options
    )
{
    struct vol  *volume;
    int         suffixlen, vlen, tmpvlen, u8mvlen, macvlen;
    int         hide = 0;
    char        tmpname[AFPVOL_U8MNAMELEN+1];
    ucs2_t      u8mtmpname[(AFPVOL_U8MNAMELEN+1)*2], mactmpname[(AFPVOL_MACNAMELEN+1)*2];
    char        suffix[6]; /* max is #FFFF */
    u_int16_t   flags;

    LOG(log_debug, logtype_afpd, "createvol: Volume '%s'", name);

    if ( name == NULL || *name == '\0' ) {
        if ((name = strrchr( path, '/' )) == NULL) {
            return -1;  /* Obviously not a fully qualified path */
        }

        /* if you wish to share /, you need to specify a name. */
        if (*++name == '\0')
            return -1;
    }

    /* suffix for mangling use (lastvid + 1)   */
    /* because v_vid has not been decided yet. */
    suffixlen = snprintf(suffix, sizeof(suffix), "%c%X", MANGLE_CHAR, lastvid + 1);

    vlen = strlen( name );

    /* Unicode Volume Name */
    /* Firstly convert name from unixcharset to UTF8-MAC */
    flags = CONV_IGNORE;
    tmpvlen = convert_charset(obj->options.unixcharset, CH_UTF8_MAC, 0, name, vlen, tmpname, AFPVOL_U8MNAMELEN, &flags);
    if (tmpvlen <= 0) {
        strcpy(tmpname, "???");
        tmpvlen = 3;
    }

    /* Do we have to mangle ? */
    if ( (flags & CONV_REQMANGLE) || (tmpvlen > obj->options.volnamelen)) {
        if (tmpvlen + suffixlen > obj->options.volnamelen) {
            flags = CONV_FORCE;
            tmpvlen = convert_charset(obj->options.unixcharset, CH_UTF8_MAC, 0, name, vlen, tmpname, obj->options.volnamelen - suffixlen, &flags);
            tmpname[tmpvlen >= 0 ? tmpvlen : 0] = 0;
        }
        strcat(tmpname, suffix);
        tmpvlen = strlen(tmpname);
    }

    /* Secondly convert name from UTF8-MAC to UCS2 */
    if ( 0 >= ( u8mvlen = convert_string(CH_UTF8_MAC, CH_UCS2, tmpname, tmpvlen, u8mtmpname, AFPVOL_U8MNAMELEN*2)) )
        return -1;

    LOG(log_maxdebug, logtype_afpd, "createvol: Volume '%s' -> UTF8-MAC Name: '%s'", name, tmpname);

    /* Maccharset Volume Name */
    /* Firsty convert name from unixcharset to maccharset */
    flags = CONV_IGNORE;
    tmpvlen = convert_charset(obj->options.unixcharset, obj->options.maccharset, 0, name, vlen, tmpname, AFPVOL_U8MNAMELEN, &flags);
    if (tmpvlen <= 0) {
        strcpy(tmpname, "???");
        tmpvlen = 3;
    }

    /* Do we have to mangle ? */
    if ( (flags & CONV_REQMANGLE) || (tmpvlen > AFPVOL_MACNAMELEN)) {
        if (tmpvlen + suffixlen > AFPVOL_MACNAMELEN) {
            flags = CONV_FORCE;
            tmpvlen = convert_charset(obj->options.unixcharset,
                                      obj->options.maccharset,
                                      0,
                                      name,
                                      vlen,
                                      tmpname,
                                      AFPVOL_MACNAMELEN - suffixlen,
                                      &flags);
            tmpname[tmpvlen >= 0 ? tmpvlen : 0] = 0;
        }
        strcat(tmpname, suffix);
        tmpvlen = strlen(tmpname);
    }

    /* Secondly convert name from maccharset to UCS2 */
    if ( 0 >= ( macvlen = convert_string(obj->options.maccharset,
                                         CH_UCS2,
                                         tmpname,
                                         tmpvlen,
                                         mactmpname,
                                         AFPVOL_U8MNAMELEN*2)) )
        return -1;

    LOG(log_maxdebug, logtype_afpd, "createvol: Volume '%s' ->  Longname: '%s'", name, tmpname);

    /* check duplicate */
    for ( volume = Volumes; volume; volume = volume->v_next ) {
        if ((utf8_encoding() && (strcasecmp_w(volume->v_u8mname, u8mtmpname) == 0))
             ||
            (!utf8_encoding() && (strcasecmp_w(volume->v_macname, mactmpname) == 0))) {
            if (volume->v_deleted) {
                volume->v_new = hide = 1;
            } else {
                LOG (log_error, logtype_afpd,
                     "Duplicate volume name, check AppleVolumes files: previous: \"%s\", new: \"%s\"",
                     volume->v_localname, name);
                return -1;  /* Won't be able to access it, anyway... */
            }
        }
    }

    if (!( volume = (struct vol *)calloc(1, sizeof( struct vol ))) ) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        return -1;
    }
    if ( NULL == ( volume->v_localname = strdup(name))) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        free(volume);
        return -1;
    }

    if ( NULL == ( volume->v_u8mname = strdup_w(u8mtmpname))) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        volume_free(volume);
        free(volume);
        return -1;
    }
    if ( NULL == ( volume->v_macname = strdup_w(mactmpname))) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        volume_free(volume);
        free(volume);
        return -1;
    }
    if (!( volume->v_path = (char *)malloc( strlen( path ) + 1 )) ) {
        LOG(log_error, logtype_afpd, "creatvol: malloc: %s", strerror(errno) );
        volume_free(volume);
        free(volume);
        return -1;
    }

    volume->v_name = utf8_encoding()?volume->v_u8mname:volume->v_macname;
    volume->v_hide = hide;
    strcpy( volume->v_path, path );

#ifdef __svr4__
    volume->v_qfd = -1;
#endif /* __svr4__ */
    /* os X start at 1 and use network order ie. 1 2 3 */
    volume->v_vid = ++lastvid;
    volume->v_vid = htons(volume->v_vid);
#ifdef HAVE_ACLS
    if (!check_vol_acl_support(volume)) {
        LOG(log_debug, logtype_afpd, "creatvol(\"%s\"): disabling ACL support", volume->v_path);
        options[VOLOPT_FLAGS].i_value &= ~AFPVOL_ACLS;
    }
#endif

    /* handle options */
    if (options) {
        volume->v_casefold = options[VOLOPT_CASEFOLD].i_value;
        volume->v_flags |= options[VOLOPT_FLAGS].i_value;

        if (options[VOLOPT_EA_VFS].i_value)
            volume->v_vfs_ea = options[VOLOPT_EA_VFS].i_value;

        volume->v_ad_options = 0;
        if ((volume->v_flags & AFPVOL_NODEV))
            volume->v_ad_options |= ADVOL_NODEV;
        if ((volume->v_flags & AFPVOL_CACHE))
            volume->v_ad_options |= ADVOL_CACHE;
        if ((volume->v_flags & AFPVOL_UNIX_PRIV))
            volume->v_ad_options |= ADVOL_UNIXPRIV;
        if ((volume->v_flags & AFPVOL_INV_DOTS))
            volume->v_ad_options |= ADVOL_INVDOTS;
        if ((volume->v_flags & AFPVOL_NOADOUBLE))
            volume->v_ad_options |= ADVOL_NOADOUBLE;
        if ((volume->v_flags & AFPVOL_FOLLOWSYM))
            volume->v_ad_options |= ADVOL_FOLLO_SYML;

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

        if (options[VOLOPT_CNIDSERVER].c_value)
            volume->v_cnidserver = strdup(options[VOLOPT_CNIDSERVER].c_value);

        if (options[VOLOPT_CNIDPORT].c_value)
            volume->v_cnidport = strdup(options[VOLOPT_CNIDPORT].c_value);

        if (options[VOLOPT_UMASK].i_value)
            volume->v_umask = (mode_t)options[VOLOPT_UMASK].i_value;

        if (options[VOLOPT_DPERM].i_value)
            volume->v_dperm = (mode_t)options[VOLOPT_DPERM].i_value;

        if (options[VOLOPT_FPERM].i_value)
            volume->v_fperm = (mode_t)options[VOLOPT_FPERM].i_value;

        if (options[VOLOPT_DFLTPERM].i_value)
            volume->v_perm = (mode_t)options[VOLOPT_DFLTPERM].i_value;

        if (options[VOLOPT_ADOUBLE].i_value)
            volume->v_adouble = options[VOLOPT_ADOUBLE].i_value;
        else
            volume->v_adouble = AD_VERSION;

        if (options[VOLOPT_LIMITSIZE].i_value)
            volume->v_limitsize = options[VOLOPT_LIMITSIZE].i_value;

        /* Mac to Unix conversion flags*/
        volume->v_mtou_flags = 0;
        if (!(volume->v_flags & AFPVOL_NOHEX))
            volume->v_mtou_flags |= CONV_ESCAPEHEX;
        if (!(volume->v_flags & AFPVOL_USEDOTS))
            volume->v_mtou_flags |= CONV_ESCAPEDOTS;
        if ((volume->v_flags & AFPVOL_EILSEQ))
            volume->v_mtou_flags |= CONV__EILSEQ;

        if ((volume->v_casefold & AFPVOL_MTOUUPPER))
            volume->v_mtou_flags |= CONV_TOUPPER;
        else if ((volume->v_casefold & AFPVOL_MTOULOWER))
            volume->v_mtou_flags |= CONV_TOLOWER;

        /* Unix to Mac conversion flags*/
        volume->v_utom_flags = CONV_IGNORE | CONV_UNESCAPEHEX;
        if ((volume->v_casefold & AFPVOL_UTOMUPPER))
            volume->v_utom_flags |= CONV_TOUPPER;
        else if ((volume->v_casefold & AFPVOL_UTOMLOWER))
            volume->v_utom_flags |= CONV_TOLOWER;

        if ((volume->v_flags & AFPVOL_EILSEQ))
            volume->v_utom_flags |= CONV__EILSEQ;

    }
    volume->v_dperm |= volume->v_perm;
    volume->v_fperm |= volume->v_perm;

    /* Check EA support on volume */
    if (volume->v_vfs_ea == AFPVOL_EA_AUTO)
        check_ea_sys_support(volume);
    initvol_vfs(volume);

    /* get/store uuid from file in afpd master*/
    if ((parent_or_child == 0) && (volume->v_flags & AFPVOL_TM)) {
        char *uuid = get_vol_uuid(obj, volume->v_localname);
        if (!uuid) {
            LOG(log_error, logtype_afpd, "Volume '%s': couldn't get UUID",
                volume->v_localname);
        } else {
            volume->v_uuid = uuid;
            LOG(log_debug, logtype_afpd, "Volume '%s': UUID '%s'",
                volume->v_localname, volume->v_uuid);
        }
    }

    volume->v_next = Volumes;
    Volumes = volume;
    return 0;
}

/* ---------------- */
static char *myfgets( char *buf, int size, FILE *fp)
{
    char    *p;
    int     c;

    p = buf;
    while ((EOF != ( c = getc( fp )) ) && ( size > 1 )) {
        if ( c == '\n' || c == '\r' ) {
            if (p != buf && *(p -1) == '\\') {
                p--;
                size++;
                continue;
            }
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

static int accessvol(const char *args, const char *name)
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

static int hostaccessvol(int type, const char *volname, const char *args, const AFPObj *obj)
{
    int mask_int;
    char buf[MAXPATHLEN + 1], *p, *b;
    DSI *dsi = obj->handle;
    struct sockaddr_storage client;

    if (!args)
        return -1;

    strlcpy(buf, args, sizeof(buf));
    if ((p = strtok_r(buf, ",", &b)) == NULL) /* nothing, return okay */
        return -1;

    if (obj->proto != AFPPROTO_DSI)
        return -1;

    while (p) {
        int ret;
        char *ipaddr, *mask_char;
        struct addrinfo hints, *ai;

        ipaddr = strtok(p, "/");
        mask_char = strtok(NULL,"/");

        /* Get address from string with getaddrinfo */
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if ((ret = getaddrinfo(ipaddr, NULL, &hints, &ai)) != 0) {
            LOG(log_error, logtype_afpd, "hostaccessvol: getaddrinfo: %s\n", gai_strerror(ret));
            continue;
        }

        /* netmask */
        if (mask_char != NULL)
            mask_int = atoi(mask_char); /* apply_ip_mask does range checking on it */
        else {
            if (ai->ai_family == AF_INET) /* IPv4 */
                mask_int = 32;
            else                          /* IPv6 */
                mask_int = 128;
        }

        /* Apply mask to addresses */
        client = dsi->client;
        apply_ip_mask((struct sockaddr *)&client, mask_int);
        apply_ip_mask(ai->ai_addr, mask_int);

        if (compare_ip((struct sockaddr *)&client, ai->ai_addr) == 0) {
            if (type == VOLOPT_DENIED_HOSTS)
                LOG(log_info, logtype_afpd, "AFP access denied for client IP '%s' to volume '%s' by denied list",
                    getip_string((struct sockaddr *)&client), volname);
            freeaddrinfo(ai);
            return 1;
        }

        /* next address */
        freeaddrinfo(ai);
        p = strtok_r(NULL, ",", &b);
    }

    if (type == VOLOPT_ALLOWED_HOSTS)
        LOG(log_info, logtype_afpd, "AFP access denied for client IP '%s' to volume '%s', not in allowed list",
            getip_string((struct sockaddr *)&dsi->client), volname);
    return 0;
}

static void setextmap(char *ext, char *type, char *creator, int user)
{
    struct extmap   *em;
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
        void *Extmap_check = realloc(Extmap, sizeof(struct extmap) * (cnt + 2));
        if (Extmap_check) {
            Extmap = Extmap_check;
        } else {
            LOG(log_error, logtype_afpd, "setextmap: realloc: %s", strerror(errno) );
            free(Extmap);
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
        memcpy(em->em_type, "\0\0\0\0", sizeof( em->em_type ));
    } else {
        memcpy(em->em_type, type, sizeof( em->em_type ));
    }
    if ( *creator == '\0' ) {
        memcpy(em->em_creator, "\0\0\0\0", sizeof( em->em_creator ));
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
    struct extmap   *em;

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
    struct extmap   *em;

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
 * the global volume list. This gets called from the forked afpd childs.
 * The master now reads this too for Zeroconf announcements.
 *
 * If p2 is non-NULL, the file that is opened is
 * p1/p2
 *
 * Lines that begin with # and blank lines are ignored.
 * Volume lines are of the form:
 *      <unix path> [<volume name>] [allow:<user>,<@group>,...] \
 *                           [codepage:<file>] [casefold:<num>]
 *      <extension> TYPE [CREATOR]
 *
 */
static int readvolfile(AFPObj *obj, struct afp_volume_name *p1, char *p2, int user, struct passwd *pwent)
{
    FILE        *fp;
    char        path[MAXPATHLEN + 1];
    char        tmp[MAXPATHLEN + 1];
    char        volname[AFPVOL_U8MNAMELEN + 1];
    char        buf[BUFSIZ];
    char        type[5], creator[5];
    char        *u, *p;
    int         fd;
    int         i;
    struct passwd   *pw;
    struct vol_option   save_options[VOLOPT_NUM];
    struct vol_option   default_options[VOLOPT_NUM];
    struct vol_option   options[VOLOPT_NUM];
    struct stat         st;

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

    /* try putting a read lock on the volume file twice, sleep 1 second if first attempt fails */
    int retries = 2;
    while (1) {
        if ((read_lock(fd, 0, SEEK_SET, 0)) != 0) {
            retries--;
            if (!retries) {
                LOG(log_error, logtype_afpd, "readvolfile: can't lock volume file \"%s\"", path);
                if ( fclose( fp ) != 0 ) {
                    LOG(log_error, logtype_afpd, "readvolfile: fclose: %s", strerror(errno) );
                }
                return -1;
            }
            sleep(1);
            continue;
        }
        break;
    }

    memset(default_options, 0, sizeof(default_options));

    /* Enable some default options for all volumes */
    default_options[VOLOPT_FLAGS].i_value |= AFPVOL_CACHE;
#ifdef HAVE_ACLS
    default_options[VOLOPT_FLAGS].i_value |= AFPVOL_ACLS;
#endif
    default_options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_AUTO;
    LOG(log_maxdebug, logtype_afpd, "readvolfile: seeding default umask: %04o",
        obj->options.umask);
    default_options[VOLOPT_UMASK].i_value = obj->options.umask;
    memcpy(save_options, default_options, sizeof(options));

    LOG(log_debug, logtype_afpd, "readvolfile: \"%s\"", path);

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
                volfree(default_options, save_options);
                memcpy(default_options, save_options, sizeof(options));
                *tmp = '\0';
                for (i = 0; i < VOLOPT_NUM; i++) {
                    if (parseline( sizeof( path ) - VOLOPT_DEFAULT_LEN - 1,
                                   path + VOLOPT_DEFAULT_LEN) < 0)
                        break;
                    volset(default_options, NULL, tmp, sizeof(tmp) - 1,
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
            /* fall through */

        case '/' :
            /* send path through variable substitution */
            if (*path != '~') /* need to copy path to tmp */
                strcpy(tmp, path);
            if (!pwent && (strlen(obj->username) > 0))
                pwent = getpwnam(obj->username);

            if (volxlate(obj, path, sizeof(path) - 1, tmp, pwent, NULL, NULL) == NULL)
                continue;

            /* this is sort of braindead. basically, i want to be
             * able to specify things in any order, but i don't want to
             * re-write everything. */

            memcpy(options, default_options, sizeof(options));
            *volname = '\0';

            /* read in up to VOLOP_NUM possible options */
            for (i = 0; i < VOLOPT_NUM; i++) {
                if (parseline( sizeof( tmp ) - 1, tmp ) < 0)
                    break;

                volset(options, default_options, volname, sizeof(volname) - 1, tmp);
            }

            /* check allow/deny lists (if not afpd master loading volumes for Zeroconf reg.):
               allow -> either no list (-1), or in list (1)
               deny -> either no list (-1), or not in list (0) */
            if (parent_or_child == 0
                ||
                (accessvol(options[VOLOPT_ALLOW].c_value, obj->username) &&
                 (accessvol(options[VOLOPT_DENY].c_value, obj->username) < 1) &&
                 hostaccessvol(VOLOPT_ALLOWED_HOSTS, volname, options[VOLOPT_ALLOWED_HOSTS].c_value, obj) &&
                 (hostaccessvol(VOLOPT_DENIED_HOSTS, volname, options[VOLOPT_DENIED_HOSTS].c_value, obj) < 1))) {

                /* handle read-only behaviour. semantics:
                 * 1) neither the rolist nor the rwlist exist -> rw
                 * 2) rolist exists -> ro if user is in it.
                 * 3) rwlist exists -> ro unless user is in it. */
                if (parent_or_child == 1
                    &&
                    ((options[VOLOPT_FLAGS].i_value & AFPVOL_RO) == 0)
                    &&
                    ((accessvol(options[VOLOPT_ROLIST].c_value, obj->username) == 1) ||
                     !accessvol(options[VOLOPT_RWLIST].c_value, obj->username)))
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_RO;

                /* do variable substitution for volname */
                if (volxlate(obj, tmp, sizeof(tmp) - 1, volname, pwent, path, NULL) == NULL)
                    continue;

                creatvol(obj, pwent, path, tmp, options);
            }
            volfree(options, default_options);
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
    free(vol->v_localname);
    vol->v_localname = NULL;
    free(vol->v_u8mname);
    vol->v_u8mname = NULL;
    free(vol->v_macname);
    vol->v_macname = NULL;
    free(vol->v_path);
    free(vol->v_password);
    free(vol->v_veto);
    free(vol->v_volcodepage);
    free(vol->v_maccodepage);
    free(vol->v_cnidscheme);
    free(vol->v_dbpath);
    free(vol->v_gvs);
    if (vol->v_uuid)
        free(vol->v_uuid);
}

/* ------------------------------- */
static void free_volumes(void )
{
    struct vol  *vol;
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

        if (vol->v_localname == NULL) {
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
/*!
 * Read band-size info from Info.plist XML file of an TM sparsebundle
 *
 * @param path   (r) path to Info.plist file
 * @return           band-size in bytes, -1 on error
 */
static long long int get_tm_bandsize(const char *path)
{
    EC_INIT;
    FILE *file = NULL;
    char buf[512];
    long long int bandsize = -1;

    EC_NULL_LOGSTR( file = fopen(path, "r"),
                    "get_tm_bandsize(\"%s\"): %s",
                    path, strerror(errno) );

    while (fgets(buf, sizeof(buf), file) != NULL) {
        if (strstr(buf, "band-size") == NULL)
            continue;

        if (fscanf(file, " <integer>%lld</integer>", &bandsize) != 1) {
            LOG(log_error, logtype_afpd, "get_tm_bandsize(\"%s\"): can't parse band-size", path);
            EC_FAIL;
        }
        break;
    }

EC_CLEANUP:
    if (file)
        fclose(file);
    LOG(log_debug, logtype_afpd, "get_tm_bandsize(\"%s\"): bandsize: %lld", path, bandsize);
    if (ret != 0)
        return -1;
    return bandsize;
}

/*!
 * Return number on entries in a directory
 *
 * @param path   (r) path to dir
 * @return           number of entries, -1 on error
 */
static long long int get_tm_bands(const char *path)
{
    EC_INIT;
    long long int count = 0;
    DIR *dir = NULL;
    const struct dirent *entry;

    EC_NULL( dir = opendir(path) );

    while ((entry = readdir(dir)) != NULL)
        count++;
    count -= 2; /* All OSens I'm aware of return "." and "..", so just substract them, avoiding string comparison in loop */
        
EC_CLEANUP:
    if (dir)
        closedir(dir);
    if (ret != 0)
        return -1;
    return count;
}

/*!
 * Calculate used size of a TimeMachine volume
 *
 * This assumes that the volume is used only for TimeMachine.
 *
 * 1) readdir(path of volume)
 * 2) for every element that matches regex "\(.*\)\.sparsebundle$" :
 * 3) parse "\1.sparsebundle/Info.plist" and read the band-size XML key integer value
 * 4) readdir "\1.sparsebundle/bands/" counting files
 * 5) calculate used size as: (file_count - 1) * band-size
 *
 * The result of the calculation is returned in "volume->v_tm_used".
 * "volume->v_appended" gets reset to 0.
 * "volume->v_tm_cachetime" is updated with the current time from time(NULL).
 *
 * "volume->v_tm_used" is cached for TM_USED_CACHETIME seconds and updated by
 * "volume->v_appended". The latter is increased by X every time the client
 * appends X bytes to a file (in fork.c).
 *
 * @param vol     (rw) volume to calculate
 * @return             0 on success, -1 on error
 */
#define TM_USED_CACHETIME 60    /* cache for 60 seconds */
static int get_tm_used(struct vol * restrict vol)
{
    EC_INIT;
    long long int bandsize;
    VolSpace used = 0;
    bstring infoplist = NULL;
    bstring bandsdir = NULL;
    DIR *dir = NULL;
    const struct dirent *entry;
    const char *p;
    long int links;
    time_t now = time(NULL);

    if (vol->v_tm_cachetime
        && ((vol->v_tm_cachetime + TM_USED_CACHETIME) >= now)) {
        if (vol->v_tm_used == -1)
            EC_FAIL;
        vol->v_tm_used += vol->v_appended;
        vol->v_appended = 0;
        LOG(log_debug, logtype_afpd, "getused(\"%s\"): cached: %" PRIu64 " bytes",
            vol->v_path, vol->v_tm_used);
        return 0;
    }

    vol->v_tm_cachetime = now;

    EC_NULL( dir = opendir(vol->v_path) );

    while ((entry = readdir(dir)) != NULL) {
        if (((p = strstr(entry->d_name, "sparsebundle")) != NULL)
            && (strlen(entry->d_name) == (p + strlen("sparsebundle") - entry->d_name))) {

            EC_NULL_LOG( infoplist = bformat("%s/%s/%s", vol->v_path, entry->d_name, "Info.plist") );
            
            if ((bandsize = get_tm_bandsize(cfrombstr(infoplist))) == -1)
                continue;

            EC_NULL_LOG( bandsdir = bformat("%s/%s/%s/", vol->v_path, entry->d_name, "bands") );

            if ((links = get_tm_bands(cfrombstr(bandsdir))) == -1)
                continue;

            used += (links - 1) * bandsize;
            LOG(log_debug, logtype_afpd, "getused(\"%s\"): bands: %" PRIu64 " bytes",
                cfrombstr(bandsdir), used);
        }
    }

    vol->v_tm_used = used;

EC_CLEANUP:
    if (infoplist)
        bdestroy(infoplist);
    if (bandsdir)
        bdestroy(bandsdir);
    if (dir)
        closedir(dir);

    LOG(log_debug, logtype_afpd, "getused(\"%s\"): %" PRIu64 " bytes", vol->v_path, vol->v_tm_used);

    EC_EXIT;
}

static int getvolspace(struct vol *vol,
                       u_int32_t *bfree, u_int32_t *btotal,
                       VolSpace *xbfree, VolSpace *xbtotal, u_int32_t *bsize)
{
    int         rc;
    u_int32_t   maxsize;
#ifndef NO_QUOTA_SUPPORT
    VolSpace    qfree, qtotal;
    int         spaceflag = AFPVOL_GVSMASK & vol->v_flags;
#endif

    /* report up to 2GB if afp version is < 2.2 (4GB if not) */
    maxsize = (vol->v_flags & AFPVOL_A2VOL) ? 0x01fffe00 :
        (((afp_version < 22) || (vol->v_flags & AFPVOL_LIMITSIZE))
         ? 0x7fffffffL : 0xffffffffL);

    if (( rc = ustatfs_getvolspace( vol, xbfree, xbtotal, bsize)) != AFP_OK ) {
        return( rc );
    }

#ifndef NO_QUOTA_SUPPORT
    if ( spaceflag == AFPVOL_NONE || spaceflag == AFPVOL_UQUOTA ) {
        if ( uquota_getvolspace( vol, &qfree, &qtotal, *bsize ) == AFP_OK ) {
            vol->v_flags = ( ~AFPVOL_GVSMASK & vol->v_flags ) | AFPVOL_UQUOTA;
            *xbfree = MIN(*xbfree, qfree);
            *xbtotal = MIN(*xbtotal, qtotal);
            goto getvolspace_done;
        }
    }
#endif
    vol->v_flags = ( ~AFPVOL_GVSMASK & vol->v_flags ) | AFPVOL_USTATFS;

getvolspace_done:
    if (vol->v_limitsize) {
        if (get_tm_used(vol) != 0)
            return AFPERR_MISC;

        *xbtotal = MIN(*xbtotal, (vol->v_limitsize * 1024 * 1024));
        *xbfree = MIN(*xbfree, *xbtotal < vol->v_tm_used ? 0 : *xbtotal - vol->v_tm_used);

        LOG(log_debug, logtype_afpd,
            "volparams: total: %" PRIu64 ", used: %" PRIu64 ", free: %" PRIu64 " bytes",
            *xbtotal, vol->v_tm_used, *xbfree);
    }

    *bfree = MIN(*xbfree, maxsize);
    *btotal = MIN(*xbtotal, maxsize);
    return( AFP_OK );
}

#define FCE_TM_DELTA 10  /* send notification every 10 seconds */
void vol_fce_tm_event(void)
{
    static time_t last;
    time_t now = time(NULL);
    struct vol  *vol = Volumes;

    if ((last + FCE_TM_DELTA) < now) {
        last = now;
        for ( ; vol; vol = vol->v_next ) {
            if (vol->v_flags & AFPVOL_TM)
                (void)fce_register_tm_size(vol->v_path, vol->v_tm_used + vol->v_appended);
        }
    }
}

/* -----------------------
 * set volume creation date
 * avoid duplicate, well at least it tries
 */
static void vol_setdate(u_int16_t id, struct adouble *adp, time_t date)
{
    struct vol  *volume;
    struct vol  *vol = Volumes;

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
static int getvolparams( u_int16_t bitmap, struct vol *vol, struct stat *st, char *buf, size_t *buflen)
{
    struct adouble  ad;
    int bit = 0, isad = 1;
    u_int32_t aint;
    u_int16_t ashort;
    u_int32_t bfree = 0, btotal = 0, bsize = 0;
    VolSpace xbfree = 0, xbtotal = 0; /* extended bytes */
    char *data = NULL, *nameoff = NULL;
    char *slash = NULL;

    LOG(log_debug, logtype_afpd, "getvolparams: Volume '%s'", vol->v_localname);

    /* courtesy of jallison@whistle.com:
     * For MacOS8.x support we need to create the
     * .Parent file here if it doesn't exist. */

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    if ( ad_open_metadata( vol->v_path, ADFLAGS_DIR, O_CREAT, &ad) < 0 ) {
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
                          &bsize) != AFP_OK ) {
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
            /* check for read-only.
             * NOTE: we don't actually set the read-only flag unless
             *       it's passed in that way as it's possible to mount
             *       a read-write filesystem under a read-only one. */
            if ((vol->v_flags & AFPVOL_RO) ||
                ((utime(vol->v_path, NULL) < 0) && (errno == EROFS))) {
                ashort |= VOLPBIT_ATTR_RO;
            }
            /* prior 2.1 only VOLPBIT_ATTR_RO is defined */
            if (afp_version > 20) {
                if (0 == (vol->v_flags & AFPVOL_NOFILEID) && vol->v_cdb != NULL &&
                    (vol->v_cdb->flags & CNID_FLAG_PERSISTENT)) {
                    ashort |= VOLPBIT_ATTR_FILEID;
                }
                ashort |= VOLPBIT_ATTR_CATSEARCH;

                if (afp_version >= 30) {
                    ashort |= VOLPBIT_ATTR_UTF8;
                    if (vol->v_flags & AFPVOL_UNIX_PRIV)
                        ashort |= VOLPBIT_ATTR_UNIXPRIV;
                    if (vol->v_flags & AFPVOL_TM)
                        ashort |= VOLPBIT_ATTR_TM;
                    if (vol->v_flags & AFPVOL_NONETIDS)
                        ashort |= VOLPBIT_ATTR_NONETIDS;
                    if (afp_version >= 32) {
                        if (vol->v_vfs_ea)
                            ashort |= VOLPBIT_ATTR_EXT_ATTRS;
                        if (vol->v_flags & AFPVOL_ACLS)
                            ashort |= VOLPBIT_ATTR_ACLS;
                    }
                }
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
            memcpy(data, &xbfree, sizeof( xbfree ));
            data += sizeof( xbfree );
            break;

        case VOLPBIT_XBTOTAL :
            xbtotal = hton64( xbtotal );
            memcpy(data, &xbtotal, sizeof( xbtotal ));
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
        /* name is always in mac charset */
        aint = ucs2_to_charset( vol->v_maccharset, vol->v_macname, data+1, AFPVOL_MACNAMELEN + 1);
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
static int stat_vol(u_int16_t bitmap, struct vol *vol, char *rbuf, size_t *rbuflen)
{
    struct stat st;
    int     ret;
    size_t  buflen;

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
    int ret;
    struct passwd *pwent = NULL;

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

    if (parent_or_child == 0) {
        LOG(log_debug, logtype_afpd, "load_volumes: AFP MASTER");
    } else {
        LOG(log_debug, logtype_afpd, "load_volumes: user: %s", obj->username);
        if ((pwent = getpwnam(obj->username))) {
            if (seteuid(0) < 0)
                LOG(log_error, logtype_afpd, "could not seteuid(%i)", 0);
            ret = set_groups(obj, pwent);
            if (seteuid(obj->uid) < 0)
                LOG(log_error, logtype_afpd, "could not seteuid(%i)", obj->uid);
            if (ret != 0) {
                LOG(log_error, logtype_afpd, "load_volumes: set_groups: %s", strerror(errno));
                return;
            }
        }
        LOG(log_debug, logtype_afpd, "login: supplementary groups: %s", print_groups(ngroups, groups));
    }

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
                creatvol(obj, pwent, pwent->pw_dir, NULL, NULL);
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
int afp_getsrvrparms(AFPObj *obj, char *ibuf _U_, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct timeval  tv;
    struct stat     st;
    struct vol      *volume;
    char    *data;
    char        *namebuf;
    int         vcnt;
    size_t      len;

    load_volumes(obj);

    data = rbuf + 5;
    for ( vcnt = 0, volume = Volumes; volume && vcnt < 255; volume = volume->v_next ) {
        if (!(volume->v_flags)) {
            struct maccess ma;

            if ( stat( volume->v_path, &st ) < 0 ) {
                LOG(log_info, logtype_afpd, "afp_getsrvrparms(%s): stat: %s",
                    volume->v_path, strerror(errno) );
                continue;       /* can't access directory */
            }
            if (!S_ISDIR(st.st_mode)) {
                continue;       /* not a dir */
            }
            accessmode(volume, volume->v_path, &ma, NULL, &st);
            if ((ma.ma_user & (AR_UREAD | AR_USEARCH)) != (AR_UREAD | AR_USEARCH)) {
                continue;   /* no r-x access */
            }
        }
        if (volume->v_hide) {
            continue;       /* config file changed but the volume was mounted */
        }

        if (utf8_encoding()) {
            len = ucs2_to_charset_allocate(CH_UTF8_MAC, &namebuf, volume->v_u8mname);
        } else {
            len = ucs2_to_charset_allocate(obj->options.maccharset, &namebuf, volume->v_macname);
        }

        if (len == (size_t)-1)
            continue;

        /*
         * There seems to be an undocumented limit on how big our reply can get
         * before the client chokes and closes the connection.
         * Testing with 10.8.4 found the limit at ~4600 bytes. Go figure. 
         */
        if (((data + len + 3) - rbuf) > 4600)
            break;

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
    if ( gettimeofday( &tv, NULL ) < 0 ) {
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

/* ------------------------- */
static int volume_codepage(AFPObj *obj, struct vol *volume)
{
    struct charset_functions *charset;
    /* Codepages */

    if (!volume->v_volcodepage)
        volume->v_volcodepage = strdup("UTF8");

    if ( (charset_t) -1 == ( volume->v_volcharset = add_charset(volume->v_volcodepage)) ) {
        LOG (log_error, logtype_afpd, "Setting codepage %s as volume codepage failed", volume->v_volcodepage);
        return -1;
    }

    if ( NULL == (charset = find_charset_functions(volume->v_volcodepage)) || charset->flags & CHARSET_ICONV ) {
        LOG (log_warning, logtype_afpd, "WARNING: volume encoding %s is *not* supported by netatalk, expect problems !!!!", volume->v_volcodepage);
    }

    if (!volume->v_maccodepage)
        volume->v_maccodepage = strdup(obj->options.maccodepage);

    if ( (charset_t) -1 == ( volume->v_maccharset = add_charset(volume->v_maccodepage)) ) {
        LOG (log_error, logtype_afpd, "Setting codepage %s as mac codepage failed", volume->v_maccodepage);
        return -1;
    }

    if ( NULL == ( charset = find_charset_functions(volume->v_maccodepage)) || ! (charset->flags & CHARSET_CLIENT) ) {
        LOG (log_error, logtype_afpd, "Fatal error: mac charset %s not supported", volume->v_maccodepage);
        return -1;
    }
    volume->v_kTextEncoding = htonl(charset->kTextEncoding);
    return 0;
}

/* ------------------------- */
static int volume_openDB(struct vol *volume)
{
    int flags = 0;

    if ((volume->v_flags & AFPVOL_NODEV)) {
        flags |= CNID_FLAG_NODEV;
    }

    if (volume->v_cnidscheme == NULL) {
        volume->v_cnidscheme = strdup(DEFAULT_CNID_SCHEME);
        LOG(log_info, logtype_afpd, "Volume %s use CNID scheme %s.",
            volume->v_path, volume->v_cnidscheme);
    }

    LOG(log_info, logtype_afpd, "CNID server: %s:%s",
        volume->v_cnidserver ? volume->v_cnidserver : Cnid_srv,
        volume->v_cnidport ? volume->v_cnidport : Cnid_port);

    volume->v_cdb = cnid_open(volume->v_path,
                              volume->v_umask,
                              volume->v_cnidscheme,
                              flags,
                              volume->v_cnidserver ? volume->v_cnidserver : Cnid_srv,
                              volume->v_cnidport ? volume->v_cnidport : Cnid_port);

    if ( ! volume->v_cdb ) {
        LOG(log_error, logtype_afpd, "Can't open volume \"%s\" CNID backend \"%s\" ",
            volume->v_path, volume->v_cnidscheme);
        return -1;
    }

    return (!volume->v_cdb)?-1:0;
}

/*
  Check if the underlying filesystem supports EAs for ea:sys volumes.
  If not, switch to ea:ad.
  As we can't check (requires write access) on ro-volumes, we switch ea:auto
  volumes that are options:ro to ea:none.
*/
static void check_ea_sys_support(struct vol *vol)
{
    uid_t process_uid = 0;
    char eaname[] = {"/tmp/org.netatalk.supports-eas.XXXXXX"};
    const char *eacontent = "yes";
    int fd;

    if (vol->v_vfs_ea == AFPVOL_EA_AUTO) {

        if ((vol->v_flags & AFPVOL_RO) == AFPVOL_RO) {
            LOG(log_info, logtype_afpd, "read-only volume '%s', can't test for EA support, disabling EAs", vol->v_localname);
            vol->v_vfs_ea = AFPVOL_EA_NONE;
            return;
        }

        fd = mkstemp(eaname);
        close(fd);

        process_uid = geteuid();
        if (process_uid)
            if (seteuid(0) == -1) {
                LOG(log_error, logtype_afpd, "check_ea_sys_support: can't seteuid(0): %s", strerror(errno));
                unlink(eaname);
                exit(EXITERR_SYS);
            }

        if ((sys_setxattr(vol->v_path, eaname, eacontent, 4, 0)) == 0) {
            sys_removexattr(vol->v_path, eaname);
            vol->v_vfs_ea = AFPVOL_EA_SYS;
        } else {
            LOG(log_warning, logtype_afpd, "volume \"%s\" does not support Extended Attributes, using ea:ad instead",
                vol->v_localname);
            vol->v_vfs_ea = AFPVOL_EA_AD;
        }

        if (process_uid) {
            if (seteuid(process_uid) == -1) {
                LOG(log_error, logtype_afpd, "can't seteuid back %s", strerror(errno));
                unlink(eaname);
                exit(EXITERR_SYS);
            }
        }

        unlink(eaname);
    }
}

/* -------------------------
 * we are the user here
 */
int afp_openvol(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct stat st;
    char    *volname;
    char        *p;

    struct vol  *volume;
    struct dir  *dir;
    int     len, ret;
    size_t  namelen;
    u_int16_t   bitmap;
    char        path[ MAXPATHLEN + 1];
    char        *vol_uname;
    char        *vol_mname;
    char        *volname_tmp;

    ibuf += 2;
    memcpy(&bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    *rbuflen = 0;
    if (( bitmap & (1<<VOLPBIT_VID)) == 0 ) {
        return AFPERR_BITMAP;
    }

    len = (unsigned char)*ibuf++;
    volname = obj->oldtmp;

    if ((volname_tmp = strchr(volname,'+')) != NULL)
        volname = volname_tmp+1;

    if (utf8_encoding()) {
        namelen = convert_string(CH_UTF8_MAC, CH_UCS2, ibuf, len, volname, sizeof(obj->oldtmp));
    } else {
        namelen = convert_string(obj->options.maccharset, CH_UCS2, ibuf, len, volname, sizeof(obj->oldtmp));
    }

    if ( namelen <= 0) {
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
        return AFPERR_NOOBJ;
    }

    /* check for a volume password */
    if (volume->v_password && strncmp(ibuf, volume->v_password, VOLPASSLEN)) {
        return AFPERR_ACCESS;
    }

    if (( volume->v_flags & AFPVOL_OPEN  ) ) {
        /* the volume is already open */
        return stat_vol(bitmap, volume, rbuf, rbuflen);
    }


    if ( stat( volume->v_path, &st ) < 0 ) {
        return AFPERR_PARAM;
    }

    if ( chdir( volume->v_path ) < 0 ) {
        return AFPERR_PARAM;
    }

    if ( NULL == getcwd(path, MAXPATHLEN)) {
        /* shouldn't be fatal but it will fail later */
        LOG(log_error, logtype_afpd, "afp_openvol(%s): volume pathlen too long", volume->v_path);
        return AFPERR_MISC;
    }

    /* Normalize volume path */
#ifdef REALPATH_TAKES_NULL
    if ((volume->v_path = realpath(path, NULL)) == NULL)
        return AFPERR_MISC;
#else
    if ((volume->v_path = malloc(MAXPATHLEN+1)) == NULL)
        return AFPERR_MISC;
    if (realpath(path, volume->v_path) == NULL) {
        free(volume->v_path);
        return AFPERR_MISC;
    }
    /* Safe some memory */
    char *tmp;
    if ((tmp = strdup(volume->v_path)) == NULL) {
        free(volume->v_path);
        return AFPERR_MISC;
    }
    free(volume->v_path);
    volume->v_path = tmp;
#endif

    if (volume_codepage(obj, volume) < 0) {
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    /* initialize volume variables
     * FIXME file size
     */
    if (utf8_encoding()) {
        volume->max_filename = UTF8FILELEN_EARLY;
    }
    else {
        volume->max_filename = MACFILELEN;
    }

    volume->v_flags |= AFPVOL_OPEN;
    volume->v_cdb = NULL;

    if (utf8_encoding()) {
        len = convert_string_allocate(CH_UCS2, CH_UTF8_MAC, volume->v_u8mname, namelen, &vol_mname);
    } else {
        len = convert_string_allocate(CH_UCS2, obj->options.maccharset, volume->v_macname, namelen, &vol_mname);
    }
    if ( !vol_mname || len <= 0) {
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    if ((vol_uname = strrchr(path, '/')) == NULL)
        vol_uname = path;
    else if (*(vol_uname + 1) != '\0')
        vol_uname++;

    if ((dir = dir_new(vol_mname,
                       vol_uname,
                       volume,
                       DIRDID_ROOT_PARENT,
                       DIRDID_ROOT,
                       bfromcstr(volume->v_path),
                       &st)
            ) == NULL) {
        free(vol_mname);
        LOG(log_error, logtype_afpd, "afp_openvol(%s): malloc: %s", volume->v_path, strerror(errno) );
        ret = AFPERR_MISC;
        goto openvol_err;
    }
    free(vol_mname);
    volume->v_root = dir;
    curdir = dir;

    if (volume_openDB(volume) < 0) {
        LOG(log_error, logtype_afpd, "Fatal error: cannot open CNID or invalid CNID backend for %s: %s",
            volume->v_path, volume->v_cnidscheme);
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    ret  = stat_vol(bitmap, volume, rbuf, rbuflen);

    if (ret == AFP_OK) {
        handle_special_folders(volume);
        savevolinfo(volume,
                    volume->v_cnidserver ? volume->v_cnidserver : Cnid_srv,
                    volume->v_cnidport   ? volume->v_cnidport   : Cnid_port);


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
            cname( volume, volume->v_root, &p );
        }
        return( AFP_OK );
    }

openvol_err:
    if (volume->v_root) {
        dir_free( volume->v_root );
        volume->v_root = NULL;
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
static void closevol(struct vol *vol)
{
    if (!vol)
        return;

    dir_free( vol->v_root );
    vol->v_root = NULL;
    if (vol->v_cdb != NULL) {
        cnid_close(vol->v_cdb);
        vol->v_cdb = NULL;
    }
}

/* ------------------------- */
void close_all_vol(void)
{
    struct vol  *ovol;
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
    struct vol  *ovol;

    vol->v_flags &= ~AFPVOL_OPEN;
    for ( ovol = Volumes; ovol; ovol = ovol->v_next ) {
        if ( (ovol->v_flags & AFPVOL_OPEN) ) {
            break;
        }
    }
    if ( ovol != NULL ) {
        /* Even if chdir fails, we can't say afp_closevol fails. */
        if ( chdir( ovol->v_path ) == 0 ) {
            curdir = ovol->v_root;
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
int afp_closevol(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol  *vol;
    u_int16_t   vid;

    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof( vid ));
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    deletevol(vol);
    current_vol = NULL;

    return( AFP_OK );
}

/* ------------------------- */
struct vol *getvolbyvid(const u_int16_t vid )
{
    struct vol  *vol;

    for ( vol = Volumes; vol; vol = vol->v_next ) {
        if ( vid == vol->v_vid ) {
            break;
        }
    }
    if ( vol == NULL || ( vol->v_flags & AFPVOL_OPEN ) == 0 ) {
        return( NULL );
    }


    current_vol = vol;

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
    char      *p;
    struct extmap *em;

    if (!Extmap_cnt || NULL == ( p = strrchr( path, '.' )) ) {
        return( Defextmap );
    }
    p++;
    if (!*p) {
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
   return
   0 no attention msg sent
   1 attention msg sent
   -1 error (socket closed)

   Note: if attention return -1 no packet has been
   sent because the buffer is full, we don't care
   either there's no reader or there's a lot of
   traffic and another pollvoltime will follow
*/
int  pollvoltime(AFPObj *obj)

{
    struct vol       *vol;
    struct timeval   tv;
    struct stat      st;

    if (!(afp_version > 21 && obj->options.server_notif))
        return 0;

    if ( gettimeofday( &tv, NULL ) < 0 )
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
void setvoltime(AFPObj *obj, struct vol *vol)
{
    struct timeval  tv;

    if ( gettimeofday( &tv, NULL ) < 0 ) {
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
        /* or finder doesn't update free space
         * AFP 3.2 and above clients seem to be ok without so many notification
         */
        if (afp_version < 32 && obj->options.server_notif) {
            obj->attention(obj->handle, AFPATTN_NOTIFY | AFPATTN_VOLCHANGED);
        }
    }
}

/* ------------------------- */
int afp_getvolparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,char *rbuf, size_t *rbuflen)
{
    struct vol  *vol;
    u_int16_t   vid, bitmap;

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
int afp_setvolparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,  size_t *rbuflen)
{
    struct adouble ad;
    struct vol  *vol;
    u_int16_t   vid, bitmap;
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
    char        *p,*q,*r;
    struct adouble  ad;
    u_int16_t   attr;
    struct stat st;
    int     ret;


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
        if (ad_open_metadata( p, ADFLAGS_DIR, O_CREAT, &ad) < 0) {
            free (p);
            free(q);
            return (-1);
        }

        ad_setname(&ad, folder->name);

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
        ad_close_metadata( &ad);
    }
    free(p);
    free(q);
    return 0;
}

static void handle_special_folders (const struct vol * vol)
{
    const _special_folder *p = &special_folders[0];
    uid_t process_uid;

    process_uid = geteuid();
    if (process_uid) {
        if (seteuid(0) == -1) {
            process_uid = 0;
        }
    }

    for (; p->name != NULL; p++) {
        create_special_folder (vol, p);
    }

    if (process_uid) {
        if (seteuid(process_uid) == -1) {
            LOG(log_error, logtype_logger, "can't seteuid back %s", strerror(errno));
            exit(EXITERR_SYS);
        }
    }
}

const struct vol *getvolumes(void)
{
    return Volumes;
}

void unload_volumes_and_extmap(void)
{
    LOG(log_debug, logtype_afpd, "unload_volumes_and_extmap");
    free_extmap();
    free_volumes();
}

/* 
 * Get a volumes UUID from the config file.
 * If there is none, it is generated and stored there.
 *
 * Returns pointer to allocated storage on success, NULL on error.
 */
static char *get_vol_uuid(const AFPObj *obj, const char *volname)
{
    char *volname_conf;
    char buf[1024], uuid[UUID_PRINTABLE_STRING_LENGTH], *p;
    FILE *fp;
    struct stat tmpstat;
    int fd;
    
    if ((fp = fopen(obj->options.uuidconf, "r")) != NULL) {  /* read open? */
        /* scan in the conf file */
        while (fgets(buf, sizeof(buf), fp) != NULL) { 
            p = buf;
            while (p && isblank(*p))
                p++;
            if (!p || (*p == '#') || (*p == '\n'))
                continue;                             /* invalid line */
            if (*p == '"') {
                p++;
                if ((volname_conf = strtok( p, "\"" )) == NULL)
                    continue;                         /* syntax error */
            } else {
                if ((volname_conf = strtok( p, " \t" )) == NULL)
                    continue;                         /* syntax error: invalid name */
            }
            p = strchr(p, '\0');
            p++;
            if (*p == '\0')
                continue;                             /* syntax error */
            
            if (strcmp(volname, volname_conf) != 0)
                continue;                             /* another volume name */
                
            while (p && isblank(*p))
                p++;

            if (sscanf(p, "%36s", uuid) == 1 ) {
                for (int i=0; uuid[i]; i++)
                    uuid[i] = toupper(uuid[i]);
                LOG(log_debug, logtype_afpd, "get_uuid('%s'): UUID: '%s'", volname, uuid);
                fclose(fp);
                return strdup(uuid);
            }
        }
    }

    if (fp)
        fclose(fp);

    /*  not found or no file, reopen in append mode */

    if (stat(obj->options.uuidconf, &tmpstat)) {                /* no file */
        if (( fd = creat(obj->options.uuidconf, 0644 )) < 0 ) {
            LOG(log_error, logtype_afpd, "ERROR: Cannot create %s (%s).",
                obj->options.uuidconf, strerror(errno));
            return NULL;
        }
        if (( fp = fdopen( fd, "w" )) == NULL ) {
            LOG(log_error, logtype_afpd, "ERROR: Cannot fdopen %s (%s).",
                obj->options.uuidconf, strerror(errno));
            close(fd);
            return NULL;
        }
    } else if ((fp = fopen(obj->options.uuidconf, "a+")) == NULL) { /* not found */
        LOG(log_error, logtype_afpd, "Cannot create or append to %s (%s).",
            obj->options.uuidconf, strerror(errno));
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    if(ftell(fp) == 0) {                     /* size = 0 */
        fprintf(fp, "# DON'T TOUCH NOR COPY THOUGHTLESSLY!\n");
        fprintf(fp, "# This file is auto-generated by afpd\n");
        fprintf(fp, "# and stores UUIDs for TM volumes.\n\n");
    } else {
        fseek(fp, -1L, SEEK_END);
        if (fgetc(fp) != '\n')
        {
            /* Required - C11 7.21.5.3p7 */
            fseek(fp, 0L, SEEK_END);
            fputc('\n', fp); /* last char is \n? */
        }
    }                    
    
    /* generate uuid and write to file */
    atalk_uuid_t id = "\0";
    const char *cp;
    randombytes((void *)id, 16);
    cp = uuid_bin2string(id);

    LOG(log_debug, logtype_afpd, "get_uuid('%s'): generated UUID '%s'", volname, cp);

    fprintf(fp, "\"%s\"\t%36s\n", volname, cp);
    fclose(fp);
    
    return strdup(cp);
}
