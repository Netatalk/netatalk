dnl $Id: quota-check.m4,v 1.1 2001-12-01 15:25:54 srittau Exp $
dnl Autoconf macro to check for quota support
dnl FIXME: This is in now way complete.

AC_DEFUN([AC_CHECK_QUOTA], [
	QUOTA_LIBS=
	AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS=-lrpcsvc])
	AC_SUBST(QUOTA_LIBS)
])

