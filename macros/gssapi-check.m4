dnl $Id: gssapi-check.m4,v 1.3 2005-04-28 20:50:05 bfernhomberg Exp $
dnl Autoconf macro to check for kerberos

AC_DEFUN([NETATALK_GSSAPI_CHECK], 
[
	FOUND_GSSAPI=no
	GSSAPI_LIBS=""
	GSSAPI_CFLAGS=""

        AC_ARG_WITH(gssapi,
                [  --with-gssapi[[=PATH]]    path to GSSAPI for Kerberos V UAM [[auto]]],
                [compilegssapi=$withval],
                [compilegssapi=auto]
        )

	if test x"$compilegssapi" != x"no"; then

                if test "x$compilegssapi" != "xyes" -a "x$compilegssapi" != "xauto"; then
            		GSSAPI_CFLAGS="-I$withval/include"
            		GSSAPI_CPPFLAGS="-I$withval/include"
            		GSSAPI_LDFLAGS="-L$withval/${atalk_libname}"
            		FOUND_GSSAPI=yes
			AC_MSG_CHECKING([checking for GSSAPI support in])
			AC_MSG_RESULT([$compilegssapi])
                fi


  	  # Do no harm to the values of CFLAGS and LIBS while testing for
	  # Kerberos support.

		ac_save_CFLAGS=$CFLAGS
		ac_save_CPPFLAGS=$CPPFLAGS
		ac_save_LDFLAGS=$LDFLAGS
		ac_save_LIBS=$LIBS

	if test x$FOUND_GSSAPI = x"no"; then
	  #################################################
	  # check for krb5-config from recent MIT and Heimdal kerberos 5
	  AC_PATH_PROG(KRB5_CONFIG, krb5-config)
	  AC_MSG_CHECKING(for working krb5-config)
	  if test -x "$KRB5_CONFIG"; then
	    ac_save_CFLAGS=$CFLAGS
	    CFLAGS="";export CFLAGS
	    ac_save_LDFLAGS=$LDFLAGS
	    LDFLAGS="";export LDFLAGS
	    GSSAPI_LIBS="`$KRB5_CONFIG --libs gssapi`"
	    GSSAPI_CFLAGS="`$KRB5_CONFIG --cflags | sed s/@INCLUDE_des@//`"
	    GSSAPI_CPPFLAGS="`$KRB5_CONFIG --cflags | sed s/@INCLUDE_des@//`"
	    CFLAGS=$ac_save_CFLAGS;export CFLAGS
	    LDFLAGS=$ac_save_LDFLAGS;export LDFLAGS
	    FOUND_GSSAPI=yes
	    AC_MSG_RESULT(yes)
	  else
	    AC_MSG_RESULT(no. Fallback to previous krb5 detection strategy)
	  fi
	fi

	if test x$FOUND_GSSAPI = x"no"; then
	#################################################
	# see if this box has the SuSE location for the heimdal krb implementation
	  AC_MSG_CHECKING(for /usr/include/heimdal)
	  if test -d /usr/include/heimdal; then
	    if test -f /usr/lib/heimdal/lib/libkrb5.a; then
		GSSAPI_CFLAGS="-I/usr/include/heimdal"
		GSSAPI_CPPFLAGS="-I/usr/include/heimdal"
		GSSAPI_LDFLAGS="-L/usr/lib/heimdal/lib"
		AC_MSG_RESULT(yes)
            	FOUND_GSSAPI=yes
	    else
		GSSAPI_CFLAGS="-I/usr/include/heimdal"
		GSSAPI_CPPFLAGS="-I/usr/include/heimdal"
		AC_MSG_RESULT(yes)
            	FOUND_GSSAPI=yes
	    fi
	  else
	    AC_MSG_RESULT(no)
	  fi
	fi

	if test x$FOUND_GSSAPI = x"no"; then
	#################################################
	# see if this box has the RedHat location for kerberos
	  AC_MSG_CHECKING(for /usr/kerberos)
	  if test -d /usr/kerberos -a -f /usr/kerberos/lib/libkrb5.a; then
		GSSAPI_LDFLAGS="-L/usr/kerberos/lib"
		GSSAPI_CFLAGS="-I/usr/kerberos/include"
		GSSAPI_CPPFLAGS="-I/usr/kerberos/include"
		AC_MSG_RESULT(yes)
	  else
		AC_MSG_RESULT(no)
	  fi
	fi

	CFLAGS="$CFLAGS $GSSAPI_CFLAGS"
	CPPFLAGS="$CPPFLAGS $GSSAPI_CPPFLAGS"
	LDFLAGS="$LDFLAGS $GSSAPI_LDFLAGS"
	LIBS="$GSSAPI_LIBS"


	# check for gssapi headers

	gss_headers_found=no
	AC_CHECK_HEADERS(gssapi.h gssapi/gssapi_generic.h gssapi/gssapi.h gssapi/gssapi_krb5.h,[gss_headers_found=yes],[],[])
	if test x"$gss_headers_found" = x"no"; then
		AC_MSG_ERROR([GSSAPI installation not found, headers missing])
	fi

	# check for libs

	AC_CHECK_LIB(gssapi, gss_display_status) 
	AC_CHECK_LIB(gssapi_krb5, gss_display_status) 
	AC_CHECK_LIB(gss, gss_display_status) 

	# check for functions

  	AC_CHECK_FUNC(gss_acquire_cred,[],[AC_MSG_ERROR([GSSAPI: required function gss_acquire_cred missing])])

	# Heimdal/MIT compatibility fix
	if test "$ac_cv_header_gssapi_h" = "yes"; then
	    AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi.h, AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,1,[Wheter GSS_C_NT_HOSTBASED_SERVICE is in gssapi.h]))
	else
	    AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi/gssapi.h, AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,1,[Wheter GSS_C_NT_HOSTBASED_SERVICE is in gssapi.h]))
	fi


	AC_MSG_CHECKING(whether GSSAPI support is used)
	if test x"$ac_cv_func_gss_acquire_cred" = x"yes"; then
   		AC_DEFINE(HAVE_GSSAPI,1,[Whether to enable GSSAPI support])
		AC_MSG_RESULT([yes])
		GSSAPI_LIBS="$LDFLAGS $LIBS"
	else
		AC_MSG_RESULT([no])
		if test x"$compilegssapi" = x"yes"; then
			AC_MSG_ERROR([GSSAPI installation not found])
		fi
        	GSSAPI_LIBS=""
	fi

        LIBS="$ac_save_LIBS"
        CFLAGS="$ac_save_CFLAGS"
        LDFLAGS="$ac_save_LDFLAGS"
        CPPFLAGS="$ac_save_CPPFLAGS"
	fi

        if test x"$ac_cv_func_gss_acquire_cred" = x"yes"; then
                ifelse([$1], , :, [$1])
        else
                ifelse([$2], , :, [$2])
        fi


	AC_SUBST(GSSAPI_LIBS)
	AC_SUBST(GSSAPI_CFLAGS)

])
