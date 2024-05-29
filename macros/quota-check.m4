dnl Autoconf macro to check for quota support
dnl FIXME: This is in now way complete.

AC_DEFUN([AC_NETATALK_CHECK_QUOTA], [
	AC_ARG_ENABLE(quota,
	[  --enable-quota           Turn on quota support (default=auto)])
	AC_ARG_WITH([libtirpc], [AS_HELP_STRING([--with-libtirpc], [Use libtirpc as RPC implementation (instead of sunrpc)])])

	if test x$enable_quota != xno; then
		if test "x$with_libtirpc" = xyes; then
			PKG_CHECK_MODULES([TIRPC],
				[libtirpc],
				[QUOTA_CFLAGS=$TIRPC_CFLAGS
				QUOTA_LIBS=$TIRPC_LIBS
				netatalk_cv_quotasupport="yes"
				AC_DEFINE(NEED_RQUOTA, 1, [Define various xdr functions])],
				[AC_MSG_ERROR([libtirpc requested, but library not found.])]
			)
			old_CFLAGS=$CFLAGS
			CFLAGS="$CFLAGS $QUOTA_CFLAGS"
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
					#include <rpcsvc/rquota.h>
				]], [[
					enum qr_status foo;
					foo = Q_OK;
				]])],
				[netatalk_cv_rquota_qr_status="yes"], [netatalk_cv_rquota_qr_status="no"]
			)
			CFLAGS=$old_CFLAGS
			if test x"$netatalk_cv_rquota_qr_status" = x"yes"; then
				AC_DEFINE(HAVE_RQUOTA_H_QR_STATUS, 1, [rquota.h has enum qr_status member])
			fi
		else
			QUOTA_CFLAGS=""
			QUOTA_LIBS=""
			netatalk_cv_quotasupport="yes"
			AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS="-lrpcsvc"])
			AC_CHECK_HEADERS([rpc/rpc.h rpc/pmap_prot.h rpcsvc/rquota.h],[],[
				QUOTA_LIBS=""
				netatalk_cv_quotasupport="no"
				AC_DEFINE(NO_QUOTA_SUPPORT, 1, [Define if quota support should not compiled])
			])
			AC_CHECK_LIB(quota, getfsquota, [QUOTA_LIBS="-lquota -lprop -lrpcsvc"
				AC_DEFINE(HAVE_LIBQUOTA, 1, [define if you have libquota])], [], [-lprop -lrpcsvc])
		fi
	else
		netatalk_cv_quotasupport="no"
		AC_DEFINE(NO_QUOTA_SUPPORT, 1, [Define if quota support should not compiled])
	fi

	AC_SUBST(QUOTA_CFLAGS)
	AC_SUBST(QUOTA_LIBS)
])
