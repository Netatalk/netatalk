dnl Autoconf macro to check for the existence of ps
dnl $Id: ps-check.m4,v 1.2 2002-02-14 18:02:04 jmarcus Exp $

AC_DEFUN([AC_PROG_PS], [
AC_REQUIRE([AC_EXEEXT])dnl
test x$PS = x && AC_PATH_PROG(PS, ps$EXEEXT, ps$EXEEXT)
test x$PS = x && AC_MSG_ERROR([no acceptable ps found in \$PATH])
])

AC_SUBST(PS)
