dnl $Id: config-checks.m4,v 1.1 2002-02-13 16:44:04 srittau Exp $
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
				NLSDIR = "$withval"
			fi
		]
	)

	AC_SUBST(PKGCONFDIR)
	AC_SUBST(NLSDIR)
])
