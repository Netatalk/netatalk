dnl $Id: ssl-check.m4,v 1.6 2003-01-26 16:40:46 srittau Exp $
dnl Autoconf macro to check for SSL or OpenSSL

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

dnl GnuTLS test

		AM_PATH_LIBGNUTLS_EXTRA(, [
			SSL_CFLAGS="$SSL_CFLAGS $LIBGNUTLS_EXTRA_CFLAGS"
			SSL_LIBS="$SSL_LIBS $LIBGNUTLS_EXTRA_LIBS"
			AC_DEFINE(GNUTLS_DHX, 1)
			AC_DEFINE(UAM_DHX,    1)
			compile_ssl=yes
		], [

dnl OpenSSL test

			AC_MSG_CHECKING([for OpenSSL])
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

			 		AC_DEFINE(OPENSSL_DHX,	1)
					AC_DEFINE(UAM_DHX,	1)
					compile_ssl=yes
					break
				fi
			done
			if test "x$compile_ssl" = "xno"; then
				AC_MSG_RESULT([no])
			fi
		])
	fi
	AC_SUBST(SSL_CFLAGS)
	AC_SUBST(SSL_LIBS)
])
