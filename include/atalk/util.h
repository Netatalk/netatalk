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

#include <sys/cdefs.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <poll.h>
#include <netatalk/at.h>

#include <atalk/unicode.h>
#include <atalk/bstrlib.h>

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

#define STRCMP(a,b,c) (strcmp(a,c) b 0)

#if BYTE_ORDER == BIG_ENDIAN
#define hton64(x)       (x)
#define ntoh64(x)       (x)
#else /* BYTE_ORDER == BIG_ENDIAN */
#define hton64(x)       ((uint64_t) (htonl(((x) >> 32) & 0xffffffffLL)) | \
                         (uint64_t) ((htonl(x) & 0xffffffffLL) << 32))
#define ntoh64(x)       (hton64(x))
#endif /* BYTE_ORDER == BIG_ENDIAN */

#ifdef WITH_SENDFILE
extern ssize_t sys_sendfile (int __out_fd, int __in_fd, off_t *__offset,size_t __count);
#endif

extern const int _diacasemap[], _dialowermap[];

extern char **getifacelist(void);
extern void freeifacelist(char **);

#define diatolower(x)     _dialowermap[(unsigned char) (x)]
#define diatoupper(x)     _diacasemap[(unsigned char) (x)]
extern int atalk_aton     (char *, struct at_addr *);
extern void bprint        (char *, int);
extern int strdiacasecmp  (const char *, const char *);
extern int strndiacasecmp (const char *, const char *, size_t);
extern pid_t server_lock  (char * /*program*/, char * /*file*/, int /*debug*/);
extern int check_lockfile (const char *program, const char *pidfile);
extern int create_lockfile(const char *program, const char *pidfile);
extern void fault_setup	  (void (*fn)(void *));
extern void netatalk_panic(const char *why);
#define server_unlock(x)  (unlink(x))

/* strlcpy and strlcat are used by pam modules */
#ifndef UAM_MODULE_EXPORT
#define UAM_MODULE_EXPORT 
#endif

#ifndef HAVE_STRLCPY
UAM_MODULE_EXPORT size_t strlcpy (char *, const char *, size_t);
#endif
 
#ifndef HAVE_STRLCAT
UAM_MODULE_EXPORT size_t strlcat (char *, const char *, size_t);
#endif

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
extern ssize_t writet(int socket, void *data, const size_t length, int setnonblocking, int timeout);
extern const char *getip_string(const struct sockaddr *sa);
extern unsigned int getip_port(const struct sockaddr *sa);
extern void apply_ip_mask(struct sockaddr *ai, int maskbits);
extern int compare_ip(const struct sockaddr *sa1, const struct sockaddr *sa2);

/* Structures and functions dealing with dynamic pollfd arrays */
enum fdtype {IPC_FD, LISTEN_FD, DISASOCIATED_IPC_FD};
struct polldata {
    enum fdtype fdtype; /* IPC fd or listening socket fd                 */
    void *data;         /* pointer to AFPconfig for listening socket and *
                         * pointer to afp_child_t for IPC fd             */
};

extern void fdset_add_fd(int maxconns,
                         struct pollfd **fdsetp,
                         struct polldata **polldatap,
                         int *fdset_usedp,
                         int *fdset_sizep,
                         int fd,
                         enum fdtype fdtype,
                         void *data);
extern void fdset_del_fd(struct pollfd **fdsetp,
                         struct polldata **polldatap,
                         int *fdset_usedp,
                         int *fdset_sizep,
                         int fd);
extern int send_fd(int socket, int fd);
extern int recv_fd(int fd, int nonblocking);

/******************************************************************
 * unix.c
 *****************************************************************/

extern const char *getcwdpath(void);
extern char *stripped_slashes_basename(char *p);
extern int lchdir(const char *dir);
extern void randombytes(void *buf, int n);
extern int daemonize(int nochdir, int noclose);

/******************************************************************
 * cnid.c
 *****************************************************************/

extern bstring rel_path_in_vol(const char *path, const char *volpath);

#endif  /* _ATALK_UTIL_H */
