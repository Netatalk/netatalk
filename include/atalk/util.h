/*!
 * @file
 * Netatalk utility functions
 *
 * Utility functions for these areas: \n
 * * sockets \n
 * * locking \n
 * * misc UNIX function wrappers, eg for getcwd
 */

#ifndef _ATALK_UTIL_H
#define _ATALK_UTIL_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef NO_DDP
#include <netatalk/at.h>
#endif
#include <atalk/unicode.h>

/* exit error codes */
#define EXITERR_CLNT 1  /* client related error */
#define EXITERR_CONF 2  /* error in config files/cmd line parameters */
#define EXITERR_SYS  3  /* local system error */

/* Print a SBT and exit */
#define AFP_PANIC(why) \
    do {                                            \
        netatalk_panic(why);                        \
        abort();                                    \
    } while(0);

/* LOG assert errors */
#ifndef NDEBUG
#define AFP_ASSERT(b) \
    do {                                                                \
        if (!(b)) {                                                     \
            AFP_PANIC(#b);                                              \
        } \
    } while(0);
#else
#define AFP_ASSERT(b)
#endif /* NDEBUG */

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#endif

#define STRCMP(a,b,c) (strcmp(a,c) b 0)
#define ZERO_STRUCT(a) memset(&(a), 0, sizeof(a))
#define ZERO_STRUCTP(a) memset((a), 0, sizeof(a))

#ifdef WITH_SENDFILE
extern ssize_t sys_sendfile (int __out_fd, int __in_fd, off_t *__offset,size_t __count);
#endif

extern const int _diacasemap[], _dialowermap[];

extern char **getifacelist(void);
extern void freeifacelist(char **);

#define diatolower(x)     _dialowermap[(unsigned char) (x)]
#define diatoupper(x)     _diacasemap[(unsigned char) (x)]
#ifndef NO_DDP
extern int atalk_aton     (char *, struct at_addr *);
#endif
extern void bprint        (char *, int);
extern int strdiacasecmp  (const char *, const char *);
extern int strndiacasecmp (const char *, const char *, size_t);
extern pid_t server_lock  (char * /*program*/, char * /*file*/, int /*debug*/);
extern void fault_setup	  (void (*fn)(void *));
extern void netatalk_panic(const char *why);
#define server_unlock(x)  (unlink(x))

#ifndef HAVE_DLFCN_H
extern void *mod_open    (const char *);
extern void *mod_symbol  (void *, const char *);
extern void mod_close    (void *);
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
extern void *mod_symbol  (void *, const char *);
#endif /* ! DLSYM_PREPEND_UNDERSCORE */
#define mod_error()      dlerror()
#define mod_close(a)     dlclose(a)
#endif /* ! HAVE_DLFCN_H */

/******************************************************************
 * locking.c
 ******************************************************************/

extern int lock_reg(int fd, int cmd, int type, off_t offest, int whence, off_t len);
#define read_lock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define write_lock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define unlock(fd, offset, whence, len) \
    lock_reg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))

/******************************************************************
 * socket.c
 ******************************************************************/

extern int setnonblock(int fd, int cmd);
extern ssize_t readt(int socket, void *data, const size_t length, int setnonblocking, int timeout);
extern const char *getip_string(const struct sockaddr *sa);
extern unsigned int getip_port(const struct sockaddr *sa);
extern void apply_ip_mask(struct sockaddr *ai, int maskbits);
extern int compare_ip(const struct sockaddr *sa1, const struct sockaddr *sa2);

/******************************************************************
 * unix.c
 *****************************************************************/

extern const char *abspath(const char *name);
extern const char *getcwdpath(void);
extern char *stripped_slashes_basename(char *p);
extern int lchdir(const char *dir);
extern void randombytes(void *buf, int n);
#endif  /* _ATALK_UTIL_H */
