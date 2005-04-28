dnl $Id: tcp-wrappers.m4,v 1.3 2005-04-28 20:50:05 bfernhomberg Exp $

AC_DEFUN([NETATALK_TCP_WRAPPERS], [
	check=maybe
	AC_ARG_ENABLE(tcp-wrappers,
		[  --disable-tcp-wrappers  disable TCP wrappers support],
		[
			if test "x$enableval" = "xno"; then
				wrapcheck=no
			else
				wrapcheck=yes
			fi
		]
	)

	enable=no
	netatalk_cv_tcpwrap=no
	if test "x$wrapcheck" != "xno"; then
		saved_LIBS=$LIBS
		LIBS="$LIBS -lwrap"
		AC_TRY_LINK([
#include <tcpd.h>
int allow_severity = 0;
int deny_severity = 0;
],[
	tcpd_warn ("link test");
], netatalk_cv_tcpwrap=yes, netatalk_cv_tcpwrap=no, netatalk_cv_tcpwrap=cross)

		LIBS=$saved_LIBS
	fi

	AC_MSG_CHECKING([whether to enable the TCP wrappers])
	if test "x$netatalk_cv_tcpwrap" = "xyes"; then
		AC_DEFINE(TCPWRAP, 1, [Define if TCP wrappers should be used])
		WRAP_LIBS="-lwrap"
		AC_MSG_RESULT([yes])
	else
		if test "x$wrapcheck" = "xyes"; then
			AC_MSG_ERROR([libwrap not found])
		else
			AC_MSG_RESULT([no])
		fi
	fi

	AC_SUBST(WRAP_LIBS)
])
