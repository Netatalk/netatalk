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
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB*/

#define MAC_CHARSET 0
#define VOL_CHARSET 1
#define ADOUBLE_VER 2
#define CNIDBACKEND 3
#define CNIDDBDHOST 4
#define CNIDDBDPORT 5
#define CNID_DBPATH 6
#define VOLUME_OPTS 7
#define VOLCASEFOLD 8

typedef struct _info_option {
    const char *name;
    int type;
} _info_option;

static const _info_option info_options[] = {
    {"MAC_CHARSET", MAC_CHARSET},
    {"VOL_CHARSET", VOL_CHARSET},
    {"ADOUBLE_VER", ADOUBLE_VER},
    {"CNIDBACKEND", CNIDBACKEND},
    {"CNIDDBDHOST", CNIDDBDHOST},
    {"CNIDDBDPORT", CNIDDBDPORT},
    {"CNID_DBPATH", CNID_DBPATH},
    {"VOLUME_OPTS", VOLUME_OPTS},
    {"VOLCASEFOLD", VOLCASEFOLD},
    {NULL, 0}
};

typedef struct _vol_opt_name {
    const u_int32_t option;
    const char      *name;
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
    {AFPVOL_MAPASCII,	"MAPASCII"},	/* map the ascii range as well */
    {AFPVOL_DROPBOX,	"DROPBOX"},	/* dropkludge dropbox support */
    {AFPVOL_NOFILEID,	"NOFILEID"},	/* don't advertise createid resolveid and deleteid calls */
    {AFPVOL_NOSTAT,	"NOSTAT"},	/* advertise the volume even if we can't stat() it
					 * maybe because it will be mounted later in preexec */
    {AFPVOL_UNIX_PRIV,  "UNIXPRIV"},    /* support unix privileges */
    {AFPVOL_NODEV,      "NODEV"},       /* always use 0 for device number in cnid calls */
    {0, NULL}
};

static const _vol_opt_name vol_opt_casefold[] = {
    {AFPVOL_MTOUUPPER,  "MTOULOWER"},
    {AFPVOL_MTOULOWER,  "MTOULOWER"},
    {AFPVOL_UTOMUPPER,  "UTOMUPPER"},
    {AFPVOL_UTOMLOWER,  "UTOMLOWER"},
    {0, NULL}
};

static char* find_in_path( char *path, char *subdir, size_t maxlen)
{
    char 	*pos;
    struct stat st;

    strlcat(path, subdir, maxlen);
    pos = strrchr(path, '/');

    while ( stat(path, &st) != 0) {
#ifdef DEBUG
        fprintf(stderr, "searching in path %s\n", path);
#endif
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
#ifdef DEBUG
    fprintf(stderr, "%s path %s\n", subdir, path);
#endif
    return path;
}

static char * make_path_absolute(char *path, size_t bufsize)
{
    struct	stat st;
    char	savecwd[MAXPATHLEN];
    char 	abspath[MAXPATHLEN];
    char	*p;

    if (stat(path, &st) != 0) {
        return NULL;
    }

    strlcpy (abspath, path, sizeof(abspath));

    if (!S_ISDIR(st.st_mode)) {
        if (NULL == (p=strrchr(abspath, '/')) )
            return NULL;
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

static int parse_options (char *buf, int *flags, const _vol_opt_name* options)
{
    char *p, *q;
    const _vol_opt_name *op;

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
    const _info_option  *p  = &info_options[0];

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
        vol->v_dbd_port = atoi(value);
        break;
      case CNID_DBPATH:
        if ((vol->v_dbpath = strdup(value)) == NULL) {
	    fprintf (stderr, "strdup: %s", strerror(errno));
            return -1;
        }
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
      default:
	fprintf (stderr, "unknown volume information: %s, %s", buf, value);
	return (-1);
        break;
    }
#ifdef DEBUG
    printf ("volume information: %s, %s", buf, value);
#endif
        
    return 0;
}
    

int loadvolinfo (char *path, struct volinfo *vol)
{

    char   volinfofile[MAXPATHLEN];
    char   buf[MAXPATHLEN];
    struct flock lock;
    int    fd;
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

    fclose(fp);
    return 0;
}
