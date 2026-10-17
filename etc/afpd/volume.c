/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Volume commands: FPOpenVol, FPCloseVol, FPGetVolParms, and volume
 * space
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <time.h>
#include <utime.h>

#include <bstrlib.h>

#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif

#include <atalk/adouble.h>
#include <atalk/afp.h>

#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB*/

#include <atalk/dsi.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>

#ifdef HAVE_LDAP
#include <atalk/ldapconfig.h>
#endif /* HAVE_LDAP */

#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_ipc.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/uuid.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

#ifdef WITH_SPOTLIGHT
#include "etc/spotlight/sl_backends.h"
#endif

#include "acls.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "fork.h"
#include "hash.h"
#include "mangle.h"
#include "pfd_cache.h"
#include "unix.h"
#include "virtual_icon.h"
#include "volume.h"

/*! Time Machine cache for 60 seconds */
#define TM_USED_CACHETIME 60
#define VOLPASSLEN  8

extern int afprun(char *cmd, int *outfd);

/*!
 * @brief Read band-size info from Info.plist XML file of an TM sparsebundle
 *
 * @param[in] path    path to Info.plist file
 * @returns           band-size in bytes, -1 on error
 */
static long long int get_tm_bandsize(const char *path)
{
    EC_INIT;
    FILE *file = NULL;
    char buf[512];
    long long int bandsize = -1;
    EC_NULL_LOGSTR(file = fopen(path, "r"),
                   "get_tm_bandsize(\"%s\"): %s",
                   path, strerror(errno));

    while (fgets(buf, sizeof(buf), file) != NULL) {
        if (strstr(buf, "band-size") == NULL) {
            continue;
        }

        if (fscanf(file, " <integer>%lld</integer>", &bandsize) != 1) {
            LOG(log_error, logtype_afpd, "get_tm_bandsize(\"%s\"): can't parse band-size",
                path);
            EC_FAIL;
        }

        break;
    }

EC_CLEANUP:

    if (file) {
        fclose(file);
    }

    LOG(log_debug, logtype_afpd, "get_tm_bandsize(\"%s\"): bandsize: %lld", path,
        bandsize);

    if (ret != 0) {
        return -1;
    }

    return bandsize;
}

/*!
 * @brief Return number on entries in a directory
 *
 * @param[in] path    path to dir
 * @returns           number of entries, -1 on error
 */
static long long int get_tm_bands(const char *path)
{
    EC_INIT;
    long long int count = 0;
    DIR *dir = NULL;
    const struct dirent *entry;
    EC_NULL(dir = opendir(path));

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0
                && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

EC_CLEANUP:

    if (dir) {
        closedir(dir);
    }

    if (ret != 0) {
        return -1;
    }

    return count;
}

/*!
 * @brief Calculate used size of a TimeMachine volume
 *
 * This assumes that the volume is used only for TimeMachine.
 *
 * 1. readdir(path of volume)
 * 2. for every element that matches regex "\(.*\)\.sparsebundle$" :
 * 3. parse "\1.sparsebundle/Info.plist" and read the band-size XML key integer value
 * 4. readdir "\1.sparsebundle/bands/" counting files
 * 5. calculate used size as: (file_count - 1) * band-size
 *
 * The result of the calculation is returned in "volume->v_tm_used".
 * "volume->v_appended" gets reset to 0.
 * "volume->v_tm_cachetime" is updated with the current time from time(NULL)
 * after a successful scan.
 *
 * "volume->v_tm_used" is cached for TM_USED_CACHETIME seconds and updated by
 * "volume->v_appended". The latter is increased by X every time the client
 * appends X bytes to a file (in fork.c).
 *
 * @param vol     (rw) volume to calculate
 * @returns       0 on success, -1 on error
 */
static int get_tm_used(struct vol *vol)
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
        vol->v_tm_used += vol->v_appended;
        vol->v_appended = 0;
        LOG(log_debug, logtype_afpd, "getused(\"%s\"): cached: %" PRIu64 " bytes",
            vol->v_path, vol->v_tm_used);
        return 0;
    }

    EC_NULL(dir = opendir(vol->v_path));

    while ((entry = readdir(dir)) != NULL) {
        if (((p = strstr(entry->d_name, "sparsebundle")) != NULL)
                && (strlen(entry->d_name) == (p + strlen("sparsebundle") - entry->d_name))) {
            EC_NULL_LOG(infoplist = bformat("%s/%s/%s", vol->v_path, entry->d_name,
                                            "Info.plist"));
            bandsize = get_tm_bandsize(cfrombstr(infoplist));
            bdestroy(infoplist);
            infoplist = NULL;

            /* the band size comes from a file the volume's users write */
            if (bandsize <= 0) {
                continue;
            }

            EC_NULL_LOG(bandsdir = bformat("%s/%s/%s/", vol->v_path, entry->d_name,
                                           "bands"));

            if ((links = get_tm_bands(cfrombstr(bandsdir))) > 0) {
                used += (VolSpace)(links - 1) * (VolSpace)bandsize;
            }

            LOG(log_debug, logtype_afpd, "getused(\"%s\"): bands: %" PRIu64 " bytes",
                cfrombstr(bandsdir), used);
            bdestroy(bandsdir);
            bandsdir = NULL;
        }
    }

    vol->v_tm_used = used;
    vol->v_tm_cachetime = now;
    /* the scan counts what was appended before it */
    vol->v_appended = 0;
EC_CLEANUP:

    if (infoplist) {
        bdestroy(infoplist);
    }

    if (bandsdir) {
        bdestroy(bandsdir);
    }

    if (dir) {
        closedir(dir);
    }

    LOG(log_debug, logtype_afpd, "getused(\"%s\"): %" PRIu64 " bytes", vol->v_path,
        vol->v_tm_used);
    EC_EXIT;
}

/*!
 * @brief Free and total space of a volume in 32-bit and 64-bit form
 *
 * The values come from statfs, limited by the user quota when one applies,
 * and by the Time Machine size limit of the volume. The 32-bit values cap at
 * 0xffffffff, at 0x7fffffff before AFP 2.2 or with AFPVOL_LIMITSIZE, and at
 * 0x01fffe00 for a ProDOS volume.
 *
 * @param[in]     obj      session object
 * @param[in,out] vol      volume, its space source flag updated
 * @param[out]    bfree    receives the free bytes, capped to 32 bits
 * @param[out]    btotal   receives the total bytes, capped to 32 bits
 * @param[out]    xbfree   receives the free bytes
 * @param[out]    xbtotal  receives the total bytes
 * @param[out]    bsize    receives the block size
 * @returns AFP_OK, AFPERR_PARAM when statfs failed, or AFPERR_MISC when the
 *          Time Machine usage is unknown
 */
static int getvolspace(const AFPObj *obj, struct vol *vol,
                       uint32_t *bfree, uint32_t *btotal,
                       VolSpace *xbfree, VolSpace *xbtotal, uint32_t *bsize)
{
    int         rc;
    uint32_t   maxsize;
#ifndef NO_QUOTA_SUPPORT
    VolSpace    qfree, qtotal;
    int         spaceflag = AFPVOL_GVSMASK & vol->v_flags;
#endif
    /* report up to 2GB if afp version is < 2.2 (4GB if not) */
    maxsize = (vol->v_flags & AFPVOL_A2VOL) ? 0x01fffe00 :
              (((obj->afp_version < 22) || (vol->v_flags & AFPVOL_LIMITSIZE))
               ? 0x7fffffffL : 0xffffffffL);

    if ((rc = ustatfs_getvolspace(vol, xbfree, xbtotal, bsize)) != AFP_OK) {
        return rc;
    }

#ifndef NO_QUOTA_SUPPORT

    if (spaceflag == AFPVOL_NONE || spaceflag == AFPVOL_UQUOTA) {
        if (uquota_getvolspace(obj, vol, &qfree, &qtotal, *bsize) == AFP_OK) {
            vol->v_flags = (~AFPVOL_GVSMASK & vol->v_flags) | AFPVOL_UQUOTA;
            *xbfree = MIN(*xbfree, qfree);
            *xbtotal = MIN(*xbtotal, qtotal);
            goto getvolspace_done;
        }
    }

#endif
    vol->v_flags = (~AFPVOL_GVSMASK & vol->v_flags) | AFPVOL_USTATFS;
getvolspace_done:

    if (vol->v_limitsize) {
        if (get_tm_used(vol) != 0) {
            return AFPERR_MISC;
        }

        *xbtotal = MIN(*xbtotal, (vol->v_limitsize * 1024 * 1024));
        *xbfree = MIN(*xbfree, *xbtotal < vol->v_tm_used ? 0 : *xbtotal -
                      vol->v_tm_used);
        LOG(log_debug, logtype_afpd,
            "volparams: total: %" PRIu64 ", used: %" PRIu64 ", free: %" PRIu64 " bytes",
            *xbtotal, vol->v_tm_used, *xbfree);
    }

    *bfree = MIN(*xbfree, maxsize);
    *btotal = MIN(*xbtotal, maxsize);
    return AFP_OK;
}

/*!
 * @brief Set the creation date of a volume, unique among the configured volumes
 *
 * While another volume carries the same creation date, the date moves on by
 * one second and the search starts over.
 *
 * @param[in]     id    volume ID
 * @param[in,out] adp   AppleDouble of the volume root
 * @param[in]     date  creation date as a Unix time
 */
static void vol_setdate(uint16_t id, struct adouble *adp, time_t date)
{
    struct vol  *volume;
    struct vol  *vol = getvolumes();

    for (volume = getvolumes(); volume; volume = volume->v_next) {
        if (volume->v_vid == id) {
            vol = volume;
        } else if ((time_t)(AD_DATE_FROM_UNIX(date)) == volume->v_ctime) {
            date = date + 1;
            /* restart */
            volume = getvolumes();
        }
    }

    vol->v_ctime = AD_DATE_FROM_UNIX(date);
    ad_setdate(adp, AD_DATE_CREATE | AD_DATE_UNIX, date);
}

/*!
 * @brief Pack the volume parameters that bitmap selects into buf
 *
 * The call first converts version 2 AppleDouble data of the volume root to
 * the ea layout unless the volume disables that. It then opens or creates the
 * root metadata, and a new root metadata file gets a unique creation date. The
 * attribute word reports read-only state, file IDs, catalog search, UTF-8
 * names, UNIX privileges, Time Machine, extended attributes, ACLs, and case
 * sensitivity. The name always uses the Mac charset.
 *
 * @param[in]  obj     session object
 * @param[in]  bitmap  AFP volume bitmap, VOLPBIT_* bits
 * @param[in]  vol     volume
 * @param[in]  st      stat of the volume root
 * @param[out] buf     reply buffer receiving the parameters
 * @param[out] buflen  receives the bytes written
 * @returns AFP_OK, AFPERR_BITMAP for an unknown bit, AFPERR_PARAM when the
 *          space lookup failed, or AFPERR_MISC
 */
static int getvolparams(const AFPObj *obj, uint16_t bitmap, struct vol *vol,
                        struct stat *st, char *buf, size_t *buflen)
{
    struct adouble  ad;
    int bit = 0, isad = 1;
    uint32_t aint;
    uint16_t ashort;
    uint32_t bfree = 0, btotal = 0, bsize = 0;
    /* extended bytes */
    VolSpace xbfree = 0, xbtotal = 0;
    char *data = NULL;
    char *nameoff = NULL;
    const char *slash = NULL;
    char *ade = NULL;
    LOG(log_debug, logtype_afpd, "getvolparams: Volume '%s'", vol->v_localname);

    /* courtesy of jallison@whistle.com:
     * For MacOS8.x support we need to create the
     * .Parent file here if it doesn't exist. */
    if (!(vol->v_flags & AFPVOL_NOV2TOEACONV)
            && ad_convert(vol->v_path, st, vol, NULL) < 0) {
        LOG(log_warning, logtype_afpd,
            "getvolparams: AppleDouble to EA conversion failed for volume '%s'",
            vol->v_localname);
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, vol->v_path,
                ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) != 0) {
        isad = 0;
        vol->v_ctime = AD_DATE_FROM_UNIX(st->st_mtime);
    } else if (ad_get_MD_flags(&ad) & O_CREAT) {
        slash = strrchr(vol->v_path, '/');

        if (slash) {
            slash++;
        } else {
            slash = vol->v_path;
        }

        if (ad_getentryoff(&ad, ADEID_NAME)
                && (ade = ad_entry(&ad, ADEID_NAME)) != NULL) {
            ad_setentrylen(&ad, ADEID_NAME, strlen(slash));
            memcpy(ade, slash, ad_getentrylen(&ad, ADEID_NAME));
        }

        vol_setdate(vol->v_vid, &ad, st->st_mtime);
        ad_flush(&ad);
    } else {
        if (ad_getdate(&ad, AD_DATE_CREATE, &aint) < 0) {
            vol->v_ctime = AD_DATE_FROM_UNIX(st->st_mtime);
        } else {
            vol->v_ctime = aint;
        }
    }

    if ((bitmap & ((1 << VOLPBIT_BFREE) | (1 << VOLPBIT_BTOTAL) |
                   (1 << VOLPBIT_XBFREE) | (1 << VOLPBIT_XBTOTAL) |
                   (1 << VOLPBIT_BSIZE))) != 0) {
        if (getvolspace(obj, vol, &bfree, &btotal, &xbfree, &xbtotal,
                        &bsize) != AFP_OK) {
            if (isad) {
                ad_close(&ad, ADFLAGS_HF);
            }

            return AFPERR_PARAM;
        }
    }

    data = buf;

    while (bitmap != 0) {
        while ((bitmap & 1) == 0) {
            bitmap = bitmap >> 1;
            bit++;
        }

        switch (bit) {
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
            if (obj->afp_version > 20) {
                if (vol->v_cdb != NULL && (vol->v_cdb->cnid_db_flags & CNID_FLAG_PERSISTENT)) {
                    ashort |= VOLPBIT_ATTR_FILEID;
                }

                ashort |= VOLPBIT_ATTR_CATSEARCH;

                if (obj->afp_version >= 30) {
                    ashort |= VOLPBIT_ATTR_UTF8;

                    if (vol->v_flags & AFPVOL_UNIX_PRIV) {
                        ashort |= VOLPBIT_ATTR_UNIXPRIV;
                    }

                    if (vol->v_flags & AFPVOL_TM) {
                        ashort |= VOLPBIT_ATTR_TM;
                    }

#ifdef HAVE_LDAP

                    if (!ldap_config_valid || vol->v_flags & AFPVOL_NONETIDS) {
                        ashort |= VOLPBIT_ATTR_NONETIDS;
                    }

#else
                    ashort |= VOLPBIT_ATTR_NONETIDS;
#endif

                    if (obj->afp_version >= 32) {
                        if (vol->v_vfs_ea) {
                            ashort |= VOLPBIT_ATTR_EXT_ATTRS;
                        }

                        if (vol->v_flags & AFPVOL_ACLS) {
                            ashort |= VOLPBIT_ATTR_ACLS;
                        }

                        if (vol->v_casefold & AFPVOL_CASESENS) {
                            ashort |= VOLPBIT_ATTR_CASESENS;
                        }
                    }
                }
            }

            ashort = htons(ashort);
            memcpy(data, &ashort, sizeof(ashort));
            data += sizeof(ashort);
            break;

        case VOLPBIT_SIG :
            ashort = htons(AFPVOLSIG_DEFAULT);
            memcpy(data, &ashort, sizeof(ashort));
            data += sizeof(ashort);
            break;

        case VOLPBIT_CDATE :
            aint = vol->v_ctime;
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case VOLPBIT_MDATE :
            if (st->st_mtime > vol->v_mtime) {
                vol->v_mtime = st->st_mtime;
            }

            aint = AD_DATE_FROM_UNIX(vol->v_mtime);
            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case VOLPBIT_BDATE :
            if (!isad || (ad_getdate(&ad, AD_DATE_BACKUP, &aint) < 0)) {
                aint = AD_DATE_START;
            }

            memcpy(data, &aint, sizeof(aint));
            data += sizeof(aint);
            break;

        case VOLPBIT_VID :
            memcpy(data, &vol->v_vid, sizeof(vol->v_vid));
            data += sizeof(vol->v_vid);
            break;

        case VOLPBIT_BFREE :
            bfree = htonl(bfree);
            memcpy(data, &bfree, sizeof(bfree));
            data += sizeof(bfree);
            break;

        case VOLPBIT_BTOTAL :
            btotal = htonl(btotal);
            memcpy(data, &btotal, sizeof(btotal));
            data += sizeof(btotal);
            break;

        case VOLPBIT_XBFREE :
            xbfree = hton64(xbfree);
            memcpy(data, &xbfree, sizeof(xbfree));
            data += sizeof(xbfree);
            break;

        case VOLPBIT_XBTOTAL :
            xbtotal = hton64(xbtotal);
            memcpy(data, &xbtotal, sizeof(xbtotal));
            data += sizeof(xbfree);
            break;

        case VOLPBIT_NAME :
            nameoff = data;
            data += sizeof(uint16_t);
            break;

        case VOLPBIT_BSIZE:
            /* block size */
            bsize = htonl(bsize);
            memcpy(data, &bsize, sizeof(bsize));
            data += sizeof(bsize);
            break;

        default :
            if (isad) {
                ad_close(&ad, ADFLAGS_HF);
            }

            return AFPERR_BITMAP;
        }

        bitmap = bitmap >> 1;
        bit++;
    }

    if (nameoff) {
        ashort = htons(data - buf);
        memcpy(nameoff, &ashort, sizeof(ashort));
        /* name is always in mac charset */
        aint = ucs2_to_charset(vol->v_maccharset, vol->v_macname, data + 1,
                               AFPVOL_MACNAMELEN + 1);

        if (aint <= 0) {
            *buflen = 0;
            return AFPERR_MISC;
        }

        *data++ = aint;
        data += aint;
    }

    if (isad) {
        ad_close(&ad, ADFLAGS_HF);
    }

    *buflen = data - buf;
    return AFP_OK;
}

/*!
 * @brief Reply with the bitmap and the volume parameters it selects
 *
 * The stat of the volume root also records the device number of the volume.
 *
 * @param[in]     obj      session object
 * @param[in]     bitmap   AFP volume bitmap
 * @param[in,out] vol      volume
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_PARAM when the stat failed, or the error of
 *          getvolparams
 */
static int stat_vol(const AFPObj *obj, uint16_t bitmap, struct vol *vol,
                    char *rbuf, size_t *rbuflen)
{
    struct stat st;
    int     ret;
    size_t  buflen;

    if (stat(vol->v_path, &st) < 0) {
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    /* save the volume device number */
    vol->v_dev = st.st_dev;
    buflen = *rbuflen - sizeof(bitmap);

    if ((ret = getvolparams(obj, bitmap, vol, &st,
                            rbuf + sizeof(bitmap), &buflen)) != AFP_OK) {
        *rbuflen = 0;
        return ret;
    }

    *rbuflen = buflen + sizeof(bitmap);
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    return AFP_OK;
}

/*!
 * @brief FPGetSrvrParms: list the volumes that the user may see, with the
 * server time
 *
 * The call reloads the volume configuration first. A volume appears when its
 * root is a directory that the user may read and search, unless the volume
 * skips that test with AFPVOL_NOSTAT. Each entry carries the password and
 * Apple II flags and the name. The list stops at 255 volumes or 4600 bytes,
 * the reply size at which clients disconnect.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block, unused
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or AFPERR_PARAM when the server time is unavailable
 */
int afp_getsrvrparms(AFPObj *obj, char *ibuf _U_, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen)
{
    struct timeval  tv;
    struct stat     st;
    struct vol      *volume;
    char    *data;
    char        *namebuf;
    int         vcnt;
    size_t      len;
    uint32_t    aint;
    load_afp_conf_vols(obj, LV_DEFAULT);
    data = rbuf + 5;

    for (vcnt = 0, volume = getvolumes(); volume
            && vcnt < 255; volume = volume->v_next) {
        if (!(volume->v_flags & AFPVOL_NOSTAT)) {
            struct maccess ma;

            if (stat(volume->v_path, &st) < 0) {
                LOG(log_info, logtype_afpd, "afp_getsrvrparms(%s): stat: %s",
                    volume->v_path, strerror(errno));
                /* can't access directory */
                continue;
            }

            if (!S_ISDIR(st.st_mode)) {
                /* not a dir */
                continue;
            }

            accessmode(obj, volume, volume->v_path, &ma, NULL, &st);

            if ((ma.ma_user & (AR_UREAD | AR_USEARCH)) != (AR_UREAD | AR_USEARCH)) {
                /* no r-x access */
                continue;
            }
        }

        if (utf8_encoding(obj)) {
            len = ucs2_to_charset_allocate(CH_UTF8_MAC, &namebuf, volume->v_u8mname);
        } else {
            len = ucs2_to_charset_allocate(obj->options.maccharset, &namebuf,
                                           volume->v_macname);
        }

        if (len == (size_t) -1) {
            continue;
        }

        /*
         * There seems to be an undocumented limit on how big our reply can get
         * before the client chokes and closes the connection.
         * Testing with 10.8.4 found the limit at ~4600 bytes. Go figure.
         */
        if (((data + len + 3) - rbuf) > 4600) {
            break;
        }

        /* set password bit if there's a volume password */
        *data = (volume->v_password) ? (char) AFPSRVR_PASSWD : 0;
        /* Apple 2 clients running ProDOS-8 expect one volume to have
           bit 0 of this byte set.  They will not recognize anything
           on the server unless this is the case.  I have not
           completely worked this out, but it's related to booting
           from the server.  Support for that function is a ways
           off.. <shirsch@ibm.net> */
        *data |= (volume->v_flags & AFPVOL_A2VOL) ? AFPSRVR_CONFIGINFO : 0;
        /* UNIX PRIVS BIT ..., OSX doesn't seem to use it, so we don't either */
        *data++ |= 0;
        *data++ = len;
        memcpy(data, namebuf, len);
        data += len;
        free(namebuf);
        vcnt++;
    }

    *rbuflen = data - rbuf;
    data = rbuf;

    if (gettimeofday(&tv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_getsrvrparms: gettimeofday: %s",
            strerror(errno));
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    aint = AD_DATE_FROM_UNIX(tv.tv_sec);
    memcpy(data, &aint, sizeof(uint32_t));
    data += sizeof(uint32_t);
    *data = vcnt;
    return AFP_OK;
}

/*!
 * @brief Load the volume charset and the Mac charset of a volume
 *
 * The volume charset defaults to UTF8 and the Mac charset to the server
 * setting. A volume charset that only iconv provides logs a warning. A Mac
 * charset that is not a client charset fails.
 *
 * @param[in]     obj     session object
 * @param[in,out] volume  volume whose charsets to set
 * @returns 0, or -1 when a charset failed to load
 */
static int volume_codepage(AFPObj *obj, struct vol *volume)
{
    struct charset_functions *charset;
    /* Codepages */

    if (!volume->v_volcodepage) {
        volume->v_volcodepage = strdup("UTF8");
    }

    if ((charset_t) -1 == (volume->v_volcharset = add_charset(
                                                      volume->v_volcodepage))) {
        LOG(log_error, logtype_afpd, "Setting codepage %s as volume codepage failed",
            volume->v_volcodepage);
        return -1;
    }

    if (NULL == (charset = find_charset_functions(volume->v_volcodepage))
            || charset->flags & CHARSET_ICONV) {
        LOG(log_warning, logtype_afpd,
            "WARNING: volume encoding %s is *not* supported by netatalk, expect problems !!!!",
            volume->v_volcodepage);
    }

    if (!volume->v_maccodepage) {
        volume->v_maccodepage = strdup(obj->options.maccodepage);
    }

    if ((charset_t) -1 == (volume->v_maccharset = add_charset(
                                                      volume->v_maccodepage))) {
        LOG(log_error, logtype_afpd, "Setting codepage %s as mac codepage failed",
            volume->v_maccodepage);
        return -1;
    }

    if (NULL == (charset = find_charset_functions(volume->v_maccodepage))
            || !(charset->flags & CHARSET_CLIENT)) {
        LOG(log_error, logtype_afpd, "Fatal error: mac charset %s not supported",
            volume->v_maccodepage);
        return -1;
    }

    volume->v_kTextEncoding = htonl(charset->kTextEncoding);
    return 0;
}

/*!
 * @brief Open the CNID backend of a volume
 *
 * A volume with AFPVOL_NODEV opens it with CNID_FLAG_NODEV.
 *
 * @param[in]     obj     session object, unused
 * @param[in,out] volume  volume receiving the CNID database
 * @returns 0, or -1 when cnid_open failed
 */
static int volume_openDB(const AFPObj *obj _U_, struct vol *volume)
{
    int flags = 0;

    if (volume->v_flags & AFPVOL_NODEV) {
        flags |= CNID_FLAG_NODEV;
    }

    volume->v_cdb = cnid_open(volume, volume->v_cnidscheme, flags);

    if (! volume->v_cdb) {
        LOG(log_error, logtype_afpd, "Can't open volume \"%s\" CNID backend \"%s\" ",
            volume->v_path, volume->v_cnidscheme);
        return -1;
    }

    return (!volume->v_cdb) ? -1 : 0;
}

/*!
 * @brief Send the names of the open volumes to the master afpd
 *
 * The IPC_VOLUMES message carries the names separated by a comma and a space.
 *
 * @param[in] obj  session object
 */
static void server_ipc_volumes(AFPObj *obj)
{
    struct vol *volume, *vols;
    volume = vols = getvolumes();
    bstring openvolnames = bfromcstr("");
    bool firstvol = true;

    while (volume) {
        if (volume->v_flags & AFPVOL_OPEN) {
            if (!firstvol) {
                bcatcstr(openvolnames, ", ");
            } else {
                firstvol = false;
            }

            bcatcstr(openvolnames, volume->v_localname);
        }

        volume = volume->v_next;
    }

    ipc_child_write(obj, IPC_VOLUMES, blength(openvolnames),
                    bdata(openvolnames));
    bdestroy(openvolnames);
}

/*!
 * @brief FPOpenVol: open a volume and return its parameters
 *
 * The bitmap must include VOLPBIT_VID. The volume name comes in UTF-8 or the
 * Mac charset, and a volume password must match. An open volume only returns
 * its parameters. Otherwise the call runs the preexec command, loads the
 * charsets, creates the dircache entry of the root, and opens the CNID and
 * Spotlight backends. A failure closes the volume again. The session runs as
 * the user here.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_BITMAP, AFPERR_NOOBJ for an unknown name,
 *          AFPERR_ACCESS for a wrong password, AFPERR_PARAM, or AFPERR_MISC
 */
int afp_openvol(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                size_t *rbuflen)
{
    struct stat st;
    char    *volname;
    struct vol  *volume;
    struct dir  *dir;
    int     len, ret;
    size_t  namelen;
    uint16_t   bitmap;
    char        *vol_uname;
    char        *vol_mname = NULL;
    char        *volname_tmp;
    ibuf += 2;
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    *rbuflen = 0;

    if ((bitmap & (1 << VOLPBIT_VID)) == 0) {
        return AFPERR_BITMAP;
    }

    len = (unsigned char) * ibuf++;
    volname = obj->oldtmp;

    if ((volname_tmp = strchr(volname, '+')) != NULL) {
        volname = volname_tmp + 1;
    }

    if (utf8_encoding(obj)) {
        namelen = convert_string(CH_UTF8_MAC, CH_UCS2, ibuf, len, volname,
                                 sizeof(obj->oldtmp));
    } else {
        namelen = convert_string(obj->options.maccharset, CH_UCS2, ibuf, len, volname,
                                 sizeof(obj->oldtmp));
    }

    if (namelen <= 0) {
        return AFPERR_PARAM;
    }

    ibuf += len;

    /* pad to an even boundary */
    if ((len + 1) & 1) {
        ibuf++;
    }

    load_afp_conf_vols(obj, LV_DEFAULT);

    for (volume = getvolumes(); volume; volume = volume->v_next) {
        if (strcasecmp_w((ucs2_t *) volname, volume->v_name) == 0) {
            break;
        }
    }

    if (volume == NULL) {
        return AFPERR_NOOBJ;
    }

    /* check for a volume password */
    if (volume->v_password && strncmp(ibuf, volume->v_password, VOLPASSLEN)) {
        return AFPERR_ACCESS;
    }

    if (volume->v_flags & AFPVOL_OPEN) {
        /* the volume is already open */
        return stat_vol(obj, bitmap, volume, rbuf, rbuflen);
    }

    if (volume->v_preexec) {
        if ((ret = afprun(volume->v_preexec, NULL)) && volume->v_preexec_close) {
            LOG(log_error, logtype_afpd, "afp_openvol(%s): preexec : %d", volume->v_path,
                ret);
            return AFPERR_MISC;
        }
    }

    if (stat(volume->v_path, &st) < 0) {
        return AFPERR_PARAM;
    }

    if (chdir(volume->v_path) < 0) {
        return AFPERR_PARAM;
    }

    if (volume_codepage(obj, volume) < 0) {
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    /* initialize volume variables
     * FIXME file size
     */
    if (utf8_encoding(obj)) {
        volume->max_filename = UTF8FILELEN_EARLY;
    } else {
        volume->max_filename = MACFILELEN;
    }

    volume->v_flags |= AFPVOL_OPEN;
    volume->v_cdb = NULL;

    if (utf8_encoding(obj)) {
        len = convert_string_allocate(CH_UCS2, CH_UTF8_MAC, volume->v_u8mname, namelen,
                                      &vol_mname);
    } else {
        len = convert_string_allocate(CH_UCS2, obj->options.maccharset,
                                      volume->v_macname, namelen, &vol_mname);
    }

    if (!vol_mname || len <= 0) {
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    if ((vol_uname = strrchr(volume->v_path, '/')) == NULL) {
        vol_uname = volume->v_path;
    } else if (vol_uname[1] != '\0') {
        vol_uname++;
    }

    if ((dir = dir_new(vol_mname,
                       vol_uname,
                       volume,
                       DIRDID_ROOT_PARENT,
                       DIRDID_ROOT,
                       bfromcstr(volume->v_path),
                       &st)
        ) == NULL) {
        LOG(log_error, logtype_afpd, "afp_openvol(%s): malloc: %s", volume->v_path,
            strerror(errno));
        ret = AFPERR_MISC;
        goto openvol_err;
    }

    volume->v_root = dir;
    curdir = dir;

    if (volume_openDB(obj, volume) < 0) {
        LOG(log_error, logtype_afpd,
            "Fatal error: cannot open CNID or invalid CNID backend for %s: %s",
            volume->v_path, volume->v_cnidscheme);
        ret = AFPERR_MISC;
        goto openvol_err;
    }

#ifdef WITH_SPOTLIGHT

    if (volume->v_flags & AFPVOL_SPOTLIGHT) {
        const char *bname = volume->v_sl_backend_name
                            ? volume->v_sl_backend_name : "cnid";
        volume->v_sl_backend = NULL;
#ifdef SPOTLIGHT_BACKEND_CNID

        if (strcasecmp(bname, "cnid") == 0) {
            volume->v_sl_backend = &sl_cnid_ops;
        }

#endif
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH

        if (strcasecmp(bname, "localsearch") == 0) {
            volume->v_sl_backend = &sl_localsearch_ops;
        }

#endif
#ifdef SPOTLIGHT_BACKEND_XAPIAN

        if (strcasecmp(bname, "xapian") == 0) {
            volume->v_sl_backend = &sl_xapian_ops;
        }

#endif

        if (volume->v_sl_backend == NULL) {
            LOG(log_warning, logtype_afpd,
                "Unknown or unsupported Spotlight search backend \"%s\" for volume %s, "
                "Spotlight disabled for this volume",
                bname, volume->v_path);
            volume->v_flags &= ~AFPVOL_SPOTLIGHT;
        } else {
            LOG(log_info, logtype_afpd,
                "Volume %s: using \"%s\" Spotlight search backend",
                volume->v_path, volume->v_sl_backend->sbo_name);
        }
    }

#endif /* WITH_SPOTLIGHT */
    virtual_icon_init(volume);
    ret  = stat_vol(obj, bitmap, volume, rbuf, rbuflen);

    if (ret == AFP_OK) {
        /*
         * If you mount a volume twice, the second time the trash appears on
         * the desk-top.  That's because the Mac remembers the DID for the
         * trash (even for volumes in different zones, on different servers).
         * Just so this works better, we prime the DID cache with the trash,
         * fixing the trash at DID 17.
         * FIXME (RL): should it be done inside a CNID backend ? (always returning Trash DID when asked) ?
         */
        if (volume->v_cdb->cnid_db_flags & CNID_FLAG_PERSISTENT) {
            /* FIXME find db time stamp */
            if (cnid_getstamp(volume->v_cdb, volume->v_stamp,
                              sizeof(volume->v_stamp)) < 0) {
                LOG(log_error, logtype_afpd,
                    "afp_openvol(%s): Fatal error: Unable to get stamp value from CNID backend",
                    volume->v_path);
                ret = AFPERR_MISC;
                goto openvol_err;
            }
        }

        const char *msg;
        char option[MAXOPTLEN];
        snprintf(option, sizeof(option), "%s:login message", volume->v_configname);

        if ((msg = iniparser_getstring(obj->iniconfig, option,  NULL)) != NULL) {
            setmessage(msg);
        }

        {
            struct adouble ad;
            ad_init(&ad, volume);

            if (!ad_metadata(".", ADFLAGS_DIR, &ad)) {
                const char *fi = ad_entry(&ad, ADEID_FINDERI);

                if (fi) {
                    uint16_t fflags;
                    memcpy(&fflags, fi + FINDERINFO_FRFLAGOFF, sizeof(fflags));

                    if (fflags & htons(FINDERINFO_NAMELOCKED)) {
                        LOG(log_note, logtype_afpd,
                            "afp_openvol: volume root of \"%s\" has name locked FinderFlag set",
                            volume->v_localname);
                    }
                }

                ad_close(&ad, ADFLAGS_HF);
            }
        }

        free(vol_mname);
        server_ipc_volumes(obj);
        return AFP_OK;
    }

openvol_err:

    if (volume->v_root) {
        dir_free(volume->v_root);
        volume->v_root = NULL;
    }

    volume->v_flags &= ~AFPVOL_OPEN;

    if (volume->v_cdb != NULL) {
        cnid_close(volume->v_cdb);
        volume->v_cdb = NULL;
    }

    free(vol_mname);
    *rbuflen = 0;
    return ret;
}

/*!
 * @brief Identifier of the CNID table of a volume that every afpd process
 * shares
 *
 * The v_vid field differs between processes, so the same value names
 * different volumes in different children. Every child agrees on v_uuid. The
 * tag is the FNV-1a hash of v_uuid, and 0 stays reserved for no tag.
 *
 * @param[in] vol  volume, or NULL
 * @returns the tag, never 0 for a volume with a UUID, or 0 without one
 */
uint32_t cnid_volume_tag(const struct vol *vol)
{
    uint32_t hash = 2166136261u;

    if (!vol || !vol->v_uuid) {
        return 0;
    }

    for (const unsigned char *p = (const unsigned char *)vol->v_uuid; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }

    /* 0 is reserved for "no tag" */
    return hash ? hash : 1;
}

/*!
 * @brief Announce a CNID table reset and end this session
 *
 * Only the session that reset the table sends the hint, because a responder
 * that sent it on would loop between children. AFP has no attention for a
 * recycled ID space, since the protocol promises that IDs stay unique, so the
 * disconnect is the signal. The SIGTERM takes effect between requests, and
 * the request in flight still gets its reply.
 *
 * @param[in] vol  volume whose CNID table the session reset, or NULL
 */
void cnid_volume_reset(const struct vol *vol)
{
    extern AFPObj *AFPobj;

    if (!AFPobj || !vol) {
        return;
    }

    uint32_t tag = cnid_volume_tag(vol);

    if (tag == 0) {
        LOG(log_error, logtype_afpd,
            "cnid_volume_reset: volume '%s' has no UUID, cannot tell other "
            "sessions its CNID table was reset", vol->v_path);
    } else if (ipc_send_cache_hint(AFPobj, vol->v_vid, htonl(tag),
                                   CACHE_HINT_VOLUME_RESET) != 0) {
        LOG(log_error, logtype_afpd,
            "cnid_volume_reset: could not tell other sessions that the CNID "
            "table for volume '%s' was reset; they may serve stale CNIDs until "
            "they reconnect", vol->v_path);
    }

    LOG(log_warning, logtype_afpd,
        "cnid_volume_reset: CNID table for volume '%s' was reset, disconnecting "
        "so the client picks up the rebuilt table", vol->v_path);
    /* Both transports defer, so the session ends between requests: the request
     * in flight is still answered, and callers that could return recycled
     * CNIDs stop their own walk for that reason. */
    raise(SIGTERM);
}

/*!
 * @brief Close a volume and release everything it holds
 *
 * The call closes the forks of the volume and retires its parent descriptor
 * slots, which would otherwise pin directories and match after a reopen. It
 * then purges the dircache entries, frees the root entry, closes the CNID
 * database, and runs the postexec command.
 *
 * @param[in]     obj  session object
 * @param[in,out] vol  volume to close, or NULL
 */
void closevol(const AFPObj *obj, struct vol *vol)
{
    if (!vol) {
        return;
    }

    vol->v_flags &= ~AFPVOL_OPEN;
    of_closevol(obj, vol);
    /* Retire the volume's pfd slots: their O_PATH fds would otherwise
     * outlive the close (pinning directories against unmount), and since
     * struct vol and v_vid persist across logout/re-login a surviving
     * slot would satisfy (vid, did) matches after re-open. */
    pfd_purge_vol(vol->v_vid);
    /* Before v_root is freed, so removals still have a valid volume */
    dircache_purge_vol(vol);
    dir_free(vol->v_root);
    vol->v_root = NULL;

    if (vol->v_cdb != NULL) {
        cnid_close(vol->v_cdb);
        vol->v_cdb = NULL;
    }

    if (vol->v_postexec) {
        afprun(vol->v_postexec, NULL);
    }
}

/*!
 * @brief Close every open volume of the session
 *
 * @param[in] obj  session object
 */
void close_all_vol(const AFPObj *obj)
{
    struct vol  *ovol;
    curdir = NULL;

    for (ovol = getvolumes(); ovol; ovol = ovol->v_next) {
        if (ovol->v_flags & AFPVOL_OPEN) {
            ovol->v_flags &= ~AFPVOL_OPEN;
            closevol(obj, ovol);
        }
    }
}

/*!
 * @brief FPCloseVol: close a volume
 *
 * The call moves to the root directory, finishes the deferred child scans of
 * the volume, and closes it. The master afpd then gets the new list of open
 * volumes.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, or AFPERR_PARAM for an unknown volume or a failed chdir
 */
int afp_closevol(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
                 size_t *rbuflen)
{
    struct vol  *vol;
    uint16_t   vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    if (chdir("/") < 0) {
        LOG(log_error, logtype_afpd, "afp_closevol: chdir: %s", strerror(errno));
        return AFPERR_PARAM;
    }

    curdir = NULL;
    dircache_flush_deferred_for_vol(vid);
    closevol(obj, vol);
    server_ipc_volumes(obj);
    return AFP_OK;
}

/*!
   @brief poll if a volume is changed by other processes.
   @param obj AFP connection object
   @return
   0 no attention msg sent,
   1 attention msg sent,
   -1 error (socket closed)

   @note if attention return -1 no packet has been
   sent because the buffer is full, we don't care
   either there's no reader or there's a lot of
   traffic and another pollvoltime will follow
*/
int  pollvoltime(AFPObj *obj)

{
    struct vol       *vol;
    struct timeval   tv;
    struct stat      st;

    if (obj->proto == AFPPROTO_DSI) {
        obj->handle = obj->dsi;
    }

    if (!(obj->afp_version > 21 && obj->options.flags & OPTION_SERVERNOTIF)) {
        return 0;
    }

    if (gettimeofday(&tv, NULL) < 0) {
        return 0;
    }

    for (vol = getvolumes(); vol; vol = vol->v_next) {
        if ((vol->v_flags & AFPVOL_OPEN)  && vol->v_mtime + 30 < tv.tv_sec) {
            if (!stat(vol->v_path, &st) && vol->v_mtime != st.st_mtime) {
                vol->v_mtime = st.st_mtime;

                if (!obj->attention(obj->handle, AFPATTN_NOTIFY | AFPATTN_VOLCHANGED)) {
                    return -1;
                }

                return 1;
            }
        }
    }

    return 0;
}

/*!
 * @brief Record a change on a volume in its modification time
 *
 * The call touches the volume root and raises v_mtime when the second
 * advanced. Before AFP 3.2, with server notifications on, that also sends a
 * volume-changed attention, so the Finder refreshes the free space.
 *
 * @param[in]     obj  session object
 * @param[in,out] vol  volume that changed
 */
void setvoltime(AFPObj *obj, struct vol *vol)
{
    struct timeval  tv;

    if (obj->proto == AFPPROTO_DSI) {
        obj->handle = obj->dsi;
    }

    if (gettimeofday(&tv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "setvoltime(%s): gettimeofday: %s", vol->v_path,
            strerror(errno));
        return;
    }

    if (utime(vol->v_path, NULL) < 0) {
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
        if (obj->afp_version < 32 && obj->options.flags & OPTION_SERVERNOTIF) {
            obj->attention(obj->handle, AFPATTN_NOTIFY | AFPATTN_VOLCHANGED);
        }
    }
}

/*!
 * @brief FPGetVolParms: return the parameters of an open volume
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_PARAM for an unknown volume, or the error of stat_vol
 */
int afp_getvolparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                     size_t *rbuflen)
{
    struct vol  *vol;
    uint16_t   vid, bitmap;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);

    if (NULL == (vol = getvolbyvid(vid))) {
        *rbuflen = 0;
        return AFPERR_PARAM;
    }

    return stat_vol(obj, bitmap, vol, rbuf, rbuflen);
}

/*!
 * @brief FPSetVolParms: set the backup date of a volume
 *
 * The backup date is the only parameter that a client may set, and any other
 * bitmap fails with AFPERR_BITMAP. The date lands in the AppleDouble metadata
 * of the volume root.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_BITMAP, AFPERR_VLOCK for a read-only volume,
 *          AFPERR_ACCESS, or AFPERR_PARAM
 */
int afp_setvolparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                     char *rbuf _U_,  size_t *rbuflen)
{
    struct adouble ad;
    struct vol  *vol;
    uint16_t   vid, bitmap;
    uint32_t   aint;
    ibuf += 2;
    *rbuflen = 0;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);

    if ((vol = getvolbyvid(vid)) == NULL) {
        return AFPERR_PARAM;
    }

    if (vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    /* we can only set the backup date. */
    if (bitmap != (1 << VOLPBIT_BDATE)) {
        return AFPERR_BITMAP;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad,  vol->v_path, ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDWR) < 0) {
        if (errno == EROFS) {
            return AFPERR_VLOCK;
        }

        return AFPERR_ACCESS;
    }

    memcpy(&aint, ibuf, sizeof(aint));
    ad_setdate(&ad, AD_DATE_BACKUP, aint);
    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
    return AFP_OK;
}
