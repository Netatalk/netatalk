dnl configure.in for netatalk

AC_INIT(etc/afpd/main.c)

NETATALK_VERSION=`cat $srcdir/VERSION`
AC_SUBST(NETATALK_VERSION)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(netatalk, ${NETATALK_VERSION})
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE([enable])

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_PERL
AC_PROG_GREP
AC_PROG_PS
AM_PROG_CC_C_O

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

dnl Request SUSv3 standard interfaces plus anything else the platform may have
CFLAGS="$CFLAGS -D_XOPEN_SOURCE=600 -D__EXTENSIONS__ -D_GNU_SOURCE"

dnl Check if we can use attribute unused (gcc only) from ethereal
AC_MSG_CHECKING(to see if we can add '__attribute__((unused))' to CFLAGS)
if test x$GCC != x ; then
  CFLAGS="-D_U_=\"__attribute__((unused))\" $CFLAGS"
  AC_MSG_RESULT(yes)
else
  CFLAGS="-D_U_=\"\" $CFLAGS"
  AC_MSG_RESULT(no)
fi

dnl Checks for header files, some checks are obsolete, unfortunately the code
dnl uses the resulting macros, so the code has to cleaned up too before
dnl we can remove the checks here.
AC_CHECK_HEADERS(mntent.h unistd.h termios.h ufs/quota.h)
AC_CHECK_HEADERS(netdb.h sgtty.h statfs.h dlfcn.h langinfo.h locale.h)
AC_CHECK_HEADERS(sys/param.h sys/fcntl.h sys/termios.h)
AC_CHECK_HEADERS(sys/mnttab.h sys/statvfs.h sys/stat.h sys/vfs.h)
dnl Checks for header files, confirmed to be required as of 2011
AC_CHECK_HEADERS(sys/epoll.h)
AC_CHECK_HEADERS([sys/mount.h], , , 
[#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])

AC_SYS_LARGEFILE([], AC_MSG_ERROR([AFP 3.x support requires Large File Support.]))

dnl --------------------------------------------------------------------------
dnl check if dlsym needs to add an underscore, uses libtool macros 
dnl --------------------------------------------------------------------------
AC_LTDL_DLLIB
AC_CHECK_FUNCS(dlopen dlsym dlclose)
AC_LTDL_DLSYM_USCORE
if test x"$libltdl_cv_need_uscore" = xyes; then
    AC_DEFINE(DLSYM_PREPEND_UNDERSCORE, 1, [BSD compatibility macro])
fi

dnl Special hecks
ac_neta_haveatfuncs=yes
AC_CHECK_FUNCS(openat renameat fstatat unlinkat, , ac_neta_haveatfuncs=no)
if test x"$ac_neta_haveatfuncs" = x"yes" ; then
   AC_DEFINE([_ATFILE_SOURCE], 1, AT file source)
   AC_DEFINE([HAVE_ATFUNCS], 1, whether at funcs are available)
fi
AC_CHECK_MEMBERS(struct tm.tm_gmtoff,,, [#include <time.h>])

dnl these tests have been comfirmed to be needed in 2011
AC_CHECK_FUNC(epoll_create, AC_DEFINE([HAVE_EPOLL], 1, Whether Linux epoll is available)) 
AC_CHECK_FUNCS(backtrace_symbols dirfd getusershell pread pwrite pselect)
AC_CHECK_FUNCS(setlinebuf strlcat strlcpy strnlen)
AC_CHECK_FUNCS(mmap utime getpagesize) dnl needed by tbd

dnl search for necessary libraries
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(connect, socket)
AC_SEARCH_LIBS(pthread_sigmask, pthread,,[AC_MSG_ERROR([missing pthread_sigmask])])
if test x"$ac_cv_search_pthread_sigmask" != x"none required" ; then
   PTHREAD_LIBS=$ac_cv_search_pthread_sigmask
fi
AC_SUBST(PTHREAD_LIBS)

AC_DEFINE(OPEN_NOFOLLOW_ERRNO, ELOOP, errno returned by open with O_NOFOLLOW)

AC_CACHE_SAVE

dnl --------------------------------------------------------------------------
dnl 64bit platform check
dnl --------------------------------------------------------------------------

AC_MSG_CHECKING([whether to check for 64bit libraries])
# Test if the compiler is in 64bit mode
echo 'int i;' > conftest.$ac_ext
atalk_cv_cc_64bit_output=no
if AC_TRY_EVAL(ac_compile); then
    case `/usr/bin/file conftest.$ac_objext` in
    *"ELF 64"*)
      atalk_cv_cc_64bit_output=yes
      ;;
    esac
fi
rm -rf conftest*

case $host_cpu:$atalk_cv_cc_64bit_output in
powerpc64:yes | s390x:yes | sparc*:yes | x86_64:yes | i386:yes)
    case $target_os in
    solaris2*)
        AC_MSG_RESULT([yes])
        atalk_libname="lib/64"
        ;;
    *bsd* | dragonfly*)
        AC_MSG_RESULT([no])
        atalk_libname="lib"
        ;;
    *)
        AC_MSG_RESULT([yes])
        atalk_libname="lib64"
        ;;
    esac
    ;;
*:*)
    AC_MSG_RESULT([no])
    atalk_libname="lib"
    ;;
esac

dnl --------------------------------------------------------------------------
dnl OS specific configuration comes in here:
dnl --------------------------------------------------------------------------

AC_NETATALK_OS_SPECIFIC

dnl --------------------------------------------------------------------------
dnl specific configuration comes in here:
dnl --------------------------------------------------------------------------

dnl Check for optional admin group support
AC_NETATALK_ADMIN_GROUP

dnl Check for optional AFS support
AC_NETATALK_AFS_CHECK

dnl --with-pkgconfdir check to change configuration directory location
AC_NETATALK_CONFIG_DIRS

dnl Check for optional cracklib support
AC_NETATALK_CRACKLIB

dnl Check whether to enable debug code
AC_NETATALK_DEBUG

dnl Check whethe to disable tickle SIGALARM stuff, which eases debugging
AC_NETATALK_DEBUGGING

dnl	Check for libiconv support
AC_NETATALK_CHECK_ICONV

dnl Check for CNID database backends
AC_NETATALK_CNID([bdb_required=yes],[bdb_required=no])

dnl Check for quota support
AC_NETATALK_CHECK_QUOTA

dnl Check for optional server location protocol support (used by MacOS X)
AC_NETATALK_SRVLOC

dnl Check for optional Zeroconf support
AC_NETATALK_ZEROCONF

dnl Check for optional TCP-wrappers support
AC_NETATALK_TCP_WRAPPERS

dnl Check for PAM libs
AC_NETATALK_PATH_PAM

dnl Check for optional shadow password support
AC_NETATALK_SHADOW
	
dnl Check for optional valid-shell-check support
AC_NETATALK_SHELL_CHECK

dnl Check for optional Webmin
AC_NETATALK_WEBMIN

dnl Check for optional sysv initscript install
AC_NETATALK_SYSV_STYLE

dnl Path where UAM modules shall be installed
AC_ARG_WITH(uams-path, [  --with-uams-path=PATH   path to UAMs [[PKGCONF/uams]]], [uams_path="$withval"], [uams_path="${PKGCONFDIR}/uams"])

dnl Check for libgcrypt, if found enables DHX2 UAM
AC_NETATALK_PATH_LIBGCRYPT([1:1.2.3])

dnl Check for openssl, if found enables DHX UAM and Randnum UAM
AC_NETATALK_PATH_SSL

dnl Check for Berkeley DB library
AC_NETATALK_PATH_BDB

dnl Check for crypt
AC_NETATALK_CRYPT

dnl Check for building PGP UAM module
AC_NETATALK_PGP_UAM

dnl Check for building Kerberos V UAM module
AC_NETATALK_KRB5_UAM

dnl Check for overwrite the config files or not
AC_NETATALK_OVERWRITE_CONFIG

dnl Check for LDAP support, for client-side ACL visibility
AC_NETATALK_LDAP

dnl Check for ACL support
AC_NETATALK_ACL

dnl Check for Extended Attributes support
AC_NETATALK_EXTENDED_ATTRIBUTES

dnl Check for libsmbsharemodes from Samba for Samba/Netatalk access/deny/share modes interop
AC_NETATALK_SMB_SHAREMODES

dnl Check if realpath() takes NULL
AC_NETATALK_REALPATH

dnl Check for sendfile()
AC_NETATALK_SENDFILE

dnl --------------------------------------------------------------------------
dnl FHS stuff has to be done last because it overrides other defaults
dnl --------------------------------------------------------------------------

AC_MSG_CHECKING([whether to use Filesystem Hierarchy Standard (FHS) compatibility])
AC_ARG_ENABLE(fhs,
	[  --enable-fhs            use Filesystem Hierarchy Standard (FHS) compatibility],[
	if test "$enableval" = "yes"; then
		uams_path="${libdir}/netatalk"
		sysconfdir="/etc"
		PKGCONFDIR=${sysconfdir}/netatalk
		SERVERTEXT="${PKGCONFDIR}/msg"
		use_pam_so=yes
		mandir="/usr/share/man"
		AC_DEFINE(FHS_COMPATIBILITY, 1, [Define if you want compatibily with the FHS])
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

dnl --------------------------------------------------------------------------
dnl post-FHS substitutions, etc
dnl --------------------------------------------------------------------------

UAMS_PATH="${uams_path}"
AC_SUBST(UAMS_PATH)

dnl --------------------------------------------------------------------------
dnl drop in includes for top level directory structures here...
dnl --------------------------------------------------------------------------

dnl Note: $(top_srcdir)/include should be added before all other includes
dnl       so that includes from that directory a preferred to includes from
dnl       /usr/include or similar places.
LIBS="$LIBS -L\$(top_srcdir)/libatalk"
CFLAGS="-I\$(top_srcdir)/include -I\$(top_srcdir)/sys $CFLAGS"


dnl --------------------------------------------------------------------------
dnl Last minute substitutions
dnl --------------------------------------------------------------------------

dnl ac_cv_target_os is now host_os, ac_cv_target_cpu is now host_cpu
AC_SUBST(LIBS)
AC_SUBST(CFLAGS)

AM_CONDITIONAL(SOLARIS_MODULE, test x$solaris_module = xyes)
AM_CONDITIONAL(HAVE_LIBGCRYPT, test x$neta_cv_have_libgcrypt = xyes)
AM_CONDITIONAL(HAVE_OPENSSL, test x$neta_cv_have_openssl = xyes)
AM_CONDITIONAL(HAVE_ACLS, test x"$with_acl_support" = x"yes")
AM_CONDITIONAL(HAVE_LDAP, test x"$with_ldap" = x"yes")
AM_CONDITIONAL(USE_DHX, test x$neta_cv_compile_dhx = xyes)
AM_CONDITIONAL(USE_DHX2, test x$neta_cv_compile_dhx2 = xyes)
AM_CONDITIONAL(USE_RANDNUM, test x$neta_cv_have_openssl = xyes)
AM_CONDITIONAL(USE_PAM_SO, test x$use_pam_so = xyes)
AM_CONDITIONAL(USE_PAM, test x$netatalk_cv_install_pam = xyes)
AM_CONDITIONAL(BUILD_PAM, test x$compile_pam = xyes)
AM_CONDITIONAL(USE_PGP, test x$compile_pgp = xyes)
AM_CONDITIONAL(DEFAULT_HOOK, test x$neta_cv_have_libgcrypt != xyes && test x$neta_cv_have_openssl != xyes)
AM_CONDITIONAL(USE_BDB, test x$bdb_required = xyes)
AM_CONDITIONAL(HAVE_ATFUNCS, test x"$ac_neta_haveatfuncs" = x"yes")

dnl --------------------- generate files

AC_OUTPUT([Makefile
	bin/Makefile
	bin/ad/Makefile
	bin/afppasswd/Makefile
	bin/cnid/Makefile
	bin/cnid/cnid2_create
	bin/megatron/Makefile
    bin/misc/Makefile
	bin/uniconv/Makefile
	config/Makefile
	config/pam/Makefile
	contrib/Makefile
	contrib/macusers/Makefile
	contrib/macusers/macusers
	contrib/shell_utils/Makefile
	contrib/shell_utils/apple_dump
	contrib/shell_utils/asip-status.pl
	distrib/Makefile
	distrib/config/Makefile
	distrib/config/netatalk-config
	distrib/initscripts/Makefile
	distrib/m4/Makefile
	distrib/systemd/Makefile
	doc/Makefile
	etc/Makefile
	etc/afpd/Makefile
	etc/cnid_dbd/Makefile
	etc/uams/Makefile
	include/Makefile
	include/atalk/Makefile
	libatalk/Makefile
	libatalk/acl/Makefile
	libatalk/adouble/Makefile
	libatalk/bstring/Makefile
	libatalk/cnid/Makefile
	libatalk/cnid/cdb/Makefile
	libatalk/cnid/last/Makefile
	libatalk/cnid/dbd/Makefile
	libatalk/cnid/tdb/Makefile
	libatalk/compat/Makefile
	libatalk/dsi/Makefile
	libatalk/iniparser/Makefile
	libatalk/tdb/Makefile
	libatalk/unicode/Makefile
	libatalk/unicode/charsets/Makefile
	libatalk/util/Makefile
	libatalk/vfs/Makefile
	macros/Makefile
	man/Makefile
	man/man1/Makefile
	man/man5/Makefile
	man/man8/Makefile
	test/Makefile
	test/afpd/Makefile
	],
	[chmod a+x distrib/config/netatalk-config contrib/shell_utils/apple_*]
)

AC_NETATALK_LIBS_SUMMARY
AC_NETATALK_CONFIG_SUMMARY
