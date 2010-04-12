dnl $Id: cups.m4,v 1.3 2010-04-12 14:28:47 franklahm Exp $
dnl Autoconf macros to check for CUPS

AC_DEFUN([NETATALK_AC_CUPS], [

	dnl Don't use spool unless it's needed
	spool_required=no
	netatalk_cv_use_cups=no

	AC_ARG_ENABLE(cups,
	[  --enable-cups           Turn on CUPS support (default=auto)])

	if test x$enable_cups != xno; then
        	AC_PATH_PROG(CUPS_CONFIG, cups-config)

        	if test "x$CUPS_CONFIG" != x; then
                	AC_DEFINE(HAVE_CUPS, 1, [Define to enable CUPS Support])
	                CUPS_CFLAGS="`$CUPS_CONFIG --cflags`"
        	        CUPS_LDFLAGS="`$CUPS_CONFIG --ldflags`"
                	CUPS_LIBS="`$CUPS_CONFIG --libs`"
			CUPS_VERSION="`$CUPS_CONFIG --version`"
			AC_DEFINE_UNQUOTED(CUPS_API_VERSION, "`$CUPS_CONFIG --api-version`", [CUPS API Version])
			AC_SUBST(CUPS_CFLAGS)
			AC_SUBST(CUPS_LDFLAGS)
			AC_SUBST(CUPS_LIBS)
	
			AC_MSG_CHECKING([CUPS version])
                	AC_MSG_RESULT([$CUPS_VERSION])
			netatalk_cv_use_cups=yes
	
			if test x"$netatalk_cv_HAVE_USABLE_ICONV" = x"no" ; then
				AC_WARN([*** Warning: iconv not found on your system, using simple ascii mapping***])
			fi
	                spool_required="yes"
		elif test x"$enable_cups" = "xyes"; then
			AC_MSG_ERROR([*** CUPS not found. You might need to specify the path to cups-config ***])
	        fi
	fi

	AC_MSG_CHECKING([whether CUPS support can be enabled])
	AC_MSG_RESULT([$netatalk_cv_use_cups])


	AC_ARG_WITH(spooldir,
       	[  --with-spooldir=PATH     path for spooldir used for CUPS support (LOCALSTATEDIR/spool/netatalk)],[

	        if test "$withval" = "no"; then
        	       if test x"$spool_required" = x"yes"; then
                	       AC_MSG_ERROR([*** CUPS support requires a spooldir ***])
	               else
        	               AC_DEFINE(DISABLE_SPOOL, 1, [Define to enable spooldir support])
                	       AC_MSG_RESULT([spool disabled])
               		fi
        	elif test "$withval" != "yes"; then
			SPOOLDIR="$withval"
	                AC_MSG_RESULT([spooldir set to $withval])
        	else
			SPOOLDIR="${localstatedir}/spool/netatalk"
	                AC_MSG_RESULT([spool set to default])
        	fi
	],[
		SPOOLDIR="${localstatedir}/spool/netatalk"
	])

	AM_CONDITIONAL(USE_SPOOLDIR, test x"$spool_required" = x"yes")
	AC_SUBST(SPOOLDIR)
])
