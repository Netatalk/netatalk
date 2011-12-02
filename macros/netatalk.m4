dnl Kitchen sink for configuration macros

dnl Check for optional admin group support
AC_DEFUN([AC_NETATALK_ADMIN_GROUP], [
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
	])
])

dnl Check for optional cracklib support
AC_DEFUN([AC_NETATALK_CRACKLIB], [
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
])

dnl Check whether to enable debug code
AC_DEFUN([AC_NETATALK_DEBUG], [
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
])

dnl Check whethe to disable tickle SIGALARM stuff, which eases debugging
AC_DEFUN([AC_NETATALK_DEBUGGING], [
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

])

dnl Check for optional shadow password support
AC_DEFUN([AC_NETATALK_SHADOW], [
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
])

dnl Check for optional valid-shell-check support
AC_DEFUN([AC_NETATALK_SHELL_CHECK], [
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
])

dnl Check for optional sysv initscript install
AC_DEFUN([AC_NETATALK_SYSV_STYLE], [
    AC_ARG_WITH(sysv-style,
                [  --with-sysv-style       use OS specific sysv config [[redhat-sysv|redhat-systemd|suse-sysv|suse-systemd|gentoo|netbsd|debian|systemd]]],
                sysv_style="$withval", sysv_style=none
    )
    case "$sysv_style" in 
    "redhat")
	    AC_MSG_ERROR([--enable-redhat is obsoleted. Use --enable-redhat-sysv or --enable-redhat-systemd.])
        ;;
    "redhat-sysv")
	    AC_MSG_RESULT([enabling redhat-style sysv (upstart) configuration])
	    ;;
    "redhat-systemd")
	    AC_MSG_RESULT([enabling redhat-style systemd support])
	    ;;
    "suse")
	    AC_MSG_RESULT([--enable-suse is obsoleted. Use --enable-suse-sysv or --enable-suse-systemd])
        ;;
    "suse-sysv")
	    AC_MSG_RESULT([enabling suse-style sysv configuration])
	    ;;
    "suse-systemd")
	    AC_MSG_RESULT([enabling suse-style systemd support (>=openSUSE12.1)])
	    ;;
    "gentoo")
	    AC_MSG_RESULT([enabling gentoo-style sysv support])
        ;;
    "netbsd")
	    AC_MSG_RESULT([enabling netbsd-style sysv support])
        ;;
    "debian")
	    AC_MSG_RESULT([enabling debian-style sysv support])
        ;;
    "systemd")
	    AC_MSG_RESULT([use general systemd configuration])
        ;;
    *)
	    AC_MSG_RESULT([disabling sysv support])
        ;;
    esac
])

dnl OS specific configuration
AC_DEFUN([AC_NETATALK_OS_SPECIFIC], [
case "$host_os" in
	*aix*)				this_os=aix ;;
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

dnl ----- FreeBSD specific -----
if test x"$this_os" = "xfreebsd"; then 
	AC_MSG_RESULT([ * FreeBSD specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(FREEBSD, 1, [Define if OS is FreeBSD])
    AC_DEFINE(OPEN_NOFOLLOW_ERRNO, EMLINK, errno returned by open with O_NOFOLLOW)
fi

dnl ----- GNU/kFreeBSD specific -----
if test x"$this_os" = "xkfreebsd-gnu"; then 
	AC_MSG_RESULT([ * GNU/kFreeBSD specific configuration])
	AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
	AC_DEFINE(FREEBSD, 1, [Define if OS is FreeBSD])
    AC_DEFINE(OPEN_NOFOLLOW_ERRNO, EMLINK, errno returned by open with O_NOFOLLOW)
fi

dnl ----- Linux specific -----
if test x"$this_os" = "xlinux"; then 
	AC_MSG_RESULT([ * Linux specific configuration])
	
	dnl ----- check if we need the quotactl wrapper
    AC_CHECK_HEADERS(linux/dqblk_xfs.h,,
		[AC_CHECK_HEADERS(linux/xqm.h linux/xfs_fs.h)
        	AC_CHECK_HEADERS(xfs/libxfs.h xfs/xqm.h xfs/xfs_fs.h)]
	)


	dnl ----- as far as I can tell, dbtob always does the wrong thing
	dnl ----- on every single version of linux I've ever played with.
	dnl ----- see etc/afpd/quota.c
	AC_DEFINE(HAVE_BROKEN_DBTOB, 1, [Define if dbtob is broken])

	need_dash_r=no
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
    AC_DEFINE(BSD4_4, 1, [BSD compatiblity macro])
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

])

dnl Check for building PGP UAM module
AC_DEFUN([AC_NETATALK_PGP_UAM], [
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
])

dnl Check for building Kerberos V UAM module
AC_DEFUN([AC_NETATALK_KRB5_UAM], [
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
])

dnl Check for overwrite the config files or not
AC_DEFUN([AC_NETATALK_OVERWRITE_CONFIG], [
AC_MSG_CHECKING([whether configuration files should be overwritten])
AC_ARG_ENABLE(overwrite,
	[  --enable-overwrite      overwrite configuration files during installation],
	[OVERWRITE_CONFIG="${enable_overwrite}"],
	[OVERWRITE_CONFIG="no"]
)
AC_MSG_RESULT([$OVERWRITE_CONFIG])
AC_SUBST(OVERWRITE_CONFIG)
])

dnl Check for LDAP support, for client-side ACL visibility
AC_DEFUN([AC_NETATALK_LDAP], [
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
])

dnl Check for ACL support
AC_DEFUN([AC_NETATALK_ACL], [
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
])

dnl Check for Extended Attributes support
AC_DEFUN([AC_NETATALK_EXTENDED_ATTRIBUTES], [
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
])

dnl Check for libsmbsharemodes from Samba for Samba/Netatalk access/deny/share modes interop
dnl Defines "neta_cv_have_smbshmd" to "yes" or "no"
dnl AC_SUBST's "SMB_SHAREMODES_CFLAGS" and "SMB_SHAREMODES_LDFLAGS"
dnl AM_CONDITIONAL's "USE_SMB_SHAREMODES"
AC_DEFUN([AC_NETATALK_SMB_SHAREMODES], [
    neta_cv_have_smbshmd=no
    AC_ARG_WITH(smbsharemodes-lib,
                [  --with-smbsharemodes-lib=PATH        PATH to libsmbsharemodes lib from Samba],
                [SMB_SHAREMODES_LDFLAGS="-L$withval -lsmbsharemodes"]
    )
    AC_ARG_WITH(smbsharemodes-include,
                [  --with-smbsharemodes-include=PATH    PATH to libsmbsharemodes header from Samba],
                [SMB_SHAREMODES_CFLAGS="-I$withval"]
    )
    AC_ARG_WITH(smbsharemodes,
                [AS_HELP_STRING([--with-smbsharemodes],[Samba interop (default is yes)])],
                [use_smbsharemodes=$withval],
                [use_smbsharemodes=yes]
    )

    if test x"$use_smbsharemodes" = x"yes" ; then
        AC_MSG_CHECKING([whether to enable Samba/Netatalk access/deny/share-modes interop])

        saved_CFLAGS="$CFLAGS"
        saved_LDFLAGS="$LDFLAGS"
        CFLAGS="$SMB_SHAREMODES_CFLAGS $CFLAGS"
        LDFLAGS="$SMB_SHAREMODES_LDFLAGS $LDFLAGS"

        AC_LINK_IFELSE(
            [#include <unistd.h>
             #include <stdio.h>
             #include <sys/time.h>
             #include <time.h>
             #include <stdint.h>
             /* From messages.h */
             struct server_id {
                 pid_t pid;
             };
             #include "smb_share_modes.h"
             int main(void) { (void)smb_share_mode_db_open(""); return 0;}],
            [neta_cv_have_smbshmd=yes]
        )

        AC_MSG_RESULT($neta_cv_have_smbshmd)
        AC_SUBST(SMB_SHAREMODES_CFLAGS, [$SMB_SHAREMODES_CFLAGS])
        AC_SUBST(SMB_SHAREMODES_LDFLAGS, [$SMB_SHAREMODES_LDFLAGS])
        CFLAGS="$saved_CFLAGS"
        LDFLAGS="$saved_LDFLAGS"
    fi

    AM_CONDITIONAL(USE_SMB_SHAREMODES, test x"$neta_cv_have_smbshmd" = x"yes")
])

dnl ------ Check for sendfile() --------
AC_DEFUN([AC_NETATALK_SENDFILE], [
netatalk_cv_search_sendfile=yes
AC_ARG_ENABLE(sendfile,
    [  --disable-sendfile       disable sendfile syscall],
    [if test x"$enableval" = x"no"; then
            netatalk_cv_search_sendfile=no
        fi]
)

if test x"$netatalk_cv_search_sendfile" = x"yes"; then
   case "$host_os" in
   *linux*)
        AC_DEFINE(SENDFILE_FLAVOR_LINUX,1,[Whether linux sendfile() API is available])
        AC_CHECK_FUNC([sendfile], [netatalk_cv_HAVE_SENDFILE=yes])
        ;;

    *solaris*)
        AC_DEFINE(SENDFILE_FLAVOR_SOLARIS, 1, [Solaris sendfile()])
        AC_SEARCH_LIBS(sendfile, sendfile)
        AC_CHECK_FUNC([sendfile], [netatalk_cv_HAVE_SENDFILE=yes])
        ;;

    *freebsd*)
        AC_DEFINE(SENDFILE_FLAVOR_BSD, 1, [Define if the sendfile() function uses BSD semantics])
        AC_CHECK_FUNC([sendfile], [netatalk_cv_HAVE_SENDFILE=yes])
        ;;

    *)
        ;;

    esac

    if test x"$netatalk_cv_HAVE_SENDFILE" = x"yes"; then
        AC_DEFINE(WITH_SENDFILE,1,[Whether sendfile() should be used])
    fi
fi
])

dnl --------------------- Check if realpath() takes NULL
AC_DEFUN([AC_NETATALK_REALPATH], [
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
])