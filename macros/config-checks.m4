dnl $Id: config-checks.m4,v 1.5 2003-06-06 19:45:51 srittau Exp $
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

	NLSDIR="${datadir}/netatalk/nls"

	AC_ARG_WITH(nls-dir,
		[  --with-nls-dir=PATH     path to NLS files [DATA/netatalk/nls]],
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
			if test x"$withval" = x"no";  then 
				AC_MSG_WARN([*** message-dir is mandatory and cannot be disabled, using default ***])
			elif test "x$withval" != "x" && test x"$withval" != x"yes"; then
				SERVERTEXT="$withval"
			fi
		]
	)

	AC_SUBST(PKGCONFDIR)
	AC_SUBST(NLSDIR)
	AC_SUBST(SERVERTEXT)
])
