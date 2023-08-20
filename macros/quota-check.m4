dnl Autoconf macro to check for quota support

AC_DEFUN([AC_CHECK_QUOTA], [
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
		else
			QUOTA_CFLAGS=""
			QUOTA_LIBS=""
			netatalk_cv_quotasupport="yes"
			AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS="-lrpcsvc"])
			AC_CHECK_HEADERS([rpc/rpc.h rpc/pmap_prot.h rpcsvc/rquota.h],[],[
				QUOTA_LIBS=""
				netatalk_cv_quotasupport="no"
				AC_DEFINE(NO_QUOTA_SUPPORT, 1, [Define if quota support should not be compiled])
			])
			AC_CHECK_LIB(quota, getfsquota, [QUOTA_LIBS="-lquota -lprop -lrpcsvc"
				AC_DEFINE(HAVE_LIBQUOTA, 1, [define if you have libquota])], [], [-lprop -lrpcsvc])
		fi
	else
		netatalk_cv_quotasupport="no"
		AC_DEFINE(NO_QUOTA_SUPPORT, 1, [Define if quota support should not be compiled])
	fi

	AC_SUBST(QUOTA_CFLAGS)
	AC_SUBST(QUOTA_LIBS)
])

