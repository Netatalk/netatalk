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

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h stdint.h strings.h time.h sys/param.h sys/fcntl.h sys/file.h sys/ioctl.h sys/time.h sys/mnttab.h sys/statvfs.h sys/stat.h sys/vfs.h mntent.h syslog.h unistd.h termios.h sys/termios.h netdb.h sgtty.h ufs/quota.h mount.h statfs.h sys/types.h dlfcn.h errno.h sys/errno.h sys/uio.h langinfo.h locale.h sys/filio.h)
AC_CHECK_HEADER(sys/cdefs.h,,
	AC_MSG_RESULT([enabling generic cdefs.h from tree])
	CFLAGS="-I\$(top_srcdir)/sys/generic $CFLAGS"
)
AC_CHECK_HEADERS([sys/mount.h], , , 
[#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_RDEV
AC_HEADER_TIME
AC_STRUCT_TM

dnl --------------------------------------------------------------------------
dnl check if dlsym needs to add an underscore, uses libtool macros 
dnl --------------------------------------------------------------------------
AC_LTDL_DLLIB
AC_CHECK_FUNCS(dlopen dlsym dlclose)
AC_LTDL_DLSYM_USCORE
if test x"$libltdl_cv_need_uscore" = xyes; then
    AC_DEFINE(DLSYM_PREPEND_UNDERSCORE, 1, [BSD compatibility macro])
fi

dnl Checks for library functions.
AC_TYPE_GETGROUPS
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_HEADER_MAJOR
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_FUNC_WAIT3
AC_CHECK_FUNCS(getcwd gethostname gettimeofday getusershell mkdir rmdir select socket strdup strcasestr strstr strtoul strchr memcpy)
AC_CHECK_FUNCS(backtrace_symbols setlocale nl_langinfo strlcpy strlcat setlinebuf dirfd pselect access pread pwrite)
AC_CHECK_FUNCS(waitpid getcwd strdup strndup strnlen strtoul strerror chown fchown chmod fchmod chroot link mknod mknod64)
ac_neta_haveatfuncs=yes
AC_CHECK_FUNCS(openat renameat fstatat unlinkat, , ac_neta_haveatfuncs=no)
if test x"$ac_neta_haveatfuncs" = x"yes" ; then
   AC_DEFINE([_ATFILE_SOURCE], 1, AT file source)
   AC_DEFINE([HAVE_ATFUNCS], 1, whether at funcs are available)
fi

AC_CHECK_MEMBERS(struct tm.tm_gmtoff,,, [#include <time.h>])

AC_CHECK_FUNC(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname)])
AC_CHECK_FUNC(connect,,[AC_CHECK_LIB(socket,connect)])
dnl search for necessary libs for libpthread stuff
AC_SEARCH_LIBS(pthread_sigmask, pthread,, 
               [AC_MSG_ERROR([cannot find pthread_sigmask in libc or libpthread])])
if test x"$ac_cv_search_pthread_sigmask" != x"none required" ; then
   PTHREAD_LIBS=$ac_cv_search_pthread_sigmask
fi
AC_SUBST(PTHREAD_LIBS)
AC_CACHE_SAVE

dnl Checks for (v)snprintf
NETATALK_SNPRINTF_CHECK

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
    AC_MSG_RESULT([yes])
    case $target_os in
    solaris2*)
        atalk_libname="lib/64"
        ;;
    *)
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
dnl specific configuration comes in here:
dnl --------------------------------------------------------------------------


netatalk_cv_admin_group=yes
AC_MSG_CHECKING([for administrative group support])
AC_ARG_ENABLE(admin-group,
 	[  --disable-admin-group   disable admin group],[
        if test x"$enableval" = x"no"; then
		AC_DEFINE(ADMIN_GRP, 0, [Define if the admin group should be enabled])
		netatalk_cv_admin_group=no
		AC_MSG_RESULT([no])
	else
		AC_DEFINE(ADMIN_GRP, 1, [Define if the admin group should be enabled])
		AC_MSG_RESULT([yes])
        fi],[
		AC_DEFINE(ADMIN_GRP, 1, [Define if the admin group should be enabled])
		AC_MSG_RESULT([yes])
	]
)

NETATALK_AFS_CHECK

NETATALK_CONFIG_DIRS

netatalk_cv_with_cracklib=no
AC_ARG_WITH(cracklib,
	[  --with-cracklib=DICT    enable/set location of cracklib dictionary],[
	if test "x$withval" != "xno" ; then
		cracklib="$withval"
		AC_CHECK_LIB(crack, main, [
			AC_DEFINE(USE_CRACKLIB, 1, [Define if cracklib should be used])
			LIBS="$LIBS -lcrack"
			if test "$cracklib" = "yes"; then
				cracklib="/usr/$atalk_libname/cracklib_dict"
			fi
			AC_DEFINE_UNQUOTED(_PATH_CRACKLIB, "$cracklib",
				[path to cracklib dictionary])
			AC_MSG_RESULT([setting cracklib dictionary to $cracklib])
			netatalk_cv_with_cracklib=yes
			],[
			AC_MSG_ERROR([cracklib not found!])
			]
		)
	fi
	]
)
AC_MSG_CHECKING([for cracklib support])
AC_MSG_RESULT([$netatalk_cv_with_cracklib])

netatalk_cv_ddp_enabled=no
AC_MSG_CHECKING([whether to enable DDP])
AC_ARG_ENABLE(ddp,
	[  --enable-ddp            enable DDP (AppleTalk)],[
	if test "$enableval" = "yes"; then 
		AC_MSG_RESULT([yes])
		netatalk_cv_ddp_enabled=yes
	else
		AC_MSG_RESULT([yes])
		AC_DEFINE(NO_DDP, 1, [Define if DDP should be disabled])
	fi
	],[
		AC_MSG_RESULT([no])
		AC_DEFINE(NO_DDP, 1, [Define if DDP should be disabled])
	]
)

AC_MSG_CHECKING([whether to enable debug code])
AC_ARG_ENABLE(debug1,
	[  --enable-debug1         enable debug code],[
	if test "$enableval" != "no"; then
		if test "$enableval" = "yes"; then
			AC_DEFINE(DEBUG1, 1, [Define if debugging information should be included])
		else
			AC_DEFINE_UNQUOTED(DEBUG1, $enableval, [Define if debugging information should be included])
		fi 
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

AC_MSG_CHECKING([whether to enable verbose debug code])
AC_ARG_ENABLE(debug,
	[  --enable-debug          enable verbose debug code],[
	if test "$enableval" != "no"; then
		if test "$enableval" = "yes"; then
			AC_DEFINE(DEBUG, 1, [Define if verbose debugging information should be included])
		else
			AC_DEFINE_UNQUOTED(DEBUG, $enableval, [Define if verbose debugging information should be included])
		fi 
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
        AC_DEFINE(NDEBUG, 1, [Disable assertions])
	fi
	],[
		AC_MSG_RESULT([no])
        AC_DEFINE(NDEBUG, 1, [Disable assertions])
	]
)

AC_MSG_CHECKING([whether to enable debugging with debuggers])
AC_ARG_ENABLE(debugging,
	[  --enable-debugging      disable SIGALRM timers and DSI tickles (eg for debugging with gdb/dbx/...)],[
	if test "$enableval" != "no"; then
		if test "$enableval" = "yes"; then
			AC_DEFINE(DEBUGGING, 1, [Define if you want to disable SIGALRM timers and DSI tickles])
		else
			AC_DEFINE_UNQUOTED(DEBUGGING, $enableval, [Define if you want to disable SIGALRM timers and DSI tickles])
		fi 
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

AC_SYS_LARGEFILE([], AC_MSG_ERROR([AFP 3.x support requires Large File Support.]))
AC_CHECK_ICONV

dnl ----------- A NOTE ABOUT DROPKLUDGE
dnl The trouble with this fix is that if you know what the file is called, it
dnl can be read from the Unix side.  That's okay for most academic institutions
dnl since the students don't have telnet access to the Mac servers.  There is
dnl currently no one working on further development/fixes of DROPKLUDGE.
dnl -----------

netatalk_cv_dropkludge=no
AC_MSG_CHECKING([whether to enable experimental dropbox support])
AC_ARG_ENABLE(dropkludge,
	[  --enable-dropkludge     enable the experimental dropbox fix (INSECURE!) ],[
	if test "$enableval" = "yes"; then 
		AC_DEFINE(DROPKLUDGE, 1, [Define if you want to use the experimental dropkludge support])
		AC_MSG_RESULT([yes])
		netatalk_cv_dropkludge=yes
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

netatalk_cv_force_uidgid=no
AC_MSG_CHECKING([whether to enable forcing of uid/gid per volume])
AC_ARG_ENABLE(force-uidgid,
	[  --enable-force-uidgid   allow forcing of uid/gid per volume (BROKEN) ],[
	if test "$enableval" = "yes"; then
		AC_DEFINE(FORCE_UIDGID, 1, [Define if you want forcing of uid/gid per volume])
		AC_MSG_RESULT([enabling forcing of uid/gid per volume])
		AC_MSG_RESULT([yes])
		netatalk_cv_force_uidgid=yes
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

dnl Check for CNID database backends
bdb_required=no
AC_NETATALK_CNID([bdb_required=yes],[bdb_required=no])

dnl Check for quota support
AC_CHECK_QUOTA

dnl Check for optional server location protocol support (used by MacOS X)
NETATALK_SRVLOC

dnl Check for optional Zeroconf support
NETATALK_ZEROCONF

dnl Check for PAM libs
netatalk_cv_use_pam=no
AC_PATH_PAM([
	use_pam_so=yes
	compile_pam=yes
	netatalk_cv_use_pam=yes
	AC_DEFINE(USE_PAM, 1, [Define to enable PAM support])
])

netatalk_cv_use_shadowpw=no
AC_ARG_WITH(shadow,
	[  --with-shadow           enable shadow password support [[auto]]],
	[netatalk_cv_use_shadowpw="$withval"],
	[netatalk_cv_use_shadowpw=auto]
)

if test "x$netatalk_cv_use_shadowpw" != "xno"; then
    AC_CHECK_HEADER([shadow.h])
    if test x"$ac_cv_header_shadow_h" = x"yes"; then
	netatalk_cv_use_shadowpw=yes
	AC_DEFINE(SHADOWPW, 1, [Define if shadow passwords should be used])
    else 
      if test "x$shadowpw" = "xyes"; then
        AC_MSG_ERROR([shadow support not available])
      else
       	netatalk_cv_use_shadowpw=no
      fi
    fi 
fi

AC_MSG_CHECKING([whether shadow support should be enabled])
if test "x$netatalk_cv_use_shadowpw" = "xyes"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
	
	
	
netatalk_cv_use_shellcheck=yes
AC_MSG_CHECKING([whether checking for a valid shell should be enabled])
AC_ARG_ENABLE(shell-check,
	[  --disable-shell-check   disable checking for a valid shell],[
	if test "$enableval" = "no"; then 
		AC_DEFINE(DISABLE_SHELLCHECK, 1, [Define if shell check should be disabled])
		AC_MSG_RESULT([no])
		netatalk_cv_use_shellcheck=no
	else
		AC_MSG_RESULT([yes])
	fi
	],[
		AC_MSG_RESULT([yes])
	]
)

NETATALK_TCP_WRAPPERS

AC_MSG_CHECKING([whether system (fcntl) locking should be disabled])
AC_ARG_ENABLE(locking,
        [  --disable-locking       disable system locking],[
        	if test "$enableval" = "no"; then
                	AC_DEFINE(DISABLE_LOCKING, 1, [Define if system (fcntl) locking should be disabled])
	                AC_MSG_RESULT([yes])
		else
	                AC_MSG_RESULT([no])
        	fi
		
	],[
		AC_MSG_RESULT([no])
	]

)

AC_ARG_ENABLE(redhat,
	[  --enable-redhat         obsoleted ],[
	echo "ERROR: --enable-redhat is obsoleted. Use --enable-redhat-sysv or --enable-redhat-systemd."
	echo "exit 1"
	exit 1
	]
)

AC_ARG_ENABLE(redhat-sysv,
	[  --enable-redhat-sysv    use redhat-style sysv (upstart) configuration ],[
	if test "$enableval" = "yes"; then
		sysv_style=redhat-sysv
	fi
	AC_MSG_RESULT([enabling redhat-style sysv support])
	]
)

AC_ARG_ENABLE(redhat-systemd,
	[  --enable-redhat-systemd use redhat-style systemd (>=Fedora15) configuration ],[
	if test "$enableval" = "yes"; then
		sysv_style=redhat-systemd
	fi
	AC_MSG_RESULT([enabling redhat-style systemd support])
	]
)

AC_ARG_ENABLE(suse,
	[  --enable-suse           use suse-style sysv configuration ],[
	if test "$enableval" = "yes"; then
		sysv_style=suse
	fi
	AC_MSG_RESULT([enabling suse-style sysv support])
	]
)

AC_ARG_ENABLE(gentoo,
	[  --enable-gentoo         use gentoo-style sysv configuration ],[
	if test "$enableval" = "yes"; then
		sysv_style=gentoo
	fi
	AC_MSG_RESULT([enabling gentoo-style sysv support])
	]
)

AC_ARG_ENABLE(netbsd,
	[  --enable-netbsd         use NetBSD-style rc.d configuration ],
	if test "x$enableval" = "xyes"; then
		sysv_style=netbsd
	fi
	AC_MSG_RESULT([enabling NetBSD-style rc.d support])
)

AC_ARG_ENABLE(debian,
	[  --enable-debian         use debian-style sysv configuration ],[
	if test "$enableval" = "yes"; then
		sysv_style=debian
	fi
	AC_MSG_RESULT([enabling debian-style sysv support])
	]
)

dnl ----- timelord compilation (disabled by default)
AC_MSG_CHECKING([whether timelord should be compiled])
compile_timelord=no
AC_ARG_ENABLE(timelord,
	[  --enable-timelord       enable compilation of timelord server],
	[compile_timelord="$enableval"],
	[compile_timelord="no"]
)
AC_MSG_RESULT([$compile_timelord])

dnl ----- a2boot compilation (disabled by default)
AC_MSG_CHECKING([whether a2boot should be compiled])
compile_a2boot=no
AC_ARG_ENABLE(a2boot,
	[  --enable-a2boot         enable compilation of Apple2 boot server],
	[compile_a2boot="$enableval"],
	[compile_a2boot="no"]
)
AC_MSG_RESULT([$compile_a2boot])

AC_ARG_WITH(uams-path,
	[  --with-uams-path=PATH   path to UAMs [[PKGCONF/uams]]],[
		uams_path="$withval"
	],[
		uams_path="${PKGCONFDIR}/uams"
	]
)

NETATALK_AC_CUPS

dnl check if we can use attribute unused (gcc only) from ethereal
AC_MSG_CHECKING(to see if we can add '__attribute__((unused))' to CFLAGS)
if test x$GCC != x ; then
  CFLAGS="-D_U_=\"__attribute__((unused))\" $CFLAGS"
  AC_MSG_RESULT(yes)
else
  CFLAGS="-D_U_=\"\" $CFLAGS"
  AC_MSG_RESULT(no)
fi

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

dnl ***** UAMS_PATH
dnl AC_DEFINE_UNQUOTED(UAMS_PATH, "${uams_path}",
dnl 	[path to UAMs [default=PKGCONF/uams]])
UAMS_PATH="${uams_path}"
AC_SUBST(UAMS_PATH)

dnl --------------------------------------------------------------------------
dnl drop in includes for top level directory structures here...
dnl --------------------------------------------------------------------------

dnl Note: $(top_srcdir)/include should be added before all other includes
dnl       so that includes from that directory a preferred to includes from
dnl       /usr/include or similar places.
LIBS="$LIBS -L\$(top_srcdir)/libatalk"
CFLAGS="-I\$(top_srcdir)/include $CFLAGS -I\$(top_srcdir)/sys"

AC_DEFINE(OPEN_NOFOLLOW_ERRNO, ELOOP, errno returned by open with O_NOFOLLOW)

dnl --------------------------------------------------------------------------
dnl specific configuration comes in here:
dnl --------------------------------------------------------------------------

dnl ac_cv_target_os is now host_os, ac_cv_target_cpu is now host_cpu

dnl --------------------- determine operating system from "target"
case "$host_os" in
	*aix*)				this_os=aix ;;
	*kfreebsd*-gnu)			this_os=kfreebsd-gnu ;;
	*freebsd*) 			this_os=freebsd ;;
	*hpux11*)			this_os=hpux11 ;;
	*irix*)				this_os=irix ;;
	*linux*)   			this_os=linux ;;
	*osx*)				this_os=macosx ;;
	*darwin*)			this_os=macosx ;;
	*netbsd*) 			this_os=netbsd ;;
	*openbsd*) 			this_os=openbsd ;;
	*osf*) 				this_os=tru64 ;;
	*solaris*) 			this_os=solaris ;;
esac

case "$host_cpu" in
	i386|i486|i586|i686|k7)		this_cpu=x86 ;;
	alpha)						this_cpu=alpha ;;
	mips)						this_cpu=mips ;;
	powerpc|ppc)				this_cpu=ppc ;;
esac

dnl --------------------- GNU source
case "$this_os" in
	linux)	AC_DEFINE(_GNU_SOURCE, 1, [Whether to use GNU libc extensions])
        ;;
     kfreebsd-gnu) AC_DEFINE(_GNU_SOURCE, 1, [Whether to use GNU libc extensions])
        ;;
esac

dnl --------------------- operating system specific flags (port from sys/*)

dnl ----- AIX specific -----
if test x"$this_os" = "xaix"; then
	AC_MSG_RESULT([ * AIX specific configuration])
	AC_DEFINE(NO_STRUCT_TM_GMTOFF, 1, [Define if the gmtoff member of struct tm is not available])

	dnl This is probably a lie; AIX 4.3 supports a 64-bit long
	dnl compilation environment.  It's enough to get things defined
	dnl right in endian.h provided that long long is supported, though.
	AC_DEFINE(HAVE_32BIT_LONGS, 1, [Define if the data type long has 32 bit])
fi

dnl ----- FreeBSD specific -----
if test x"$this_os" = "xfreebsd"; then 
	AC_MSG_RESULT([ * FreeBSD specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(FREEBSD, 1, [Define if OS is FreeBSD])
	AC_DEFINE(SENDFILE_FLAVOR_BSD, 1, [Define if the sendfile() function uses BSD semantics])
    AC_DEFINE(OPEN_NOFOLLOW_ERRNO, EMLINK, errno returned by open with O_NOFOLLOW)
fi

dnl ----- GNU/kFreeBSD specific -----
if test x"$this_os" = "xkfreebsd-gnu"; then 
	AC_MSG_RESULT([ * GNU/kFreeBSD specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(FREEBSD, 1, [Define if OS is FreeBSD])
	AC_DEFINE(SENDFILE_FLAVOR_BSD, 1, [Define if the sendfile() function uses BSD semantics])
    AC_DEFINE(OPEN_NOFOLLOW_ERRNO, EMLINK, errno returned by open with O_NOFOLLOW)
fi

dnl ----- HP-UX 11 specific -----
if test x"$this_os" = "xhpux11"; then
	AC_MSG_RESULT([ * HP-UX 11 specific configuration])

	AC_DEFINE(_ISOC9X_SOURCE, 1, [Compatibility macro])
	AC_DEFINE(NO_STRUCT_TM_GMTOFF, 1, [Define if the gmtoff member of struct tm is not available])
fi

dnl ----- IRIX specific -----
if test x"$this_os" = "xirix"; then
	AC_MSG_RESULT([ * IRIX specific configuration])

	AC_DEFINE(NO_STRUCT_TM_GMTOFF, 1, [Define if the gmtoff member of struct tm is not available])
fi

dnl ----- Linux specific -----
if test x"$this_os" = "xlinux"; then 
	AC_MSG_RESULT([ * Linux specific configuration])
	
	dnl ----- kernel 2.6 changed struct at_addr to atalk_addr
	AC_MSG_CHECKING([for struct atalk_addr])
dnl	AC_COMPILE_IFELSE([
	AC_TRY_COMPILE([
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/atalk.h>

	struct atalk_addr foo;
],
[ ], [
		ac_have_atalk_addr=yes
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])

if test "x$ac_have_atalk_addr" = "xyes"; then
	AC_DEFINE(HAVE_ATALK_ADDR, 1, [set if struct at_addr is called atalk_addr])
fi

	dnl ----- check if we need the quotactl wrapper
#	AC_CHECK_HEADERS(sys/quota.h linux/quota.h)
#	AC_CHECK_FUNC(quotactl,,
#		AC_DEFINE(NEED_QUOTACTL_WRAPPER, 1, [Define if the quotactl wrapper is needed])
#		AC_MSG_RESULT([enabling quotactl wrapper])
#	)

        # For quotas on Linux XFS filesystems
        
        # For linux > 2.5.56
        AC_CHECK_HEADERS(linux/dqblk_xfs.h,,
		[AC_CHECK_HEADERS(linux/xqm.h linux/xfs_fs.h)
        	AC_CHECK_HEADERS(xfs/libxfs.h xfs/xqm.h xfs/xfs_fs.h)]
	)


	dnl ----- as far as I can tell, dbtob always does the wrong thing
	dnl ----- on every single version of linux I've ever played with.
	dnl ----- see etc/afpd/quota.c
	AC_DEFINE(HAVE_BROKEN_DBTOB, 1, [Define if dbtob is broken])

	netatalk_cv_linux_sendfile=yes
	AC_MSG_CHECKING([use sendfile syscall])
        AC_ARG_ENABLE(sendfile,
	    [  --disable-sendfile       disable linux sendfile syscall],[
	    	if test x"$enableval" = x"no"; then
	  		netatalk_cv_linux_sendfile=no
			AC_MSG_RESULT([no])
		else
			AC_MSG_RESULT([yes])

	    	fi
	    ],[
		AC_MSG_RESULT([yes])
	]

       )

	if test x"$netatalk_cv_linux_sendfile" = "xyes"; then 
	    AC_CACHE_CHECK([for linux sendfile support],netatalk_cv_HAVE_SENDFILE,[
	    AC_TRY_LINK([#include <sys/sendfile.h>],
[\
int tofd, fromfd;
off_t offset;
size_t total;
ssize_t nwritten = sendfile(tofd, fromfd, &offset, total);
],
netatalk_cv_HAVE_SENDFILE=yes,netatalk_cv_HAVE_SENDFILE=no)])

# Try and cope with broken Linux sendfile....
	    AC_CACHE_CHECK([for broken linux sendfile support],netatalk_cv_HAVE_BROKEN_LINUX_SENDFILE,[
	    AC_TRY_LINK([\
#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#undef _FILE_OFFSET_BITS
#endif
#include <sys/sendfile.h>],
[\
int tofd, fromfd;
off_t offset;
size_t total;
ssize_t nwritten = sendfile(tofd, fromfd, &offset, total);
],
netatalk_cv_HAVE_BROKEN_LINUX_SENDFILE=yes,netatalk_cv_HAVE_BROKEN_LINUX_SENDFILE=no,netatalk_cv_HAVE_BROKEN_SENDFILE=cross)])

	    if test x"$netatalk_cv_HAVE_SENDFILE" = x"yes"; then
    		AC_DEFINE(HAVE_SENDFILE,1,[Whether sendfile() is available])
		AC_DEFINE(SENDFILE_FLAVOR_LINUX,1,[Whether linux sendfile() API is available])
		AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() should be used])
	    elif test x"$netatalk_cv_HAVE_BROKEN_LINUX_SENDFILE" = x"yes"; then
		AC_DEFINE(SENDFILE_FLAVOR_LINUX,1,[Whether linux sendfile() API is available])
		AC_DEFINE(LINUX_BROKEN_SENDFILE_API,1,[Whether (linux) sendfile() is broken])
		AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile should be used])
	    else
	  	netatalk_cv_linux_sendfile=no
		AC_MSG_RESULT(no);
	    fi
	fi

	dnl ----- Linux/alpha specific -----
	if test x"$this_cpu" = "xalpha"; then 
		AC_MSG_RESULT([enabling gcc memcpy bug workaround])
		AC_DEFINE(HAVE_GCC_MEMCPY_BUG, 1, [Define if memcpy is buggy])
	fi
	need_dash_r=no


fi

dnl ----- Mac OSX specific -----
if test x"$this_os" = "xmacosx"; then 
	AC_MSG_RESULT([ * Mac OSX specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(HAVE_2ARG_DBTOB, 1, [Define if dbtob takes two arguments])
	dnl AC_DEFINE(NO_DLFCN_H)
	AC_DEFINE(NO_DDP, 1, [Define if DDP should be disabled])
	AC_DEFINE(NO_QUOTA_SUPPORT, 1, [Define if Quota support should be disabled])
	AC_DEFINE(MACOSX_SERVER, 1, [Define if compiling for MacOS X Server])
fi

dnl ----- NetBSD specific -----
if test x"$this_os" = "xnetbsd"; then 
	AC_MSG_RESULT([ * NetBSD specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(NETBSD, 1, [Define if OS is NetBSD])
    AC_DEFINE(OPEN_NOFOLLOW_ERRNO, EFTYPE, errno returned by open with O_NOFOLLOW)

	CFLAGS="-I\$(top_srcdir)/sys/netbsd $CFLAGS"
	need_dash_r=yes 

	dnl ----- NetBSD does not have crypt.h, uses unistd.h -----
	AC_DEFINE(UAM_DHX, 1, [Define if the DHX UAM modules should be compiled])
fi

dnl ----- OpenBSD specific -----
if test x"$this_os" = "xopenbsd"; then 
	AC_MSG_RESULT([ * OpenBSD specific configuration])
	dnl ----- OpenBSD does not have crypt.h, uses unistd.h -----
	AC_DEFINE(UAM_DHX, 1, [Define if the DHX UAM modules should be compiled])
fi

dnl ----- Solaris specific -----
if test x"$this_os" = "xsolaris"; then 
	AC_MSG_RESULT([ * Solaris specific configuration])
	AC_DEFINE(__svr4__, 1, [Solaris compatibility macro])
	AC_DEFINE(_ISOC9X_SOURCE, 1, [Compatibility macro])
	AC_DEFINE(NO_STRUCT_TM_GMTOFF, 1, [Define if the gmtoff member of struct tm is not available])
	AC_DEFINE(SOLARIS, 1, [Solaris compatibility macro])
	CFLAGS="-I\$(top_srcdir)/sys/generic $CFLAGS"
	need_dash_r=yes
	sysv_style=solaris

	solaris_module=no
	AC_MSG_CHECKING([if we can build Solaris kernel module])
	if test -x /usr/ccs/bin/ld && test x"$netatalk_cv_ddp_enabled" = x"yes" ; then
		solaris_module=yes
	fi
	AC_MSG_RESULT([$solaris_module])

	COMPILE_64BIT_KMODULE=no
	KCFLAGS=""
	KLDFLAGS=""
	COMPILE_KERNEL_GCC=no

	if test "$solaris_module" = "yes"; then
	   dnl Solaris kernel module stuff
           AC_MSG_CHECKING([if we have to build a 64bit kernel module])

	   # check for isainfo, if not found it has to be a 32 bit kernel (<=2.6)	
	   if test -x /usr/bin/isainfo; then
		# check for 64 bit platform
		if isainfo -kv | grep '^64-bit'; then
			COMPILE_64BIT_KMODULE=yes
		fi
	   fi

	   AC_MSG_RESULT([$COMPILE_64BIT_KMODULE])

	   if test "${GCC}" = yes; then
		COMPILE_KERNEL_GCC=yes
		if test "$COMPILE_64BIT_KMODULE" = yes; then
  	        
                        AC_MSG_CHECKING([if we can build a 64bit kernel module])
		        
                        case `$CC --version 2>/dev/null` in
			[[12]].* | 3.0.*)
				COMPILE_64BIT_KMODULE=no
				COMPILE_KERNEL_GCC=no	
				solaris_module=no;;
			*)
			       	# use for 64 bit
				KCFLAGS="-m64"
				#KLDFLAGS="-melf64_sparc"
				KLDFLAGS="-64";;
			esac	
			
			AC_MSG_RESULT([$COMPILE_64BIT_KMODULE])
			
		else
			KCFLAGS=""
			KLDFLAGS=""
		fi
		KCFLAGS="$KCFLAGS -D_KERNEL -Wall -Wstrict-prototypes"
           else
		if test "$COMPILE_64BIT_KMODULE" = yes; then
                # use Sun CC (for a 64-bit kernel, uncomment " -xarch=v9 -xregs=no%appl ")
 			KCFLAGS="-xarch=v9 -xregs=no%appl"
			KLDFLAGS="-64"
		else
 			KCFLAGS=""
			KLDFLAGS=""
		fi
		KCFLAGS="-D_KERNEL $KCFLAGS -mno-app-regs -munaligned-doubles -fpcc-struct-return"
	   fi

           AC_CACHE_CHECK([for timeout_id_t],netatalk_cv_HAVE_TIMEOUT_ID_T,[
           AC_TRY_LINK([\
#include <sys/stream.h>
#include <sys/ddi.h>],
[\
timeout_id_t dummy;
],
netatalk_cv_HAVE_TIMEOUT_ID_T=yes,netatalk_cv_HAVE_TIMEOUT_ID_T=no,netatalk_cv_HAVE_TIMEOUT_ID_T=cross)])

	   AC_DEFINE(HAVE_TIMEOUT_ID_T, test x"$netatalk_cv_HAVE_TIMEOUT_ID" = x"yes", [define for timeout_id_t])
	fi

	AC_SUBST(COMPILE_KERNEL_GCC)
	AC_SUBST(COMPILE_64BIT_KMODULE)
	AC_SUBST(KCFLAGS)
	AC_SUBST(KLDFLAGS)
fi

dnl ----- Tru64 specific -----
if test x"$this_os" = "xtru64"; then 
	AC_MSG_RESULT([ * Tru64 specific configuration])
	AC_DEFINE(NO_DDP, 1, [Define if DDP should be disabled])
	AC_DEFINE(HAVE_64BIT_LONGS, 1, [Define if the data type long has 64 bit])
	dnl AC_DEFINE(USE_MOUNT_H)
	AC_DEFINE(USE_OLD_RQUOTA, 1, [Define to use old rquota])
	dnl AC_DEFINE(USE_UFS_QUOTA_H)
	AC_DEFINE(TRU64, 1, [Define on Tru64 platforms])
	AC_DEFINE(_OSF_SOURCE, 1, [Define if the *passwd UAMs should be used])
	AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, [Define for Berkeley DB 4])
	AC_CHECK_LIB(security,set_auth_parameters)
	CFLAGS="-I\$(top_srcdir)/sys/tru64 $CFLAGS"
	need_dash_r=no
	sysv_style=tru64
fi

dnl -- check for libgcrypt, if found enables DHX UAM
AM_PATH_LIBGCRYPT([1:1.2.3],[neta_cv_compile_dhx2=yes
			neta_cv_have_libgcrypt=yes
			AC_MSG_NOTICE([Enabling DHX2 UAM])
			AC_DEFINE(HAVE_LIBGCRYPT, 1, [Define if the DHX2 modules should be built with libgcrypt])
			AC_DEFINE(UAM_DHX2, 1, [Define if the DHX2 UAM modules should be compiled])
			])

dnl -- look for openssl, if found enables DHX UAM and Randnum UAM
AC_PATH_SSL

dnl Check for Berkeley DB library
if test "x$bdb_required" = "xyes"; then
	AC_PATH_BDB(, [
		AC_MSG_RESULT([])
		AC_MSG_RESULT([Make sure you have the required Berkeley DB libraries AND headers installed.])
		AC_MSG_RESULT([You can download the latest version from http://www.sleepycat.com.])
		AC_MSG_RESULT([If you have installed BDB in a non standard location use the])
		AC_MSG_RESULT([--with-bdb=/path/to/bdb configure option and make sure])
		AC_MSG_RESULT([your linker is configured to check for libraries there.])
		AC_MSG_ERROR([Berkeley DB library required but not found!])
	])
fi

dnl -- check for crypt
AC_CRYPT

dnl --------------------- check for building PGP UAM module

AC_MSG_CHECKING([whether the PGP UAM should be build])
AC_ARG_ENABLE(pgp-uam,
	[  --enable-pgp-uam        enable build of PGP UAM module],[
	if test "$enableval" = "yes"; then 
		if test "x$neta_cv_have_openssl" = "xyes"; then 
			AC_DEFINE(UAM_PGP, 1, [Define if the PGP UAM module should be compiled])
			compile_pgp=yes
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
		fi
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

dnl --------------------- check for building Kerberos v4 UAM module

AC_MSG_CHECKING([whether the Kerberos IV UAM should be build])
AC_ARG_ENABLE(krb4-uam,
	[  --enable-krb4-uam       enable build of Kerberos v4 UAM module],[
	if test "$enableval" = "yes"; then
		AC_DEFINE(UAM_KRB4, 1, [Define if the Kerberos 4 UAM module should be compiled])
		compile_kerberos=yes
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	],[
		AC_MSG_RESULT([no])
	]
)

dnl --------------------- check for building Kerberos V UAM module

netatalk_cv_build_krb5_uam=no
AC_ARG_ENABLE(krbV-uam,
	[  --enable-krbV-uam       enable build of Kerberos V UAM module],
	[
		if test x"$enableval" = x"yes"; then
			NETATALK_GSSAPI_CHECK([
				netatalk_cv_build_krb5_uam=yes
			],[
				AC_MSG_ERROR([need GSSAPI to build Kerberos V UAM])
			])
		fi
	]
	
)

AC_MSG_CHECKING([whether Kerberos V UAM should be build])
if test x"$netatalk_cv_build_krb5_uam" = x"yes"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL(USE_GSSAPI, test x"$netatalk_cv_build_krb5_uam" = x"yes")

dnl --------------------- overwrite the config files . . . or not.

AC_MSG_CHECKING([whether configuration files should be overwritten])
AC_ARG_ENABLE(overwrite,
	[  --enable-overwrite      overwrite configuration files during installation],
	[OVERWRITE_CONFIG="${enable_overwrite}"],
	[OVERWRITE_CONFIG="no"]
)
AC_MSG_RESULT([$OVERWRITE_CONFIG])

dnl --------------------- check for LDAP support, for client-side ACL visibility
AC_MSG_CHECKING(for LDAP (necessary for client-side ACL visibility))
AC_ARG_WITH(ldap,
    [AS_HELP_STRING([--with-ldap],
        [LDAP support (default=auto)])],
    [ case "$withval" in
      yes|no)
          with_ldap="$withval"
		  ;;
      *)
          with_ldap=auto
          ;;
      esac ])
AC_MSG_RESULT($with_ldap)

if test x"$with_ldap" != x"no" ; then
   	AC_CHECK_HEADER([ldap.h], with_ldap=yes,
        [ if test x"$with_ldap" = x"yes" ; then
            AC_MSG_ERROR([Missing LDAP headers])
        fi
		with_ldap=no
        ])
	AC_CHECK_LIB(ldap, ldap_init, with_ldap=yes,
        [ if test x"$with_ldap" = x"yes" ; then
            AC_MSG_ERROR([Missing LDAP library])
        fi
		with_ldap=no
        ])
fi

if test x"$with_ldap" = x"yes"; then
	AC_DEFINE(HAVE_LDAP,1,[Whether LDAP is available])
fi

dnl --------------------- check for ACL support
AC_MSG_CHECKING(whether to support ACLs)
AC_ARG_WITH(acls,
    [AS_HELP_STRING([--with-acls],
        [Include ACL support (default=auto)])],
    [ case "$withval" in
      yes|no)
          with_acl_support="$withval"
		  ;;
      *)
          with_acl_support=auto
          ;;
      esac ],
    [with_acl_support=auto])
AC_MSG_RESULT($with_acl_support)

if test x"$with_acl_support" = x"no"; then
	AC_MSG_RESULT(Disabling ACL support)
	AC_DEFINE(HAVE_NO_ACLS,1,[Whether no ACLs support should be built in])
else
    with_acl_support=yes
fi

if test x"$with_acl_support" = x"yes" ; then
	AC_MSG_NOTICE(checking whether ACL support is available:)
	case "$host_os" in
	*sysv5*)
		AC_MSG_NOTICE(Using UnixWare ACLs)
		AC_DEFINE(HAVE_UNIXWARE_ACLS,1,[Whether UnixWare ACLs are available])
		;;
	*solaris*)
		AC_MSG_NOTICE(Using solaris ACLs)
		AC_DEFINE(HAVE_SOLARIS_ACLS,1,[Whether solaris ACLs are available])
		ACL_LIBS="$ACL_LIBS -lsec"
		;;
	*hpux*)
		AC_MSG_NOTICE(Using HPUX ACLs)
		AC_DEFINE(HAVE_HPUX_ACLS,1,[Whether HPUX ACLs are available])
		;;
	*irix*)
		AC_MSG_NOTICE(Using IRIX ACLs)
		AC_DEFINE(HAVE_IRIX_ACLS,1,[Whether IRIX ACLs are available])
		;;
	*aix*)
		AC_MSG_NOTICE(Using AIX ACLs)
		AC_DEFINE(HAVE_AIX_ACLS,1,[Whether AIX ACLs are available])
		;;
	*osf*)
		AC_MSG_NOTICE(Using Tru64 ACLs)
		AC_DEFINE(HAVE_TRU64_ACLS,1,[Whether Tru64 ACLs are available])
		ACL_LIBS="$ACL_LIBS -lpacl"
		;;
	*darwin*)
		AC_MSG_NOTICE(ACLs on Darwin currently not supported)
		AC_DEFINE(HAVE_NO_ACLS,1,[Whether no ACLs support is available])
		;;
	*)
		AC_CHECK_LIB(acl,acl_get_file,[ACL_LIBS="$ACL_LIBS -lacl"])
		case "$host_os" in
		*linux*)
			AC_CHECK_LIB(attr,getxattr,[ACL_LIBS="$ACL_LIBS -lattr"])
			;;
		esac
		AC_CACHE_CHECK([for POSIX ACL support],netatalk_cv_HAVE_POSIX_ACLS,[
			acl_LIBS=$LIBS
			LIBS="$LIBS $ACL_LIBS"
			AC_TRY_LINK([
				#include <sys/types.h>
				#include <sys/acl.h>
			],[
				acl_t acl;
				int entry_id;
				acl_entry_t *entry_p;
				return acl_get_entry(acl, entry_id, entry_p);
			],
			[netatalk_cv_HAVE_POSIX_ACLS=yes],
			[netatalk_cv_HAVE_POSIX_ACLS=no
                with_acl_support=no])
			LIBS=$acl_LIBS
		])
		if test x"$netatalk_cv_HAVE_POSIX_ACLS" = x"yes"; then
			AC_MSG_NOTICE(Using POSIX ACLs)
			AC_DEFINE(HAVE_POSIX_ACLS,1,[Whether POSIX ACLs are available])
			AC_CACHE_CHECK([for acl_get_perm_np],netatalk_cv_HAVE_ACL_GET_PERM_NP,[
				acl_LIBS=$LIBS
				LIBS="$LIBS $ACL_LIBS"
				AC_TRY_LINK([
					#include <sys/types.h>
					#include <sys/acl.h>
				],[
					acl_permset_t permset_d;
					acl_perm_t perm;
					return acl_get_perm_np(permset_d, perm);
				],
				[netatalk_cv_HAVE_ACL_GET_PERM_NP=yes],
				[netatalk_cv_HAVE_ACL_GET_PERM_NP=no])
				LIBS=$acl_LIBS
			])
			if test x"$netatalk_cv_HAVE_ACL_GET_PERM_NP" = x"yes"; then
				AC_DEFINE(HAVE_ACL_GET_PERM_NP,1,[Whether acl_get_perm_np() is available])
			fi

			AC_CACHE_CHECK([for acl_from_mode], netatalk_cv_HAVE_ACL_FROM_MODE,[
				acl_LIBS=$LIBS
				LIBS="$LIBS $ACL_LIBS"
                AC_CHECK_FUNCS(acl_from_mode,
				[netatalk_cv_HAVE_ACL_FROM_MODE=yes],
				[netatalk_cv_HAVE_ACL_FROM_MODE=no])
				LIBS=$acl_LIBS
			])
			if test x"netatalk_cv_HAVE_ACL_FROM_MODE" = x"yes"; then
				AC_DEFINE(HAVE_ACL_FROM_MODE,1,[Whether acl_from_mode() is available])
			fi


		else
			AC_MSG_NOTICE(ACL support is not avaliable)
			AC_DEFINE(HAVE_NO_ACLS,1,[Whether no ACLs support is available])
		fi
		;;
    esac
fi

if test x"$with_acl_support" = x"yes" ; then
   AC_CHECK_HEADERS([acl/libacl.h])
    AC_DEFINE(HAVE_ACLS,1,[Whether ACLs support is available])
    AC_SUBST(ACL_LIBS)
fi

dnl --------------------- check for Extended Attributes support
neta_cv_eas="ad"
neta_cv_eas_sys_found=no
neta_cv_eas_sys_not_found=no

AC_CHECK_HEADERS(sys/attributes.h attr/xattr.h sys/xattr.h sys/extattr.h sys/uio.h sys/ea.h)

case "$this_os" in

  *osf*)
	AC_SEARCH_LIBS(getproplist, [proplist])
	AC_CHECK_FUNCS([getproplist fgetproplist setproplist fsetproplist],
                   [neta_cv_eas_sys_found=yes],
                   [neta_cv_eas_sys_not_found=yes])
	AC_CHECK_FUNCS([delproplist fdelproplist add_proplist_entry get_proplist_entry],,
                   [neta_cv_eas_sys_not_found=yes])
	AC_CHECK_FUNCS([sizeof_proplist_entry],,
                   [neta_cv_eas_sys_not_found=yes])
  ;;

  *solaris*)
	AC_CHECK_FUNCS([attropen],
                   [neta_cv_eas_sys_found=yes],
                   [neta_cv_eas_sys_not_found=yes])
  ;;

  'freebsd')
    AC_CHECK_FUNCS([extattr_delete_fd extattr_delete_file extattr_delete_link],
                   [neta_cv_eas_sys_found=yes],
                   [neta_cv_eas_sys_not_found=yes])
    AC_CHECK_FUNCS([extattr_get_fd extattr_get_file extattr_get_link],,
                   [neta_cv_eas_sys_not_found=yes])
    AC_CHECK_FUNCS([extattr_list_fd extattr_list_file extattr_list_link],,
                   [neta_cv_eas_sys_not_found=yes])
    AC_CHECK_FUNCS([extattr_set_fd extattr_set_file extattr_set_link],,
                   [neta_cv_eas_sys_not_found=yes])
  ;;

  *freebsd4* | *dragonfly* )
    AC_DEFINE(BROKEN_EXTATTR, 1, [Does extattr API work])
  ;;

  *)
	AC_SEARCH_LIBS(getxattr, [attr])

    if test "x$neta_cv_eas_sys_found" != "xyes" ; then
       AC_CHECK_FUNCS([getxattr lgetxattr fgetxattr listxattr llistxattr],
                      [neta_cv_eas_sys_found=yes],
                      [neta_cv_eas_sys_not_found=yes])
	   AC_CHECK_FUNCS([flistxattr removexattr lremovexattr fremovexattr],,
                      [neta_cv_eas_sys_not_found=yes])
	   AC_CHECK_FUNCS([setxattr lsetxattr fsetxattr],,
                      [neta_cv_eas_sys_not_found=yes])
    fi

    if test "x$neta_cv_eas_sys_found" != "xyes" ; then
	   AC_CHECK_FUNCS([getea fgetea lgetea listea flistea llistea],
                      [neta_cv_eas_sys_found=yes],
                      [neta_cv_eas_sys_not_found=yes])
	   AC_CHECK_FUNCS([removeea fremoveea lremoveea setea fsetea lsetea],,
                      [neta_cv_eas_sys_not_found=yes])
    fi

    if test "x$neta_cv_eas_sys_found" != "xyes" ; then
	   AC_CHECK_FUNCS([attr_get attr_list attr_set attr_remove],,
                      [neta_cv_eas_sys_not_found=yes])
       AC_CHECK_FUNCS([attr_getf attr_listf attr_setf attr_removef],,
                      [neta_cv_eas_sys_not_found=yes])
    fi
  ;;
esac

# Do xattr functions take additional options like on Darwin?
if test x"$ac_cv_func_getxattr" = x"yes" ; then
	AC_CACHE_CHECK([whether xattr interface takes additional options], smb_attr_cv_xattr_add_opt, [
		old_LIBS=$LIBS
		LIBS="$LIBS $ACL_LIBS"
		AC_TRY_COMPILE([
			#include <sys/types.h>
			#if HAVE_ATTR_XATTR_H
			#include <attr/xattr.h>
			#elif HAVE_SYS_XATTR_H
			#include <sys/xattr.h>
			#endif
		],[
			getxattr(0, 0, 0, 0, 0, 0);
		],
	        [smb_attr_cv_xattr_add_opt=yes],
		[smb_attr_cv_xattr_add_opt=no;LIBS=$old_LIBS])
	])
	if test x"$smb_attr_cv_xattr_add_opt" = x"yes"; then
		AC_DEFINE(XATTR_ADD_OPT, 1, [xattr functions have additional options])
	fi
fi

if test "x$neta_cv_eas_sys_found" = "xyes" ; then
   if test "x$neta_cv_eas_sys_not_found" != "xyes" ; then
      neta_cv_eas="$neta_cv_eas | sys"
   fi
fi
AC_DEFINE_UNQUOTED(EA_MODULES,["$neta_cv_eas"],[Available Extended Attributes modules])

dnl --------------------- Check if realpath() takes NULL
AC_CACHE_CHECK([if the realpath function allows a NULL argument],
    neta_cv_REALPATH_TAKES_NULL, [
        AC_TRY_RUN([
            #include <stdio.h>
            #include <limits.h>
            #include <signal.h>

            void exit_on_core(int ignored) {
                 exit(1);
            }

            main() {
                char *newpath;
                signal(SIGSEGV, exit_on_core);
                newpath = realpath("/tmp", NULL);
                exit((newpath != NULL) ? 0 : 1);
            }],
            neta_cv_REALPATH_TAKES_NULL=yes,
            neta_cv_REALPATH_TAKES_NULL=no,
            neta_cv_REALPATH_TAKES_NULL=cross
        )
    ]
)

if test x"$neta_cv_REALPATH_TAKES_NULL" = x"yes"; then
    AC_DEFINE(REALPATH_TAKES_NULL,1,[Whether the realpath function allows NULL])
fi

dnl --------------------- Netatalk Webmin
NETATALK_WEBMIN

dnl --------------------- last minute substitutions

AC_SUBST(LIBS)
AC_SUBST(CFLAGS)
AC_SUBST(OVERWRITE_CONFIG)

AM_CONDITIONAL(SOLARIS_MODULE, test x$solaris_module = xyes)
AM_CONDITIONAL(COMPILE_TIMELORD, test x$compile_timelord = xyes)
AM_CONDITIONAL(COMPILE_A2BOOT, test x$compile_a2boot = xyes)
AM_CONDITIONAL(HAVE_LIBGCRYPT, test x$neta_cv_have_libgcrypt = xyes)
AM_CONDITIONAL(HAVE_OPENSSL, test x$neta_cv_have_openssl = xyes)
AM_CONDITIONAL(HAVE_ACLS, test x"$with_acl_support" = x"yes")
AM_CONDITIONAL(HAVE_LDAP, test x"$with_ldap" = x"yes")
AM_CONDITIONAL(USE_DHX, test x$neta_cv_compile_dhx = xyes)
AM_CONDITIONAL(USE_DHX2, test x$neta_cv_compile_dhx2 = xyes)
AM_CONDITIONAL(USE_RANDNUM, test x$neta_cv_have_openssl = xyes)
AM_CONDITIONAL(USE_KERBEROS, test x$compile_kerberos = xyes)
AM_CONDITIONAL(USE_PAM_SO, test x$use_pam_so = xyes)
AM_CONDITIONAL(USE_PAM, test x$netatalk_cv_install_pam = xyes)
AM_CONDITIONAL(BUILD_PAM, test x$compile_pam = xyes)
AM_CONDITIONAL(USE_PGP, test x$compile_pgp = xyes)
AM_CONDITIONAL(DEFAULT_HOOK, test x$neta_cv_have_libgcrypt != xyes && test x$neta_cv_have_openssl != xyes)
AM_CONDITIONAL(USE_NETBSD, test x$sysv_style = xnetbsd)
AM_CONDITIONAL(USE_REDHAT_SYSV, test x$sysv_style = xredhat-sysv)
AM_CONDITIONAL(USE_REDHAT_SYSTEMD, test x$sysv_style = xredhat-systemd)
AM_CONDITIONAL(USE_SUSE, test x$sysv_style = xsuse)
AM_CONDITIONAL(USE_SHADOWPW, test x$shadowpw = xyes)
AM_CONDITIONAL(USE_TRU64, test x$sysv_style = xtru64)
AM_CONDITIONAL(USE_SOLARIS, test x$sysv_style = xsolaris)
AM_CONDITIONAL(USE_GENTOO, test x$sysv_style = xgentoo)
AM_CONDITIONAL(USE_DEBIAN, test x$sysv_style = xdebian)
AM_CONDITIONAL(USE_UNDEF, test x$sysv_style = x)
AM_CONDITIONAL(USE_BDB, test x$bdb_required = xyes)
AM_CONDITIONAL(USE_APPLETALK, test x$netatalk_cv_ddp_enabled = xyes)
AM_CONDITIONAL(HAVE_ATFUNCS, test x"$ac_neta_haveatfuncs" = x"yes")

dnl --------------------- generate files

AC_OUTPUT([Makefile
	bin/Makefile
	bin/ad/Makefile
	bin/adv1tov2/Makefile
	bin/aecho/Makefile
	bin/afppasswd/Makefile
	bin/cnid/Makefile
	bin/cnid/cnid2_create
	bin/getzones/Makefile
	bin/megatron/Makefile
	bin/misc/Makefile
	bin/nbp/Makefile
	bin/pap/Makefile
	bin/psorder/Makefile
	bin/uniconv/Makefile
	config/Makefile
	config/pam/Makefile
	contrib/Makefile
	contrib/macusers/Makefile
	contrib/macusers/macusers
	contrib/misc/Makefile
	contrib/printing/Makefile
	contrib/shell_utils/Makefile
	contrib/shell_utils/apple_dump
	contrib/shell_utils/asip-status.pl
	contrib/timelord/Makefile
	contrib/a2boot/Makefile
	distrib/Makefile
	distrib/config/Makefile
	distrib/config/netatalk-config
	distrib/initscripts/Makefile
	distrib/m4/Makefile
	doc/Makefile
	etc/Makefile
	etc/afpd/Makefile
	etc/atalkd/Makefile
	etc/cnid_dbd/Makefile
	etc/uams/Makefile
	etc/uams/uams_krb4/Makefile
	etc/papd/Makefile
	etc/psf/Makefile
	include/Makefile
	include/atalk/Makefile
	libatalk/Makefile
	libatalk/acl/Makefile
	libatalk/adouble/Makefile
	libatalk/asp/Makefile
	libatalk/atp/Makefile
	libatalk/bstring/Makefile
	libatalk/cnid/Makefile
	libatalk/cnid/cdb/Makefile
	libatalk/cnid/last/Makefile
	libatalk/cnid/dbd/Makefile
	libatalk/cnid/tdb/Makefile
	libatalk/compat/Makefile
	libatalk/dsi/Makefile
	libatalk/nbp/Makefile
	libatalk/netddp/Makefile
	libatalk/util/Makefile
	libatalk/tdb/Makefile
	libatalk/unicode/Makefile
	libatalk/unicode/charsets/Makefile
	libatalk/vfs/Makefile
	macros/Makefile
	man/Makefile
	man/man1/Makefile
	man/man3/Makefile
	man/man4/Makefile
	man/man5/Makefile
	man/man8/Makefile
	sys/Makefile
	sys/generic/Makefile
	sys/generic/sys/Makefile
	sys/netatalk/Makefile
	sys/netbsd/Makefile
	sys/netbsd/netatalk/Makefile
	sys/solaris/Makefile
	sys/sunos/Makefile
	sys/ultrix/Makefile
	test/Makefile
	test/afpd/Makefile
	],
	[chmod a+x distrib/config/netatalk-config contrib/shell_utils/apple_*]
)

AC_NETATALK_LIBS_SUMMARY
AC_NETATALK_CONFIG_SUMMARY
