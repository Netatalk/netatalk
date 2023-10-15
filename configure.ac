dnl configure.ac for netatalk

AC_INIT([netatalk],m4_normalize(m4_include([VERSION])))
AC_CONFIG_SRCDIR([etc/afpd/main.c])

NETATALK_VERSION=`cat $srcdir/VERSION`
AC_SUBST(NETATALK_VERSION)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([macros])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([subdir-objects dist-xz no-dist-gzip])
AM_MAINTAINER_MODE([enable])

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([dlopen])
AC_PROG_PERL
AC_PROG_GREP
AC_PROG_PS
AM_PROG_CC_C_O
AC_C_BIGENDIAN
AC_C_INLINE
AC_PROG_LEX([noyywrap])
AC_PROG_YACC

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
AC_CHECK_HEADERS([sys/mount.h], , ,
[#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])
AC_CHECK_TYPES([fshare_t], [], [], [[#include <fcntl.h>]])

AC_SYS_LARGEFILE([], AC_MSG_ERROR([AFP 3.x support requires Large File Support.]))

dnl --------------------------------------------------------------------------
dnl check if dlsym needs to add an underscore, uses libtool macros
dnl --------------------------------------------------------------------------
LT_LIB_DLLOAD
AC_CHECK_FUNCS(dlopen dlsym dlclose)
LT_FUNC_DLSYM_USCORE
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
AC_CHECK_FUNCS(backtrace_symbols dirfd getusershell pread pwrite pselect)
AC_CHECK_FUNCS(setlinebuf strlcat strlcpy strnlen mempcpy vasprintf asprintf)
AC_CHECK_FUNCS(mmap utime getpagesize) dnl needed by tbd

dnl search for necessary libraries
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(connect, socket)
AC_CHECK_FUNCS(getifaddrs) dnl comes after gethostbyname and connect so it picks up the libs

AX_PTHREAD(, [AC_MSG_ERROR([missing pthread_sigmask])])

AC_DEFINE(OPEN_NOFOLLOW_ERRNO, ELOOP, errno returned by open with O_NOFOLLOW)

dnl 64bit platform check
AC_NETATALK_64BIT_LIBS

dnl OS specific checks
AC_NETATALK_OS_SPECIFIC

dnl Whether to set rpath
AC_NETATALK_SET_RPATH

dnl Check for optional AFS support
AC_NETATALK_AFS_CHECK

dnl --with-pkgconfdir check to change configuration directory location
AC_NETATALK_CONFIG_DIRS

dnl Check for optional cracklib support
AC_NETATALK_CRACKLIB

dnl Check whether to enable debug code
AC_NETATALK_DEBUG

dnl Check whether to disable tickle SIGALARM stuff, which eases debugging
AC_NETATALK_DEBUGGING

dnl	Check for libiconv support
AC_NETATALK_CHECK_ICONV

dnl Check for CNID database backends
AC_NETATALK_CNID([bdb_required=yes],[bdb_required=no])

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

dnl Check for optional sysv initscript install
AC_NETATALK_INIT_STYLE

dnl Path where UAM modules shall be installed
AC_ARG_WITH(uams-path, [  --with-uams-path=PATH   path to UAMs [[$libdir/netatalk]]], [uams_path="$withval"], [uams_path="$libdir/netatalk"])

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

dnl Check for Kerberos 5 support
AC_NETATALK_KERBEROS

dnl Check for overwrite the config files or not
AC_NETATALK_OVERWRITE_CONFIG

dnl Check for LDAP support, for client-side ACL visibility
AC_NETATALK_LDAP

dnl Check for ACL support
AC_NETATALK_ACL

dnl Check for Extended Attributes support
AC_NETATALK_EXTENDED_ATTRIBUTES

dnl Check if realpath() takes NULL
AC_NETATALK_REALPATH

dnl Check for sendfile()
AC_NETATALK_SENDFILE
AC_NETATALK_RECVFILE

dnl Check whether bundled libevent shall not be used
AC_NETATALK_LIBEVENT

dnl Check whether bundled talloc shall be used
AC_NETATALK_TALLOC

dnl Check for Tracker
AC_NETATALK_SPOTLIGHT

dnl Check for dtrace
AC_NETATALK_DTRACE

dnl Check for dbus-glib, for AFP stats on dbus
AC_NETATALK_DBUS_GLIB

dnl FHS stuff has to be done last because it overrides other defaults
AC_NETATALK_FHS

dnl netatalk lockfile path, must come after AC_NETATALK_FHS
AC_NETATALK_LOCKFILE

dnl Check for Docbook and build documentation if found
AX_CHECK_DOCBOOK

CPPFLAGS="-I\$(top_srcdir)/include -I\$(top_builddir)/include -I\$(top_srcdir) $CPPFLAGS"
UAMS_PATH="${uams_path}"

AC_SUBST(LIBS)
AC_SUBST(CFLAGS)
AC_SUBST(UAMS_PATH)

AM_CONDITIONAL(SOLARIS_MODULE, test x$solaris_module = xyes)
AM_CONDITIONAL(HAVE_LIBGCRYPT, test x$neta_cv_have_libgcrypt = xyes)
AM_CONDITIONAL(HAVE_OPENSSL, test x$neta_cv_have_openssl = xyes)
AM_CONDITIONAL(HAVE_ACLS, test x"$ac_cv_have_acls" = x"yes")
AM_CONDITIONAL(HAVE_LDAP, test x"$netatalk_cv_ldap" = x"yes")
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
AM_CONDITIONAL(USE_SHADOWPW, test x$shadowpw = xyes)

dnl Enable silent Automake rules if present
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl --------------------- generate files
AC_CONFIG_FILES([
  Makefile
  bin/Makefile
  bin/ad/Makefile
  bin/afppasswd/Makefile
  bin/cnid/Makefile
  bin/cnid/cnid2_create
  bin/misc/Makefile
  config/Makefile
  config/pam/Makefile
  contrib/Makefile
  contrib/macusers/Makefile
  contrib/macusers/macusers
  contrib/shell_utils/Makefile
  contrib/shell_utils/apple_dump
  contrib/shell_utils/asip-status
  distrib/Makefile
  distrib/config/Makefile
  distrib/config/netatalk-config
  distrib/initscripts/Makefile
  distrib/m4/Makefile
  doc/Makefile
  doc/html.xsl
  doc/man.xsl
  doc/manual/Makefile
  doc/manual/manual.xml
  doc/manpages/Makefile
  doc/manpages/man1/Makefile
  doc/manpages/man5/Makefile
  doc/manpages/man8/Makefile
  etc/Makefile
  etc/afpd/Makefile
  etc/cnid_dbd/Makefile
  etc/netatalk/Makefile
  etc/spotlight/Makefile
  etc/uams/Makefile
  include/Makefile
  include/atalk/Makefile
  libatalk/Makefile
  libatalk/acl/Makefile
  libatalk/adouble/Makefile
  libatalk/bstring/Makefile
  libatalk/cnid/Makefile
  libatalk/cnid/last/Makefile
  libatalk/cnid/dbd/Makefile
  libatalk/cnid/mysql/Makefile
  libatalk/compat/Makefile
  libatalk/dsi/Makefile
  libatalk/iniparser/Makefile
  libatalk/talloc/Makefile
  libatalk/unicode/Makefile
  libatalk/unicode/charsets/Makefile
  libatalk/util/Makefile
  libatalk/vfs/Makefile
  macros/Makefile
  man/Makefile
  man/man1/Makefile
  man/man1/ad.1
  man/man1/afpldaptest.1
  man/man1/afppasswd.1
  man/man1/afpstats.1
  man/man1/apple_dump.1
  man/man1/asip-status.1
  man/man1/dbd.1
  man/man1/macusers.1
  man/man1/netatalk-config.1
  man/man5/Makefile
  man/man5/afp.conf.5
  man/man5/afp_signature.conf.5
  man/man5/afp_voluuid.conf.5
  man/man5/extmap.conf.5
  man/man8/Makefile
  man/man8/afpd.8
  man/man8/cnid_dbd.8
  man/man8/cnid_metad.8
  man/man8/netatalk.8
  test/Makefile
  test/afpd/Makefile
])

AC_CONFIG_COMMANDS([default],
  [chmod a+x distrib/config/netatalk-config contrib/shell_utils/apple_*],
  []
)

AC_OUTPUT

AC_NETATALK_LIBS_SUMMARY
AC_NETATALK_CONFIG_SUMMARY
