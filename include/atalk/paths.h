#ifndef ATALK_PATHS_H
#define ATALK_PATHS_H 1

/* we need a way of concatenating strings */
#ifdef HAVE_BROKEN_CPP
#define BROKEN_ECHO(a)    a
#define ATALKPATHCAT(a,b) BROKEN_ECHO(a)##BROKEN_ECHO(b)
#else
#define ATALKPATHCAT(a,b) a b
#endif


/* lock file path. this should be re-organized a bit. */
#if ! defined (_PATH_LOCKDIR)
#  if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__) || defined (__APPLE__)
#    define _PATH_LOCKDIR	"/var/run/"
#  elif defined (BSD4_4)
#      define _PATH_LOCKDIR	"/var/spool/lock/"
#  elif defined (__linux__)
#    define _PATH_LOCKDIR	"/var/lock/"
#  else
#    define _PATH_LOCKDIR	"/var/spool/locks/"
#  endif
#endif

/*
 * papd paths
 */
#define _PATH_PAPDPRINTCAP	"/etc/printcap"
#define _PATH_PAPDSPOOLDIR	"/var/spool/lpd"
#ifdef BSD4_4
#define _PATH_DEVPRINTER	"/var/run/printer"
#else /* !BSD4_4 */
#define _PATH_DEVPRINTER	"/dev/printer"
#endif /* BSD4_4 */

/*
 * atalkd paths
 */
#define _PATH_ATALKDEBUG	"/tmp/atalkd.debug"
#define _PATH_ATALKDTMP		"atalkd.tmp"
#if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__)
#  define _PATH_ATALKDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"atalkd.pid")
#else
#  define _PATH_ATALKDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"atalkd")
#endif

/*
 * psorder paths
 */
#define _PATH_TMPPAGEORDER	"/tmp/psorderXXXXXX"
#if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__)
#  define _PATH_PAPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"papd.pid")
#else
#  define _PATH_PAPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"papd")
#endif

/*
 * afpd paths
 */
#define _PATH_AFPTKT		"/tmp/AFPtktXXXXXX"
#define _PATH_AFP_IPC       ATALKPATHCAT(_PATH_LOCKDIR,"afpd_ipc")
#if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__)
#  define _PATH_AFPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"afpd.pid")
#else
#  define _PATH_AFPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"afpd")
#endif

/*
 * cnid_metad paths
 */
#if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__)
#  define _PATH_CNID_METAD_LOCK	ATALKPATHCAT(_PATH_LOCKDIR,"cnid_metad.pid")
#else
#  define _PATH_CNID_METAD_LOCK	ATALKPATHCAT(_PATH_LOCKDIR,"cnid_metad")
#endif

#endif /* atalk/paths.h */
