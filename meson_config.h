/* Define if building universal (internal helper macro) */
#mesondefine AC_APPLE_UNIVERSAL_BUILD

/* Does extattr API work */
#mesondefine BROKEN_EXTATTR

/* BSD compatiblity macro */
#mesondefine BSD4_4

/* Define if CNID Database Daemon backend should be compiled. */
#mesondefine CNID_BACKEND_DBD

/* Define if CNID LAST scheme backend should be compiled. */
#mesondefine CNID_BACKEND_LAST

/* CUPS API Version */
#mesondefine CUPS_API_VERSION

/* whether the MySQL CNID module is available */
#mesondefine CNID_BACKEND_MYSQL

/* Path to dbus-daemon */
#mesondefine DBUS_DAEMON_PATH

/* Required BDB version, major */
#mesondefine DB_MAJOR_REQ

/* Required BDB version, minor */
#mesondefine DB_MINOR_REQ

/* Required BDB version, patch */
#mesondefine DB_PATCH_REQ

/* Define if verbose debugging information should be included */
#mesondefine DEBUG

/* Define if you want to disable SIGALRM timers and DSI tickles */
#mesondefine DEBUGGING

/* Default CNID scheme to be used */
#mesondefine DEFAULT_CNID_SCHEME

/* Define if shell check should be disabled */
#mesondefine DISABLE_SHELLCHECK

/* Define to enable spooldir support */
#mesondefine DISABLE_SPOOL

/* BSD compatibility macro */
#mesondefine DLSYM_PREPEND_UNDERSCORE

/* Available Extended Attributes modules */
#mesondefine EA_MODULES

/* Define to 1 if built-in SSL should be enabled */
#mesondefine EMBEDDED_SSL

/* Define if you want compatibily with the FHS */
#mesondefine FHS_COMPATIBILITY

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

/* whether at funcs are available */
#mesondefine HAVE_ATFUNCS

/* Define to 1 if you have the `attr_get' function. */
#mesondefine HAVE_ATTR_GET

/* Define to 1 if you have the `attr_getf' function. */
#mesondefine HAVE_ATTR_GETF

/* Define to 1 if you have the `attr_list' function. */
#mesondefine HAVE_ATTR_LIST

/* Define to 1 if you have the `attr_listf' function. */
#mesondefine HAVE_ATTR_LISTF

/* Define to 1 if you have the `attr_remove' function. */
#mesondefine HAVE_ATTR_REMOVE

/* Define to 1 if you have the `attr_removef' function. */
#mesondefine HAVE_ATTR_REMOVEF

/* Define to 1 if you have the `attr_set' function. */
#mesondefine HAVE_ATTR_SET

/* Define to 1 if you have the `attr_setf' function. */
#mesondefine HAVE_ATTR_SETF

/* Define to 1 if you have the <attr/xattr.h> header file. */
#mesondefine HAVE_ATTR_XATTR_H

/* Use Avahi/DNS-SD registration */
#mesondefine HAVE_AVAHI

/* Uses Avahis threaded poll implementation */
#mesondefine HAVE_AVAHI_THREADED_POLL

/* Define to 1 if you have the `backtrace_symbols' function. */
#mesondefine HAVE_BACKTRACE_SYMBOLS

/* Define to 1 if dbtob implementation is broken. */
#mesondefine HAVE_BROKEN_DBTOB

/* Define to 1 if you have the <crypt.h> header file. */
#mesondefine HAVE_CRYPT_H

/* Define to enable CUPS Support */
#mesondefine HAVE_CUPS

/* Define if support for dbus-glib was found */
#mesondefine HAVE_DBUS_GLIB

/* Define to 1 if you have the `dirfd' function. */
#mesondefine HAVE_DIRFD

/* Define to 1 if you have the `dlclose' function. */
#mesondefine HAVE_DLCLOSE

/* Define if you have the GNU dld library. */
#mesondefine HAVE_DLD

/* Define to 1 if you have the `dlerror' function. */
#mesondefine HAVE_DLERROR

/* Define to 1 if you have the <dlfcn.h> header file. */
#mesondefine HAVE_DLFCN_H

/* Define to 1 if you have the `dlopen' function. */
#mesondefine HAVE_DLOPEN

/* Define to 1 if you have the `dlsym' function. */
#mesondefine HAVE_DLSYM

/* Define if you have the _dyld_func_lookup function. */
#mesondefine HAVE_DYLD

/* Define if errno declaration exists */
#mesondefine HAVE_ERRNO

/* Define to 1 if you have the <errno.h> header file. */
#mesondefine HAVE_ERRNO_H

/* extattr API has full fledged fds for EAs */
#mesondefine HAVE_EAFD

/* Define to 1 if you have the `extattr_delete_fd' function. */
#mesondefine HAVE_EXTATTR_DELETE_FD

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

/* Define to 1 if you have the `flistea' function. */
#mesondefine HAVE_FLISTEA

/* Define to 1 if you have the `flistxattr' function. */
#mesondefine HAVE_FLISTXATTR

/* Whether FreeBSD ZFS ACLs with libsunacl are available */
#mesondefine HAVE_FREEBSD_SUNACL

/* Define to 1 if you have the `fremoveea' function. */
#mesondefine HAVE_FREMOVEEA

/* Define to 1 if you have the `fremovexattr' function. */
#mesondefine HAVE_FREMOVEXATTR

/* Define to 1 if you have the `fsetea' function. */
#mesondefine HAVE_FSETEA

/* Define to 1 if you have the `fsetxattr' function. */
#mesondefine HAVE_FSETXATTR

/* Define to 1 if the system has the type `fshare_t'. */
#mesondefine HAVE_FSHARE_T

/* Define to 1 if you have the `fstatat' function. */
#mesondefine HAVE_FSTATAT

/* Define to 1 if you have the `getea' function. */
#mesondefine HAVE_GETEA

/* Define to 1 if you have the `getifaddrs' function. */
#mesondefine HAVE_GETIFADDRS

/* Define to 1 if you have the `getpagesize' function. */
#mesondefine HAVE_GETPAGESIZE

/* Define to 1 if you have the `getusershell' function. */
#mesondefine HAVE_GETUSERSHELL

/* Define to 1 if you have the `getxattr' function. */
#mesondefine HAVE_GETXATTR

/* Whether to enable GSSAPI support */
#mesondefine HAVE_GSSAPI

/* Define to 1 if you have the <gssapi/gssapi_generic.h> header file. */
#mesondefine HAVE_GSSAPI_GSSAPI_GENERIC_H

/* Define to 1 if you have the <gssapi/gssapi.h> header file. */
#mesondefine HAVE_GSSAPI_GSSAPI_H

/* Define to 1 if you have the <gssapi/gssapi_krb5.h> header file. */
#mesondefine HAVE_GSSAPI_GSSAPI_KRB5_H

/* Define to 1 if you have the <gssapi.h> header file. */
#mesondefine HAVE_GSSAPI_H

/* Wheter GSS_C_NT_HOSTBASED_SERVICE is in gssapi.h */
#mesondefine HAVE_GSS_C_NT_HOSTBASED_SERVICE

/* Define to 1 if you have the <inttypes.h> header file. */
#mesondefine HAVE_INTTYPES_H

/* Define if Kerberos 5 is available */
#mesondefine HAVE_KERBEROS

/* Define to 1 if you have the <kerberosv5/krb5.h> header file. */
#mesondefine HAVE_KERBEROSV5_KRB5_H

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

/* Define if you have the libdl library or equivalent. */
#mesondefine HAVE_LIBDL

/* Define if libdlloader will be built on this platform */
#mesondefine HAVE_LIBDLLOADER

/* define if you have libquota */
#mesondefine HAVE_LIBQUOTA

/* Define to 1 if you have the `sunacl' library (-lsunacl). */
#mesondefine HAVE_LIBSUNACL

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

/* Define to 1 if you have the `lsetea' function. */
#mesondefine HAVE_LSETEA

/* Define to 1 if you have the `lsetxattr' function. */
#mesondefine HAVE_LSETXATTR

/* Use mDNSRespnder/DNS-SD registration */
#mesondefine HAVE_MDNS

/* Define to 1 if you have the `mempcpy' function. */
#mesondefine HAVE_MEMPCPY

/* Define to 1 if you have the `mmap' function. */
#mesondefine HAVE_MMAP

/* Define to 1 if you have the <mntent.h> header file. */
#mesondefine HAVE_MNTENT_H

/* Define to 1 if you have the <netdb.h> header file. */
#mesondefine HAVE_NETDB_H

/* Whether NFSv4 ACLs are available */
#mesondefine HAVE_NFSV4_ACLS

/* Define to 1 if you have the `openat' function. */
#mesondefine HAVE_OPENAT

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
#mesondefine HAVE_PAM_PAM_APPL_H

/* Whether POSIX ACLs are available */
#mesondefine HAVE_POSIX_ACLS

/* Define to 1 if you have the `pread' function. */
#mesondefine HAVE_PREAD

/* Define to 1 if you have the `pselect' function. */
#mesondefine HAVE_PSELECT

/* Define if you have POSIX threads libraries and header files. */
#mesondefine HAVE_PTHREAD

/* Have PTHREAD_PRIO_INHERIT. */
#mesondefine HAVE_PTHREAD_PRIO_INHERIT

/* Define to 1 if you have the `pwrite' function. */
#mesondefine HAVE_PWRITE

/* Define to 1 if you have the `removeea' function. */
#mesondefine HAVE_REMOVEEA

/* Define to 1 if you have the `removexattr' function. */
#mesondefine HAVE_REMOVEXATTR

/* Define to 1 if you have the `renameat' function. */
#mesondefine HAVE_RENAMEAT

/* Define to 1 if you have the <rpcsvc/rquota.h> header file. */
#mesondefine HAVE_RPCSVC_RQUOTA_H

/* Define to 1 if you have the <rpc/pmap_prot.h> header file. */
#mesondefine HAVE_RPC_PMAP_PROT_H

/* Define to 1 if you have the <rpc/rpc.h> header file. */
#mesondefine HAVE_RPC_RPC_H

/* Define to 1 if the <rpcsvc/rquota.h> header file has a qr_status member. */
#mesondefine HAVE_RQUOTA_H_QR_STATUS

/* Define to 1 if you have the <security/pam_appl.h> header file. */
#mesondefine HAVE_SECURITY_PAM_APPL_H

/* Define to 1 if you have the `sendfilev' function. */
#mesondefine HAVE_SENDFILEV

/* Define to 1 if you have the `setea' function. */
#mesondefine HAVE_SETEA

/* Define to 1 if you have the `setlinebuf' function. */
#mesondefine HAVE_SETLINEBUF

/* Define to 1 if you have the `setxattr' function. */
#mesondefine HAVE_SETXATTR

/* Define to 1 if you have the <sgtty.h> header file. */
#mesondefine HAVE_SGTTY_H

/* Define if you have the shl_load function. */
#mesondefine HAVE_SHL_LOAD

/* Whether Solaris ACLs are available */
#mesondefine HAVE_SOLARIS_ACLS

/* Define to 1 if you have the `splice' function. */
#mesondefine HAVE_SPLICE

/* Define to 1 if you have the <statfs.h> header file. */
#mesondefine HAVE_STATFS_H

/* Define to 1 if you have the <stdint.h> header file. */
#mesondefine HAVE_STDINT_H

/* Define to 1 if you have the <stdio.h> header file. */
#mesondefine HAVE_STDIO_H

/* Define to 1 if you have the <stdlib.h> header file. */
#mesondefine HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#mesondefine HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#mesondefine HAVE_STRING_H

/* Define to 1 if you have the `strlcat' function. */
#mesondefine HAVE_STRLCAT

/* Define to 1 if you have the `strlcpy' function. */
#mesondefine HAVE_STRLCPY

/* Define to 1 if you have the `strnlen' function. */
#mesondefine HAVE_STRNLEN

/* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
#mesondefine HAVE_STRUCT_TM_TM_GMTOFF

/* Define to 1 if you have the <sys/attributes.h> header file. */
#mesondefine HAVE_SYS_ATTRIBUTES_H

/* Define to 1 if you have the <sys/attr.h> header file. */
#mesondefine HAVE_SYS_ATTR_H

/* Define to 1 if you have the <sys/ea.h> header file. */
#mesondefine HAVE_SYS_EA_H

/* Define to 1 if you have the <sys/extattr.h> header file. */
#mesondefine HAVE_SYS_EXTATTR_H

/* Define to 1 if you have the <sys/fcntl.h> header file. */
#mesondefine HAVE_SYS_FCNTL_H

/* Define to 1 if you have the <sys/mnttab.h> header file. */
#mesondefine HAVE_SYS_MNTTAB_H

/* Define to 1 if you have the <sys/mount.h> header file. */
#mesondefine HAVE_SYS_MOUNT_H

/* Define to 1 if you have the <sys/param.h> header file. */
#mesondefine HAVE_SYS_PARAM_H

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#mesondefine HAVE_SYS_STATVFS_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#mesondefine HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/termios.h> header file. */
#mesondefine HAVE_SYS_TERMIOS_H

/* Define to 1 if you have the <sys/types.h> header file. */
#mesondefine HAVE_SYS_TYPES_H

/* Define to 1 if you have the <sys/uio.h> header file. */
#mesondefine HAVE_SYS_UIO_H

/* Define to 1 if you have the <sys/vfs.h> header file. */
#mesondefine HAVE_SYS_VFS_H

/* Define to 1 if you have the <sys/xattr.h> header file. */
#mesondefine HAVE_SYS_XATTR_H

/* Define if talloc library is available */
#mesondefine HAVE_TALLOC

/* Define to 1 if you have the <termios.h> header file. */
#mesondefine HAVE_TERMIOS_H

/* Define if Tracker3 is used */
#mesondefine HAVE_TRACKER3

/* Whether UCS-2-INTERNAL is supported */
#mesondefine HAVE_UCS2INTERNAL

/* Define to 1 if you have the <ufs/quota.h> header file. */
#mesondefine HAVE_UFS_QUOTA_H

/* Define to 1 if you have the `unlinkat' function. */
#mesondefine HAVE_UNLINKAT

/* Whether to use native iconv */
#mesondefine HAVE_USABLE_ICONV

/* Define to 1 if you have the `utime' function. */
#mesondefine HAVE_UTIME

/* Define to 1 if you have the `vasprintf' function. */
#mesondefine HAVE_VASPRINTF

/* Define as const if the declaration of iconv() needs const. */
#mesondefine ICONV_CONST

/* OS is Linux */
#mesondefine LINUX

/* Define to the extension used for runtime loadable modules, say, ".so". */
#mesondefine LT_MODULE_EXT

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#mesondefine LT_OBJDIR

/* Define to the shared library suffix, say, ".dylib". */
#mesondefine LT_SHARED_EXT

/* Define to the shared archive member specification, say "(shr.o)". */
#mesondefine LT_SHARED_LIB_MEMBER

/* Disable assertions */
#mesondefine NDEBUG

/* Define various xdr functions */
#mesondefine NEED_RQUOTA

/* Define if dlsym() requires a leading underscore in symbol names. */
#mesondefine NEED_USCORE

/* Define if OS is NetBSD */
#mesondefine NETBSD

/* Define if DDP should be disabled */
#mesondefine NO_DDP

/* Define if Quota support should be disabled */
#mesondefine NO_QUOTA_SUPPORT

/* Define if the gmtoff member of struct tm is not available */
#mesondefine NO_STRUCT_TM_GMTOFF

/* errno returned by open with O_NOFOLLOW */
#mesondefine OPEN_NOFOLLOW_ERRNO

/* Name of package */
#mesondefine PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#mesondefine PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#mesondefine PACKAGE_NAME

/* Define to the full name and version of this package. */
#mesondefine PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#mesondefine PACKAGE_TARNAME

/* Define to the home page for this package. */
#mesondefine PACKAGE_URL

/* Define to the version of this package. */
#mesondefine PACKAGE_VERSION

/* netatalk lockfile path */
#mesondefine PATH_NETATALK_LOCK

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
#mesondefine PTHREAD_CREATE_JOINABLE

/* Whether the realpath function allows NULL */
#mesondefine REALPATH_TAKES_NULL

/* Define if the sendfile() function uses BSD semantics */
#mesondefine SENDFILE_FLAVOR_BSD

/* Whether linux sendfile() API is available */
#mesondefine SENDFILE_FLAVOR_LINUX

/* Solaris sendfile() */
#mesondefine SENDFILE_FLAVOR_SOLARIS

/* Define if shadow passwords should be used */
#mesondefine SHADOWPW

/* Solaris compatibility macro */
#mesondefine SOLARIS

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#mesondefine STDC_HEADERS

/* Define if TCP wrappers should be used */
#mesondefine TCPWRAP

/* tracker managing command */
#mesondefine TRACKER_MANAGING_COMMAND

/* Path to Tracker */
#mesondefine TRACKER_PREFIX

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

/* Define if the WolfSSL DHX modules should be built */
#mesondefine WOLFSSL_DHX

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

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#mesondefine YYTEXT_POINTER

/* AT file source */
#mesondefine _ATFILE_SOURCE

/* _FILE_OFFSET_BITS (for LARGEFILE support) */
#mesondefine _FILE_OFFSET_BITS

/* Whether to use GNU libc extensions */
#mesondefine _GNU_SOURCE

/* Compatibility macro */
#mesondefine _ISOC9X_SOURCE

/* _LARGE_FILES (for LARGEFILE support) */
#mesondefine _LARGE_FILES

/* path to cracklib dictionary */
#mesondefine _PATH_CRACKLIB

/* Solaris compilation environment */
#mesondefine _XOPEN_SOURCE

/* Solaris compilation environment */
#mesondefine __EXTENSIONS__

/* Solaris compatibility macro */
#mesondefine __svr4__

/* WolfSSL configuration */

#define HAVE_DH_DEFAULT_PARAMS
#define HAVE_TLS_EXTENSIONS
#define NO_AES
#define NO_AES_CBC
#define NO_AESGCM_AEAD
#define NO_CPUID
#define NO_DO178
#define NO_DSA
#define NO_ERROR_QUEUE
#define NO_ERROR_STRINGS
#define NO_FILESYSTEM
#define NO_MD4
#define NO_MD5
#define NO_OLD_TLS
#define NO_PKCS12
#define NO_PSK
#define NO_RC4
#define NO_SHA
#define NO_WOLFSSL_MEMORY
#define OPENSSL_ALL
#define OPENSSL_EXTRA
#define TFM_TIMING_RESISTANT
#define WC_NO_ASYNC_THREADING
#define WC_RSA_BLINDING
#define WC_RSA_PSS
#define WOLFSSL_DES_ECB
#define WOLFSSL_ENCRYPTED_KEYS
#define WOLFSSL_NO_DEF_TICKET_ENC_CB

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#mesondefine inline
#endif
