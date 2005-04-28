dnl $Id: config-checks.m4,v 1.6 2005-04-28 20:50:05 bfernhomberg Exp $
dnl Autoconf macro to set the configuration directories.

AC_DEFUN([NETATALK_CONFIG_DIRS], [
	PKGCONFDIR="${sysconfdir}/netatalk"

	AC_ARG_WITH(pkgconfdir,
        	[  --with-pkgconfdir=DIR   package specific configuration in DIR
                          [[SYSCONF/netatalk]]],
               	[
			if test "x$withval" != "x"; then
				PKGCONFDIR="$withval"
			fi
		]
	)


	SERVERTEXT="${PKGCONFDIR}/msg"

	AC_ARG_WITH(message-dir,
		[  --with-message-dir=PATH path to server message files [[PKGCONF/msg]]],
		[
			if test x"$withval" = x"no";  then 
				AC_MSG_WARN([*** message-dir is mandatory and cannot be disabled, using default ***])
			elif test "x$withval" != "x" && test x"$withval" != x"yes"; then
				SERVERTEXT="$withval"
			fi
		]
	)

	AC_SUBST(PKGCONFDIR)
	AC_SUBST(SERVERTEXT)
])
