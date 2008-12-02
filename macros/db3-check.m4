dnl $Id: db3-check.m4,v 1.15 2008-12-02 18:22:01 morgana Exp $
dnl Autoconf macros to check for the Berkeley DB library


AC_DEFUN([NETATALK_BDB_LINK_TRY],
[if test $atalk_cv_lib_db = no ; then
	AC_MSG_CHECKING([for Berkeley DB link (]ifelse($2,,default,$2)[)])
	atalk_DB_LIB=ifelse($2,,-ldb,$2)
	atalk_LIBS=$LIBS
	LIBS="$atalk_DB_LIB $LIBS"

	AC_TRY_LINK([
#include <db.h>
],[
	char *version;
	int major, minor, patch;

	version = db_version( &major, &minor, &patch );
	return (0);
],[$1=yes],[$1=no])

	AC_MSG_RESULT([$$1])
	LIBS="$atalk_LIBS"
	if test $$1 = yes ; then
		atalk_cv_lib_db=ifelse($2,,-ldb,$2)
	fi
fi
])


AC_DEFUN([NETATALK_BDB_CHECK_VERSION],
[
	atalk_LIBS=$LIBS
	LIBS="${atalk_cv_lib_db} $LIBS"

	AC_MSG_CHECKING([Berkeley DB library version >= ${DB_MAJOR_REQ}.${DB_MINOR_REQ}.${DB_PATCH_REQ}])
	AC_TRY_RUN([
#if STDC_HEADERS
#include <stdlib.h>
#endif
#include <stdio.h>
#include <db.h>

int main(void) {
        int major, minor, patch;
        char *version_str;

        version_str = db_version(&major, &minor, &patch);

        /* check library version */
        if (major < DB_MAJOR_REQ || minor < DB_MINOR_REQ || patch < DB_PATCH_REQ) {
                printf("library version too old (%d.%d.%d), ",major, minor, patch);
                return (2);
        }

        /* check header and library match */
        if ( major != DB_VERSION_MAJOR || minor != DB_VERSION_MINOR || patch != DB_VERSION_PATCH) {
                printf("header/library version mismatch (%d.%d.%d/%d.%d.%d), ",
                         DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH, major, minor, patch);
                return (3);
        }

        printf("%d.%d.%d, ",major, minor, patch);
        return (0);
}
], atalk_cv_bdb_version="yes", atalk_cv_bdb_version="no", atalk_cv_bdb_version="cross")


        if test ${atalk_cv_bdb_version} = "yes"; then
        	AC_MSG_RESULT(yes)
        else
		AC_MSG_RESULT(no)
        fi
	LIBS="$atalk_LIBS"
])


AC_DEFUN([NETATALK_BDB_HEADER],
[
	savedcflags="$CFLAGS"
	CFLAGS="-I$1 $CFLAGS"
	dnl check for header version
        AC_MSG_CHECKING(ifelse($1,,default,$1)[/db.h version >= ${DB_MAJOR_REQ}.${DB_MINOR_REQ}.${DB_PATCH_REQ}])
        AC_TRY_RUN([
#if STDC_HEADERS
#include <stdlib.h>
#endif
#include <stdio.h>
#include <db.h>

int main(void) {

        /* check header version */
        if (DB_VERSION_MAJOR < DB_MAJOR_REQ || DB_VERSION_MINOR < DB_MINOR_REQ ||
            DB_VERSION_PATCH < DB_PATCH_REQ ) {
                printf("header file version too old (%d.%d.%d), ", DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH);
                return (1);
        }

        printf("%d.%d.%d, ", DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH);
        return (0);
}
], atalk_cv_bdbheader="yes", atalk_cv_bdbheader="no", atalk_cv_bdbheader="cross")

        if test ${atalk_cv_bdbheader} = "no"; then
        	bdbfound=no
	        AC_MSG_RESULT([no])
        else
                AC_MSG_RESULT([yes])
        fi
	CFLAGS="$savedcflags"
])

dnl I don't understand this stuff below
dnl AFAIK it works for 4.1 and 4.2 and (4.3 xor 4.4) 
dnl you can have 4.2 and 4.3 installed
dnl but If you have 4.3 and 4.4 it won't work with 4.3
dnl only 4.4
dnl didier 
AC_DEFUN([NETATALK_BERKELEY_LINK],
[
atalk_cv_lib_db=no
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_dot_2,[-ldb-4.2])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db42,[-ldb42])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_42,[-ldb-42])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_2,[-ldb-4-2])

NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_dot_2,[-ldb-4.4])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db42,[-ldb44])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_42,[-ldb-44])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_2,[-ldb-4-4])

NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_dot_2,[-ldb-4.3])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db42,[-ldb43])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_42,[-ldb-43])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_2,[-ldb-4-3])

NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_dot_1,[-ldb-4.1])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db41,[-ldb41])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_41,[-ldb-41])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4_1,[-ldb-4-1])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db_4,[-ldb-4])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db4,[-ldb4])
NETATALK_BDB_LINK_TRY(atalk_cv_db_db,[-ldb])
])


AC_DEFUN([AC_PATH_BDB], 
[
	trybdbdir=""
	dobdbsearch=yes
	bdb_search_dirs="/usr/local/include /usr/include"
	search_subdirs="/db4.2 /db42 /db4.3 /db43 /db4.4 /db44 /db4.1 /db41 /db4 /"

dnl required BDB version
	DB_MAJOR_REQ=4
	DB_MINOR_REQ=1
	DB_PATCH_REQ=0

dnl make sure atalk_libname is defined beforehand
[[ -n "$atalk_libname" ]] || AC_MSG_ERROR([internal error, atalk_libname undefined])

dnl define the required BDB version
	AC_DEFINE_UNQUOTED(DB_MAJOR_REQ, ${DB_MAJOR_REQ}, [Required BDB version, major])
	AC_DEFINE_UNQUOTED(DB_MINOR_REQ, ${DB_MINOR_REQ}, [Required BDB version, minor])
	AC_DEFINE_UNQUOTED(DB_PATCH_REQ, ${DB_PATCH_REQ}, [Required BDB version, patch])


	AC_ARG_WITH(bdb,
		[  --with-bdb=PATH         specify path to Berkeley DB installation[[auto]]],
		if test "x$withval" = "xno"; then
			dobdbsearch=no
		elif test "x$withval" = "xyes"; then
			dobdbsearch=yes
		else
			bdb_search_dirs="$withval/include $withval"
		fi
	)


	bdbfound=no
        savedcflags="$CFLAGS"
	savedldflags="$LDFLAGS"
	savedcppflags="$CPPFLAGS"
	savedlibs="$LIBS"

	if test "x$dobdbsearch" = "xyes"; then
	    for bdbdir in $bdb_search_dirs; do
		if test $bdbfound = "yes"; then
			break;
		fi
		for subdir in ${search_subdirs}; do
		    AC_MSG_CHECKING([for Berkeley DB headers in ${bdbdir}${subdir}])
		    if test -f "${bdbdir}${subdir}/db.h" ; then
			AC_MSG_RESULT([yes])
			NETATALK_BDB_HEADER([${bdbdir}${subdir}])
			if test ${atalk_cv_bdbheader} != "no"; then
			
dnl			  bdblibdir="`echo $bdbdir | sed 's/\/include\/db4\.*.*//'`"
			  bdblibdir="`echo $bdbdir | sed 's/\/include\/db4.*//'`"
			  bdblibdir="`echo $bdblibdir | sed 's/\/include$//'`"
			  bdblibdir="${bdblibdir}/${atalk_libname}"
dnl			  bdbbindir="`echo $bdbdir | sed 's/include\/db4\.*.*/bin/'`"
			  bdbbindir="`echo $bdbdir | sed 's/\/include\/db4.*/bin/'`"
			  bdbbindir="`echo $bdbbindir | sed 's/include$/bin/'`"

			  CPPFLAGS="-I${bdbdir}${subdir} $CFLAGS"
			  CFLAGS=""
			  LDFLAGS="-L$bdblibdir $LDFLAGS"
			  if test "x$need_dash_r" = "xyes"; then
				LDFLAGS="$LDFLAGS -R${bdblibdir}"
			  fi
			  NETATALK_BERKELEY_LINK
			  if test x"${atalk_cv_lib_db}" != x"no"; then
				NETATALK_BDB_CHECK_VERSION
				if test x"${atalk_cv_bdb_version}" != x"no"; then
				    BDB_LIBS="-L${bdblibdir} ${atalk_cv_lib_db}"
				    if test "x$need_dash_r" = "xyes"; then
					BDB_LIBS="$BDB_LIBS -R${bdblibdir}"
				    fi
				    BDB_CFLAGS="-I${bdbdir}${subdir}"
                                    BDB_BIN=$bdbbindir
                                    BDB_PATH="`echo $bdbdir | sed 's,include\/db4$,,'`"
                                    BDB_PATH="`echo $BDB_PATH | sed 's,include$,,'`"
				    bdbfound=yes
				    break;
				fi
			  fi	
			  CFLAGS="$savedcflags"
			  LDFLAGS="$savedldflags"
			  CPPFLAGS="$savedcppflags"
			  LIBS="$savedlibs"
			fi
		    else
			AC_MSG_RESULT([no])
		    fi
	        done
	    done
	fi

        CFLAGS="$savedcflags"
        LDFLAGS="$savedldflags"
        CPPFLAGS="$savedcppflags"
        LIBS="$savedlibs"

	if test "x$bdbfound" = "xyes"; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])     
	fi

        CFLAGS_REMOVE_USR_INCLUDE(BDB_CFLAGS)
        LIB_REMOVE_USR_LIB(BDB_LIBS)
	AC_SUBST(BDB_CFLAGS)
	AC_SUBST(BDB_LIBS)
	AC_SUBST(BDB_BIN)
	AC_SUBST(BDB_PATH)
])


