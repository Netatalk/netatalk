dnl $Id: admingrp-check.m4,v 1.1 2003-12-15 06:56:33 srittau Exp $
dnl Autoconf macro to check whether admin group support should be enabled

AC_DEFUN([NETATALK_ADMINGRP_CHECK], [
	admingrp=yes
	AC_MSG_CHECKING([for administrative group support])
	AC_ARG_ENABLE(admin-group,
		[  --disable-admin-group   disable admin group], [
			if test "x$enableval" = "xno"; then
				admingrp=no
			fi
		]
	)

	AC_DEFINE_UNQUOTED(ADMIN_GRP,
		`test "x$admingrp" = "xyes"`,
	[Define if the admin group should be enabled]
	)

	if test "x$admingrp" = "xyes"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
])

