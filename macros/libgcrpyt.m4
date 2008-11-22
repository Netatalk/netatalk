AC_DEFUN([NETATALK_CHECK_LIBGCRYPT], [
	AC_ARG_ENABLE(gcrypt, [ --disable-gcrypt    build without gcrypt (limits supported UAMs)])
	if test "x$enable_gcrypt" != "xno" ; then
	    ac_save_CFLAGS="$CFLAGS"
	    ac_save_LIBS="$LIBS"
	    CFLAGS=""
	    LIBS=""
	    AC_CHECK_LIB([gcrypt], [gcry_cipher_open],, [neta_cv_have_libgcrypt=no])
	    case $host in
	        *-*-darwin*)
	dnl -- Darwin is a special case: check if version > 1.4.0                                                                                                                                                        
	          AC_MSG_CHECKING([for correct gcrypt version])
	          AC_RUN_IFELSE(
		    [AC_LANG_PROGRAM([
	                #include <gcrypt.h>                                                                                                                                                                              
	                #include <stdio.h>],[                                                                                                                                                                            
		        char*p= GCRYPT_VERSION;
	                unsigned int vers;
	                vers=atoi(p)*10000;
	                p=strchr(p,'.')+1;
	                vers+=atoi(p)*100;
			p=strchr(p,'.')+1;
	                vers+=atoi(p);
	                if (vers<10400) return 1;
	            ])],
	            [AC_MSG_RESULT([yes])],
	            [
	             	AC_MSG_ERROR([version is < 1.4.0])
	                neta_cv_have_libgcrypt=no
	            ])
	          ;;
	        *)
	          ;;
	    esac
	    if test x$neta_cv_have_libgcrypt != xno ; then
	       AC_MSG_RESULT([Enabling UAM: DHX2])
	       AC_DEFINE(LIBGCRYPT_DHX, 1, [Define if the DHX modules should be built with libgcrypt])
	       AC_DEFINE(UAM_DHX2, 1, [Define if the DHX UAM modules should be compiled])
	       neta_cv_have_libgcrypt=yes
	       neta_cv_compile_dhx2=yes
	       LIBGCRYPT_CFLAGS="$CFLAGS"
	       LIBGCRYPT_LIBS="$LIBS"
	       CFLAGS_REMOVE_USR_INCLUDE(LIBGCRYPT_CFLAGS)
	       LIB_REMOVE_USR_LIB(LIBGCRYPT_LIBS)
	       AC_SUBST(LIBGCRYPT_CFLAGS)
	       AC_SUBST(LIBGCRYPT_LIBS)
	   fi
	   CFLAGS="$ac_save_CFLAGS"
	   LIBS="$ac_save_LIBS"
	fi
])