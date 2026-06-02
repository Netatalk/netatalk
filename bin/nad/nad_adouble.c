/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * All Rights Reserved.  See COPYRIGHT.
 *
 * Macintosh file format transformer based on megatron by Charles Clark.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <atalk/adouble.h>
#include <atalk/volume.h>
#include <atalk/util.h>
#include <netatalk/endian.h>
#include "nad.h"
#include "megatron.h"
#include "nad_adouble.h"

static const unsigned char hexdig[] = "0123456789abcdef";

static char mtou_buf[MAXPATHLEN + 1], utom_buf[MAXPATHLEN + 1];

static int cap_hexval(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return -1;
}

static int cap_decode_byte(const unsigned char *in, unsigned char *out)
{
    int hi, lo;
    hi = cap_hexval(in[0]);
    lo = cap_hexval(in[1]);

    if (hi < 0 || lo < 0) {
        return 0;
    }

    *out = (unsigned char)((hi << 4) | lo);
    return 1;
}

static void cap_escape_byte(unsigned char c, unsigned char **out)
{
    *(*out)++ = ':';
    *(*out)++ = hexdig[(c & 0xf0) >> 4];
    *(*out)++ = hexdig[c & 0x0f];
}

#ifdef DOWNCASE
static unsigned char lower_byte(unsigned char c)
{
    return (unsigned char)(isupper(c) ? tolower(c) : c);
}
#endif /* DOWNCASE */

static char *mtoupathcap(char *mpath)
{
    const unsigned char	*m;
    unsigned char	*u, *umax;
    int		i = 0;
    m = (const unsigned char *)mpath;
    u = (unsigned char *)mtou_buf;
    umax = u + sizeof(mtou_buf) - 4;

    while (*m != '\0' && u < umax) {
        if (!isprint(*m) || *m == '/' || (i == 0 && (*m == '.'))) {
            cap_escape_byte(*m, &u);
        } else {
#ifdef DOWNCASE
            *u++ = lower_byte(*m);
#else /* DOWNCASE */
            *u++ = *m;
#endif /* DOWNCASE */
        }

        i++;
        m++;
    }

    *u = '\0';
    return mtou_buf;
}


static char *utompathcap(char *upath)
{
    unsigned char	*m, *u;
    unsigned char	decoded;
    m = (unsigned char *)utom_buf;
    u = (unsigned char *)upath;

    while (*u != '\0') {
        if (*u == ':' && *(u + 1) != '\0' && *(u + 2) != '\0'
                && cap_decode_byte(u + 1, &decoded)) {
            u += 2;
            *m = decoded;
        } else {
#ifdef DOWNCASE
            *m = diatolower(*u);
#else /* DOWNCASE */
            *m = *u;
#endif /* DOWNCASE */
        }

        u++;
        m++;
    }

    *m = '\0';
    return utom_buf;
}

char *(*_mtoupath)(char *mpath) = mtoupathcap;
char *(*_utompath)(char *upath) = utompathcap;


static struct nad_file_data {
    char		macname[ MAXPATHLEN + 1 ];
    char		adpath[ 2 ][ MAXPATHLEN + 1];
    int			offset[ NUMFORKS ];
    int                 closeflags;
    int                 write_metadata;
    const struct nad_volume *volume;
    struct vol          *vol;
    struct adouble	ad;
} nad;

void nad_set_volume(const struct nad_volume *volume)
{
    nad.volume = volume;
    nad.vol = volume ? volume->vol : NULL;
}

static int nad_adouble_dir(char *dir, size_t dirlen)
{
    char *slash;

    if (strlcpy(dir, nad.adpath[RESOURCE], dirlen) >= dirlen) {
        errno = ENAMETOOLONG;
        return -1;
    }

    slash = strrchr(dir, '/');

    if (slash == NULL) {
        errno = EINVAL;
        return -1;
    }

    *slash = '\0';
    return 0;
}

static int nad_prepare_adouble_dir(int parentfd, char *dir, size_t dirlen,
                                   mode_t mode, struct stat *st, int *created)
{
    int dirfd;
    *created = 0;

    if (nad_adouble_dir(dir, dirlen) < 0) {
        return -1;
    }

    /*
     * New metadata directory is owner-accessible for the following ad_open(),
     * then restore the inherited mode after both fork files have been created.
     */
    if (mkdirat(parentfd, dir, mode | S_IRWXU) < 0) {
        if (errno != EEXIST) {
            return -1;
        }
    } else {
        *created = 1;
    }

    dirfd = openat(parentfd, dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);

    if (dirfd < 0) {
        return -1;
    }

    if (fstat(dirfd, st) < 0) {
        close(dirfd);
        return -1;
    }

    if (*created && fchmod(dirfd, mode | S_IRWXU) < 0) {
        close(dirfd);
        return -1;
    }

    return dirfd;
}

static int nad_set_created_modes(mode_t data_mode, mode_t header_mode,
                                 int dirfd, mode_t dir_mode,
                                 int created_dir, int created_header)
{
    if (created_dir && fchmod(dirfd, dir_mode) < 0) {
        return -1;
    }

    if (ad_data_fileno(&nad.ad) >= 0
            && fchmod(ad_data_fileno(&nad.ad), data_mode) < 0) {
        return -1;
    }

    if (created_header && ad_meta_fileno(&nad.ad) >= 0
            && fchmod(ad_meta_fileno(&nad.ad), header_mode) < 0) {
        return -1;
    }

    return 0;
}

int nad_open(char *path, int openflags, struct FHeader *fh, int options _U_)
{
    struct stat		st;
    struct stat		ad_dir_st;
    const char		*adpath;
    char		ad_dirpath[MAXPATHLEN + 1];
    int			rc;
    int			created_ad_dir;
    int			created_header;
    int			parentfd;
    int			ad_dirfd;
    mode_t		data_mode;
    mode_t		header_mode;
    mode_t		dir_mode;
    /*
     * Depending upon openflags, set up nad.adpath for the open.  If it
     * is for write, then stat the current directory to get its mode.
     * Open the file.  Either fill or grab the adouble information.
     */
    nad.write_metadata = (openflags != O_RDONLY);

    if (nad.vol == NULL) {
        fprintf(stderr, "No AFP volume selected\n");
        return -1;
    }

    if (openflags == O_RDONLY) {
        ad_init(&nad.ad, nad.vol);
        nad.closeflags = ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF;

        if (strlcpy(nad.adpath[0], path, sizeof(nad.adpath[0]))
                >= sizeof(nad.adpath[0])) {
            fprintf(stderr, "Path is too long: %s\n", path);
            return -1;
        }

        adpath = nad.vol->ad_path(nad.adpath[0], ADFLAGS_HF);

        if (adpath == NULL || strlcpy(nad.adpath[1], adpath,
                                      sizeof(nad.adpath[1]))
                >= sizeof(nad.adpath[1])) {
            fprintf(stderr, "Metadata path is too long: %s\n", path);
            return -1;
        }

        if (stat(nad.adpath[ DATA ], &st) < 0) {
            perror("stat of data fork failed");
            return -1;
        }

#if DEBUG
        NAD_DEBUG("%s is adpath[0]\n", nad.adpath[0]);
        NAD_DEBUG("%s is adpath[1]\n", nad.adpath[1]);
#endif /* DEBUG */

        if (ad_open(&nad.ad, nad.adpath[ 0 ],
                    nad.closeflags | ADFLAGS_RDONLY) < 0) {
            perror(nad.adpath[ 0 ]);
            return -1;
        }

        rc = nad_header_read(fh);

        if (rc < 0) {
            ad_close(&nad.ad, nad.closeflags);
        }

        return rc;
    } else {
        ad_init(&nad.ad, nad.vol);
        nad.closeflags = ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF;

        if (strlcpy(nad.macname, fh->name, sizeof(nad.macname))
                >= sizeof(nad.macname)
                || strlcpy(nad.adpath[0], mtoupath(nad.macname),
                           sizeof(nad.adpath[0])) >= sizeof(nad.adpath[0])) {
            fprintf(stderr, "Path is too long: %s\n", fh->name);
            return -1;
        }

        adpath = nad.vol->ad_path(nad.adpath[0], ADFLAGS_HF);

        if (adpath == NULL || strlcpy(nad.adpath[1], adpath,
                                      sizeof(nad.adpath[1]))
                >= sizeof(nad.adpath[1])) {
            fprintf(stderr, "Metadata path is too long: %s\n", nad.adpath[0]);
            return -1;
        }

#if DEBUG
        NAD_DEBUG("%s\n", nad.macname);
        NAD_DEBUG("%s is adpath[0]\n", nad.adpath[0]);
        NAD_DEBUG("%s is adpath[1]\n", nad.adpath[1]);
#endif /* DEBUG */
        parentfd = open(".", O_RDONLY | O_DIRECTORY);

        if (parentfd < 0) {
            perror("open of . failed");
            return -1;
        }

        if (fstat(parentfd, &st) < 0) {
            perror("stat of . failed");
            close(parentfd);
            return -1;
        }

        data_mode = (mode_t)(st.st_mode & 0666);
        dir_mode = (mode_t)(st.st_mode & 0777);

        if (nad.vol->v_adouble == AD_VERSION2) {
            ad_dirfd = nad_prepare_adouble_dir(parentfd, ad_dirpath,
                                               sizeof(ad_dirpath), dir_mode,
                                               &ad_dir_st, &created_ad_dir);

            if (ad_dirfd < 0) {
                perror("preparing AppleDouble directory failed");
                close(parentfd);
                return -1;
            }

            if (!created_ad_dir) {
                header_mode = ad_hf_mode((mode_t)(data_mode & ad_dir_st.st_mode));
            } else {
                header_mode = ad_hf_mode(data_mode);
            }

            created_header = (fstatat(parentfd, nad.adpath[RESOURCE], &ad_dir_st,
                                      AT_SYMLINK_NOFOLLOW) < 0
                              && errno == ENOENT);
        } else {
            ad_dirfd = -1;
            created_ad_dir = 0;
            created_header = 0;
            header_mode = data_mode;
        }

        rc = ad_openat(&nad.ad, parentfd, nad.adpath[ 0 ],
                       nad.closeflags | ADFLAGS_RDWR
                       | ADFLAGS_CREATE | ADFLAGS_EXCL,
                       data_mode);

        if (rc < 0) {
            perror(nad.adpath[ 0 ]);

            if (ad_dirfd >= 0 && created_ad_dir && fchmod(ad_dirfd, dir_mode) < 0) {
                perror("restoring AppleDouble directory mode failed");
            }

            if (ad_dirfd >= 0) {
                close(ad_dirfd);
            }

            close(parentfd);
            return -1;
        }

        if (ad_dirfd >= 0
                && nad_set_created_modes(data_mode, header_mode, ad_dirfd, dir_mode,
                                         created_ad_dir, created_header) < 0) {
            perror("setting AppleDouble modes failed");
            ad_close(&nad.ad, nad.closeflags);

            if (ad_dirfd >= 0) {
                close(ad_dirfd);
            }

            close(parentfd);
            return -1;
        }

        if (ad_dirfd >= 0) {
            close(ad_dirfd);
        }

        close(parentfd);
        rc = nad_header_write(fh);

        if (rc < 0) {
            ad_close(&nad.ad, nad.closeflags);
        }

        return rc;
    }
}

int nad_header_read(struct FHeader *fh)
{
    uint32_t		temptime;
    struct stat		st;
    size_t		name_len;
    ssize_t		rfork_len;
    const char		*name;
    const char          *finderi;
    char 		*p;
    name_len = ad_getentrylen(&nad.ad, ADEID_NAME);

    if (name_len > 0) {
        if (name_len >= sizeof(fh->name)) {
            fprintf(stderr, "Mac filename is too long\n");
            return -1;
        }

        name = ad_entry(&nad.ad, ADEID_NAME);

        if (name == NULL) {
            fprintf(stderr, "Invalid AppleDouble filename entry\n");
            return -1;
        }

        memcpy(fh->name, name, name_len);
        fh->name[name_len] = '\0';
    }

    /* just in case there's nothing in macname */
    if (fh->name[0] == '\0') {
        if (NULL == (p = strrchr(nad.adpath[DATA], '/'))) {
            p = nad.adpath[DATA];
        } else {
            p++;
        }

        if (strlcpy(fh->name, utompath(p), sizeof(fh->name))
                >= sizeof(fh->name)) {
            fprintf(stderr, "Mac filename is too long: %s\n", p);
            return -1;
        }
    }

    if (stat(nad.adpath[ DATA ], &st) < 0) {
        perror("stat of datafork failed");
        return -1;
    }

    rfork_len = ad_size(&nad.ad, ADEID_RFORK);

    if (st.st_size < 0 || (uintmax_t)st.st_size > UINT32_MAX) {
        fprintf(stderr, "Data fork is too large: %jd bytes\n",
                (intmax_t)st.st_size);
        return -1;
    }

    if (rfork_len < 0 || (uintmax_t)rfork_len > UINT32_MAX) {
        fprintf(stderr, "Resource fork is too large: %jd bytes\n",
                (intmax_t)rfork_len);
        return -1;
    }

    fh->forklen[ DATA ] = htonl((uint32_t)st.st_size);
    fh->forklen[ RESOURCE ] = htonl((uint32_t)rfork_len);
    fh->comment[0] = '\0';
#if DEBUG
    NAD_DEBUG("macname of file\t\t\t%.*s\n",
              (int)strnlen(fh->name, sizeof(fh->name)),
              fh->name);
    NAD_DEBUG("size of data fork\t\t%d\n",
              ntohl(fh->forklen[ DATA ]));
    NAD_DEBUG("size of resource fork\t\t%d\n",
              ntohl(fh->forklen[ RESOURCE ]));
    NAD_DEBUG("get info comment\t\t\"%s\"\n", fh->comment);
#endif /* DEBUG */
    ad_getdate(&nad.ad, AD_DATE_CREATE, &temptime);
    memcpy(&fh->create_date, &temptime, sizeof(temptime));
    ad_getdate(&nad.ad, AD_DATE_MODIFY, &temptime);
    memcpy(&fh->mod_date, &temptime, sizeof(temptime));
    ad_getdate(&nad.ad, AD_DATE_BACKUP, &temptime);
    memcpy(&fh->backup_date, &temptime, sizeof(temptime));
    fh->date_flags = FH_DATE_CREATE | FH_DATE_MODIFY | FH_DATE_BACKUP;
#if DEBUG
    memcpy(&temptime, &fh->create_date, sizeof(temptime));
    temptime = AD_DATE_TO_UNIX(temptime);
    NAD_DEBUG("create_date seconds\t\t%lu\n", temptime);
    memcpy(&temptime, &fh->mod_date, sizeof(temptime));
    temptime = AD_DATE_TO_UNIX(temptime);
    NAD_DEBUG("mod_date seconds\t\t%lu\n", temptime);
    memcpy(&temptime, &fh->backup_date, sizeof(temptime));
    temptime = AD_DATE_TO_UNIX(temptime);
    NAD_DEBUG("backup_date seconds\t\t%lu\n", temptime);
    NAD_DEBUG("size of finder_info\t\t%d\n", sizeof(fh->finder_info));
#endif /* DEBUG */
    finderi = ad_entry(&nad.ad, ADEID_FINDERI);

    if (finderi == NULL) {
        memset(&fh->finder_info, 0, sizeof(fh->finder_info));
        memset(&fh->finder_xinfo, 0, sizeof(fh->finder_xinfo));
    } else {
        memcpy(&fh->finder_info.fdType, finderi + FINDERIOFF_TYPE,
               sizeof(fh->finder_info.fdType));
        memcpy(&fh->finder_info.fdCreator, finderi + FINDERIOFF_CREATOR,
               sizeof(fh->finder_info.fdCreator));
        memcpy(&fh->finder_info.fdFlags, finderi + FINDERIOFF_FLAGS,
               sizeof(fh->finder_info.fdFlags));
        memcpy(&fh->finder_info.fdLocation, finderi + FINDERIOFF_LOC,
               sizeof(fh->finder_info.fdLocation));
        memcpy(&fh->finder_info.fdFldr, finderi + FINDERIOFF_FLDR,
               sizeof(fh->finder_info.fdFldr));
        memcpy(&fh->finder_xinfo.fdScript, finderi + FINDERIOFF_SCRIPT,
               sizeof(fh->finder_xinfo.fdScript));
        memcpy(&fh->finder_xinfo.fdXFlags, finderi + FINDERIOFF_XFLAGS,
               sizeof(fh->finder_xinfo.fdXFlags));
    }

#if DEBUG
    {
        short		flags;
        NAD_DEBUG("finder_info.fdType\t\t%.*s\n",
                  sizeof(fh->finder_info.fdType), &fh->finder_info.fdType);
        NAD_DEBUG("finder_info.fdCreator\t\t%.*s\n",
                  sizeof(fh->finder_info.fdCreator),
                  &fh->finder_info.fdCreator);
        NAD_DEBUG("nad type and creator\t\t%.*s\n\n",
                  sizeof(fh->finder_info.fdType) +
                  sizeof(fh->finder_info.fdCreator),
                  ad_entry(&nad.ad, ADEID_FINDERI));
        memcpy(&flags, ad_entry(&nad.ad, ADEID_FINDERI) +
               FINDERIOFF_FLAGS, sizeof(flags));
        NAD_DEBUG("nad.ad flags\t\t\t%x\n", flags);
        NAD_DEBUG("fh flags\t\t\t%x\n", fh->finder_info.fdFlags);
        NAD_DEBUG("fh script\t\t\t%x\n", fh->finder_xinfo.fdScript);
        NAD_DEBUG("fh xflags\t\t\t%x\n", fh->finder_xinfo.fdXFlags);
    }
#endif /* DEBUG */
    nad.offset[ DATA ] = nad.offset[ RESOURCE ] = 0;
    return 0;
}

int nad_header_write(struct FHeader *fh)
{
    uint32_t		temptime;
    size_t		name_len;
    size_t		comment_len;
    name_len = strnlen(nad.macname, sizeof(nad.macname));

    if (name_len > ADEDLEN_NAME) {
        fprintf(stderr, "Mac filename is too long: %s\n", nad.macname);
        return -1;
    }

    comment_len = strnlen(fh->comment, sizeof(fh->comment));

    if (comment_len > ADEDLEN_COMMENT) {
        fprintf(stderr, "Mac comment is too long\n");
        return -1;
    }

    if (nad.vol->v_adouble == AD_VERSION2) {
        ad_setentrylen(&nad.ad, ADEID_NAME, name_len);
        memcpy(ad_entry(&nad.ad, ADEID_NAME), nad.macname,
               ad_getentrylen(&nad.ad, ADEID_NAME));
    }

    ad_setentrylen(&nad.ad, ADEID_COMMENT, comment_len);
    memcpy(ad_entry(&nad.ad, ADEID_COMMENT), fh->comment,
           ad_getentrylen(&nad.ad, ADEID_COMMENT));
    ad_setentrylen(&nad.ad, ADEID_RFORK, ntohl(fh->forklen[ RESOURCE ]));
#if DEBUG
    NAD_DEBUG("ad_getentrylen\n");
    NAD_DEBUG("ADEID_FINDERI\t\t\t%d\n",
              ad_getentrylen(&nad.ad, ADEID_FINDERI));
    NAD_DEBUG("ADEID_RFORK\t\t\t%d\n",
              ad_getentrylen(&nad.ad, ADEID_RFORK));
    NAD_DEBUG("ADEID_NAME\t\t\t%d\n",
              ad_getentrylen(&nad.ad, ADEID_NAME));
    NAD_DEBUG("ad_entry of ADEID_NAME\t\t%.*s\n",
              ad_getentrylen(&nad.ad, ADEID_NAME),
              ad_entry(&nad.ad, ADEID_NAME));
    NAD_DEBUG("ADEID_COMMENT\t\t\t%d\n",
              ad_getentrylen(&nad.ad, ADEID_COMMENT));
#endif /* DEBUG */
    memcpy(&temptime, &fh->create_date, sizeof(temptime));
    ad_setdate(&nad.ad, AD_DATE_CREATE, temptime);
    memcpy(&temptime, &fh->mod_date, sizeof(temptime));
    ad_setdate(&nad.ad, AD_DATE_MODIFY, temptime);
#if DEBUG
    ad_getdate(&nad.ad, AD_DATE_CREATE, &temptime);
    temptime = AD_DATE_TO_UNIX(temptime);
    NAD_DEBUG("FILEIOFF_CREATE seconds\t\t%ld\n", temptime);
    ad_getdate(&nad.ad, AD_DATE_MODIFY, &temptime);
    temptime = AD_DATE_TO_UNIX(temptime);
    NAD_DEBUG("FILEIOFF_MODIFY seconds\t\t%ld\n", temptime);
#endif /* DEBUG */
    memset(ad_entry(&nad.ad, ADEID_FINDERI), 0, ADEDLEN_FINDERI);
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_TYPE,
           &fh->finder_info.fdType, sizeof(fh->finder_info.fdType));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_CREATOR,
           &fh->finder_info.fdCreator, sizeof(fh->finder_info.fdCreator));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_FLAGS,
           &fh->finder_info.fdFlags, sizeof(fh->finder_info.fdFlags));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_LOC,
           &fh->finder_info.fdLocation, sizeof(fh->finder_info.fdLocation));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_FLDR,
           &fh->finder_info.fdFldr, sizeof(fh->finder_info.fdFldr));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_SCRIPT,
           &fh->finder_xinfo.fdScript, sizeof(fh->finder_xinfo.fdScript));
    memcpy(ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_XFLAGS,
           &fh->finder_xinfo.fdXFlags, sizeof(fh->finder_xinfo.fdXFlags));
#if DEBUG
    {
        short		flags;
        memcpy(&flags, (ad_entry(&nad.ad, ADEID_FINDERI) + FINDERIOFF_FLAGS),
               sizeof(flags));
        NAD_DEBUG("nad.ad flags\t\t\t%x\n", flags);
        NAD_DEBUG("fh flags\t\t\t%x\n", fh->finder_info.fdFlags);
        NAD_DEBUG("fh xflags\t\t\t%x\n", fh->finder_xinfo.fdXFlags);
        NAD_DEBUG("type and creator\t\t%.*s\n\n",
                  sizeof(fh->finder_info.fdType) +
                  sizeof(fh->finder_info.fdCreator),
                  ad_entry(&nad.ad, ADEID_FINDERI));
    }
#endif /* DEBUG */
    nad.offset[ DATA ] = nad.offset[ RESOURCE ] = 0;
    ad_flush(&nad.ad);
    return 0;
}

static int		forkeid[] = { ADEID_DFORK, ADEID_RFORK };

ssize_t nad_read(int fork, char *forkbuf, size_t bufc)
{
    ssize_t		cc = 0;
#if DEBUG
    NAD_DEBUG("Entering nad_read\n");
#endif /* DEBUG */

    if ((cc = ad_read(&nad.ad, forkeid[ fork ], nad.offset[ fork ],
                      forkbuf, bufc)) < 0)  {
        perror("Reading the appledouble file:");
        return cc;
    }

    nad.offset[ fork ] += cc;
#if DEBUG
    NAD_DEBUG("Exiting nad_read\n");
#endif /* DEBUG */
    return cc;
}

ssize_t nad_write(int fork, char *forkbuf, size_t bufc)
{
    const char		*buf_ptr;
    size_t		writelen;
    ssize_t		cc = 0;
#if DEBUG
    NAD_DEBUG("Entering nad_write\n");
#endif /* DEBUG */
    writelen = bufc;
    buf_ptr = forkbuf;

    while (writelen > 0) {
        cc =  ad_write(&nad.ad, forkeid[ fork ], nad.offset[ fork ],
                       0, buf_ptr, writelen);

        if (cc <= 0) {
            break;
        }

        nad.offset[ fork ] += cc;
        buf_ptr += cc;
        writelen -= cc;
    }

    if (writelen > 0) {
        if (cc < 0) {
            perror("Writing the appledouble file:");
            return cc;
        }

        fprintf(stderr, "Writing the appledouble file: short write\n");
        return -1;
    }

    return bufc;
}

static int nad_update_cnid(void)
{
    struct stat st;
    cnid_t cnid;
    cnid_t did;

    if (!nad.write_metadata) {
        return 0;
    }

    if (nad.volume == NULL || nad.volume->fallback || nad.volume->skip_cnid
            || nad.vol == NULL) {
        return 0;
    }

    if (nad.vol->v_cdb == NULL) {
        fprintf(stderr, "No CNID database selected for %s\n", nad.adpath[DATA]);
        errno = EINVAL;
        return -1;
    }

    if (lstat(nad.adpath[DATA], &st) != 0) {
        perror(nad.adpath[DATA]);
        return -1;
    }

    cnid = cnid_for_path(nad.vol->v_cdb, nad.vol->v_path, nad.adpath[DATA],
                         &did);

    if (cnid == CNID_INVALID) {
        fprintf(stderr, "Error resolving CNID for %s\n", nad.adpath[DATA]);
        errno = EIO;
        return -1;
    }

    if (ad_setid(&nad.ad, st.st_dev, st.st_ino, cnid, did,
                 nad.volume->db_stamp) <= 0) {
        fprintf(stderr, "Error storing CNID metadata for %s\n", nad.adpath[DATA]);
        errno = EIO;
        return -1;
    }

    return 0;
}

int nad_close(int status)
{
    int			rv;
    int                 close_rv;

    if (status == KEEP) {
        if ((rv = ad_flush(&nad.ad)) < 0) {
            fprintf(stderr, "nad_close rv for flush %d\n", rv);
        } else if (nad_update_cnid() < 0) {
            rv = -1;
        } else if ((rv = ad_flush(&nad.ad)) < 0) {
            fprintf(stderr, "nad_close rv for CNID flush %d\n", rv);
        }

        if ((close_rv = ad_close(&nad.ad, nad.closeflags)) < 0) {
            fprintf(stderr, "nad_close rv for close %d\n", close_rv);
            return close_rv;
        }

        if (rv < 0) {
            return rv;
        }
    } else if (status == TRASH) {
        if ((close_rv = ad_close(&nad.ad, nad.closeflags)) < 0) {
            fprintf(stderr, "nad_close rv for close %d\n", close_rv);
        }

        if (unlink(nad.adpath[ 0 ]) < 0) {
            perror(nad.adpath[ 0 ]);
        }

        if (strcmp(nad.adpath[0], nad.adpath[1]) != 0
                && unlink(nad.adpath[ 1 ]) < 0) {
            perror(nad.adpath[ 1 ]);
        }

        return 0;
    } else {
        return -1;
    }

    return 0;
}
