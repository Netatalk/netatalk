dnl $Id: tcp-wrappers.m4,v 1.1 2003-06-12 23:15:07 srittau Exp $

AC_DEFUN([NETATALK_TCP_WRAPPERS], [
	check=maybe
	AC_ARG_ENABLE(tcp-wrappers,
		[  --disable-tcp-wrappers  disable TCP wrappers support],
		[
			if test "x$enableval" == "xno"; then
				check=no
			else
				check=yes
			fi
		]
	)

	enable=no
	if test "x$check" != "xno"; then
		AC_CHECK_LIB(wrap, tcpd_warn, enable=yes)
	fi

	AC_MSG_CHECKING([whether to enable the TCP wrappers])
	if test "x$enable" == "xyes"; then
		AC_DEFINE(TCPWRAP, 1, [Define if TCP wrappers should be used])
		WRAP_LIBS="-lwrap"
		AC_MSG_RESULT([yes])
	else
		if test "x$check" == "xyes"; then
			AC_MSG_ERROR([libwrap not found])
		else
			AC_MSG_RESULT([no])
		fi
	fi

	AC_SUBST(WRAP_LIBS)
])
