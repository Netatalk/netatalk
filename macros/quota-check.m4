dnl $Id: quota-check.m4,v 1.3 2003-12-15 04:59:45 srittau Exp $
dnl Autoconf macro to check for quota support
dnl FIXME: This is in now way complete.

AC_DEFUN([AC_CHECK_QUOTA], [
	AC_CHECK_HEADERS(sys/quota.h ufs/quota.h)

	QUOTA_LIBS=
	AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS=-lrpcsvc])
	AC_SUBST(QUOTA_LIBS)
])

