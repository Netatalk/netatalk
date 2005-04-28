dnl Check for (v)snprintf functions

AC_DEFUN([NETATALK_SNPRINTF_CHECK], [
	AC_CHECK_FUNCS(snprintf vsnprintf)

	AC_CHECK_HEADERS(stdarg.h varargs.h)

	AC_CHECK_FUNCS(strerror)
	AC_CACHE_CHECK(for errno,
	ac_cv_errno,
	[
	AC_TRY_LINK(,[extern int errno; return (errno);],
		ac_cv_errno=yes, ac_cv_errno=no)
	])
	if test "$ac_cv_errno" = yes; then
		AC_DEFINE(HAVE_ERRNO, 1, [Define if errno declaration exists])
		AC_CACHE_CHECK(for errno declaration,
		ac_cv_decl_errno,
		[
		AC_TRY_COMPILE([
		#include <stdio.h>
		#ifdef HAVE_STDLIB_H
		#include <stdlib.h>
		#endif
		#ifdef HAVE_UNISTD_H
		#include <unistd.h>
		#endif
		#ifdef HAVE_ERRNO_H
		#include <errno.h>
		#endif
		],[return(sys_nerr);],
			ac_cv_decl_errno=yes, ac_cv_decl_errno=no)
		])
		if test "$ac_cv_decl_errno" = yes; then
			AC_DEFINE(HAVE_DECL_ERRNO, 1, [Define if errno declaration exists])
		fi;
	fi

	AC_CACHE_CHECK(for sys_nerr,
	ac_cv_sys_nerr,
	[
	AC_TRY_LINK(,[extern int sys_nerr; return (sys_nerr);],
		ac_cv_sys_nerr=yes, ac_cv_sys_nerr=no)
	])
	if test "$ac_cv_sys_nerr" = yes; then
		AC_DEFINE(HAVE_SYS_NERR, 1, [Define if sys_nerr declaration exists])
		AC_CACHE_CHECK(for sys_nerr declaration,
		ac_cv_decl_sys_nerr,
		[
		AC_TRY_COMPILE([
		#include <stdio.h>
		#ifdef HAVE_STDLIB_H
		#include <stdlib.h>
		#endif
		#ifdef HAVE_UNISTD_H
		#include <unistd.h>
		#endif],[return(sys_nerr);],
		ac_cv_decl_sys_nerr_def=yes, ac_cv_decl_sys_nerr_def=no)
		])
		if test "$ac_cv_decl_sys_nerr" = yes; then
			AC_DEFINE(HAVE_DECL_SYS_NERR, 1, [Define if sys_nerr declaration exists])
		fi
	fi


	AC_CACHE_CHECK(for sys_errlist array,
	ac_cv_sys_errlist,
	[AC_TRY_LINK(,[extern char *sys_errlist[];
		sys_errlist[0];],
		ac_cv_sys_errlist=yes, ac_cv_sys_errlist=no)
	])
	if test "$ac_cv_sys_errlist" = yes; then
		AC_DEFINE(HAVE_SYS_ERRLIST, 1, [Define if sys_errlist declaration exists])
		AC_CACHE_CHECK(for sys_errlist declaration,
		ac_cv_sys_errlist_def,
		[AC_TRY_COMPILE([
		#include <stdio.h>
		#include <errno.h>
		#ifdef HAVE_STDLIB_H
		#include <stdlib.h>
		#endif
		#ifdef HAVE_UNISTD_H
		#include <unistd.h>
		#endif],[char *s = sys_errlist[0]; return(*s);],
		ac_cv_decl_sys_errlist=yes, ac_cv_decl_sys_errlist=no)
		])
		if test "$ac_cv_decl_sys_errlist" = yes; then
			AC_DEFINE(HAVE_DECL_SYS_ERRLIST, 1, [Define if sys_errlist declaration exists])
		fi
	fi



	AC_CACHE_CHECK(for long long,
	ac_cv_long_long,
	[
	AC_TRY_COMPILE([
	#include <stdio.h>
	#include <sys/types.h>
	], [printf("%d",sizeof(long long));],
	ac_cv_long_long=yes, ac_cv_long_long=no)
	])
	if test $ac_cv_long_long = yes; then
	  AC_DEFINE(HAVE_LONG_LONG, 1, [Define if long long is a valid data type])
	fi

	AC_CACHE_CHECK(for long double,
	ac_cv_long_double,
	[
	AC_TRY_COMPILE([
	#include <stdio.h>
	#include <sys/types.h>
	], [printf("%d",sizeof(long double));],
	ac_cv_long_double=yes, ac_cv_long_double=no)
	])
	if test $ac_cv_long_double = yes; then
	  AC_DEFINE(HAVE_LONG_DOUBLE, 1, [Define if long double is a valid data type])
	fi

	AC_CACHE_CHECK(for quad_t,
	ac_cv_quad_t,
	[
	AC_TRY_COMPILE([
	#include <stdio.h>
	#include <sys/types.h>
	], [printf("%d",sizeof(quad_t));],
	ac_cv_quad_t=yes, ac_cv_quad_t=no)
	])
	if test $ac_cv_quad_t = yes; then
	  AC_DEFINE(HAVE_QUAD_T, 1, [Define if quad_t is a valid data type])
	fi
])
