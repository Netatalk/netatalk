/*
 * $Id: util.h,v 1.15 2009-11-08 22:12:40 didg Exp $
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


extern int     sys_ftruncate (int fd, off_t length);

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
extern pid_t server_lock  (char * /*program*/, char * /*file*/, 
			       int /*debug*/);
extern void fault_setup	  (void (*fn)(void *));
#define server_unlock(x)  (unlink(x))

#ifndef HAVE_STRLCPY
size_t strlcpy (char *, const char *, size_t);
#endif
 
#ifndef HAVE_STRLCAT
size_t strlcat (char *, const char *, size_t);
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

/******************************************************************
 * socket.c
 ******************************************************************/

/*
 * Function: setnonblock
 *
 * Purpose: set or unset non-blocking IO on a fd
 *
 * Arguments:
 *
 *    fd         (r) File descriptor
 *    cmd        (r) 0: disable non-blocking IO, ie block
 *                   <>0: enable non-blocking IO
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * fd's file flags.
 */
extern int setnonblock(int fd, int cmd);

/* 
 * Function: getip_string
 *
 * Purpose: convert an IPv4 or IPv6 address to a static string using inet_ntop
 *
 * Arguments:
 *
 *   sa        (r) pointer to an struct sockaddr
 *
 * Returns: pointer to a static string cotaining the converted address as string.
 *          On error pointers to "0.0.0.0" or "::0" are returned.
 *
 * Effects:
 *
 * IPv6 mapped IPv4 addresses are returned as IPv4 addreses eg
 * ::ffff:10.0.0.0 is returned as "10.0.0.0".
 */
extern const char *getip_string(const struct sockaddr *sa);

/* 
 * Function: getip_string
 *
 * Purpose: return port number from struct sockaddr
 *
 * Arguments:
 *
 *   sa        (r) pointer to an struct sockaddr
 *
 * Returns: port as unsigned int
 *
 * Effects: none.
 */
extern unsigned int getip_port(const struct sockaddr *sa);

/* 
 * Function: apply_ip_mask
 *
 * Purpose: apply netmask to IP (v4 or v6)
 *
 * Arguments:
 *
 *   ai        (rw) pointer to an struct sockaddr
 *   mask      (r) number of maskbits
 *
 * Returns: void
 *
 * Effects: 
 *
 * Modifies IP address in sa->sin[6]_addr-s[6]_addr. The caller is responsible
 * for passing a value for mask that is sensible to the passed address,
 * eg 0 <= mask <= 32 for IPv4 or 0<= mask <= 128 for IPv6. mask > 32 for
 * IPv4 is treated as mask = 32, mask > 128 is set to 128 for IPv6.
 */
extern void apply_ip_mask(struct sockaddr *ai, int maskbits);

/* 
 * Function: compare_ip
 *
 * Purpose: Compare IP addresses for equality
 *
 * Arguments:
 *
 *   sa1       (r) pointer to an struct sockaddr
 *   sa2       (r) pointer to an struct sockaddr
 *
 * Returns: Addresses are converted to strings and compared with strcmp and
 *          the result of strcmp is returned.
 *
 * Effects: 
 *
 * IPv6 mapped IPv4 addresses are treated as IPv4 addresses.
 */
extern int compare_ip(const struct sockaddr *sa1, const struct sockaddr *sa2);
