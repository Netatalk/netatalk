#ifndef ATALK_PATHS_H
#define ATALK_PATHS_H 1

/* we need a way of concatenating strings */
#ifdef __STDC__
#ifdef HAVE_BROKEN_CPP
#define BROKEN_ECHO(a)    a
#define ATALKPATHCAT(a,b) BROKEN_ECHO(a)##BROKEN_ECHO(b)
#else
#define ATALKPATHCAT(a,b) a b
#endif
#else
#define ATALKPATHCAT(a,b) a/**/b
#endif


/* lock file path. this should be re-organized a bit. */
#if ! defined (_PATH_LOCKDIR)
#  if defined (FHS_COMPATIBILITY)
#    define _PATH_LOCKDIR	"/var/run/"
#  elif defined (BSD4_4)
#    ifdef MACOSX_SERVER
#      define _PATH_LOCKDIR	"/var/run/"
#    else
#      define _PATH_LOCKDIR	"/var/spool/lock/"
#    endif
#  elif defined (linux)
#    define _PATH_LOCKDIR	"/var/lock/"
#  else
#    define _PATH_LOCKDIR	"/var/spool/locks/"
#  endif
#endif

/*
 * papd paths
 */
#define _PATH_PAPDPRINTCAP	"/etc/printcap"
#ifdef ultrix
#define _PATH_PAPDSPOOLDIR	"/usr/spool/lpd"
#else /* !ultrix */
#define _PATH_PAPDSPOOLDIR	"/var/spool/lpd"
#endif /* ultrix */
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
#ifdef FHS_COMPATIBILITY
#  define _PATH_ATALKDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"atalkd.pid")
#else
#  define _PATH_ATALKDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"atalkd")
#endif

/*
 * psorder paths
 */
#define _PATH_TMPPAGEORDER	"/tmp/psorderXXXXXX"
#ifdef FHS_COMPATIBILITY
#  define _PATH_PAPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"papd.pid")
#else
#  define _PATH_PAPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"papd")
#endif

/*
 * afpd paths
 */
#define _PATH_AFPTKT		"/tmp/AFPtktXXXXXX"
#ifdef FHS_COMPATIBILITY
#  define _PATH_AFPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"afpd.pid")
#else
#  define _PATH_AFPDLOCK	ATALKPATHCAT(_PATH_LOCKDIR,"afpd")
#endif

#endif /* atalk/paths.h */
