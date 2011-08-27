dnl $Id: summary.m4,v 1.6 2009-10-02 09:32:41 franklahm Exp $
dnl Autoconf macros, display configure summary

AC_DEFUN([AC_NETATALK_CONFIG_SUMMARY], [

	AC_MSG_RESULT([Configure summary:])
	AC_MSG_RESULT([    Install style:])
	if test "x$sysv_style" != "x"; then
		AC_MSG_RESULT([         $sysv_style])
	else
		AC_MSG_RESULT([         none])
	fi
	AC_MSG_RESULT([    AFP:])
	AC_MSG_RESULT([         Large file support (>2GB) for AFP3: $wx_largefile])
	AC_MSG_RESULT([         Extended Attributes: $neta_cv_eas])
	AC_MSG_RESULT([    CNID:])
	AC_MSG_RESULT([         backends: $compiled_backends])
	AC_MSG_RESULT([    UAMS:])
	uams_using_options=""
	if test x"$netatalk_cv_use_pam" != x"no"; then
        	uams_using_options="PAM"
	fi
	if test "x$netatalk_cv_use_shadowpw" = "xyes"; then
        	uams_using_options="$uams_using_options SHADOW"
	fi
	if test "x$neta_cv_compile_dhx" = "xyes"; then
		AC_MSG_RESULT([         DHX     ($uams_using_options)])
	fi
        if test "x$neta_cv_compile_dhx2" = "xyes"; then
                AC_MSG_RESULT([         DHX2    ($uams_using_options)])
        fi
	if test "x$neta_cv_have_openssl" = "xyes"; then
		AC_MSG_RESULT([         RANDNUM ($uams_using_options)])
	fi
	if test x"$netatalk_cv_build_krb5_uam" = x"yes"; then
		AC_MSG_RESULT([         Kerberos V])
	fi
	if test x"$compile_kerberos" = x"yes"; then
		AC_MSG_RESULT([         Kerberos IV])
	fi
	if test x"$compile_pgp" = x"yes"; then
		AC_MSG_RESULT([         PGP])
	fi
	AC_MSG_RESULT([         passwd  ($uams_using_options)])
	AC_MSG_RESULT([         guest])
	AC_MSG_RESULT([    Options:])
	AC_MSG_RESULT([         DDP (AppleTalk) support: $netatalk_cv_ddp_enabled])
	if test "x$netatalk_cv_ddp_enabled" = "xyes"; then
		AC_MSG_RESULT([         CUPS support:            $netatalk_cv_use_cups])
		AC_MSG_RESULT([         Apple 2 boot support:    $compile_a2boot])
	fi
	AC_MSG_RESULT([         SLP support:             $netatalk_cv_srvloc])
	AC_MSG_RESULT([         Zeroconf support:        $netatalk_cv_zeroconf])
	AC_MSG_RESULT([         tcp wrapper support:     $netatalk_cv_tcpwrap])
dnl	if test x"$netatalk_cv_linux_sendfile" != x; then
dnl		AC_MSG_RESULT([         Linux sendfile support:  $netatalk_cv_linux_sendfile])
dnl	fi
	AC_MSG_RESULT([         quota support:           $netatalk_cv_quotasupport])
	AC_MSG_RESULT([         admin group support:     $netatalk_cv_admin_group])
	AC_MSG_RESULT([         valid shell check:       $netatalk_cv_use_shellcheck])
	AC_MSG_RESULT([         cracklib support:        $netatalk_cv_with_cracklib])
	AC_MSG_RESULT([         dropbox kludge:          $netatalk_cv_dropkludge])
	AC_MSG_RESULT([         force volume uid/gid:    $netatalk_cv_force_uidgid])
	AC_MSG_RESULT([         ACL support:             $with_acl_support])
	AC_MSG_RESULT([         LDAP support:            $with_ldap])
	if test x"$use_pam_so" = x"yes" -a x"$netatalk_cv_install_pam" = x"no"; then
		AC_MSG_RESULT([])
		AC_MSG_WARN([ PAM support was configured for your system, but the netatalk PAM configuration file])
		AC_MSG_WARN([ cannot be installed. Please install the config/netatalk.pamd file manually.])
		AC_MSG_WARN([ If you're running Solaris or BSD you'll have to edit /etc/pam.conf to get PAM working.])
		AC_MSG_WARN([ You can also re-run configure and specify --without-pam to disable PAM support.])
	fi

])


AC_DEFUN([AC_NETATALK_LIBS_SUMMARY], [
	dnl #################################################
	dnl # Display summary of libraries detected

	AC_MSG_RESULT([Using libraries:])
	AC_MSG_RESULT([    LIBS = $LIBS])
	AC_MSG_RESULT([    CFLAGS = $CFLAGS])
	if test x"$neta_cv_have_openssl" = x"yes"; then
		AC_MSG_RESULT([    SSL:])
		AC_MSG_RESULT([        LIBS   = $SSL_LIBS])
		AC_MSG_RESULT([        CFLAGS = $SSL_CFLAGS])
	fi
        if test x"$neta_cv_have_libgcrypt" = x"yes"; then
                AC_MSG_RESULT([    LIBGCRYPT:])
                AC_MSG_RESULT([        LIBS   = $LIBGCRYPT_LIBS])
                AC_MSG_RESULT([        CFLAGS = $LIBGCRYPT_CFLAGS])
        fi
	if test x"$netatalk_cv_use_pam" = x"yes"; then
		AC_MSG_RESULT([    PAM:])
		AC_MSG_RESULT([        LIBS   = $PAM_LIBS])
		AC_MSG_RESULT([        CFLAGS = $PAM_CFLAGS])
	fi
	if test x"$netatalk_cv_use_pam" = x"yes"; then
		AC_MSG_RESULT([    WRAP:])
		AC_MSG_RESULT([        LIBS   = $WRAP_LIBS])
		AC_MSG_RESULT([        CFLAGS = $WRAP_CFLAGS])
	fi
	if test x"$bdb_required" = x"yes"; then
		AC_MSG_RESULT([    BDB:])
		AC_MSG_RESULT([        LIBS   = $BDB_LIBS])
		AC_MSG_RESULT([        CFLAGS = $BDB_CFLAGS])
	fi
	if test x"$netatalk_cv_build_krb5_uam" = x"yes"; then
		AC_MSG_RESULT([    GSSAPI:])
		AC_MSG_RESULT([        LIBS   = $GSSAPI_LIBS])
		AC_MSG_RESULT([        CFLAGS = $GSSAPI_CFLAGS])
	fi
	if test x"$netatalk_cv_srvloc" = x"yes"; then
		AC_MSG_RESULT([    SRVLOC:])
		AC_MSG_RESULT([        LIBS   = $SLP_LIBS])
		AC_MSG_RESULT([        CFLAGS = $SLP_CFLAGS])
	fi
	if test x"$netatalk_cv_use_cups" = x"yes"; then
		AC_MSG_RESULT([    CUPS:])
		AC_MSG_RESULT([        LIBS   = $CUPS_LIBS])
		AC_MSG_RESULT([        CFLAGS = $CUPS_CFLAGS])
	fi
])
