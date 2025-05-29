/* Define if building universal (internal helper macro) */
#mesondefine AC_APPLE_UNIVERSAL_BUILD

/* BSD compatiblity macro */
#mesondefine BSD4_4

/* Define if CNID Database Daemon backend should be compiled. */
#mesondefine CNID_BACKEND_DBD

/* Define if CNID LAST scheme backend should be compiled. */
#mesondefine CNID_BACKEND_LAST

/* whether the MySQL CNID module is available */
#mesondefine CNID_BACKEND_MYSQL

/* CUPS API Version */
#mesondefine CUPS_API_VERSION

/* Path to dbus-daemon */
#mesondefine DBUS_DAEMON_PATH

/* Define if verbose debugging information should be included */
#mesondefine DEBUG

/* Define if you want to disable SIGALRM timers and DSI tickles */
#mesondefine DEBUGGING

/* Default CNID scheme to be used */
#mesondefine DEFAULT_CNID_SCHEME

/* Define if shell check should be disabled */
#mesondefine DISABLE_SHELLCHECK

/* Available Extended Attributes modules */
#mesondefine EA_MODULES

/* Define if OS is FreeBSD */
#mesondefine FREEBSD

/* Whether ACLs support is available */
#mesondefine HAVE_ACLS

/* Whether acl_from_mode() is available */
#mesondefine HAVE_ACL_FROM_MODE

/* Whether acl_get_perm_np() is available */
#mesondefine HAVE_ACL_GET_PERM_NP

/* Define to 1 if you have the <acl/libacl.h> header file. */
#mesondefine HAVE_ACL_LIBACL_H

/* Define to 1 if you have the `asprintf' function. */
#mesondefine HAVE_ASPRINTF

/* set if struct at_addr is called atalk_addr */
#mesondefine HAVE_ATALK_ADDR

/* Define to 1 if you have the <attr/xattr.h> header file. */
#mesondefine HAVE_ATTR_XATTR_H

/* Use Avahi/DNS-SD registration */
#mesondefine HAVE_AVAHI

/* Define to 1 if you have the `backtrace_symbols' function. */
#mesondefine HAVE_BACKTRACE_SYMBOLS

/* Define to 1 if dbtob implementation is broken. */
#mesondefine HAVE_BROKEN_DBTOB

/* Define to 1 if you have the `crypt_checkpass' function. */
#mesondefine HAVE_CRYPT_CHECKPASS

/* Define to 1 if you have the <crypt.h> header file. */
#mesondefine HAVE_CRYPT_H

/* Define to enable CUPS Support */
#mesondefine HAVE_CUPS

/* Define if support for dbus-glib was found */
#mesondefine HAVE_DBUS_GLIB

/* Define to 1 if you have the `dirfd' function. */
#mesondefine HAVE_DIRFD

/* Define to 1 if you have the <dlfcn.h> header file. */
#mesondefine HAVE_DLFCN_H

/* extattr API has full fledged fds for EAs */
#mesondefine HAVE_EAFD

/* Define to 1 if you have the `extattr_delete_file' function. */
#mesondefine HAVE_EXTATTR_DELETE_FILE

/* Define to 1 if you have the `extattr_delete_link' function. */
#mesondefine HAVE_EXTATTR_DELETE_LINK

/* Define to 1 if you have the `extattr_get_fd' function. */
#mesondefine HAVE_EXTATTR_GET_FD

/* Define to 1 if you have the `extattr_get_file' function. */
#mesondefine HAVE_EXTATTR_GET_FILE

/* Define to 1 if you have the `extattr_get_link' function. */
#mesondefine HAVE_EXTATTR_GET_LINK

/* Define to 1 if you have the `extattr_list_fd' function. */
#mesondefine HAVE_EXTATTR_LIST_FD

/* Define to 1 if you have the `extattr_list_file' function. */
#mesondefine HAVE_EXTATTR_LIST_FILE

/* Define to 1 if you have the `extattr_list_link' function. */
#mesondefine HAVE_EXTATTR_LIST_LINK

/* Define to 1 if you have the `extattr_set_fd' function. */
#mesondefine HAVE_EXTATTR_SET_FD

/* Define to 1 if you have the `extattr_set_file' function. */
#mesondefine HAVE_EXTATTR_SET_FILE

/* Define to 1 if you have the `extattr_set_link' function. */
#mesondefine HAVE_EXTATTR_SET_LINK

/* Define to 1 if you have the `fgetea' function. */
#mesondefine HAVE_FGETEA

/* Define to 1 if you have the `fgetxattr' function. */
#mesondefine HAVE_FGETXATTR

/* Whether FreeBSD ZFS ACLs with libsunacl are available */
#mesondefine HAVE_FREEBSD_SUNACL

/* Define to 1 if you have the `fsetea' function. */
#mesondefine HAVE_FSETEA

/* Define to 1 if you have the `fsetxattr' function. */
#mesondefine HAVE_FSETXATTR

/* Define to 1 if the system has the type `fshare_t'. */
#mesondefine HAVE_FSHARE_T

/* Define to 1 if you have the `getea' function. */
#mesondefine HAVE_GETEA

/* Define to 1 if you have the `getifaddrs' function. */
#mesondefine HAVE_GETIFADDRS

/* Define to 1 if you have the `getpwnam_shadow' function. */
#mesondefine HAVE_GETPWNAM_SHADOW

/* Define to 1 if you have the `getxattr' function. */
#mesondefine HAVE_GETXATTR

/* Whether to enable GSSAPI support */
#mesondefine HAVE_GSSAPI

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
#mesondefine HAVE_GSSAPI_GSSAPI_H

/* Define to 1 if you have the <gssapi.h> header file. */
#mesondefine HAVE_GSSAPI_H

/* Define to 1 if you have the <iniparser/iniparser.h> header file. */
#mesondefine HAVE_INIPARSER_INIPARSER_H

/* Define if Kerberos 5 is available */
#mesondefine HAVE_KERBEROS

/* Define to 1 if you have the `krb5_free_error_message' function. */
#mesondefine HAVE_KRB5_FREE_ERROR_MESSAGE

/* Define to 1 if you have the `krb5_free_keytab_entry_contents' function. */
#mesondefine HAVE_KRB5_FREE_KEYTAB_ENTRY_CONTENTS

/* Define to 1 if you have the `krb5_free_unparsed_name' function. */
#mesondefine HAVE_KRB5_FREE_UNPARSED_NAME

/* Define to 1 if you have the <krb5.h> header file. */
#mesondefine HAVE_KRB5_H

/* Define to 1 if you have the <krb5/krb5.h> header file. */
#mesondefine HAVE_KRB5_KRB5_H

/* Define to 1 if you have the `krb5_kt_free_entry' function. */
#mesondefine HAVE_KRB5_KT_FREE_ENTRY

/* Define to 1 if you have the <langinfo.h> header file. */
#mesondefine HAVE_LANGINFO_H

/* Whether LDAP is available */
#mesondefine HAVE_LDAP

/* Define to 1 if you have the `lgetea' function. */
#mesondefine HAVE_LGETEA

/* Define to 1 if you have the `lgetxattr' function. */
#mesondefine HAVE_LGETXATTR

/* define if you have libquota */
#mesondefine HAVE_LIBQUOTA

/* Define to 1 if you have the `listea' function. */
#mesondefine HAVE_LISTEA

/* Define to 1 if you have the `listxattr' function. */
#mesondefine HAVE_LISTXATTR

/* Define to 1 if you have the `llistea' function. */
#mesondefine HAVE_LLISTEA

/* Define to 1 if you have the `llistxattr' function. */
#mesondefine HAVE_LLISTXATTR

/* Define to 1 if you have the <locale.h> header file. */
#mesondefine HAVE_LOCALE_H

/* Define to 1 if you have the `lremoveea' function. */
#mesondefine HAVE_LREMOVEEA

/* Define to 1 if you have the `lremovexattr' function. */
#mesondefine HAVE_LREMOVEXATTR

/* Define to 1 if you have the `lsetxattr' function. */
#mesondefine HAVE_LSETXATTR

/* Use mDNSRespnder/DNS-SD registration */
#mesondefine HAVE_MDNS

/* Define to 1 if you have the `mempcpy' function. */
#mesondefine HAVE_MEMPCPY

/* Define to 1 if you have the <mntent.h> header file. */
#mesondefine HAVE_MNTENT_H

/* Whether NFSv4 ACLs are available */
#mesondefine HAVE_NFSV4_ACLS

/* Whether the PAM header has a pam_conv struct with const pam_message member. */
#mesondefine HAVE_PAM_CONV_CONST_PAM_MESSAGE

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
#mesondefine HAVE_PAM_PAM_APPL_H

/* Whether POSIX ACLs are available */
#mesondefine HAVE_POSIX_ACLS

/* Define to 1 if you have the `pread' function. */
#mesondefine HAVE_PREAD

/* Define to 1 if you have the `pselect' function. */
#mesondefine HAVE_PSELECT

/* Define to 1 if you have the `pwrite' function. */
#mesondefine HAVE_PWRITE

/* Define to 1 if you have the `removeea' function. */
#mesondefine HAVE_REMOVEEA

/* Define to 1 if you have the `removexattr' function. */
#mesondefine HAVE_REMOVEXATTR

/* Define to 1 if the <rpcsvc/rquota.h> header file has a qr_status member. */
#mesondefine HAVE_RQUOTA_H_QR_STATUS

/* Define to 1 if you have the `rresvport' function. */
#mesondefine HAVE_RRESVPORT

/* Define to 1 if you have the <security/pam_appl.h> header file. */
#mesondefine HAVE_SECURITY_PAM_APPL_H

/* Define to 1 if you have the `sendfilev' function. */
#mesondefine HAVE_SENDFILEV

/* Define to 1 if you have the `setea' function. */
#mesondefine HAVE_SETEA

/* Define to 1 if you have the `setxattr' function. */
#mesondefine HAVE_SETXATTR

/* Define to 1 if you have the <sgtty.h> header file. */
#mesondefine HAVE_SGTTY_H

/* Whether Solaris ACLs are available */
#mesondefine HAVE_SOLARIS_ACLS

/* Define to 1 if you have the `splice' function. */
#mesondefine HAVE_SPLICE

/* Define to 1 if you have the `strlcat' function. */
#mesondefine HAVE_STRLCAT

/* Define to 1 if you have the `strlcpy' function. */
#mesondefine HAVE_STRLCPY

/* Define to 1 if you have the `strnlen' function. */
#mesondefine HAVE_STRNLEN

/* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
#mesondefine HAVE_STRUCT_TM_TM_GMTOFF

/* Define to 1 if you have the <sys/attr.h> header file. */
#mesondefine HAVE_SYS_ATTR_H

/* Define to 1 if you have the <sys/ea.h> header file. */
#mesondefine HAVE_SYS_EA_H

/* Define to 1 if you have the <sys/extattr.h> header file. */
#mesondefine HAVE_SYS_EXTATTR_H

/* Define to 1 if you have the <sys/mnttab.h> header file. */
#mesondefine HAVE_SYS_MNTTAB_H

/* Define to 1 if you have the <sys/mount.h> header file. */
#mesondefine HAVE_SYS_MOUNT_H

/* Define to 1 if you have the <sys/vfs.h> header file. */
#mesondefine HAVE_SYS_VFS_H

/* Define to 1 if you have the <sys/xattr.h> header file. */
#mesondefine HAVE_SYS_XATTR_H

/* Define if Tracker3 is used */
#mesondefine HAVE_TRACKER3

/* Whether UCS-2-INTERNAL is supported */
#mesondefine HAVE_UCS2INTERNAL

/* Define to 1 if you have the <ufs/quota.h> header file. */
#mesondefine HAVE_UFS_QUOTA_H

/* Whether to use native iconv */
#mesondefine HAVE_USABLE_ICONV

/* Define to 1 if you have the `vasprintf' function. */
#mesondefine HAVE_VASPRINTF

/* Define as const if the declaration of iconv() needs const. */
#mesondefine ICONV_CONST

/* OS is Linux */
#mesondefine LINUX

/* Disable assertions */
#mesondefine NDEBUG

/* Define various xdr functions */
#mesondefine NEED_RQUOTA

/* Define if OS is NetBSD */
#mesondefine NETBSD

/* Define if DDP should be disabled */
#mesondefine NO_DDP

/* Define if Quota support should be disabled */
#mesondefine NO_QUOTA_SUPPORT

/* errno returned by open with O_NOFOLLOW */
#mesondefine OPEN_NOFOLLOW_ERRNO

/* atalkd lockfile path */
#mesondefine PATH_ATALKD_LOCK

/* netatalk lockfile path */
#mesondefine PATH_NETATALK_LOCK

/* papd lockfile path */
#mesondefine PATH_PAPD_LOCK

/* Whether the realpath function allows NULL */
#mesondefine REALPATH_TAKES_NULL

/* Define if the sendfile() function uses BSD semantics */
#mesondefine SENDFILE_FLAVOR_FREEBSD

/* Whether linux sendfile() API is available */
#mesondefine SENDFILE_FLAVOR_LINUX

/* Solaris sendfile() */
#mesondefine SENDFILE_FLAVOR_SOLARIS

/* Define if shadow passwords should be used */
#mesondefine SHADOWPW

/* Solaris compatibility macro */
#mesondefine SOLARIS

/* Define if TCP wrappers should be used */
#mesondefine TCPWRAP

/* Indexer managing command */
#mesondefine INDEXER_COMMAND

/* Define if cracklib should be used */
#mesondefine USE_CRACKLIB

/* Define to enable Zeroconf support */
#mesondefine USE_ZEROCONF

/* Version number of package */
#mesondefine VERSION

/* dtrace probes */
#mesondefine WITH_DTRACE

/* Whether recvfile should be used */
#mesondefine WITH_RECVFILE

/* Whether sendfile() should be used */
#mesondefine WITH_SENDFILE

/* Define whether to enable Spotlight support */
#mesondefine WITH_SPOTLIGHT

/* Define when the test suite should be executed */
#mesondefine WITH_TESTS

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

/* xattr functions have additional options */
#mesondefine XATTR_ADD_OPT

/* _FILE_OFFSET_BITS (for LARGEFILE support) */
#mesondefine _FILE_OFFSET_BITS

/* Whether to use GNU libc extensions */
#mesondefine _GNU_SOURCE

/* Compatibility macro */
#mesondefine _ISOC9X_SOURCE

/* path to cracklib dictionary */
#mesondefine _PATH_CRACKLIB

/* Solaris compilation environment */
#mesondefine _XOPEN_SOURCE

/* Solaris compilation environment */
#mesondefine __EXTENSIONS__

/* Solaris compatibility macro */
#mesondefine __svr4__

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#mesondefine inline
#endif
