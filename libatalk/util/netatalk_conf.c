/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
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
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <time.h>

#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/errchk.h>
#include <atalk/iniparser.h>
#include <atalk/unix.h>
#include <atalk/cnid.h>
#include <atalk/dsi.h>
#include <atalk/uuid.h>
#include <atalk/netatalk_conf.h>

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
#define VOLOPT_PREEXEC       11  /* preexec command */
#define VOLOPT_ROOTPREEXEC   12  /* root preexec command */
#define VOLOPT_POSTEXEC      13  /* postexec command */
#define VOLOPT_ROOTPOSTEXEC  14  /* root postexec command */
#define VOLOPT_ENCODING      15  /* mac encoding (pre OSX)*/
#define VOLOPT_MACCHARSET    16
#define VOLOPT_CNIDSCHEME    17
#define VOLOPT_ADOUBLE       18  /* adouble version */
/* Usable slot: 19/20 */
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

#define IS_VAR(a, b) (strncmp((a), (b), 2) == 0)

struct vol_option {
    char *c_value;
    int i_value;
};

/**************************************************************
 * Locals
 **************************************************************/

static struct vol *Volumes = NULL;
static uint16_t    lastvid = 0;

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
        if(fgetc(fp) != '\n') fputc('\n', fp); /* last char is \n? */
    }                    
    
    /* generate uuid and write to file */
    atalk_uuid_t id;
    const char *cp;
    randombytes((void *)id, 16);
    cp = uuid_bin2string(id);

    LOG(log_debug, logtype_afpd, "get_uuid('%s'): generated UUID '%s'", volname, cp);

    fprintf(fp, "\"%s\"\t%36s\n", volname, cp);
    fclose(fp);
    
    return strdup(cp);
}

/*
  Check if the underlying filesystem supports EAs.
  If not, switch to ea:ad.
  As we can't check (requires write access) on ro-volumes, we switch ea:auto
  volumes that are options:ro to ea:none.
*/
static int do_check_ea_support(const struct vol *vol)
{
    int haseas;
    char eaname[] = {"org.netatalk.supports-eas.XXXXXX"};
    const char *eacontent = "yes";

    if ((vol->v_flags & AFPVOL_RO) == AFPVOL_RO) {
        LOG(log_note, logtype_afpd, "read-only volume '%s', can't test for EA support, assuming yes", vol->v_localname);
        return 1;
    }

    mktemp(eaname);

    become_root();

    if ((sys_setxattr(vol->v_path, eaname, eacontent, 4, 0)) == 0) {
        sys_removexattr(vol->v_path, eaname);
        haseas = 1;
    } else {
        LOG(log_warning, logtype_afpd, "volume \"%s\" does not support Extended Attributes",
            vol->v_localname);
        haseas = 0;
    }

    unbecome_root();

    return haseas;
}

static void check_ea_support(struct vol *vol)
{
    int haseas;
    char eaname[] = {"org.netatalk.supports-eas.XXXXXX"};
    const char *eacontent = "yes";

    haseas = do_check_ea_support(vol);

    if (vol->v_vfs_ea == AFPVOL_EA_AUTO) {
        if ((vol->v_flags & AFPVOL_RO) == AFPVOL_RO) {
            LOG(log_info, logtype_afpd, "read-only volume '%s', can't test for EA support, disabling EAs", vol->v_localname);
            vol->v_vfs_ea = AFPVOL_EA_NONE;
            return;
        }

        if (haseas) {
            vol->v_vfs_ea = AFPVOL_EA_SYS;
        } else {
            LOG(log_warning, logtype_afpd, "volume \"%s\" does not support Extended Attributes, using ea:ad instead",
                vol->v_localname);
            vol->v_vfs_ea = AFPVOL_EA_AD;
        }
    }

    if (vol->v_adouble == AD_VERSION_EA) {
        if (!haseas)
            vol->v_adouble = AD_VERSION2;
    }
}

/*!
 * Check whether a volume supports ACLs
 *
 * @param vol  (r) volume
 *
 * @returns        0 if not, 1 if yes
 */
static int check_vol_acl_support(const struct vol *vol)
{
    int ret = 0;

#ifdef HAVE_SOLARIS_ACLS
    ace_t *aces = NULL;
    ret = 1;
    if (get_nfsv4_acl(vol->v_path, &aces) == -1)
        ret = 0;
#endif
#ifdef HAVE_POSIX_ACLS
    acl_t acl = NULL;
    ret = 1;
    if ((acl = acl_get_file(vol->v_path, ACL_TYPE_ACCESS)) == NULL)
        ret = 0;
#endif

#ifdef HAVE_SOLARIS_ACLS
    if (aces) free(aces);
#endif
#ifdef HAVE_POSIX_ACLS
    if (acl) acl_free(acl);
#endif /* HAVE_POSIX_ACLS */

    LOG(log_debug, logtype_afpd, "Volume \"%s\" ACL support: %s",
        vol->v_path, ret ? "yes" : "no");
    return ret;
}

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
static char *volxlate(const AFPObj *obj,
                      char *dest,
                      size_t destlen,
                      char *src,
                      const struct passwd *pwd,
                      char *path,
                      char *volname)
{
    char *p, *r;
    const char *q;
    int len;
    char *ret;
    int xlatevolname = 0;

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
        if (IS_VAR(p, "$b")) {
            if (!obj->uid && xlatevolname)
                return NULL;
            if (path) {
                if ((q = strrchr(path, '/')) == NULL)
                    q = path;
                else if (*(q + 1) != '\0')
                    q++;
            }
        } else if (IS_VAR(p, "$c")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            DSI *dsi = obj->dsi;
            len = sprintf(dest, "%s:%u",
                          getip_string((struct sockaddr *)&dsi->client),
                          getip_port((struct sockaddr *)&dsi->client));
            dest += len;
            destlen -= len;
        } else if (IS_VAR(p, "$d")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            q = path;
        } else if (pwd && IS_VAR(p, "$f")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            if ((r = strchr(pwd->pw_gecos, ',')))
                *r = '\0';
            q = pwd->pw_gecos;
        } else if (pwd && IS_VAR(p, "$g")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            struct group *grp = getgrgid(pwd->pw_gid);
            if (grp)
                q = grp->gr_name;
        } else if (IS_VAR(p, "$h")) {
            q = obj->options.hostname;
        } else if (IS_VAR(p, "$i")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            DSI *dsi = obj->dsi;
            q = getip_string((struct sockaddr *)&dsi->client);
        } else if (IS_VAR(p, "$s")) {
            q = obj->options.hostname;
        } else if (obj->username && IS_VAR(p, "$u")) {
            if (!obj->uid  && xlatevolname)
                return NULL;
            char* sep = NULL;
            if ( obj->options.ntseparator && (sep = strchr(obj->username, obj->options.ntseparator[0])) != NULL)
                q = sep+1;
            else
                q = obj->username;
        } else if (IS_VAR(p, "$v")) {
            if (!obj->uid  && xlatevolname)
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
        } else if (IS_VAR(p, "$$")) {
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

/* -------------------- */
static void setoption(struct vol_option *options, const struct vol_option *save, int opt, const char *val)
{
    if (options[opt].c_value && (!save || options[opt].c_value != save[opt].c_value))
        free(options[opt].c_value);
    options[opt].c_value = strdup(val);
}

/* Parse iniconfig and initalize volume options */
static void volset(const dictionary *conf, const char *vol, struct vol_option *options, const struct vol_option *save)
{
    const char *val;
    char *p, *q;

    if (val = iniparser_getstring(conf, vol, "allow", NULL))
        setoption(options, save, VOLOPT_ALLOW, val);

    if (val = iniparser_getstring(conf, vol, "deny", NULL))
        setoption(options, save, VOLOPT_DENY, val);

    if (val = iniparser_getstring(conf, vol, "rwlist", NULL))
        setoption(options, save, VOLOPT_RWLIST, val);

    if (val = iniparser_getstring(conf, vol, "rolist", NULL))
        setoption(options, save, VOLOPT_ROLIST, val);

    if (val = iniparser_getstring(conf, vol, "volcharset", NULL))
        setoption(options, save, VOLOPT_ENCODING, val);

    if (val = iniparser_getstring(conf, vol, "maccharset", NULL))
        setoption(options, save, VOLOPT_MACCHARSET, val);

    if (val = iniparser_getstring(conf, vol, "veto", NULL))
        setoption(options, save, VOLOPT_VETO, val);

    if (val = iniparser_getstring(conf, vol, "cnidscheme", NULL))
        setoption(options, save, VOLOPT_CNIDSCHEME, val);

    if (val = iniparser_getstring(conf, vol, "dbpath", NULL))
        setoption(options, save, VOLOPT_DBPATH, val);

    if (val = iniparser_getstring(conf, vol, "password", NULL))
        setoption(options, save, VOLOPT_PASSWORD, val);

    if (val = iniparser_getstring(conf, vol, "root_preexec", NULL))
        setoption(options, save, VOLOPT_ROOTPREEXEC, val);

    if (val = iniparser_getstring(conf, vol, "preexec", NULL))
        setoption(options, save, VOLOPT_PREEXEC, val);

    if (val = iniparser_getstring(conf, vol, "root_postexec", NULL))
        setoption(options, save, VOLOPT_ROOTPOSTEXEC, val);

    if (val = iniparser_getstring(conf, vol, "postexec", NULL))
        setoption(options, save, VOLOPT_POSTEXEC, val);

    if (val = iniparser_getstring(conf, vol, "allowed_hosts", NULL))
        setoption(options, save, VOLOPT_ALLOWED_HOSTS, val);

    if (val = iniparser_getstring(conf, vol, "denied_hosts", NULL))
        setoption(options, save, VOLOPT_DENIED_HOSTS, val);

    if (val = iniparser_getstring(conf, vol, "umask", NULL))
        options[VOLOPT_UMASK].i_value = (int)strtol(val, NULL, 8);

    if (val = iniparser_getstring(conf, vol, "dperm", NULL))
        options[VOLOPT_DPERM].i_value = (int)strtol(val, NULL, 8);

    if (val = iniparser_getstring(conf, vol, "fperm", NULL))
        options[VOLOPT_FPERM].i_value = (int)strtol(val, NULL, 8);

    if (val = iniparser_getstring(conf, vol, "perm", NULL))
        options[VOLOPT_DFLTPERM].i_value = (int)strtol(val, NULL, 8);

    if (val = iniparser_getstring(conf, vol, "volsizelimit", NULL))
        options[VOLOPT_LIMITSIZE].i_value = (uint32_t)strtoul(val, NULL, 10);

    if (val = iniparser_getstring(conf, vol, "casefold", NULL)) {
        if (strcasecmp(val, "tolower") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UMLOWER;
        else if (strcasecmp(val, "toupper") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UMUPPER;
        else if (strcasecmp(val, "xlatelower") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_UUPPERMLOWER;
        else if (strcasecmp(val, "xlateupper") == 0)
            options[VOLOPT_CASEFOLD].i_value = AFPVOL_ULOWERMUPPER;
    }

    if (val = iniparser_getstring(conf, vol, "adouble", NULL)) {
        if (strcasecmp(val, "v2") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION2;
        else if (strcasecmp(val, "ea") == 0)
            options[VOLOPT_ADOUBLE].i_value = AD_VERSION_EA;
    }

    if (val = iniparser_getstring(conf, vol, "ea", NULL)) {
        if (strcasecmp(val, "ad") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_AD;
        else if (strcasecmp(val, "sys") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_SYS;
        else if (strcasecmp(val, "none") == 0)
            options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_NONE;
    }

    if (p = iniparser_getstrdup(conf, vol, "cnidserver", NULL)) {
        if (q = strrchr(val, ':')) {
            *q = 0;
            setoption(options, save, VOLOPT_CNIDPORT, q + 1);
        }
        setoption(options, save, VOLOPT_CNIDSERVER, p);
        LOG(log_debug, logtype_afpd, "CNID Server for volume '%s': %s:%s",
            vol, p, q ? q + 1 : "4700");
        free(p);
    }

    if (q = iniparser_getstrdup(conf, vol, "options", NULL)) {
        if (p = strtok(q, ", ")) {
            while (p) {
                if (strcasecmp(p, "ro") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_RO;
                else if (strcasecmp(p, "nohex") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_NOHEX;
                else if (strcasecmp(p, "nousedots") == 0)
                    options[VOLOPT_FLAGS].i_value &= ~AFPVOL_USEDOTS;
                else if (strcasecmp(p, "invisibledots") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_USEDOTS;
                else if (strcasecmp(p, "nostat") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_NOSTAT;
                else if (strcasecmp(p, "preexec_close") == 0)
                    options[VOLOPT_PREEXEC].i_value = 1;
                else if (strcasecmp(p, "root_preexec_close") == 0)
                    options[VOLOPT_ROOTPREEXEC].i_value = 1;
                else if (strcasecmp(p, "noupriv") == 0)
                    options[VOLOPT_FLAGS].i_value &= ~AFPVOL_UNIX_PRIV;
                else if (strcasecmp(p, "nodev") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_NODEV;
                else if (strcasecmp(p, "caseinsensitive") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_CASEINSEN;
                else if (strcasecmp(p, "illegalseq") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_EILSEQ;
                else if (strcasecmp(p, "tm") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_TM;
                else if (strcasecmp(p, "searchdb") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_SEARCHDB;
                else if (strcasecmp(p, "nonetids") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_NONETIDS;
                else if (strcasecmp(p, "noacls") == 0)
                    options[VOLOPT_FLAGS].i_value &= ~AFPVOL_ACLS;
                else if (strcasecmp(p, "nov2toeaconv") == 0)
                    options[VOLOPT_FLAGS].i_value |= AFPVOL_NOV2TOEACONV;
                p = strtok(NULL, ", ");
            }
        }
        free(q);
    }
}

/* ------------------------------- */
static int creatvol(const AFPObj *obj, const struct passwd *pwd,
                    char *path, char *name,
                    struct vol_option *options)
{
    struct vol  *volume;
    int         suffixlen, vlen, tmpvlen, u8mvlen, macvlen;
    int         hide = 0;
    char        tmpname[AFPVOL_U8MNAMELEN+1];
    ucs2_t      u8mtmpname[(AFPVOL_U8MNAMELEN+1)*2], mactmpname[(AFPVOL_MACNAMELEN+1)*2];
    char        suffix[6]; /* max is #FFFF */
    uint16_t   flags;

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
    suffixlen = sprintf(suffix, "#%X", lastvid + 1 );

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
        if ((utf8_encoding(obj) && (strcasecmp_w(volume->v_u8mname, u8mtmpname) == 0))
             ||
            (!utf8_encoding(obj) && (strcasecmp_w(volume->v_macname, mactmpname) == 0))) {
            LOG (log_error, logtype_afpd,
                 "Duplicate volume name, check AppleVolumes files: previous: \"%s\", new: \"%s\"",
                 volume->v_localname, name);
            if (volume->v_deleted) {
                volume->v_new = hide = 1;
            }
            else {
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

    volume->v_name = utf8_encoding(obj)?volume->v_u8mname:volume->v_macname;
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
    volume->v_dperm |= volume->v_perm;
    volume->v_fperm |= volume->v_perm;

    /* Check EA support on volume */
    if (volume->v_vfs_ea == AFPVOL_EA_AUTO || volume->v_adouble == AD_VERSION_EA)
        check_ea_support(volume);
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
    volume->v_obj = obj;
    return 0;
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

static int accessvol(const AFPObj *obj, const char *args, const char *name)
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
            if ((gr = getgrnam(p + 1)) && gmem(gr->gr_gid, obj->ngroups, obj->groups))
                return 1;
        } else if (access_strcmp(p, name) == 0) /* it's a user name */
            return 1;
        p = strtok(NULL, ",");
    }

    return 0;
}

static int hostaccessvol(const AFPObj *obj, int type, const char *volname, const char *args)
{
    int mask_int;
    char buf[MAXPATHLEN + 1], *p, *b;
    struct sockaddr_storage client;
    const DSI *dsi = obj->dsi;

    if (!args)
        return -1;

    strlcpy(buf, args, sizeof(buf));
    if ((p = strtok_r(buf, ",", &b)) == NULL) /* nothing, return okay */
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

/* ----------------------
 */
static int volfile_changed(struct afp_options *p)
{
    struct stat      st;

    if (!stat(p->configfile, &st) && st.st_mtime > p->volfile.mtime) {
        p->volfile.mtime = st.st_mtime;
        return 1;
    }
    return 0;
}

static int vol_section(const char *sec)
{
    if (STRCMP(sec, ==, INISEC_GLOBAL))
        return 0;
    if (STRCMP(sec, ==, INISEC_AFP))
        return 0;
    if (STRCMP(sec, ==, INISEC_CNID))
        return 0;
    return 1;
}

/* ----------------------
 * Read volumes from iniconfig and add the volumes contained within to
 * the global volume list. This gets called from the forked afpd childs.
 * The master now reads this too for Zeroconf announcements.
 */
static int readvolfile(const AFPObj *obj, const struct passwd *pwent)
{
    char        path[MAXPATHLEN + 1];
    char        volname[AFPVOL_U8MNAMELEN + 1];
    char        tmp[MAXPATHLEN + 1];
    char        *u;
    const char  *p;
    int         fd;
    int         i;
    struct passwd     *pw;
    struct vol_option save_options[VOLOPT_NUM];
    struct vol_option default_options[VOLOPT_NUM];
    struct vol_option options[VOLOPT_NUM];

    LOG(log_debug, logtype_afpd, "readvolfile: BEGIN");

    memset(default_options, 0, sizeof(default_options));

    /* Enable some default options for all volumes */
    default_options[VOLOPT_FLAGS].i_value |= AFPVOL_USEDOTS | AFPVOL_UNIX_PRIV;
#ifdef HAVE_ACLS
    default_options[VOLOPT_FLAGS].i_value |= AFPVOL_ACLS;
#endif
    default_options[VOLOPT_EA_VFS].i_value = AFPVOL_EA_AUTO;
    default_options[VOLOPT_UMASK].i_value = obj->options.umask;
    memcpy(save_options, default_options, sizeof(options));

    int secnum = iniparser_getnsec(obj->iniconfig);    
    LOG(log_debug, logtype_afpd, "readvolfile: sections: %d", secnum);
    const char *secname;

    for (i = 0; i < secnum; i++) { 
        secname = iniparser_getsecname(obj->iniconfig, i);
        if (!vol_section(secname))
            continue;

        strlcpy(volname, secname, AFPVOL_U8MNAMELEN);
        LOG(log_debug, logtype_afpd, "readvolfile: volume: %s", volname);

        if ((p = iniparser_getstrdup(obj->iniconfig, secname, "path", NULL)) == NULL)
            continue;
        strlcpy(path, p, MAXPATHLEN);
        strcpy(tmp, path);

        if (volxlate(obj, path, sizeof(path) - 1, tmp, pwent, NULL, NULL) == NULL)
            continue;

        memcpy(options, default_options, sizeof(options));
        volset(obj->iniconfig, secname, options, default_options);

        /* check allow/deny lists (if not afpd master loading volumes for Zeroconf reg.):
           allow -> either no list (-1), or in list (1)
           deny -> either no list (-1), or not in list (0) */

        if (pwent) {
            if (accessvol(obj, options[VOLOPT_DENY].c_value, pwent->pw_name) == 1)
                continue;
            if (accessvol(obj, options[VOLOPT_ALLOW].c_value, pwent->pw_name) == 0)
                continue;
            if (hostaccessvol(obj, VOLOPT_DENIED_HOSTS, volname, options[VOLOPT_DENIED_HOSTS].c_value) == 1)
                continue;
            if (hostaccessvol(obj, VOLOPT_ALLOWED_HOSTS, volname, options[VOLOPT_ALLOWED_HOSTS].c_value) == 0)
                continue;
        }

            /* handle read-only behaviour. semantics:
             * 1) neither the rolist nor the rwlist exist -> rw
             * 2) rolist exists -> ro if user is in it.
             * 3) rwlist exists -> ro unless user is in it. */
        if (pwent && !(options[VOLOPT_FLAGS].i_value & AFPVOL_RO)) {
            if (accessvol(obj, options[VOLOPT_ROLIST].c_value, pwent->pw_name) == 1
                || accessvol(obj, options[VOLOPT_RWLIST].c_value, pwent->pw_name) == 0)
                options[VOLOPT_FLAGS].i_value |= AFPVOL_RO;
        }

        /* do variable substitution for volname */
        if (volxlate(obj, tmp, sizeof(tmp) - 1, volname, pwent, path, NULL) == NULL) {
            volfree(options, default_options);
            continue;
        }

        creatvol(obj, pwent, path, tmp, options);
        volfree(options, default_options);
    }
    volfree(save_options, NULL);
    return( 0 );
}

/* ------------------------------- */
static void free_volumes(void)
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

/**************************************************************
 * API functions
 **************************************************************/

/*!
 * Remove a volume from the linked list of volumes
 */
void volume_unlink(struct vol *volume)
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
 * Free all resources allocated in a struct vol
 */
void volume_free(struct vol *vol)
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

/*!
 * Initialize volumes and load ini configfile
 *
 * Depending on the value of obj->uid either access checks are done (!=0) or skipped (=0)
 *
 * @param obj       (r) handle
 * @param delvol_fn (r) callback called for deleted volumes
 */
int load_volumes(AFPObj *obj, void (*delvol_fn)(const struct vol *))
{
    EC_INIT;
    int fd = -1;
    struct passwd   *pwent = NULL;
    struct stat         st;
    int retries = 0;
    struct vol *vol;

    LOG(log_debug, logtype_afpd, "load_volumes: BEGIN");

    if (Volumes) {
        if (!volfile_changed(&obj->options))
            goto EC_CLEANUP;
        /* TODO: volume reloading */
//        free_volumes();
            goto EC_CLEANUP;
    }

    /* try putting a read lock on the volume file twice, sleep 1 second if first attempt fails */

    fd = open(obj->options.configfile, O_RDONLY);

    while (retries < 2) {
        if ((read_lock(fd, 0, SEEK_SET, 0)) != 0) {
            retries++;
            if (!retries) {
                LOG(log_error, logtype_afpd, "readvolfile: can't lock configfile \"%s\"",
                    obj->options.configfile);
                EC_FAIL;
            }
            sleep(1);
            continue;
        }
        break;
    }

    if (obj->uid)
        pwent = getpwuid(obj->uid);

    if (obj->iniconfig)
        iniparser_freedict(obj->iniconfig);
    LOG(log_debug, logtype_afpd, "load_volumes: reloading: %s", obj->options.configfile);
    obj->iniconfig = iniparser_load(obj->options.configfile);

    EC_ZERO_LOG( readvolfile(obj, pwent) );

    for ( vol = Volumes; vol; vol = vol->v_next ) {
        if (vol->v_deleted && !vol->v_new ) {
            delvol_fn(vol);
            vol = Volumes;
        }
    }

EC_CLEANUP:
    if (fd != -1)
        (void)close(fd);
    EC_EXIT;
}

void unload_volumes(void)
{
    LOG(log_debug, logtype_afpd, "unload_volumes");
    free_volumes();
}

struct vol *getvolumes(void)
{
    return Volumes;
}

struct vol *getvolbyvid(const uint16_t vid )
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

    return( vol );
}

struct vol *getvolbypath(const char *path)
{
    struct vol *vol = NULL;
    struct vol *tmp;

    for (tmp = Volumes; tmp; tmp = tmp->v_next) {
        if (strncmp(path, tmp->v_path, strlen(tmp->v_path)) == 0) {
            vol = tmp;
            break;
        }
    }
    return vol;
}

#define MAXVAL 1024
/*!
 * Initialize an AFPObj and options from ini config file
 */
int afp_config_parse(AFPObj *AFPObj)
{
    EC_INIT;
    dictionary *config;
    struct afp_options *options = &AFPObj->options;
    int i, c;
    const char *p, *tmp;
    char *q, *r;
    char val[MAXVAL];

    AFPObj->afp_version = 11;
    options->configfile  = AFPObj->cmdlineconfigfile ? strdup(AFPObj->cmdlineconfigfile) : strdup(_PATH_CONFDIR "afp.conf");
    options->sigconffile = strdup(_PATH_CONFDIR "afp_signature.conf");
    options->uuidconf    = strdup(_PATH_CONFDIR "afp_voluuid.conf");
    options->flags       = OPTION_ACL2MACCESS | OPTION_UUID | OPTION_SERVERNOTIF | AFPObj->cmdlineflags;
    
    if ((config = iniparser_load(AFPObj->options.configfile)) == NULL)
        return -1;
    AFPObj->iniconfig = config;

    /* [Global] */
    options->logconfig = iniparser_getstrdup(config, INISEC_GLOBAL, "loglevel", "default:note");
    options->logfile   = iniparser_getstrdup(config, INISEC_GLOBAL, "logfile",  NULL);

    /* [AFP] "options" options wo values */
    if (p = iniparser_getstrdup(config, INISEC_AFP, "options", NULL)) {
        if (p = strtok(q, ", ")) {
            while (p) {
                if (strcasecmp(p, "nozeroconf"))
                    options->flags |= OPTION_NOZEROCONF;
                if (strcasecmp(p, "icon"))
                    options->flags |= OPTION_CUSTOMICON;
                if (strcasecmp(p, "noicon"))
                    options->flags &= ~OPTION_CUSTOMICON;
                if (strcasecmp(p, "advertise_ssh"))
                    options->flags |= OPTION_ANNOUNCESSH;
                if (strcasecmp(p, "noacl2maccess"))
                    options->flags &= ~OPTION_ACL2MACCESS;
                if (strcasecmp(p, "keepsessions"))
                    options->flags |= OPTION_KEEPSESSIONS;
                if (strcasecmp(p, "closevol"))
                    options->flags |= OPTION_CLOSEVOL;
                if (strcasecmp(p, "client_polling"))
                    options->flags &= ~OPTION_SERVERNOTIF;
                if (strcasecmp(p, "nosavepassword"))
                    options->passwdbits |= PASSWD_NOSAVE;
                if (strcasecmp(p, "savepassword"))
                    options->passwdbits &= ~PASSWD_NOSAVE;
                if (strcasecmp(p, "nosetpassword"))
                    options->passwdbits &= ~PASSWD_SET;
                if (strcasecmp(p, "setpassword"))
                    options->passwdbits |= PASSWD_SET;
                p = strtok(NULL, ", ");
            }
        }
    }
    /* figure out options w values */

    options->loginmesg      = iniparser_getstrdup(config, INISEC_AFP, "loginmesg",      "");
    options->guest          = iniparser_getstrdup(config, INISEC_AFP, "guestname",      "nobody");
    options->passwdfile     = iniparser_getstrdup(config, INISEC_AFP, "passwdfile",     _PATH_AFPDPWFILE);
    options->uampath        = iniparser_getstrdup(config, INISEC_AFP, "uampath",        _PATH_AFPDUAMPATH);
    options->uamlist        = iniparser_getstrdup(config, INISEC_AFP, "uamlist",        "uams_dhx.so,uams_dhx2.so");
    options->port           = iniparser_getstrdup(config, INISEC_AFP, "port",           "548");
    options->signatureopt   = iniparser_getstrdup(config, INISEC_AFP, "signature",      "auto");
    options->k5service      = iniparser_getstrdup(config, INISEC_AFP, "k5service",      NULL);
    options->k5realm        = iniparser_getstrdup(config, INISEC_AFP, "k5realm",        NULL);
    options->listen         = iniparser_getstrdup(config, INISEC_AFP, "listen",         NULL);
    options->ntdomain       = iniparser_getstrdup(config, INISEC_AFP, "ntdomain",       NULL);
    options->ntseparator    = iniparser_getstrdup(config, INISEC_AFP, "ntseparator",    NULL);
    options->mimicmodel     = iniparser_getstrdup(config, INISEC_AFP, "mimicmodel",     NULL);
    options->adminauthuser  = iniparser_getstrdup(config, INISEC_AFP, "adminauthuser",  NULL);
    options->connections    = iniparser_getint   (config, INISEC_AFP, "maxcon",         200);
    options->passwdminlen   = iniparser_getint   (config, INISEC_AFP, "passwdminlen",   0);
    options->tickleval      = iniparser_getint   (config, INISEC_AFP, "tickleval",      30);
    options->timeout        = iniparser_getint   (config, INISEC_AFP, "timeout",        4);
    options->dsireadbuf     = iniparser_getint   (config, INISEC_AFP, "dsireadbuf",     12);
    options->server_quantum = iniparser_getint   (config, INISEC_AFP, "server_quantum", DSI_SERVQUANT_DEF);
    options->volnamelen     = iniparser_getint   (config, INISEC_AFP, "volnamelen",     80);
    options->dircachesize   = iniparser_getint   (config, INISEC_AFP, "dircachesize",   DEFAULT_MAX_DIRCACHE_SIZE);
    options->tcp_sndbuf     = iniparser_getint   (config, INISEC_AFP, "tcpsndbuf",      0);
    options->tcp_rcvbuf     = iniparser_getint   (config, INISEC_AFP, "tcprcvbuf",      0);
    options->fce_fmodwait   = iniparser_getint   (config, INISEC_AFP, "fceholdfmod",    60);
    options->sleep          = iniparser_getint   (config, INISEC_AFP, "sleep",          10) * 60 * 2;
    options->disconnected   = iniparser_getint   (config, INISEC_AFP, "disconnect",     24) * 60 * 2;

    if ((p = iniparser_getstring(config, INISEC_AFP, "hostname", NULL))) {
        EC_NULL_LOG( options->hostname = strdup(p) );
    } else {
        if (gethostname(val, sizeof(val)) < 0 ) {
            perror( "gethostname" );
            EC_FAIL;
        }
        if ((q = strchr(val, '.')))
            *q = '\0';
        options->hostname = strdup(val);
    }

    if ((p = iniparser_getstring(config, INISEC_AFP, "k5keytab", NULL))) {
        EC_NULL_LOG( options->k5keytab = malloc(strlen(p) + 14) );
        snprintf(options->k5keytab, strlen(p) + 14, "KRB5_KTNAME=%s", p);
        putenv(options->k5keytab);
    }

#ifdef ADMIN_GRP
    if ((p = iniparser_getstring(config, INISEC_AFP, "admingroup",  NULL))) {
         struct group *gr = getgrnam(p);
         if (gr != NULL)
             options->admingid = gr->gr_gid;
    }
#endif /* ADMIN_GRP */

    q = iniparser_getstrdup(config, INISEC_AFP, "cnidserver", "localhost:4700");
    r = strrchr(q, ':');
    if (r)
        *r = 0;
    options->Cnid_srv = strdup(q);
    if (r)
        options->Cnid_port = strdup(r + 1);
    else
        options->Cnid_port = strdup("4700");
    LOG(log_debug, logtype_afpd, "CNID Server: %s:%s", options->Cnid_srv, options->Cnid_port);
    if (q)
        free(q);

    if ((q = iniparser_getstrdup(config, INISEC_AFP, "fqdn", NULL))) {
        /* do a little checking for the domain name. */
        r = strchr(q, ':');
        if (r)
            *r = '\0';
        if (gethostbyname(q)) {
            if (r)
                *r = ':';
            EC_NULL_LOG( options->fqdn = strdup(q) );
        } else {
            LOG(log_error, logtype_afpd, "error parsing -fqdn, gethostbyname failed for: %s", c);
        }
        free(q);
    }

    if (!(p = iniparser_getstring(config, INISEC_AFP, "unixcodepage", NULL))) {
        options->unixcharset = CH_UNIX;
        options->unixcodepage = strdup("LOCALE");
    } else {
        if ((options->unixcharset = add_charset(p)) == (charset_t)-1) {
            options->unixcharset = CH_UNIX;
            options->unixcodepage = strdup("LOCALE");
            LOG(log_warning, logtype_afpd, "Setting Unix codepage to '%s' failed", p);
        } else {
            options->unixcodepage = strdup(p);
        }
    }
	
    if (!(p = iniparser_getstring(config, INISEC_AFP, "maccodepage", NULL))) {
        options->maccharset = CH_MAC;
        options->maccodepage = strdup("MAC_ROMAN");
    } else {
        if ((options->maccharset = add_charset(p)) == (charset_t)-1) {
            options->maccharset = CH_MAC;
            options->maccodepage = strdup("MAC_ROMAN");
            LOG(log_warning, logtype_afpd, "Setting Mac codepage to '%s' failed", p);
        } else {
            options->maccodepage = strdup(p);
        }
    }

    /* Check for sane values */
    if (options->tickleval <= 0)
        options->tickleval = 30;
    if (options->timeout <= 0)
        options->timeout = 4;
    if (options->sleep <= 4)
        options->disconnected = options->sleep = 4;
    if (options->dsireadbuf < 6)
        options->dsireadbuf = 6;
    if (options->volnamelen < 8)
        options->volnamelen = 8; /* max mangled volname "???#FFFF" */
    if (options->volnamelen > 255)
	    options->volnamelen = 255; /* AFP3 spec */

EC_CLEANUP:
    EC_EXIT;
}

