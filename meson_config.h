/* Define if the admin group should be enabled */
#mesondefine ADMIN_GRP

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

/* Required BDB version, major */
#mesondefine DB_MAJOR_REQ

/* Required BDB version, minor */
#mesondefine DB_MINOR_REQ

/* Required BDB version, patch */
#mesondefine DB_PATCH_REQ

/* Define if you want to disable SIGALRM timers and DSI tickles */
#mesondefine DEBUGGING

/* Default CNID scheme to be used */
#mesondefine DEFAULT_CNID_SCHEME

/* Define if system (fcntl) locking should be disabled */
#mesondefine DISABLE_LOCKING

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

/* Define to the type of elements in the array argument to 'getgroups'.
   Usually this is either 'int' or 'gid_t'. */
#mesondefine GETGROUPS_T

/* Define to 1 if you have the 'access' function. */
#mesondefine HAVE_ACCESS

/* Whether ACLs support is available */
#mesondefine HAVE_ACLS

/* Whether acl_from_mode() is available */
#mesondefine HAVE_ACL_FROM_MODE

/* Whether acl_get_perm_np() is available */
#mesondefine HAVE_ACL_GET_PERM_NP

/* Define to 1 if you have the <acl/libacl.h> header file. */
#mesondefine HAVE_ACL_LIBACL_H

/* set if struct at_addr is called atalk_addr */
#mesondefine HAVE_ATALK_ADDR

/* whether at funcs are available */
#mesondefine HAVE_ATFUNCS

/* Define to 1 if you have the 'attropen' function. */
#mesondefine HAVE_ATTROPEN

/* Define to 1 if you have the 'attr_get' function. */
#mesondefine HAVE_ATTR_GET

/* Define to 1 if you have the 'attr_getf' function. */
#mesondefine HAVE_ATTR_GETF

/* Define to 1 if you have the 'attr_list' function. */
#mesondefine HAVE_ATTR_LIST

/* Define to 1 if you have the 'attr_listf' function. */
#mesondefine HAVE_ATTR_LISTF

/* Define to 1 if you have the 'attr_remove' function. */
#mesondefine HAVE_ATTR_REMOVE

/* Define to 1 if you have the 'attr_removef' function. */
#mesondefine HAVE_ATTR_REMOVEF

/* Define to 1 if you have the 'attr_set' function. */
#mesondefine HAVE_ATTR_SET

/* Define to 1 if you have the 'attr_setf' function. */
#mesondefine HAVE_ATTR_SETF

/* Define to 1 if you have the <attr/xattr.h> header file. */
#mesondefine HAVE_ATTR_XATTR_H

/* Use Avahi/DNS-SD registration */
#mesondefine HAVE_AVAHI

/* Uses Avahis threaded poll implementation */
#mesondefine HAVE_AVAHI_THREADED_POLL

/* Define to 1 if you have the 'backtrace_symbols' function. */
#mesondefine HAVE_BACKTRACE_SYMBOLS

/* Define to 1 if you have the 'chmod' function. */
#mesondefine HAVE_CHMOD

/* Define to 1 if you have the 'chown' function. */
#mesondefine HAVE_CHOWN

/* Define to 1 if you have the 'chroot' function. */
#mesondefine HAVE_CHROOT

/* Define to 1 if you have the <crypt.h> header file. */
#mesondefine HAVE_CRYPT_H

/* Define to enable CUPS Support */
#mesondefine HAVE_CUPS

/* Define to 1 if you have the declaration of 'cygwin_conv_path', and to 0 if
   you don't. */
#mesondefine HAVE_DECL_CYGWIN_CONV_PATH

/* Define if errno declaration exists */
#mesondefine HAVE_DECL_ERRNO

/* Define if sys_errlist declaration exists */
#mesondefine HAVE_DECL_SYS_ERRLIST

/* Define if sys_nerr declaration exists */
#mesondefine HAVE_DECL_SYS_NERR

/* Define to 1 if you have the <dirent.h> header file, and it defines 'DIR'.
   */
#mesondefine HAVE_DIRENT_H

/* Define to 1 if you have the 'dirfd' function. */
#mesondefine HAVE_DIRFD

/* Define to 1 if you have the 'dlclose' function. */
#mesondefine HAVE_DLCLOSE

/* Define if you have the GNU dld library. */
#mesondefine HAVE_DLD

/* Define to 1 if you have the 'dlerror' function. */
#mesondefine HAVE_DLERROR

/* Define to 1 if you have the <dlfcn.h> header file. */
#mesondefine HAVE_DLFCN_H

/* Define to 1 if you have the 'dlopen' function. */
#mesondefine HAVE_DLOPEN

/* Define to 1 if you have the 'dlsym' function. */
#mesondefine HAVE_DLSYM

/* Define if you have the _dyld_func_lookup function. */
#mesondefine HAVE_DYLD

/* Define if errno declaration exists */
#mesondefine HAVE_ERRNO

/* Define to 1 if you have the <errno.h> header file. */
#mesondefine HAVE_ERRNO_H

/* Define to 1 if you have the 'extattr_delete_fd' function. */
#mesondefine HAVE_EXTATTR_DELETE_FD

/* Define to 1 if you have the 'extattr_delete_file' function. */
#mesondefine HAVE_EXTATTR_DELETE_FILE

/* Define to 1 if you have the 'extattr_delete_link' function. */
#mesondefine HAVE_EXTATTR_DELETE_LINK

/* Define to 1 if you have the 'extattr_get_fd' function. */
#mesondefine HAVE_EXTATTR_GET_FD

/* Define to 1 if you have the 'extattr_get_file' function. */
#mesondefine HAVE_EXTATTR_GET_FILE

/* Define to 1 if you have the 'extattr_get_link' function. */
#mesondefine HAVE_EXTATTR_GET_LINK

/* Define to 1 if you have the 'extattr_list_fd' function. */
#mesondefine HAVE_EXTATTR_LIST_FD

/* Define to 1 if you have the 'extattr_list_file' function. */
#mesondefine HAVE_EXTATTR_LIST_FILE

/* Define to 1 if you have the 'extattr_list_link' function. */
#mesondefine HAVE_EXTATTR_LIST_LINK

/* Define to 1 if you have the 'extattr_set_fd' function. */
#mesondefine HAVE_EXTATTR_SET_FD

/* Define to 1 if you have the 'extattr_set_file' function. */
#mesondefine HAVE_EXTATTR_SET_FILE

/* Define to 1 if you have the 'extattr_set_link' function. */
#mesondefine HAVE_EXTATTR_SET_LINK

/* Define to 1 if you have the 'fchmod' function. */
#mesondefine HAVE_FCHMOD

/* Define to 1 if you have the 'fchown' function. */
#mesondefine HAVE_FCHOWN

/* Define to 1 if you have the <fcntl.h> header file. */
#mesondefine HAVE_FCNTL_H

/* Define to 1 if you have the 'fgetea' function. */
#mesondefine HAVE_FGETEA

/* Define to 1 if you have the 'fgetxattr' function. */
#mesondefine HAVE_FGETXATTR

/* Define to 1 if you have the 'flistea' function. */
#mesondefine HAVE_FLISTEA

/* Define to 1 if you have the 'flistxattr' function. */
#mesondefine HAVE_FLISTXATTR

/* Define to 1 if you have the 'fremoveea' function. */
#mesondefine HAVE_FREMOVEEA

/* Define to 1 if you have the 'fremovexattr' function. */
#mesondefine HAVE_FREMOVEXATTR

/* Define to 1 if you have the 'fsetea' function. */
#mesondefine HAVE_FSETEA

/* Define to 1 if you have the 'fsetxattr' function. */
#mesondefine HAVE_FSETXATTR

/* Define to 1 if you have the 'fstatat' function. */
#mesondefine HAVE_FSTATAT

/* Define to 1 if you have the 'getcwd' function. */
#mesondefine HAVE_GETCWD

/* Define to 1 if you have the 'getea' function. */
#mesondefine HAVE_GETEA

/* Define to 1 if you have the 'gethostname' function. */
#mesondefine HAVE_GETHOSTNAME

/* Define to 1 if you have the 'getpagesize' function. */
#mesondefine HAVE_GETPAGESIZE

/* Define to 1 if you have the 'gettimeofday' function. */
#mesondefine HAVE_GETTIMEOFDAY

/* Define to 1 if you have the 'getusershell' function. */
#mesondefine HAVE_GETUSERSHELL

/* Define to 1 if you have the 'getxattr' function. */
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

/* Define to 1 if you have the <langinfo.h> header file. */
#mesondefine HAVE_LANGINFO_H

/* LARGEFILE support */
#mesondefine HAVE_LARGEFILE_SUPPORT

/* Whether LDAP is available */
#mesondefine HAVE_LDAP

/* Define to 1 if you have the 'lgetea' function. */
#mesondefine HAVE_LGETEA

/* Define to 1 if you have the 'lgetxattr' function. */
#mesondefine HAVE_LGETXATTR

/* Define to 1 if you have the 'crypt' library (-lcrypt). */
#mesondefine HAVE_LIBCRYPT

/* Define to 1 if you have the 'crypto' library (-lcrypto). */
#mesondefine HAVE_LIBCRYPTO

/* Define to 1 if you have the 'des' library (-ldes). */
#mesondefine HAVE_LIBDES

/* Define if you have the libdl library or equivalent. */
#mesondefine HAVE_LIBDL

/* Define if libdlloader will be built on this platform */
#mesondefine HAVE_LIBDLLOADER

/* Define if the DHX2 modules should be built with libgcrypt */
#mesondefine HAVE_LIBGCRYPT

/* Define to 1 if you have the 'gss' library (-lgss). */
#mesondefine HAVE_LIBGSS

/* Define to 1 if you have the 'gssapi' library (-lgssapi). */
#mesondefine HAVE_LIBGSSAPI

/* Define to 1 if you have the 'gssapi_krb5' library (-lgssapi_krb5). */
#mesondefine HAVE_LIBGSSAPI_KRB5

/* Define to 1 if you have the 'nsl' library (-lnsl). */
#mesondefine HAVE_LIBNSL

/* define if you have libquota */
#mesondefine HAVE_LIBQUOTA

/* Define to 1 if you have the 'socket' library (-lsocket). */
#mesondefine HAVE_LIBSOCKET

/* Define to 1 if you have the <limits.h> header file. */
#mesondefine HAVE_LIMITS_H

/* Define to 1 if you have the 'link' function. */
#mesondefine HAVE_LINK

/* Define to 1 if you have the 'listea' function. */
#mesondefine HAVE_LISTEA

/* Define to 1 if you have the 'listxattr' function. */
#mesondefine HAVE_LISTXATTR

/* Define to 1 if you have the 'llistea' function. */
#mesondefine HAVE_LLISTEA

/* Define to 1 if you have the 'llistxattr' function. */
#mesondefine HAVE_LLISTXATTR

/* Define to 1 if you have the <locale.h> header file. */
#mesondefine HAVE_LOCALE_H

/* Define if long double is a valid data type */
#mesondefine HAVE_LONG_DOUBLE

/* Define if long long is a valid data type */
#mesondefine HAVE_LONG_LONG

/* Define to 1 if you have the 'lremoveea' function. */
#mesondefine HAVE_LREMOVEEA

/* Define to 1 if you have the 'lremovexattr' function. */
#mesondefine HAVE_LREMOVEXATTR

/* Define to 1 if you have the 'lsetea' function. */
#mesondefine HAVE_LSETEA

/* Define to 1 if you have the 'lsetxattr' function. */
#mesondefine HAVE_LSETXATTR

/* Use mDNSRespnder/DNS-SD registration */
#mesondefine HAVE_MDNS

/* Define to 1 if you have the 'memcpy' function. */
#mesondefine HAVE_MEMCPY

/* Define to 1 if you have the 'mkdir' function. */
#mesondefine HAVE_MKDIR

/* Define to 1 if you have the 'mknod' function. */
#mesondefine HAVE_MKNOD

/* Define to 1 if you have the 'mknod64' function. */
#mesondefine HAVE_MKNOD64

/* Define to 1 if you have a working 'mmap' system call. */
#mesondefine HAVE_MMAP

/* Define to 1 if you have the <mntent.h> header file. */
#mesondefine HAVE_MNTENT_H

/* Define to 1 if you have the <mount.h> header file. */
#mesondefine HAVE_MOUNT_H

/* Define to 1 if you have the <ndir.h> header file, and it defines 'DIR'. */
#mesondefine HAVE_NDIR_H

/* Define to 1 if you have the <netdb.h> header file. */
#mesondefine HAVE_NETDB_H

/* Define to 1 if you have the 'nl_langinfo' function. */
#mesondefine HAVE_NL_LANGINFO

/* Whether no ACLs support is available */
#mesondefine HAVE_NO_ACLS

/* Define to 1 if you have the 'openat' function. */
#mesondefine HAVE_OPENAT

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
#mesondefine HAVE_PAM_PAM_APPL_H

/* Whether POSIX ACLs are available */
#mesondefine HAVE_POSIX_ACLS

/* Define to 1 if you have the 'pread' function. */
#mesondefine HAVE_PREAD

/* Define to 1 if you have the 'pselect' function. */
#mesondefine HAVE_PSELECT

/* Define to 1 if you have the 'pwrite' function. */
#mesondefine HAVE_PWRITE

/* Define if quad_t is a valid data type */
#mesondefine HAVE_QUAD_T

/* Define to 1 if you have the 'removeea' function. */
#mesondefine HAVE_REMOVEEA

/* Define to 1 if you have the 'removexattr' function. */
#mesondefine HAVE_REMOVEXATTR

/* Define to 1 if you have the 'renameat' function. */
#mesondefine HAVE_RENAMEAT

/* Define to 1 if you have the 'rmdir' function. */
#mesondefine HAVE_RMDIR

/* Define to 1 if you have the <rpcsvc/rquota.h> header file. */
#mesondefine HAVE_RPCSVC_RQUOTA_H

/* Define to 1 if you have the <rpc/pmap_prot.h> header file. */
#mesondefine HAVE_RPC_PMAP_PROT_H

/* Define to 1 if you have the <rpc/rpc.h> header file. */
#mesondefine HAVE_RPC_RPC_H

/* Define to 1 if you have the <security/pam_appl.h> header file. */
#mesondefine HAVE_SECURITY_PAM_APPL_H

/* Define to 1 if you have the 'select' function. */
#mesondefine HAVE_SELECT

/* Define to 1 if you have the 'setea' function. */
#mesondefine HAVE_SETEA

/* Define to 1 if you have the 'setlinebuf' function. */
#mesondefine HAVE_SETLINEBUF

/* Define to 1 if you have the 'setlocale' function. */
#mesondefine HAVE_SETLOCALE

/* Define to 1 if you have the 'setxattr' function. */
#mesondefine HAVE_SETXATTR

/* Define to 1 if you have the <sgtty.h> header file. */
#mesondefine HAVE_SGTTY_H

/* Define if you have the shl_load function. */
#mesondefine HAVE_SHL_LOAD

/* Define to 1 if you have the 'snprintf' function. */
#mesondefine HAVE_SNPRINTF

/* Define to 1 if you have the 'socket' function. */
#mesondefine HAVE_SOCKET

/* Whether solaris ACLs are available */
#mesondefine HAVE_SOLARIS_ACLS

/* Define to 1 if you have the <statfs.h> header file. */
#mesondefine HAVE_STATFS_H

/* Define to 1 if you have the <stdarg.h> header file. */
#mesondefine HAVE_STDARG_H

/* Define to 1 if you have the <stdint.h> header file. */
#mesondefine HAVE_STDINT_H

/* Define to 1 if you have the <stdio.h> header file. */
#mesondefine HAVE_STDIO_H

/* Define to 1 if you have the <stdlib.h> header file. */
#mesondefine HAVE_STDLIB_H

/* Define to 1 if you have the 'strcasestr' function. */
#mesondefine HAVE_STRCASESTR

/* Define to 1 if you have the 'strchr' function. */
#mesondefine HAVE_STRCHR

/* Define to 1 if you have the 'strdup' function. */
#mesondefine HAVE_STRDUP

/* Define to 1 if you have the 'strerror' function. */
#mesondefine HAVE_STRERROR

/* Define to 1 if you have the <strings.h> header file. */
#mesondefine HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#mesondefine HAVE_STRING_H

/* Define to 1 if you have the 'strlcat' function. */
#mesondefine HAVE_STRLCAT

/* Define to 1 if you have the 'strlcpy' function. */
#mesondefine HAVE_STRLCPY

/* Define to 1 if you have the 'strndup' function. */
#mesondefine HAVE_STRNDUP

/* Define to 1 if you have the 'strnlen' function. */
#mesondefine HAVE_STRNLEN

/* Define to 1 if you have the 'strstr' function. */
#mesondefine HAVE_STRSTR

/* Define to 1 if you have the 'strtoul' function. */
#mesondefine HAVE_STRTOUL

/* Define to 1 if 'tm_gmtoff' is a member of 'struct tm'. */
#mesondefine HAVE_STRUCT_TM_TM_GMTOFF

/* Define to 1 if you have the <syslog.h> header file. */
#mesondefine HAVE_SYSLOG_H

/* Define to 1 if you have the <sys/attributes.h> header file. */
#mesondefine HAVE_SYS_ATTRIBUTES_H

/* Define to 1 if you have the <sys/dir.h> header file, and it defines 'DIR'.
   */
#mesondefine HAVE_SYS_DIR_H

/* Define to 1 if you have the <sys/ea.h> header file. */
#mesondefine HAVE_SYS_EA_H

/* Define if sys_errlist declaration exists */
#mesondefine HAVE_SYS_ERRLIST

/* Define to 1 if you have the <sys/extattr.h> header file. */
#mesondefine HAVE_SYS_EXTATTR_H

/* Define to 1 if you have the <sys/file.h> header file. */
#mesondefine HAVE_SYS_FILE_H

/* Define to 1 if you have the <sys/filio.h> header file. */
#mesondefine HAVE_SYS_FILIO_H

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#mesondefine HAVE_SYS_IOCTL_H

/* Define to 1 if you have the <sys/mnttab.h> header file. */
#mesondefine HAVE_SYS_MNTTAB_H

/* Define to 1 if you have the <sys/mount.h> header file. */
#mesondefine HAVE_SYS_MOUNT_H

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines 'DIR'.
   */
#mesondefine HAVE_SYS_NDIR_H

/* Define if sys_nerr declaration exists */
#mesondefine HAVE_SYS_NERR

/* Define to 1 if you have the <sys/param.h> header file. */
#mesondefine HAVE_SYS_PARAM_H

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#mesondefine HAVE_SYS_STATVFS_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#mesondefine HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/time.h> header file. */
#mesondefine HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#mesondefine HAVE_SYS_TYPES_H

/* Define to 1 if you have the <sys/uio.h> header file. */
#mesondefine HAVE_SYS_UIO_H

/* Define to 1 if you have the <sys/vfs.h> header file. */
#mesondefine HAVE_SYS_VFS_H

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#mesondefine HAVE_SYS_WAIT_H

/* Define to 1 if you have the <sys/xattr.h> header file. */
#mesondefine HAVE_SYS_XATTR_H

/* Define to 1 if you have the <termios.h> header file. */
#mesondefine HAVE_TERMIOS_H

/* define for timeout_id_t */
#mesondefine HAVE_TIMEOUT_ID_T

/* Define to 1 if you have the <time.h> header file. */
#mesondefine HAVE_TIME_H

/* Whether UCS-2-INTERNAL is supported */
#mesondefine HAVE_UCS2INTERNAL

/* Define to 1 if you have the <ufs/quota.h> header file. */
#mesondefine HAVE_UFS_QUOTA_H

/* Define to 1 if you have the <unistd.h> header file. */
#mesondefine HAVE_UNISTD_H

/* Define to 1 if you have the 'unlinkat' function. */
#mesondefine HAVE_UNLINKAT

/* Whether to use native iconv */
#mesondefine HAVE_USABLE_ICONV

/* Define to 1 if you have the <utime.h> header file. */
#mesondefine HAVE_UTIME_H

/* Define to 1 if 'utime(file, NULL)' sets file's timestamp to the present. */
#mesondefine HAVE_UTIME_NULL

/* Define to 1 if you have the <varargs.h> header file. */
#mesondefine HAVE_VARARGS_H

/* Define to 1 if you have the 'vsnprintf' function. */
#mesondefine HAVE_VSNPRINTF

/* Define to 1 if you have the 'waitpid' function. */
#mesondefine HAVE_WAITPID

/* Define as const if the declaration of iconv() needs const. */
#mesondefine ICONV_CONST

/* Define to the extension used for runtime loadable modules, say, ".so". */
#mesondefine LT_MODULE_EXT

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#mesondefine LT_OBJDIR

/* Define to the shared library suffix, say, ".dylib". */
#mesondefine LT_SHARED_EXT

/* Define to the shared archive member specification, say "(shr.o)". */
#mesondefine LT_SHARED_LIB_MEMBER

/* Define to 1 if 'major', 'minor', and 'makedev' are declared in <mkdev.h>.
   */
#mesondefine MAJOR_IN_MKDEV

/* Define to 1 if 'major', 'minor', and 'makedev' are declared in
   <sysmacros.h>. */
#mesondefine MAJOR_IN_SYSMACROS

/* Define to 1 if musl C library is present. */
#mesondefine MUSL

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

/* Define if the OpenSSL DHX modules should be built */
#mesondefine OPENSSL_DHX

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

/* Whether the realpath function allows NULL */
#mesondefine REALPATH_TAKES_NULL

/* Define as the return type of signal handlers (`int' or `void'). */
#mesondefine RETSIGTYPE

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

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#mesondefine STDC_HEADERS

/* Define if TCP wrappers should be used */
#mesondefine TCPWRAP

/* Define to 1 if your <sys/time.h> declares 'struct tm'. */
#mesondefine TM_IN_SYS_TIME

/* Define if the DHX UAM modules should be compiled */
#mesondefine UAM_DHX

/* Define if the DHX2 UAM modules should be compiled */
#mesondefine UAM_DHX2

/* Define if the PGP UAM module should be compiled */
#mesondefine UAM_PGP

/* Define if cracklib should be used */
#mesondefine USE_CRACKLIB

/* Define to enable PAM support */
#mesondefine USE_PAM

/* Define to enable SLP support */
#mesondefine USE_SRVLOC

/* Define to enable Zeroconf support */
#mesondefine USE_ZEROCONF

/* Version number of package */
#mesondefine VERSION

/* Whether sendfile() should be used */
#mesondefine WITH_SENDFILE

/* Define if the WolfSSL DHX modules should be built */
#mesondefine WOLFSSL_DHX

/* xattr functions have additional options */
#mesondefine XATTR_ADD_OPT

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

/* Define to empty if 'const' does not conform to ANSI C. */
#mesondefine const

/* Define as 'int' if <sys/types.h> doesn't define. */
#mesondefine gid_t

/* WolfSSL configuration */
#include <wolfssl/options.h>

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#mesondefine inline
#endif

/* Define to 'int' if <sys/types.h> does not define. */
#mesondefine mode_t

/* Define to 'long int' if <sys/types.h> does not define. */
#mesondefine off_t

/* Define as a signed integer type capable of holding a process identifier. */
#mesondefine pid_t

/* Define as 'unsigned int' if <stddef.h> doesn't define. */
#mesondefine size_t

/* Define as 'int' if <sys/types.h> doesn't define. */
#mesondefine uid_t
