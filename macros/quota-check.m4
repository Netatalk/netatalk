dnl $Id: quota-check.m4,v 1.2 2003-12-15 04:55:20 srittau Exp $
dnl Autoconf macro to check for quota support
dnl FIXME: This is in now way complete.

AC_DEFUN([AC_CHECK_QUOTA], [
	AC_CHECK_HEADERS(ufs/quota.h)

	QUOTA_LIBS=
	AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS=-lrpcsvc])
	AC_SUBST(QUOTA_LIBS)
])

