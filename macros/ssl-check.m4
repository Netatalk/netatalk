dnl $Id: ssl-check.m4,v 1.9 2003-06-08 16:04:49 srittau Exp $
dnl Autoconf macro to check for SSL or OpenSSL

AC_DEFUN([AC_PATH_GCRYPT], [

	GCRYPT_CFLAGS=""
	GCRYPT_LIBS=""

	search="yes"
	errifnotfound="no"

	AC_ARG_ENABLE(libgcrypt, [  --disable-gcrypt        disable compilation with libgcrypt], [
		if test "x$enableval" != "xdisable"; then
			errifnotfound="yes"
		else
			search="no"
		fi
	])

	GCRYPT_CONFIG=""
	if test "x$search" == "xyes"; then
		AC_PATH_PROG([GCRYPT_CONFIG], [libgcrypt-config], [no])

		if test "x$GCRYPT_CONFIG" == "xno"; then
			if test "x$errifnotfound" == "xyes"; then
				AC_MSG_ERROR([libgcrypt-config not found])
			fi
		else
			GCRYPT_CFLAGS="`$GCRYPT_CONFIG --cflags`"
			GCRYPT_LIBS="`$GCRYPT_CONFIG --libs`"
		fi
	fi

	AC_SUBST(GCRYPT_CFLAGS)
	AC_SUBST(GCRYPT_LIBS)
	AM_CONDITIONAL(HAVE_GCRYPT, test -n "$GCRYPT_CONFIG")
])

AC_DEFUN([AC_PATH_SSL], [
	AC_ARG_WITH(ssl-dir, [  --with-ssl-dir=PATH     specify path to OpenSSL installation (must contain
                          lib and include dirs)],
		[
			if test "x$withval" = "xno"; then
				tryssl=no
			elif test "x$withval" = "xyes"; then
				tryssl=yes
				tryssldir=
			else
				dnl FIXME: should only try in $withval
				tryssl=yes
				tryssldir="$withval"
			fi
		], [tryssl=yes]
	)

	SSL_CFLAGS=""
	SSL_LIBS=""
	compile_ssl=no

	if test "$tryssl" = "yes"; then
		AC_MSG_CHECKING([for SSL])
		for ssldir in "" $tryssldir /usr /usr/local/openssl /usr/lib/openssl /usr/local/ssl /usr/lib/ssl /usr/local /usr/pkg /opt /opt/openssl /usr/local/ssl ; do
			if test -f "$ssldir/include/openssl/cast.h" ; then
				SSL_CFLAGS="$SSL_CFLAGS -I$ssldir/include -I$ssldir/include/openssl"
				SSL_LIBS="$SSL_LIBS -L$ssldir/lib -L$ssldir -lcrypto"
				if test "x$need_dash_r" = "xyes"; then
					SSL_LIBS="$SSL_LIBS -R$ssldir/lib -R$ssldir"
				fi
				AC_MSG_RESULT([$ssldir (enabling RANDNUM and DHX support)])

dnl FIXME: The following looks crude and probably doesn't work properly.
				dnl Check for the crypto library:
				AC_CHECK_LIB(crypto, main)
				dnl Check for "DES" library (for SSLeay, not openssl):
				AC_CHECK_LIB(des, main)

		 		AC_DEFINE(OPENSSL_DHX,	1, [Define if the OpenSSL DHX modules should be built])
				AC_DEFINE(UAM_DHX,	1, [Define if the DHX UAM modules should be compiled])
				compile_ssl=yes
				break
			fi
		done
		if test "x$compile_ssl" = "xno"; then
			AC_MSG_RESULT([no])
		fi
	fi
	AC_SUBST(SSL_CFLAGS)
	AC_SUBST(SSL_LIBS)
])
