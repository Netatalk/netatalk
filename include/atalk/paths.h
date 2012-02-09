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
#  if defined (FHS_COMPATIBILITY) || defined (__NetBSD__) || defined (__OpenBSD__)
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
