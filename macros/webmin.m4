dnl $Id: webmin.m4,v 1.1 2009-10-22 08:36:30 franklahm Exp $
dnl Autoconf macro to install webmin + netatalk webmin module

AC_DEFUN([NETATALK_WEBMIN],[
    AC_ARG_WITH(webmin,
        [  --with-webmin=PATH      path where webmin is installed [[$PKGCONFDIR/webmin]]],
        if test "x$withval" = "xyes"; then
            webminpath="$PKGCONFDIR"
        elif test "x$withval" = "xno"; then
            webminpath=""
        else
            webminpath="$withval"
        fi
    )

    AC_ARG_WITH(webminuser,
        [  --with-webminuser=NAME      name for the webmin admin user],
        if test "x$withval" = "xyes"; then
            webminuser=""
        elif test "x$withval" = "xno"; then
            webminuser=""
        else
            webminuser="$withval"
        fi
    )

    AC_ARG_WITH(webminversion,
        [  --with-webminversion=VERSION   Webmin version to fetch from sf.net [[1.490]]],
        if test "x$withval" = "xyes"; then
            webminversion="1.490"
        elif test "x$withval" = "xno"; then
            webminversions="1.490"
        else
            webminversion="$withval"
        fi
    )

    AC_ARG_WITH(webminpass,
        [  --with-webminpass=PASSWORD  password for the webmin admin user],
        if test "x$withval" = "xyes"; then
            webminpass=""
        elif test "x$withval" = "xno"; then
            webminpass=""
        else
            webminpass="$withval"
        fi
    )

    AC_ARG_WITH(webminport,
        [  --with-webminport=PORT  TCP port for webmin],
        if test "x$withval" = "xyes"; then
            webminport=""
        elif test "x$withval" = "xno"; then
            webminport=""
        else
            webminport="$withval"
        fi
    )

    AC_MSG_CHECKING([if webmin administration shall be installed])
    if test "x$webminpath" != "x" &&
            test "x$webminuser" != "x" &&
            test "x$webminpass" != "x" &&
            test "x$webminport" != "x" &&
            test "x$webminversion" != "x"; then
        AC_MSG_RESULT([yes])
        AC_MSG_CHECKING([if neccessary Perl module 'Net::SSLeay' is installed])
        $ac_cv_path_PERL -e 'use Net::SSLeay' >/dev/null 2>/dev/null
		if test "$?" != "0" ; then
            AC_MSG_RESULT([no])
            webminpath=""
        else
            AC_MSG_RESULT([yes])
            AC_MSG_NOTICE([Installing Webmin in "$webminpath/webmin"])
        fi
    else
        AC_MSG_RESULT([no])
    fi

    AC_SUBST(WEBMIN_PATH, $webminpath)
    AC_SUBST(WEBMIN_VERSION, $webminversion)
    AC_SUBST(WEBMIN_USER, $webminuser)
    AC_SUBST(WEBMIN_PASS, $webminpass)
    AC_SUBST(WEBMIN_PORT, $webminport)
])