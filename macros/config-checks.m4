dnl $Id: config-checks.m4,v 1.3 2002-04-29 06:23:58 morgana Exp $
dnl Autoconf macro to set the configuration directories.

AC_DEFUN([NETATALK_CONFIG_DIRS], [
	PKGCONFDIR="${sysconfdir}/netatalk"

	AC_ARG_WITH(pkgconfdir,
        	[  --with-pkgconfdir=DIR   package specific configuration in DIR
                          [SYSCONF/netatalk]],
               	[
			if test "x$withval" != "x"; then
				PKGCONFDIR="$withval"
			fi
		]
	)

	NLSDIR="${PKGCONFDIR}/nls"

	AC_ARG_WITH(nls-dir,
		[  --with-nls-dir=PATH     path to NLS files [PKGCONF/nls]],
		[
			if test "x$withval" != "x"; then
				NLSDIR="$withval"
			fi
		]
	)

	SERVERTEXT="${PKGCONFDIR}/msg"

	AC_ARG_WITH(message-dir,
		[  --with-message-dir=PATH path to server message files [PKGCONF/msg]],
		[
			if test "x$withval" != "x"; then
				SERVERTEXT="$withval"
				cat >> confdefs.h <<EOF
#define SERVERTEXT "$withval"
EOF
			fi
		]
	)

	AC_SUBST(PKGCONFDIR)
	AC_SUBST(NLSDIR)
	AC_SUBST(SERVERTEXT)
])
