dnl Check for optional server location protocol support (used by MacOS X)

dnl $Id: srvloc.m4,v 1.10 2009-12-13 11:58:30 franklahm Exp $

AC_DEFUN([NETATALK_SRVLOC], [

	SLP_LIBS=""
	SLP_CFLAGS=""
	found_slp=no
	srvlocdir=""

	AC_ARG_ENABLE(srvloc,
		[  --enable-srvloc[[=DIR]]   enable Server Location Protocol (SLP) support],
		[srvloc=$enableval],
		[srvloc=no]
	)

    dnl make sure atalk_libname is defined beforehand
    [[ -n "$atalk_libname" ]] || AC_MSG_ERROR([internal error, atalk_libname undefined])

	if test "x$srvloc" != "xno"; then

		savedcppflags="$CPPFLAGS"
		savedldflags="$LDFLAGS"
		if test "x$srvloc" = "xyes" ; then
			srvlocdir="/usr"
		else
			srvlocdir="$srvloc"
		fi
		CPPFLAGS="$CPPFLAGS -I$srvlocdir/include"
		LDFLAGS="$LDFLAGS -L$srvlocdir/$atalk_libname"

		AC_MSG_CHECKING([for slp.h])
		AC_TRY_CPP([#include <slp.h>],
			[
				AC_MSG_RESULT([yes])
				found_slp=yes
			],
			[
				AC_MSG_RESULT([no])
			]
		)
		
		if test "x$found_slp" = "xyes"; then
			AC_CHECK_LIB(slp, SLPOpen, [
		    	   SLP_LIBS="-L$srvlocdir/$atalk_libname -lslp"
		    	   SLP_CFLAGS="-I$srvlocdir/include"
			],[ 
		    	   AC_MSG_RESULT([no])
			   found_slp=no
			])
		fi

		CPPFLAGS="$savedcppflags"
		LDFLAGS="$savedldflags"
	fi
	
	netatalk_cv_srvloc=no
	AC_MSG_CHECKING([whether to enable srvloc (SLP) support])
	if test "x$found_slp" = "xyes"; then
		AC_MSG_RESULT([yes])
		AC_DEFINE(USE_SRVLOC, 1, [Define to enable SLP support])
		netatalk_cv_srvloc=yes
	else
		AC_MSG_RESULT([no])
		if test "x$srvloc" != "xno" -a "x$srvloc" != "xyes"; then
			AC_MSG_ERROR([SLP installation not found])
		fi
	fi
		


	LIB_REMOVE_USR_LIB(SLP_LIBS)
	CFLAGS_REMOVE_USR_INCLUDE(SLP_CFLAGS)
	AC_SUBST(SLP_LIBS)
	AC_SUBST(SLP_CFLAGS)
])
