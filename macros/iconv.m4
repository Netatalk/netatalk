AC_DEFUN([AM_ICONV],
[

dnl	#################################################
dnl	# check for libiconv support
	AC_MSG_CHECKING(whether to use libiconv)
	AC_ARG_WITH(libiconv,
	[  --with-libiconv=BASEDIR Use libiconv in BASEDIR/lib and BASEDIR/include (default=auto) ],
	[ case "$withval" in
	  no)
	    AC_MSG_RESULT(no)
	    ;;
	  *)
	    AC_MSG_RESULT(yes)
            savedcflags="$CFLAGS"
            savedldflags="$LDFLAGS"
	    CFLAGS="$CFLAGS -I$withval/include"
	    LDFLAGS="$LDFLAGS -L$withval/lib"
	    AC_CHECK_LIB(iconv, iconv_open, [
                                if test "$withval" != "/usr" && "$withval" != ""; then
                                    ICONV_CFLAGS="-I$withval/include"
                                    ICONV_LIBS ="-L$withval/lib"
                                fi
                                ICONV_LIBS="$ICONV_LIBS -liconv"
                        ])
	    AC_DEFINE_UNQUOTED(WITH_LIBICONV, "${withval}",[Path to iconv])
	    ;;
	  esac ],
	  AC_MSG_RESULT(no)
	)
	AC_SUBST(ICONV_CFLAGS)
	AC_SUBST(ICONV_LIBS)

dnl	############
dnl	# check for iconv usability
	AC_CACHE_CHECK([for working iconv],netatalk_cv_HAVE_USABLE_ICONV,[
		AC_TRY_RUN([
	#include <iconv.h>
	main() {
	       iconv_t cd = iconv_open("MAC", "UTF8");
	       if (cd == 0 || cd == (iconv_t)-1) return -1;
	       return 0;
	}
	], netatalk_cv_HAVE_USABLE_ICONV=yes,netatalk_cv_HAVE_USABLE_ICONV=no,netatalk_cv_HAVE_USABLE_ICONV=cross)])

	if test x"$netatalk_cv_HAVE_USABLE_ICONV" = x"yes"; then
	    AC_DEFINE(HAVE_USABLE_ICONV,1,[Whether to use native iconv])
	fi

dnl	###########
dnl	# check if iconv needs const
  	if test x"$cv_HAVE_USABLE_ICONV" = x"yes"; then
    		AC_CACHE_VAL(am_cv_proto_iconv, [
      		AC_TRY_COMPILE([
		#include <stdlib.h>
		#include <iconv.h>
		extern
		#ifdef __cplusplus
		"C"
		#endif
		#if defined(__STDC__) || defined(__cplusplus)
		size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
		#else
		size_t iconv();
		#endif
		], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
	      	am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    		AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      			[Define as const if the declaration of iconv() needs const.])
  	fi
        CFLAGS="$savedcflags"
        LDFLAGS="$savedldflags"
	
])
