/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   .volinfo file handling, command line utilities
   copyright Bjoern Fernhomberg, 2004
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef STDC_HEADERS
#include <string.h>
#endif
#include <sys/param.h>

#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/volinfo.h>
#include <atalk/volume.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB*/

static const vol_opt_name_t vol_opt_names[] = {
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
    {AFPVOL_EILSEQ,     "ILLEGALSEQ"},  /* encode illegal sequence */
    {AFPVOL_CACHE,      "CACHEID"},     /* Use adouble v2 CNID caching, default don't use it */
    {AFPVOL_INV_DOTS,   "INVISIBLEDOTS"}, 
    {AFPVOL_ACLS,       "ACLS"},        /* Vol supports ACLs */
    {AFPVOL_TM,         "TM"},          /* Set "kSupportsTMLockSteal" is volume attributes */
    {0, NULL}
};

static const vol_opt_name_t vol_opt_casefold[] = {
    {AFPVOL_MTOUUPPER,  "MTOULOWER"},
    {AFPVOL_MTOULOWER,  "MTOULOWER"},
    {AFPVOL_UTOMUPPER,  "UTOMUPPER"},
    {AFPVOL_UTOMLOWER,  "UTOMLOWER"},
    {0, NULL}
};

typedef struct {
    const char *name;
    int type;
} info_option_t;

#define MAC_CHARSET 0
#define VOL_CHARSET 1
#define ADOUBLE_VER 2
#define CNIDBACKEND 3
#define CNIDDBDHOST 4
#define CNIDDBDPORT 5
#define CNID_DBPATH 6
#define VOLUME_OPTS 7
#define VOLCASEFOLD 8
#define EXTATTRTYPE 9

static const info_option_t info_options[] = {
    {"MAC_CHARSET", MAC_CHARSET},
    {"VOL_CHARSET", VOL_CHARSET},
    {"ADOUBLE_VER", ADOUBLE_VER},
    {"CNIDBACKEND", CNIDBACKEND},
    {"CNIDDBDHOST", CNIDDBDHOST},
    {"CNIDDBDPORT", CNIDDBDPORT},
    {"CNID_DBPATH", CNID_DBPATH},
    {"VOLUME_OPTS", VOLUME_OPTS},
    {"VOLCASEFOLD", VOLCASEFOLD},
    {"EXTATTRTYPE", EXTATTRTYPE},
   {NULL, 0}
};

static char* find_in_path( char *path, char *subdir, size_t maxlen)
{
    char 	*pos;
    struct stat st;

    strlcat(path, subdir, maxlen);
    pos = strrchr(path, '/');

    while ( stat(path, &st) != 0) {
        path[pos-path]=0;
        if ((pos = strrchr(path, '/'))) {
            path[pos-path]=0;
            strlcat(path, subdir, maxlen);
        }
        else {
            return NULL;
        }
    }

    path[pos-path] = '/';
    path[pos-path+1] = 0;

    return path;
}

static char * make_path_absolute(char *path, size_t bufsize)
{
    struct	stat st;
    char	savecwd[MAXPATHLEN];
    char 	abspath[MAXPATHLEN];
    char	*p;

    strlcpy(abspath, path, sizeof(abspath));

    /* we might be called from `ad cp ...` with non existing target */
    if (stat(abspath, &st) != 0) {
        if (errno != ENOENT)
            return NULL;

        if (NULL == (p = strrchr(abspath, '/')) )
            /* single component `ad cp SOURCEFILE TARGETFILE`, use "." instead */
            strcpy(abspath, ".");
        else
            /* try without the last path element */
            *p = '\0';

        if (stat(abspath, &st) != 0) {
            return NULL;
        }
    }

    if (S_ISREG(st.st_mode)) {
        /* single file copy SOURCE */
        if (NULL == (p = strrchr(abspath, '/')) )
            /* no path, use "." instead */
            strcpy(abspath, ".");
        else
            /* try without the last path element */
            *p = '\0';
    }

    if (!getcwd(savecwd, sizeof(savecwd)) || chdir(abspath) < 0)	
        return NULL;

    if (!getcwd(abspath, sizeof(abspath)) || chdir (savecwd) < 0)
        return NULL;
    
    if (strlen(abspath) > bufsize)
        return NULL;

    strlcpy(path, abspath, bufsize);
    return path;
}

static char * find_volumeroot(char *path, size_t maxlen)
{
    char *volume = make_path_absolute(path, maxlen);
        
    if (volume == NULL)
       return NULL;

    if (NULL == (find_in_path(volume, "/.AppleDesktop", maxlen)) )
       return NULL;

    return volume;
}

int vol_load_charsets( struct volinfo *vol)
{
    if ( (charset_t) -1 == ( vol->v_maccharset = add_charset(vol->v_maccodepage)) ) {
        fprintf( stderr, "Setting codepage %s as Mac codepage failed", vol->v_maccodepage);
        return (-1);
    }

    if ( (charset_t) -1 == ( vol->v_volcharset = add_charset(vol->v_volcodepage)) ) {
        fprintf( stderr, "Setting codepage %s as volume codepage failed", vol->v_volcodepage);
        return (-1);
    }

    return 0;
}

static int parse_options (char *buf, int *flags, const vol_opt_name_t *options)
{
    char *p, *q;
    const vol_opt_name_t *op;

    q = p = buf; 

    while ( *p != '\0') {
        if (*p == ' ') {
            *p = '\0';
            op = options;
            for (;op->name; op++) {
                if ( !strcmp(op->name, q )) {
                    *flags |= op->option;
                    break;
                }
            }
            q = p+1;
        }
        p++;
    }

    return 0;
} 
            


static int parseline ( char *buf, struct volinfo *vol)
{
    char *value;
    size_t len;
    int  option=-1;
    const info_option_t  *p  = &info_options[0];

    if (NULL == ( value = strchr(buf, ':')) )
	return 1;

    *value = '\0';
    value++;

    if ( 0 == (len = strlen(value)) )
        return 0;

    if (value[len-1] == '\n')
        value[len-1] = '\0';

    for (;p->name; p++) {
        if ( !strcmp(p->name, buf )) {
            option=p->type;
            break;
        }
    }

    switch (option) {
      case MAC_CHARSET:
        if ((vol->v_maccodepage = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;
        }
        break;
      case VOL_CHARSET:
        if ((vol->v_volcodepage = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;
        }
        break;
      case CNIDBACKEND:
        if ((vol->v_cnidscheme = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;
        }
        break;
      case CNIDDBDHOST:
        if ((vol->v_dbd_host = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;
        }
        break;
      case CNIDDBDPORT:
        if ((vol->v_dbd_port = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;            
        }
        break;
      case CNID_DBPATH:
          if ((vol->v_dbpath = malloc(MAXPATHLEN+1)) == NULL)
              return -1;
          strcpy(vol->v_dbpath, value);
        break;
      case ADOUBLE_VER:
        if (strcasecmp(value, "v1") == 0) {
            vol->v_adouble = AD_VERSION1;
            vol->ad_path = ad_path;
        }
#if AD_VERSION == AD_VERSION2
        else if (strcasecmp(value, "v2") == 0) {
            vol->ad_path = ad_path;
            vol->v_adouble = AD_VERSION2;
        }
        else if (strcasecmp(value, "osx") == 0) {
            vol->v_adouble = AD_VERSION2_OSX;
            vol->ad_path = ad_path_osx;
        }
#endif
        else  {
	    fprintf (stderr, "unknown adouble version: %s, %s", buf, value);
	    return -1;
        }
        break;
      case VOLUME_OPTS:
        parse_options(value, &vol->v_flags, &vol_opt_names[0]);
        break;
      case VOLCASEFOLD:
        parse_options(value, &vol->v_casefold, &vol_opt_casefold[0]);
        break;
    case EXTATTRTYPE:
        if (strcasecmp(value, "AFPVOL_EA_AD") == 0)    
            vol->v_vfs_ea = AFPVOL_EA_AD;
        else if (strcasecmp(value, "AFPVOL_EA_SYS") == 0)
            vol->v_vfs_ea = AFPVOL_EA_SYS;
        break;
      default:
	fprintf (stderr, "unknown volume information: %s, %s", buf, value);
	return (-1);
        break;
    }
        
    return 0;
}
    

int loadvolinfo (char *path, struct volinfo *vol)
{

    char   volinfofile[MAXPATHLEN];
    char   buf[MAXPATHLEN];
    struct flock lock;
    int    fd, len;
    FILE   *fp;

    if ( !path || !vol)
        return -1;

    memset(vol, 0, sizeof(struct volinfo));
    strlcpy(volinfofile, path, sizeof(volinfofile));

    /* volinfo file is in .AppleDesktop */ 
    if ( NULL == find_volumeroot(volinfofile, sizeof(volinfofile)))
        return -1;

    if ((vol->v_path = strdup(volinfofile)) == NULL ) {
        fprintf (stderr, "strdup: %s", strerror(errno));
        return (-1);
    }
    /* Remove trailing slashes */
    len = strlen(vol->v_path);
    while (len && (vol->v_path[len-1] == '/')) {
        vol->v_path[len-1] = 0;
        len--;
    }

    strlcat(volinfofile, ".AppleDesktop/", sizeof(volinfofile));
    strlcat(volinfofile, VOLINFOFILE, sizeof(volinfofile));

    /* open the file read only */
    if ( NULL == (fp = fopen( volinfofile, "r")) )  {
	fprintf (stderr, "error opening volinfo (%s): %s", volinfofile, strerror(errno));
        return (-1);
    }
    fd = fileno(fp); 

    /* try to get a read lock */ 
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type   = F_RDLCK;

    /* wait for read lock */
    if (fcntl(fd, F_SETLKW, &lock) < 0) {
        fclose(fp);
        return (-1);
    }

    /* read the file */
    while (NULL != fgets(buf, sizeof(buf), fp)) {
        parseline(buf, vol);
    }

    /* unlock file */
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);

    /* Translate vol options to ad options like afp/volume.c does it */
    vol->v_ad_options = 0;
    if ((vol->v_flags & AFPVOL_NODEV))
        vol->v_ad_options |= ADVOL_NODEV;
    if ((vol->v_flags & AFPVOL_CACHE))
        vol->v_ad_options |= ADVOL_CACHE;
    if ((vol->v_flags & AFPVOL_UNIX_PRIV))
        vol->v_ad_options |= ADVOL_UNIXPRIV;
    if ((vol->v_flags & AFPVOL_INV_DOTS))
        vol->v_ad_options |= ADVOL_INVDOTS;

    vol->retaincount = 1;

    fclose(fp);
    return 0;
}

/*!
 * Allocate a struct volinfo object for refcounting usage with retain and close, and
 * call loadvolinfo with it
 */
struct volinfo *allocvolinfo(char *path)
{
    struct volinfo *p = malloc(sizeof(struct volinfo));
    if (p == NULL)
        return NULL;

    if (loadvolinfo(path, p) == -1)
        return NULL;

    p->malloced = 1;

    return p;
}

void retainvolinfo(struct volinfo *vol)
{
    vol->retaincount++;
}

/*!
 * Decrement retain count, free resources when retaincount reaches 0
 */
int closevolinfo(struct volinfo *volinfo)
{
    if (volinfo->retaincount <= 0)
        abort();

    volinfo->retaincount--;

    if (volinfo->retaincount == 0) {
        free(volinfo->v_name); volinfo->v_name = NULL;
        free(volinfo->v_path); volinfo->v_path = NULL;
        free(volinfo->v_cnidscheme); volinfo->v_cnidscheme = NULL;
        free(volinfo->v_dbpath); volinfo->v_dbpath = NULL;
        free(volinfo->v_volcodepage); volinfo->v_volcodepage = NULL;
        free(volinfo->v_maccodepage); volinfo->v_maccodepage = NULL;
        free(volinfo->v_dbd_host); volinfo->v_dbd_host = NULL;
        free(volinfo->v_dbd_port); volinfo->v_dbd_port = NULL;
        if (volinfo->malloced) {
            volinfo->malloced = 0;
            free(volinfo);
        }
    }

    return 0;
}

/*
 * Save the volume options to a file, used by shell utilities. Writing the file
 * everytime a volume is opened is unnecessary, but it shouldn't hurt much.
 */
int savevolinfo(const struct vol *vol, const char *Cnid_srv, const char *Cnid_port)
{
    uid_t process_uid;
    char buf[16348];
    char item[MAXPATHLEN];
    int fd;
    int ret = 0;
    struct flock lock;
    const vol_opt_name_t *op = &vol_opt_names[0];
    const vol_opt_name_t *cf = &vol_opt_casefold[0];

    strlcpy (item, vol->v_path, sizeof(item));
    strlcat (item, "/.AppleDesktop/", sizeof(item));
    strlcat (item, VOLINFOFILE, sizeof(item));

    process_uid = geteuid();
    if (process_uid) {
        if (seteuid(0) == -1) {
            process_uid = 0;
        }
    }

    if ((fd = open(item, O_RDWR | O_CREAT , 0666)) <0 ) {
        LOG(log_error, logtype_afpd,"Error opening %s: %s", item, strerror(errno));
        return (-1);
    }

    if (process_uid) {
        if (seteuid(process_uid) == -1) {
            LOG(log_error, logtype_logger, "can't seteuid back %s", strerror(errno));
            exit(EXITERR_SYS);
        }
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

    strlcat(buf, "CNIDDBDPORT:", sizeof(buf));
    strlcat(buf, Cnid_port, sizeof(buf));
    strlcat(buf, "\n", sizeof(buf));

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

    /* ExtendedAttributes */
    strcpy(item, "EXTATTRTYPE:");
    switch (vol->v_vfs_ea) {
    case AFPVOL_EA_SYS:
        strlcat(item, "AFPVOL_EA_SYS\n", sizeof(item));
        break;
    case AFPVOL_EA_AD:
        strlcat(item, "AFPVOL_EA_AD\n", sizeof(item));
        break;
    case AFPVOL_EA_NONE:
        strlcat(item, "AFPVOL_EA_NONE\n", sizeof(item));
        break;
    default:
        strlcat(item, "AFPVOL_EA_UNKNOWN\n", sizeof(item));
    }

    strlcat(buf, item, sizeof(buf));

    if (strlen(buf) >= sizeof(buf)-1)
        LOG(log_debug, logtype_afpd,"Error writing .volinfo file: buffer too small, %s", buf);
   if (write( fd, buf, strlen(buf)) < 0 || ftruncate(fd, strlen(buf)) < 0 ) {
       LOG(log_debug, logtype_afpd,"Error writing .volinfo file: %s", strerror(errno));
   }

   lock.l_type = F_UNLCK;
   fcntl(fd, F_SETLK, &lock);
   close (fd);
   return ret;
}
