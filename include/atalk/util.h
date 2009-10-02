/*
 * $Id: util.h,v 1.10 2009-10-02 09:32:40 franklahm Exp $
 */

#ifndef _ATALK_UTIL_H
#define _ATALK_UTIL_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <netatalk/at.h>
#include <atalk/unicode.h>

/* exit error codes */
#define EXITERR_CLNT 1  /* client related error */
#define EXITERR_CONF 2  /* error in config files/cmd line parameters */
#define EXITERR_SYS  3  /* local system error */


extern int     sys_ftruncate __P((int fd, off_t length));

#ifdef WITH_SENDFILE
extern ssize_t sys_sendfile __P((int __out_fd, int __in_fd, off_t *__offset,size_t __count));
#endif

extern const int _diacasemap[], _dialowermap[];

extern char **getifacelist(void);
extern void freeifacelist(char **);

#define diatolower(x)     _dialowermap[(unsigned char) (x)]
#define diatoupper(x)     _diacasemap[(unsigned char) (x)]
extern int atalk_aton     __P((char *, struct at_addr *));
extern void bprint        __P((char *, int));
extern int strdiacasecmp  __P((const char *, const char *));
extern int strndiacasecmp __P((const char *, const char *, size_t));
extern pid_t server_lock  __P((char * /*program*/, char * /*file*/, 
			       int /*debug*/));
extern void fault_setup	  __P((void (*fn)(void *)));
#define server_unlock(x)  (unlink(x))

#ifndef HAVE_STRLCPY
size_t strlcpy __P((char *, const char *, size_t));
#endif
 
#ifndef HAVE_STRLCAT
size_t strlcat __P((char *, const char *, size_t));
#endif

#ifndef HAVE_DLFCN_H
extern void *mod_open    __P((const char *));
extern void *mod_symbol  __P((void *, const char *));
extern void mod_close    __P((void *));
#define mod_error()      ""
#else /* ! HAVE_DLFCN_H */
#include <dlfcn.h>

#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif /* ! RTLD_NOW */

/* NetBSD doesn't like RTLD_NOW for dlopen (it fails). Use RTLD_LAZY.
 * OpenBSD currently does not use the second arg for dlopen(). For
 * future compatibility we define DL_LAZY */
#ifdef __NetBSD__
#define mod_open(a)      dlopen(a, RTLD_LAZY)
#elif defined(__OpenBSD__)
#define mod_open(a)      dlopen(a, DL_LAZY)
#else /* ! __NetBSD__ && ! __OpenBSD__ */
#define mod_open(a)      dlopen(a, RTLD_NOW)
#endif /* __NetBSD__ */

#ifndef DLSYM_PREPEND_UNDERSCORE
#define mod_symbol(a, b) dlsym(a, b)
#else /* ! DLSYM_PREPEND_UNDERSCORE */
extern void *mod_symbol  __P((void *, const char *));
#endif /* ! DLSYM_PREPEND_UNDERSCORE */
#define mod_error()      dlerror()
#define mod_close(a)     dlclose(a)
#endif /* ! HAVE_DLFCN_H */

#if 0
/* volinfo for shell utilities */
#define VOLINFOFILE ".volinfo"

struct volinfo {
    char                *v_name;
    char                *v_path;
    int                 v_flags;
    int                 v_casefold;
    char                *v_cnidscheme;
    char                *v_dbpath;
    char                *v_volcodepage;
    charset_t           v_volcharset;
    char                *v_maccodepage;
    charset_t           v_maccharset;
    int                 v_adouble;  /* default adouble format */
    char                *(*ad_path)(const char *, int);
    char                *v_dbd_host;
    int                 v_dbd_port;
};

extern int loadvolinfo __P((char *path, struct volinfo *vol));
extern int vol_load_charsets __P(( struct volinfo *vol));
#endif /* 0 */

/*
 * Function: lock_reg
 *
 * Purpose: lock a file with fctnl
 *
 * Arguments:
 *
 *    fd         (r) File descriptor
 *    cmd        (r) cmd to fcntl, only F_SETLK is usable here
 *    type       (r) F_RDLCK, F_WRLCK, F_UNLCK
 *    offset     (r) byte offset relative to l_whence
 *    whence     (r) SEEK_SET, SEEK_CUR, SEEK_END
 *    len        (r) no. of bytes (0 means to EOF)
 *
 * Returns: 0 on success, -1 on failure
 *          fcntl return value and errno
 *
 * Effects:
 *
 * Function called by macros to ease locking.
 */
extern int lock_reg(int fd, int cmd, int type, off_t offest, int whence, off_t len);

/*
 * Macros: read_lock, write_lock, un_lock
 *
 * Purpose: lock and unlock files
 *
 * Arguments:
 *
 *    fd         (r) File descriptor
 *    offset     (r) byte offset relative to l_whence
 *    whence     (r) SEEK_SET, SEEK_CUR, SEEK_END
 *    len        (r) no. of bytes (0 means to EOF)
 *
 * Returns: 0 on success, -1 on failure
 *          fcntl return value and errno
 *
 * Effects:
 *
 * Nice locking macros.
 */

#define read_lock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define write_lock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define unlock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

#endif
