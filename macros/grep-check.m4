dnl Autoconf macro to check for the existence of Perl
dnl $Id: grep-check.m4,v 1.1 2002-02-07 05:09:01 jmarcus Exp $

AC_DEFUN([AC_PROG_GREP], [
AC_REQUIRE([AC_EXEEXT])dnl
test x$GREP = x && AC_PATH_PROG(GREP, grep$EXEEXT, grep$EXEEXT)
test x$GREP = x && AC_MSG_ERROR([no acceptable grep found in \$PATH])
])

AC_SUBST(GREP)
