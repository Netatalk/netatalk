dnl $Id: pam-check.m4,v 1.3 2005-04-28 20:50:05 bfernhomberg Exp $
dnl PAM finding macro

AC_DEFUN([AC_PATH_PAM], [
	AC_ARG_WITH(pam, [  --with-pam[[=PATH]]       specify path to PAM installation [[auto]]],
		[
			require_pam="yes"
			if test "x$withval" = "xno"; then
				PAMDIR="NONE"
				require_pam="never"
			elif test "x$withval" = "xyes"; then
				PAMDIR="NONE"
			else
				PAMDIR="$withval"
			fi
		],
		[PAMDIR="NONE";require_pam="no"]
	)

	AC_MSG_CHECKING([for PAM installation directory])
	if test "$host_os" != "solaris"; then
		if test "x$PAMDIR" = "xNONE" -a "x$require_pam" != "xnever"; then
		  dnl Test for PAM
		  pam_paths="/ /usr/ /usr/local/"
		  for path in $pam_paths; do
			if test -d "${path}etc/pam.d"; then
				PAMDIR="$path"
				break
			fi
		  done
		fi

		if test "x$PAMDIR" != "xNONE"; then
			AC_MSG_RESULT([yes (path: ${PAMDIR}etc/pam.d)])
		else
			AC_MSG_RESULT([no])
		fi
	else
		AC_MSG_RESULT([/etc/pam.conf (solaris)])
	fi
		
	pam_found="no"
	if test "x$require_pam" != "xnever"; then

		savedCFLAGS="$CFLAGS"
		savedLDFLAGS="$LDFLAGS"
		savedLIBS="$LIBS"

		if test "x$PAMDIR" != "xNONE" -a "x$PAMDIR" != "x/"; then
			PAM_CFLAGS="-I${PAMDIR}include"
			PAM_LDFLAGS="-L${PAMDIR}lib"
			LDFLAGS="$LDFLAGS $PAM_LDFLAGS"
			CFLAGS="$CFLAGS $PAM_CFLAGS"
		fi

		AC_CHECK_HEADERS(security/pam_appl.h pam/pam_appl.h)

		if test x"$ac_cv_header_security_pam_appl_h" = x"no" -a x"$ac_cv_header_pam_pam_appl_h" = x"no"; then
			pam_found=no
		else
			AC_CHECK_LIB(pam, pam_set_item, [
				PAM_LIBS="$PAM_LDFLAGS -lpam"
				pam_found="yes"
			])
		fi
		CFLAGS="$savedCFLAGS"
		LDFLAGS="$savedLDFLAGS"
		LIBS="$savedLIBS"
	fi

	netatalk_cv_install_pam=yes
	if test x"$pam_found" = "xyes" -a "x$PAMDIR" = "xNONE"; then
		AC_MSG_WARN([PAM support can be compiled, but the install location for the netatalk.pamd file could not be determined. Either install this file by hand or specify the install path.])
		netatalk_cv_install_pam=no
	fi

	AC_MSG_CHECKING([whether to enable PAM support])
	if test "x$pam_found" = "xno"; then
		if test "x$require_pam" = "xyes"; then
			AC_MSG_ERROR([PAM support missing])
		else
			AC_MSG_RESULT([no])
		fi
		ifelse([$2], , :, [$2])
	else
		AC_MSG_RESULT([yes])
		ifelse([$1], , :, [$1])
	fi

        LIB_REMOVE_USR_LIB(PAM_LIBS)
        CFLAGS_REMOVE_USR_INCLUDE(PAM_CFLAGS)
	AC_SUBST(PAMDIR)
	AC_SUBST(PAM_CFLAGS)
	AC_SUBST(PAM_LIBS)
])
