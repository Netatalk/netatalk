dnl $Id: ssl-check.m4,v 1.2 2001-10-28 12:09:17 srittau Exp $
dnl Autoconf macro to check for SSL or OpenSSL

AC_DEFUN([AC_PATH_SSL], [
	tryssl=yes

	AC_ARG_WITH(ssl-dir, [
  --with-ssl-dir=PATH     specify path to OpenSSL installation (must contain
                          lib and include dirs)],
		[
			if test "x$withval" != "xno"; then
				tryssldir=$withval
			else
				tryssl=no
			fi
		]
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
				if test "$need_dash_r" = "yes"; then
					SSL_LIBS="$SSL_LIBS -R$ssldir/lib -R$ssldir"
				fi
				AC_MSG_RESULT([$ssldir (enabling RANDNUM and DHX support)])

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
	fi
	AC_SUBST(SSL_CFLAGS)
	AC_SUBST(SSL_LIBS)
])
