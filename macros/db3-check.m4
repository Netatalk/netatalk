dnl $Id: db3-check.m4,v 1.3 2001-11-14 22:36:35 jmarcus Exp $
dnl Autoconf macro to check for the Berkeley DB3 library

AC_DEFUN([AC_PATH_DB3], [
	trydb3dir=""
	AC_ARG_WITH(db3,
		[  --with-db3=PATH         specify path to Berkeley DB3 installation],
		if test "x$withval" != "xno"; then
			trydb3dir="$withval"
		fi
	)

	db3found=no
	for db3dir in "" "$trydb3dir" "$trydb3dir/include" "$trydb3dir/include/db3" "/usr/include" "/usr/local/BerkeleyDB.3.3/include" "/usr/include/db3" "/usr/local/include" "/usr/local/include/db3" ; do
		if test -f "$db3dir/db.h" ; then
			db3libdir="`echo $db3dir | sed 's/include\/db3$/lib/'`"
			db3libdir="`echo $db3libdir | sed 's/include$/lib/'`"

			savedcflags="$CFLAGS"
			savedldflags="$LDFLAGS"
			CFLAGS="$CFLAGS -I$db3dir"
			LDFLAGS="$LDFLAGS -L$db3libdir"
			AC_CHECK_LIB(db3, main, [
				db3found=yes
				DB3_CFLAGS="-I$db3dir"
				DB3_LIBS="-L$db3libdir -ldb3"
			])
			CFLAGS="$savedcflags"
			LDFLAGS="$savedldflags"
			break;
		fi
	done

	if test "x$db3found" = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])     
	fi

	AC_SUBST(DB3_CFLAGS)
	AC_SUBST(DB3_LIBS)
])
