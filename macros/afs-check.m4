dnl $Id: afs-check.m4,v 1.1 2002-01-17 05:48:51 srittau Exp $
dnl Autoconf macro to check whether AFS support should be enabled

AC_DEFUN([NETATALK_AFS_CHECK], [
	AFS_LIBS=
	AFS_CFLAGS=

	AC_ARG_ENABLE(afs,
		[  --enable-afs            enable AFS support],
		[
			if test "x$enableval" = "xyes"; then
				AC_CHECK_LIB(afsauthent, pioctl, ,
					AC_MSG_ERROR([AFS installation not found])
				)
				AFS_LIBS=-lafsauthent
				AC_DEFINE(AFS, 1)
			fi
		]
	)

	AC_SUBST(AFS_LIBS)
	AC_SUBST(AFS_CFLAGS)
])
