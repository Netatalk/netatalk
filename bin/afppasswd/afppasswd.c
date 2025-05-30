/*
 * Copyright 1999 (c) Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * format of the password file:
 * name:****************:****************:********
 *      password         last login date  failed usage count
 *
 * ***'s are illegal. they're just place holders for hex values. hex
 * values that represent actual numbers are in network byte order.
 *
 * last login date is currently a 4-byte number representing seconds
 * since 1970 (UTC). there's enough space to extend it to 8 bytes.
 *
 * the last two fields aren't currently used by the randnum uams.
 *
 * root syntax: afppasswd [-c] [-a] [-p path] [-f] [username]
 * user syntax: afppasswd
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gcrypt.h>

#include <atalk/compat.h>

#ifndef DES_KEY_SZ
#define DES_KEY_SZ 8
#endif

#ifdef USE_CRACKLIB
#include <crack.h>
#endif /* USE_CRACKLIB */

#define OPT_ISROOT  (1 << 0)
#define OPT_CREATE  (1 << 1)
#define OPT_FORCE   (1 << 2)
#define OPT_ADDUSER (1 << 3)
#define OPT_NOCRACK (1 << 4)

#define PASSWD_ILLEGAL '*'

#define FORMAT  ":****************:****************:********\n"
#define FORMAT_LEN 44
#define OPTIONS "cafnu:p:w:"
#define UID_START 100

#define HEXPASSWDLEN 16
#define PASSWDLEN 8

static char buf[MAXPATHLEN + 1];

/* if newpwd is null, convert buf from hex to binary. if newpwd isn't
 * null, convert newpwd to hex and save it in buf. */
#define unhex(x)  (isdigit(x) ? (x) - '0' : toupper(x) + 10 - 'A')
static void convert_passwd(char *buf, char *newpwd, const int keyfd)
{
    uint8_t key[HEXPASSWDLEN];
    unsigned int i, j;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;

    if (!newpwd) {
        /* convert to binary */
        for (i = j = 0; i < sizeof(key); i += 2, j++) {
            buf[j] = (unhex(buf[i]) << 4) | unhex(buf[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(buf + j, 0, sizeof(key) - j);
        }
    }

    if (keyfd > -1) {
        lseek(keyfd, 0, SEEK_SET);

        if (read(keyfd, key, sizeof(key)) < 0) {
            fprintf(stderr, "could not read key (%s)\n", strerror(errno));
        }

        /* convert to binary */
        for (i = j = 0; i < sizeof(key); i += 2, j++) {
            key[j] = (unhex(key[i]) << 4) | unhex(key[i + 1]);
        }

        if (j <= DES_KEY_SZ) {
            memset(key + j, 0, sizeof(key) - j);
        }

        ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
        ctxerror = gcry_cipher_setkey(ctx, key, DES_KEY_SZ);
        memset(key, 0, sizeof(key));

        if (newpwd) {
            ctxerror = gcry_cipher_encrypt(ctx, newpwd, DES_KEY_SZ, NULL, 0);
        } else {
            /* decrypt the password */
            ctxerror = gcry_cipher_decrypt(ctx, buf, DES_KEY_SZ, NULL, 0);
        }

        gcry_cipher_close(ctx);
    }

    if (newpwd) {
        const unsigned char hextable[] = "0123456789ABCDEF";

        /* convert to hex */
        for (i = j = 0; i < DES_KEY_SZ; i++, j += 2) {
            buf[j] = hextable[(newpwd[i] & 0xF0) >> 4];
            buf[j + 1] = hextable[newpwd[i] & 0x0F];
        }
    }
}

/* this matches the code in uam_randnum.c */
static int update_passwd(const char *path, const char *name, int flags,
                         const char *pass)
{
    char password[PASSWDLEN + 1], *p, *passwd = "";
    FILE *fp;
    off_t pos;
    int keyfd = -1, err = 0;

    if ((fp = fopen(path, "r+")) == NULL) {
        fprintf(stderr, "afppasswd: can't open %s: %s\n", path, strerror(errno));
        return -1;
    }

    /* open the key file if it exists */
    strlcpy(buf, path, sizeof(buf));

    if (strlen(path) < sizeof(buf) - 5) {
        strlcat(buf, ".key", sizeof(buf));
        keyfd = open(buf, O_RDONLY);
    }

    pos = ftell(fp);
    memset(buf, 0, sizeof(buf));

    while (fgets(buf, sizeof(buf), fp)) {
        if ((p = strchr(buf, ':'))) {
            /* check for a match */
            if (strlen(name) == (p - buf) &&
                    strncmp(buf, name, p - buf) == 0) {
                p++;

                if (!(flags & OPT_ISROOT) && (*p == PASSWD_ILLEGAL)) {
                    fprintf(stderr, "Your password is disabled. Please see your administrator.\n");
                    break;
                }

                goto found_entry;
            }
        }

        pos = ftell(fp);
        memset(buf, 0, sizeof(buf));
    }

    if (flags & OPT_ADDUSER) {
        strlcpy(buf, name, sizeof(buf));
        strlcat(buf, FORMAT, sizeof(buf));
        p = strchr(buf, ':') + 1;
        fwrite(buf, strlen(buf), 1, fp);
    } else {
        fprintf(stderr, "afppasswd: can't find %s in %s\n", name, path);
        err = -1;
        goto update_done;
    }

found_entry:

    /* need to verify against old password */
    if ((flags & OPT_ISROOT) == 0) {
        passwd = getpass("Enter OLD AFP password: ");
        convert_passwd(p, NULL, keyfd);

        if (strncmp(passwd, p, PASSWDLEN)) {
            fprintf(stderr, "afppasswd: invalid password.\n");
            err = -1;
            goto update_done;
        }
    }

    /* new password */
    if (strlen(pass) < 1) {
        passwd = getpass("Enter NEW AFP password: ");

        if (strlen(passwd) > PASSWDLEN) {
            fprintf(stderr, "afppasswd: max password length is %d.\n", PASSWDLEN);
            err = -1;
            goto update_done;
        }

        /* Make sure we null out any remaining bytes of the input string */
        if (strlen(passwd) < PASSWDLEN) {
            for (int s = (int) strlen(passwd); s <= PASSWDLEN; s++) {
                passwd[s] = '\0';
            }
        }

        memcpy(password, passwd, sizeof(password));
    } else {
        memcpy(password, pass, sizeof(password));

        if (strlen(pass) < PASSWDLEN) {
            for (int i = (int) strlen(pass); i <= PASSWDLEN; i++) {
                password[i] = '\0';
            }
        }
    }

    password[PASSWDLEN] = '\0';
#ifdef USE_CRACKLIB

    if (!(flags & OPT_NOCRACK)) {
        const char *pwcheck = FascistCheck(password, _PATH_CRACKLIB);

        if (pwcheck) {
            fprintf(stderr, "Error: %s\n", pwcheck);
            err = -1;
            goto update_done;
        }
    }

#endif /* USE_CRACKLIB */

    if (strlen(pass) < 1) {
        passwd = getpass("Enter NEW AFP password again: ");
    }

    if (strcmp(passwd, password) == 0 || strlen(pass) > 0) {
        struct flock lock;
        int fd = fileno(fp);
        convert_passwd(p, password, keyfd);
        lock.l_type = F_WRLCK;
        lock.l_start = pos;
        lock.l_len = 1;
        lock.l_whence = SEEK_SET;
        fseek(fp, pos, SEEK_SET);
        fcntl(fd, F_SETLKW, &lock);
        fwrite(buf, p - buf + HEXPASSWDLEN, 1, fp);
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        printf("afppasswd: updated password.\n");
    } else {
        fprintf(stderr, "afppasswd: passwords don't match!\n");
        err = -1;
    }

update_done:

    if (keyfd > -1) {
        close(keyfd);
    }

    fclose(fp);
    return err;
}


/* creates a file with all the password entries */
static int create_file(const char *path, uid_t minuid)
{
    struct passwd *pwd;
    int fd, len, err = 0;

    if ((fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600)) < 0) {
        fprintf(stderr, "afppasswd: can't create %s\n", path);
        return -1;
    }

    setpwent();

    while ((pwd = getpwent())) {
        if (pwd->pw_uid < minuid) {
            continue;
        }

        /* a little paranoia */
        if (strlen(pwd->pw_name) + FORMAT_LEN > sizeof(buf) - 1) {
            continue;
        }

        strlcpy(buf, pwd->pw_name, sizeof(buf));
        strlcat(buf, FORMAT, sizeof(buf));
        len = strlen(buf);

        if (write(fd, buf, len) != len) {
            fprintf(stderr, "afppasswd: problem writing to %s: %s\n", path,
                    strerror(errno));
            err = -1;
            break;
        }
    }

    endpwent();
    close(fd);
    return err;
}


int main(int argc, char **argv)
{
    struct stat st;
    int flags;
    uid_t uid_min = UID_START, uid;
    char *path = _PATH_AFPDPWFILE;
    const char *pass = "";
    int i, err = 0;
    extern char *optarg;
    extern int optind;
    flags = ((uid = getuid()) == 0) ? OPT_ISROOT : 0;

    if (((flags & OPT_ISROOT) == 0) && (argc > 1)) {
        fprintf(stderr, "afppasswd (Netatalk %s)\n", VERSION);
        fprintf(stderr,
                "Usage: afppasswd [-acfn] [-u minuid] [-p path] [-w string] [username]\n");
        fprintf(stderr, "  -a        add a new user\n");
        fprintf(stderr,
                "  -c        create and initialize password file or specific user\n");
        fprintf(stderr, "  -f        force an action\n");
#ifdef USE_CRACKLIB
        fprintf(stderr, "  -n        disable cracklib checking of passwords\n");
#endif /* USE_CRACKLIB */
        fprintf(stderr, "  -u uid    minimum uid to use, defaults to 100\n");
        fprintf(stderr, "  -p path   path to afppasswd file\n");
        fprintf(stderr, "  -w string use string as password\n");
        return -1;
    }

    while ((i = getopt(argc, argv, OPTIONS)) != EOF) {
        switch (i) {
        case 'c': /* create and initialize password file or specific user */
            flags |= OPT_CREATE;
            break;

        case 'a': /* add a new user */
            flags |= OPT_ADDUSER;
            break;

        case 'f': /* force an action */
            flags |= OPT_FORCE;
            break;

        case 'u':  /* minimum uid to use. default is 100 */
            uid_min = atoi(optarg);
            break;
#ifdef USE_CRACKLIB

        case 'n': /* disable CRACKLIB check */
            flags |= OPT_NOCRACK;
            break;
#endif /* USE_CRACKLIB */

        case 'p': /* path to afppasswd file */
            path = optarg;
            break;

        case 'w': /* password string */
            if (strlen(optarg) > PASSWDLEN) {
                fprintf(stderr, "afppasswd: max password length is %d.\n", PASSWDLEN);
                return -1;
            }

            pass = optarg;
            break;

        default:
            err++;
            break;
        }
    }

    if (err || (optind + ((flags & OPT_CREATE) ? 0 :
                          (flags & OPT_ISROOT)) != argc)) {
#ifdef USE_CRACKLIB
        fprintf(stderr,
                "Usage: afppasswd [-acfn] [-u minuid] [-p path] [-w string] [username]\n");
#else /* USE_CRACKLIB */
        fprintf(stderr,
                "Usage: afppasswd [-acf] [-u minuid] [-p path] [-w string] [username]\n");
#endif /* USE_CRACKLIB */
        fprintf(stderr, "  -a        add a new user\n");
        fprintf(stderr,
                "  -c        create and initialize password file or specific user\n");
        fprintf(stderr, "  -f        force an action\n");
#ifdef USE_CRACKLIB
        fprintf(stderr, "  -n        disable cracklib checking of passwords\n");
#endif /* USE_CRACKLIB */
        fprintf(stderr, "  -u uid    minimum uid to use, defaults to 100\n");
        fprintf(stderr, "  -p path   path to afppasswd file\n");
        fprintf(stderr, "  -w string use string as password\n");
        return -1;
    }

    i = stat(path, &st);

    if (flags & OPT_CREATE) {
        if ((flags & OPT_ISROOT) == 0) {
            fprintf(stderr, "afppasswd: only root can create the password file.\n");
            return -1;
        }

        if (!i && ((flags & OPT_FORCE) == 0)) {
            fprintf(stderr, "afppasswd: password file already exists.\n");
            return -1;
        }

        return create_file(path, uid_min);
    } else {
        struct passwd *pwd = NULL;

        if (i < 0) {
            fprintf(stderr, "afppasswd: %s doesn't exist.\n", path);
            return -1;
        }

        /* if we're root, we need to specify the username */
        pwd = (flags & OPT_ISROOT) ? getpwnam(argv[optind]) : getpwuid(uid);

        if (pwd) {
            return update_passwd(path, pwd->pw_name, flags, pass);
        }

        fprintf(stderr, "afppasswd: can't get password entry.\n");
        return -1;
    }
}
