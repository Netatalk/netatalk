# M4 macros to check for the existence of dvips and troff2ps
# $Id: tex-check.m4,v 1.1 2003-06-07 12:01:04 srittau Exp $

AC_DEFUN([AC_PROG_DVIPS], [
	AC_ARG_WITH(dvips, [  --with-dvips     path to the dvips command], [
		DVIPS="$withval"
	], [
		AC_PATH_PROG(DVIPS, dvips)
	])
	AC_SUBST(DVIPS)
])

AC_DEFUN([AC_PROG_TROFF2PS], [
	AC_ARG_WITH(troff2ps, [  --with-troff2ps    path to the troff2ps command], [
		TROFF2PS="$withval"
	], [
		AC_PATH_PROG(TROFF2PS, troff2ps)
	])
	AC_SUBST(TROFF2PS)
])
