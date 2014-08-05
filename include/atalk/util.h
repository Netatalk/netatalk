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
#include <poll.h>
#include <sys/stat.h>
#include <stdbool.h>

#include <atalk/unicode.h>
#include <atalk/bstrlib.h>
#include <atalk/cnid.h>

/* exit error codes */
#define EXITERR_CLNT 1  /* client related error */
#define EXITERR_CONF 2  /* error in config files/cmd line parameters */
#define EXITERR_SYS  3  /* local system error */
#define EXITERR_CLOSED 4  /* connection was immediately closed after TCP handshake */

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
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? a : b)
#endif

#ifdef WORDS_BIGENDIAN
#define hton64(x)       (x)
#define ntoh64(x)       (x)
#else
#define hton64(x)       ((uint64_t) (htonl(((x) >> 32) & 0xffffffffLL)) | \
                         (uint64_t) ((htonl(x) & 0xffffffffLL) << 32))
#define ntoh64(x)       (hton64(x))
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)
#endif

#ifdef WITH_SENDFILE
extern ssize_t sys_sendfile (int __out_fd, int __in_fd, off_t *__offset,size_t __count);
#endif

extern const int _diacasemap[], _dialowermap[];

extern char **getifacelist(void);
extern void freeifacelist(char **);

#define diatolower(x)     _dialowermap[(unsigned char) (x)]
#define diatoupper(x)     _diacasemap[(unsigned char) (x)]
extern void bprint        (char *, int);
extern int strdiacasecmp  (const char *, const char *);
extern int strndiacasecmp (const char *, const char *, size_t);
extern pid_t server_lock  (char * /*program*/, char * /*file*/, int /*debug*/);
extern int check_lockfile (const char *program, const char *pidfile);
extern int create_lockfile(const char *program, const char *pidfile);
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

#ifndef strequal
#define strequal(a,b) (strcmp((a),(b)) == 0)
#endif

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
extern int tokenize_ip_port(const char *ipurl, char **address, char **port);

/* Structures and functions dealing with dynamic pollfd arrays */

enum asev_fdtype {IPC_FD, LISTEN_FD};

/**
 * atalk socket event data
 **/
struct asev_data {
    enum asev_fdtype fdtype;  /* IPC fd or listening socket fd                 */
    void            *private; /* pointer to AFPconfig for listening socket and *
                               * pointer to afp_child_t for IPC fd             */
};

/**
 * atalk socket event
 **/
struct asev {
    struct pollfd         *fdset; /* struct pollfd array for poll() */
    struct asev_data      *data;  /* associated array of data       */
    int                    max;
    int                    used;
};

extern struct asev *asev_init(int max);
extern bool asev_add_fd(struct asev *sev, int fd, enum asev_fdtype fdtype, void *private);
extern bool asev_del_fd(struct asev *sev, int fd);

extern int send_fd(int socket, int fd);
extern int recv_fd(int fd, int nonblocking);

/******************************************************************
 * unix.c
 *****************************************************************/

extern const char *getcwdpath(void);
extern const char *fullpathname(const char *);
extern char *stripped_slashes_basename(char *p);
extern void randombytes(void *buf, int n);
extern int daemonize(int nochdir, int noclose);
extern int run_cmd(const char *cmd, char **cmd_argv);
extern char *realpath_safe(const char *path);
extern const char *basename_safe(const char *path);
extern char *strtok_quote (char *s, const char *delim);

extern int ochdir(const char *dir, int options);
extern int ostat(const char *path, struct stat *buf, int options);
extern int ostatat(int dirfd, const char *path, struct stat *st, int options);
extern int ochown(const char *path, uid_t owner, gid_t group, int options);
extern int ochmod(char *path, mode_t mode, const struct stat *st, int options);

/******************************************************************
 * cnid.c
 *****************************************************************/

extern bstring rel_path_in_vol(const char *path, const char *volpath);
extern cnid_t cnid_for_path(struct _cnid_db *cdb, const char *volpath, const char *path, cnid_t *did);

/******************************************************************
 * cnid.c
 *****************************************************************/

extern void initline   (int, char *);
extern int  parseline  (int, char *);

#endif  /* _ATALK_UTIL_H */
