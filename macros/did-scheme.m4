dnl $Id: did-scheme.m4,v 1.1 2003-06-08 14:52:37 srittau Exp $

AC_DEFUN([NETATALK_ARG_DID], [

dnl Don't use BDB unless it's needed
bdb_required=no

dnl Determine DID scheme

AC_MSG_CHECKING([for DID scheme to use])
AC_ARG_WITH(did,
	[  --with-did=SCHEME       set DID scheme (cnid,last) [cnid]],
	[ did_scheme="$withval" ],
	[ did_scheme="cnid" ]
)

if test "x$did_scheme" = "xlast"; then
	AC_DEFINE(USE_LASTDID, 1, [Define if the last DID scheme should be used])
	AC_MSG_RESULT([last])
elif test "x$did_scheme" = "xcnid"; then
	bdb_required="yes"
	AC_DEFINE(CNID_DB, 1, [Define if the CNID DB DID scheme should be used])
	AC_MSG_RESULT([CNID DB])
else
	AC_MSG_ERROR([unknown DID scheme])
fi
AM_CONDITIONAL(COMPILE_CNID, test "x$did_scheme" = "xcnid")

dnl Check for Berkeley DB library
if test "x$bdb_required" = "xyes"; then
	AC_PATH_BDB(, [AC_MSG_ERROR([Berkeley DB library not found!])])
fi

])
