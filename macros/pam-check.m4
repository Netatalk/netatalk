dnl $Id: pam-check.m4,v 1.2 2001-11-25 21:48:01 srittau Exp $
dnl PAM finding macro

AC_DEFUN([AC_PATH_PAM], [
	AC_MSG_CHECKING([for PAM])
	AC_ARG_WITH(pam, [  --with-pam=PATH         specify path to PAM installation],
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

	if test "x$PAMDIR" = "xNONE" -a "x$require_pam" != "xnever"; then
		dnl Test for PAM
		pam_paths="/ /usr /usr/local"
		for path in $pam_paths; do
			if test -d "$path/etc/pam.d"; then
				PAMDIR="$path"
				break
			fi
		done
	fi

	PAM_CFLAGS=""
	PAM_LIBS=""

	pam_found="no"
	if test "x$PAMDIR" != "xNONE"; then
		AC_MSG_RESULT([yes (path: $PAMDIR)])
		AC_CHECK_HEADER([security/pam_appl.h],[
			AC_CHECK_LIB(pam, pam_set_item, [
				PAM_CFLAGS="-I$PAMDIR/include"
				PAM_LIBS="-L$PAMDIR/lib -lpam"
				pam_found="yes"
			])
		])
	else
		AC_MSG_RESULT([no])
	fi

	if test "x$pam_found" = "xno"; then
		if test "x$require_pam" = "xyes"; then
			AC_MSG_ERROR([PAM support missing])
		fi
		ifelse([$2], , :, [$2])
	else
		ifelse([$1], , :, [$1])
	fi

	AC_SUBST(PAMDIR)
	AC_SUBST(PAM_CFLAGS)
	AC_SUBST(PAM_LIBS)
])
