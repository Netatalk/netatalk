dnl Check for libgcrypt
AC_DEFUN([AC_NETATALK_LIBGCRYPT], [

AC_ARG_WITH(libgcrypt-dir,
    AS_HELP_STRING([--with-libgcrypt-dir=PATH],
                   [path where LIBGCRYPT is installed (optional).
                   Must contain lib and include dirs.]),
                   netatalk_cv_libgcrypt=$withval)
    if test x$netatalk_cv_libgcrypt != x ; then
        LIBGCRYPT_CFLAGS="-I$netatalk_cv_libgcrypt/include"
        LIBGCRYPT_LIBS="-L$netatalk_cv_libgcrypt/lib"
    else
        PKG_CHECK_MODULES(LIBGCRYPT, [libgcrypt >= 1.2.3],
            ac_cv_have_libgcrypt=yes,
            [AC_MSG_ERROR([Could not find libgcrypt development files needed for the DHX2 UAM, please install libgcrypt version 1.2.3 or later])])
    fi
    if test x"$ac_cv_have_libgcrypt" = x"yes" -o x$netatalk_cv_libgcrypt != x; then
            neta_cv_compile_dhx2=yes
            neta_cv_have_libgcrypt=yes
            AC_MSG_NOTICE([Enabling DHX2 UAM])
            AC_DEFINE(HAVE_LIBGCRYPT, 1, [Define if the DHX2 modules should be built with libgcrypt])
            AC_DEFINE(UAM_DHX2, 1, [Define if the DHX2 UAM modules should be compiled])
            AC_SUBST(LIBGCRYPT_CFLAGS)
            AC_SUBST(LIBGCRYPT_LIBS)
    fi
])
