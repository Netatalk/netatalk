project(
    'netatalk',
    'c',
    version: '2.4.3dev',
    license: 'GPLv2',
    default_options: ['warning_level=3', 'c_std=c11'],
    meson_version: '>=0.61.2',
    # Reserved for future use:
    # meson_version: '>=0.62.0',
)

cc = meson.get_compiler('c')

# Reserved for future use:
# cc.has_function_attribute('unused', required: false)

####################
# Global variables #
####################

cdata = configuration_data()
fs = import('fs')
prefix = get_option('prefix')
netatalk_version = meson.project_version()
cpu = host_machine.cpu_family()
host_os = host_machine.system()
target_os = target_machine.system()

#########
# Paths #
#########

prefix = get_option('prefix')
exec_prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
includedir = prefix / get_option('includedir')
libdir = prefix / get_option('libdir')
libexecdir = prefix / get_option('libexecdir')
localstatedir = prefix / get_option('localstatedir')
mandir = prefix / get_option('mandir')
pkgconfdir = prefix / get_option('sysconfdir') / 'netatalk'
sbindir = prefix / get_option('sbindir')

##################
# Compiler flags #
##################

# Global

netatalk_common_flags = [
    '-DHAVE_CONFIG_H',
    '-D_U_=__attribute__((unused))',
    '-Wno-pedantic',
    '-Wno-extra',
    '-Wno-all',
    '-Wno-deprecated-declarations',
]

if host_os == 'linux'
    netatalk_common_flags += '-D_GNU_SOURCE'
endif

add_global_arguments(netatalk_common_flags, language: 'c')

netatalk_common_link_args = []

# Per target

ad = '-D_PATH_AD="' + bindir + '/ad"'
acl_ldapconf = '-D_PATH_ACL_LDAPCONF="' + pkgconfdir + '/afp_ldap.conf"'
afpd = '-D_PATH_AFPD="' + sbindir + '/afpd"'
afpdconf = '-D_PATH_AFPDCONF="' + pkgconfdir + '/afpd.conf"'
afpddefvol = '-D_PATH_AFPDDEFVOL="' + pkgconfdir + '/AppleVolumes.default"'
afpdpwfile = '-D_PATH_AFPDPWFILE="' + pkgconfdir + '/afppasswd"'
afpdsigconf = '-D_PATH_AFPDSIGCONF="' + pkgconfdir + '/afp_signature.conf"'
afpdsysvol = '-D_PATH_AFPDSYSVOL="' + pkgconfdir + '/AppleVolumes.system"'
afpduuidconf = '-D_PATH_AFPDUUIDCONF="' + pkgconfdir + '/afp_voluuid.conf"'
cnid_dbd = '-D_PATH_CNID_DBD="' + sbindir + '/cnid_dbd"'
cnid_metad = '-D_PATH_CNID_METAD="' + sbindir + '/cnid_metad"'
confdir = '-D_PATH_CONFDIR="' + pkgconfdir + '/"'
dversion = '-DVERSION="' + netatalk_version + '"'
messagedir = '-DSERVERTEXT="' + localstatedir + '/netatalk/msg/"'
statedir = '-D_PATH_STATEDIR="' + localstatedir + '/netatalk/"'
uamdir = '-D_PATH_AFPDUAMPATH="' + libdir + '/netatalk/"'

############
# Includes #
############

if host_os in ['dragonfly', 'freebsd', 'openbsd']
    root_includes = include_directories(
        '.',
        'include',
        'sys',
        'etc/afpd',
        '/usr/local/include',
    )
elif host_os == 'netbsd'
    root_includes = include_directories(
        '.',
        'include',
        'sys',
        'etc/afpd',
        '/usr/pkg/include',
    )
elif host_os == 'omnios'
    root_includes = include_directories(
        '.',
        'include',
        'sys',
        'etc/afpd',
        '/opt/local/include',
    )
else
    root_includes = include_directories(
        '.',
        'include',
        'etc/afpd',
        'sys',
    )
endif

############
# Programs #
############

grep = find_program('grep', required: false)
perl = find_program('perl', required: false)
uname = find_program('uname', required: false)

macos_prefix = ''
if host_os == 'darwin'
    if cpu == 'aarch64'
        macos_prefix += '/opt/homebrew'
    elif cpu == 'x86_64'
        macos_prefix += '/usr/local'
    endif
endif

################
# Dependencies #
################

threads = dependency('threads', required: true)

#############
# Libraries #
#############

libsearch_dirs = []

if host_os in ['dragonfly', 'freebsd', 'openbsd']
    libsearch_dirs += '/usr/local/lib'
endif

if host_os == 'netbsd'
    libsearch_dirs += '/usr/pkg/lib'
endif

if uname.found() and run_command(uname, '-n', check: false).stdout().strip().contains('omnios')
    libsearch_dirs += '/opt/local/lib'
endif

socket = cc.find_library('socket', required: false)
if socket.found()
    netatalk_common_link_args += '-lsocket'
    cdata.set('HAVE_LIBSOCKET', 1)
endif

sendfile = cc.find_library('sendfile', required: false)
if sendfile.found()
    netatalk_common_link_args += '-lsendfile'
endif

nsl = cc.find_library('nsl', required: false)
if nsl.found()
    netatalk_common_link_args += '-lnsl'
    cdata.set('HAVE_LIBNSL', 1)
endif

dl = cc.find_library('dl', required: false)
if dl.found()
    cdata.set('HAVE_LIBDL', 1)
    netatalk_common_link_args += '-ldl'
endif

math = cc.find_library('m', required: true)
if math.found()
    netatalk_common_link_args += '-lm'
endif

add_global_link_arguments(netatalk_common_link_args, language: 'c')

#################
# Header checks #
#################

header_dir = []
if host_os in ['dragonfly', 'freebsd', 'openbsd']
    header_dir += '/usr/local/include'
endif

check_headers = [
    'acl/libacl.h',
    'attr/xattr.h',
    'crypt.h',
    'dirent.h',
    'dlfcn.h',
    'dns_sd.h',
    'errno.h',
    'fcntl.h',
    'inttypes.h',
    'langinfo.h',
    'limits.h',
    'locale.h',
    'mntent.h',
    'mount.h',
    'ndir.h',
    'netdb.h',
    'pam/pam_appl.h',
    'security/pam_appl.h',
    'sgtty.h',
    'statfs.h',
    'stdarg.h',
    'stdint.h',
    'stdio.h',
    'stdlib.h',
    'strdarg.h',
    'string.h',
    'strings.h',
    'sys/attributes.h',
    'sys/dir.h',
    'sys/ea.h',
    'sys/extattr.h',
    'sys/file.h',
    'sys/filio.h',
    'sys/ioctl.h',
    'sys/mnttab.h',
    'sys/mount.h',
    'sys/ndir.h',
    'sys/param.h',
    'sys/stat.h',
    'sys/statvfs.h',
    'sys/time.h',
    'sys/types.h',
    'sys/uio.h',
    'sys/vfs.h',
    'sys/wait.h',
    'sys/xattr.h',
    'syslog.h',
    'termios.h',
    'time.h',
    'ufs/quota.h',
    'unistd.h',
    'utime.h',
    'varargs.h',
]

foreach h : check_headers
    if cc.has_header(h, include_directories: include_directories(header_dir))
        cdata.set('HAVE_' + h.underscorify().to_upper(), 1)
    endif
endforeach

###################
# Function checks #
###################

check_functions = [
    'access',
    'asprintf',
    'backtrace_symbols',
    'chmod',
    'chown',
    'chroot',
    'dirfd',
    'dlclose',
    'dlerror',
    'dlopen',
    'dlsym',
    'errno',
    'fchmod',
    'fchown',
    'getcwd',
    'gethostname',
    'getpagesize',
    'gettimeofday',
    'getusershell',
    'link',
    'memcpy',
    'mkdir',
    'mknod',
    'mknod64',
    'mmap',
    'nl_langinfo',
    'pread',
    'pselect',
    'pwrite',
    'quad_t',
    'rmdir',
    'select',
    'setlinebuf',
    'setlocale',
    'shl_load',
    'snprintf',
    'socket',
    'strcasestr',
    'strlcat',
    'strlcpy',
    'strnlen',
    'strndup',
    'strstr',
    'strtoul',
    'utime',
    'vasprintf',
    'vsnprintf',
    'waitpid',
]

foreach f : check_functions
    if cc.has_function(f)
        cdata.set('HAVE_' + f.underscorify().to_upper(), 1)
    endif
endforeach

if cc.has_function('getifaddrs', dependencies: socket)
    cdata.set('HAVE_GETIFADDRS', 1)
endif

at_functions = [
    'openat',
    'renameat',
    'fstatat',
    'unlinkat',
]

foreach f : at_functions
    if cc.has_function(f)
        have_atfuncs = true
        cdata.set('HAVE_' + f.underscorify().to_upper(), 1)
        cdata.set('_ATFILE_SOURCE', 1)
        cdata.set('HAVE_ATFUNCS', 1)
    endif
endforeach

if cc.compiles(
    '''
#include <sys/types.h>
#include <signal.h>
int main (void)
    {
        return *(signal (0, 0)) (0) == 1;
    ;
    return 0;
    }
''',
)
    cdata.set('RETSIGTYPE', 'int')
else
    cdata.set('RETSIGTYPE', 'void')
endif

##########################
# Type and member checks #
##########################

if cc.has_type('fshare_t', prefix: '#include <fcntl.h>')
    cdata.set('HAVE_FSHARE_T', 1)
endif
if cc.has_member('struct tm', 'tm_gmtoff', prefix: '#include <time.h>')
    cdata.set('HAVE_STRUCT_TM_TM_GMTOFF', 1)
endif

#################
# Configuration #
#################

#
# Check whether to enable rpath (the default on Solaris and NetBSD)
#

if host_os == 'sunos' or host_os == 'netbsd'
    enable_rpath = true
else
    enable_rpath = get_option('with-rpath')
endif

if host_os == 'linux'
    enable_dtags = true
else
    enable_dtags = false
endif

#
# Check for the Berkeley DB library
#

bdb_path = get_option('with-bdb-path')

bdb_header = ''
bdb_includes = []
bdb_libdir = ''
bdb_link_args = []
bdb_major_version = ''
bdb_minor_version = ''
bdb_version = ''

bdb_dirs = [
    '/usr/local',
    '/usr/pkg',
    '/opt/local',
    macos_prefix / 'opt/berkeley-db',
    '/usr',
]
bdb_subdirs = [
    'db4.6',
    'db4.7',
    'db4.8',
    'db4',
    'db46',
    'db47',
    'db48',
    'db5.0',
    'db5.1',
    'db5.2',
    'db5.3',
    'db5',
    'db50',
    'db51',
    'db6.1',
    '',
]

if bdb_path != ''
    foreach subdir : bdb_subdirs
        if fs.exists(bdb_path / 'include' / subdir / 'db.h')
            bdb_header += bdb_path / 'include' / subdir / 'db.h'
            bdb_libdir += bdb_path / 'lib'
            bdb_includes += include_directories(
                bdb_path / 'include' / subdir,
            )
        endif
        if bdb_header != ''
            break
        endif
    endforeach
else
    foreach dir : bdb_dirs
        foreach subdir : bdb_subdirs
            if fs.exists(dir / 'include' / subdir / 'db.h')
                bdb_header += dir / 'include' / subdir / 'db.h'
                bdb_libdir += dir / 'lib'
                bdb_includes += include_directories(
                    dir / 'include' / subdir,
                )
            endif
            if bdb_header != ''
                break
            endif
        endforeach
        if bdb_header != ''
            break
        endif
    endforeach
endif

if bdb_header != ''
    have_bdb = true
    bdb_major_version += run_command(
        'grep',
        'DB_VERSION_MAJOR',
        bdb_header,
        check: true,
    ).stdout().strip().substring(25)

    bdb_minor_version += run_command(
        'grep',
        'DB_VERSION_MINOR',
        bdb_header,
        check: true,
    ).stdout().strip().substring(25)

    bdb_version += bdb_major_version + '.' + bdb_minor_version
    bdb_minimum_version = bdb_version.version_compare('>=4.6')

    bdb_libnames = [
        'db',
        'db' + bdb_major_version,
        'db' + bdb_major_version + '.' + bdb_minor_version,
        'db' + bdb_major_version + bdb_minor_version,
        'db-' + bdb_major_version,
        'db-' + bdb_major_version + '.' + bdb_minor_version,
    ]

    foreach name : bdb_libnames
        db = cc.find_library(name, dirs: bdb_libdir, required: false)
        if db.found()
            break
        endif
    endforeach
else
    have_bdb = false
    error(
        'Berkeley DB library required but not found! Please specify an installation path using the -Dwith-bdb= configure option (must include lib and include dirs)',
    )
endif

#
# Check for crypt or SSL (for DHX authentication)
#

have_embedded_ssl = get_option('with-embedded-ssl')
force_embedded_ssl = get_option('with-ssl-override')

ssl_deps = []
ssl_link_args = []
ssl_provider = ''

crypt = cc.find_library('crypt', has_headers: 'crypt.h', required: false)
if crypt.found()
    cdata.set('HAVE_CRYPT_H', 1)
    cdata.set('HAVE_LIBCRYPT', 1)
endif

crypto = dependency('libcrypto', required: false)
libtls = dependency('libtls', required: false)

wolfssl = dependency('wolfssl', required: false)

if wolfssl.found()
    wolfssl_check = run_command(
        find_program('cat', required: false),
        wolfssl.get_variable(pkgconfig: 'includedir') / 'wolfssl/options.h',
        check: false,
    ).stdout().strip()

    if (
        wolfssl_check.contains('HAVE_DH_DEFAULT_PARAMS')
        and wolfssl_check.contains('WOLFSSL_DES_ECB')
        and wolfssl_check.contains('OPENSSL_EXTRA')
        and wolfssl_check.contains('OPENSSL_ALL')
    )
        have_wolfssl = true
        have_embedded_ssl = false
    else
        have_wolfssl = false
        message(
            'The WolfSSL package on this system does not have the correct configuration for DHX and RANDNUM support'
        )
    endif
else
    have_wolfssl = false
endif

if ((have_wolfssl or have_embedded_ssl) and crypto.found()) or crypto.found()
    have_ssl = true
    cdata.set('HAVE_LIBCRYPTO', 1)
    cdata.set('UAM_DHX', 1)
endif

if have_wolfssl
    if force_embedded_ssl
        ssl_deps += crypto
        ssl_provider += 'built-in'
        cdata.set('EMBEDDED_SSL', 1)
    else
        ssl_deps += [crypto, wolfssl]
        ssl_provider += 'WolfSSL'
        cdata.set('WOLFSSL_DHX', 1)
    endif
elif have_embedded_ssl
    ssl_deps += crypto
    ssl_provider += 'built-in'
    cdata.set('EMBEDDED_SSL', 1)
elif crypto.found()
    ssl_deps += crypto
    cdata.set('OPENSSL_DHX', 1)
    if libtls.found()
        ssl_provider += 'LibreSSL'
    elif not libtls.found()
        ssl_provider += 'OpenSSL'
    endif
else
    have_ssl = false
    warning(
        'Built-in WolfSSL, LibreSSL or OpenSSL version 1.1 is required for DHX and RANDNUM support',
    )
endif

#
# Check whether Kerberos V UAM can be built
#

enable_krbV_uam = get_option('with-krbV-uam')

if not enable_krbV_uam
    have_krb5_uam = false
    have_gssapi = false
else
    #
    # Check for GSSAPI
    #

    with_gssapi = get_option('with-gssapi')
    gssapi_path = get_option('with-gssapi-path')
    gssapi_includes = []
    gssapi_headers = [
        'gssapi.h',
        'gssapi/gssapi_generic.h',
        'gssapi/gssapi_krb5.h',
        'gssapi/gssapi.h',
    ]

    foreach header : gssapi_headers
        if cc.has_header(
            header,
            include_directories: include_directories(header_dir),
        )
            cdata.set('HAVE_' + header.underscorify().to_upper(), 1)
        endif

    endforeach

    if not get_option('with-gssapi')
        have_gssapi = false
    elif get_option('with-gssapi-path') != ''
        have_gssapi = true
        gss = cc.find_library(
            'gssapi',
            dirs: gssapi_path / 'lib',
            required: false,
        )
        gssapi_includes += include_directories(gssapi_path / 'include')

    else
        gss = cc.find_library('gss', dirs: libsearch_dirs, required: false)
        if gss.found()
            cdata.set('HAVE_LIBGSS', 1)
        else
            gss = cc.find_library('gssapi', dirs: libsearch_dirs, required: false)
            if gss.found()
                cdata.set('HAVE_LIBGSSAPI', 1)
            else
                gss = cc.find_library('gssapi_krb5', dirs: libsearch_dirs, required: false)
                if gss.found()
                    cdata.set('HAVE_LIBGSSAPI_KRB5', 1)
                else
                    have_gssapi = false
                endif
            endif
        endif
        have_gssapi = (
            gss.found()
            and cc.has_function('gss_acquire_cred', dependencies: gss)
        )
        if have_gssapi
            # Heimdal/MIT compatibility fix
            foreach header : gssapi_headers
                if cc.has_header_symbol(header, 'GSS_C_NT_HOSTBASED_SERVICE')
                    cdata.set('HAVE_GSS_C_NT_HOSTBASED_SERVICE', 1)
                endif
            endforeach
            cdata.set('HAVE_GSSAPI', 1)
        endif
    endif

    if with_gssapi and not have_gssapi
        warning('GSSAPI support requested but GSSAPI library not found')
    endif
    enable_krbV_uam = have_gssapi
    if enable_krbV_uam and not have_gssapi
        warning('Need GSSAPI support to build Kerberos V UAM')
    endif
endif

#
# Check for optional Zeroconf support
#

enable_zeroconf = get_option('with-zeroconf')

avahi = dependency('avahi-client', required: false)

dns_sd_libs = []

dns_sd = cc.find_library('dns_sd', required: false)
if dns_sd.found()
    dns_sd_libs += dns_sd
endif

system = cc.find_library('system', required: false)
if dns_sd.found()
    dns_sd_libs += system
endif

zeroconf_provider = ''
freebsd_zeroconf_daemon = ''

have_dns = (
    (dns_sd.found() or system.found()) and cc.has_header('dns_sd.h')
    and cc.has_function(
        'DNSServiceRegister',
        dependencies: dns_sd_libs,
    )
)

if not enable_zeroconf
    have_zeroconf = false
else
    have_zeroconf = have_dns
    if have_dns
        cdata.set('USE_ZEROCONF', 1)
        cdata.set('HAVE_MDNS', 1)
        freebsd_zeroconf_daemon = 'mdnsd'
        zeroconf_provider += 'mDNS'
    else
        have_zeroconf = avahi.found()
        if avahi.found()
            cdata.set('USE_ZEROCONF', 1)
            cdata.set('HAVE_AVAHI', 1)
            if avahi.version() >= '0.6.4'
                cdata.set('HAVE_AVAHI_THREADED_POLL', 1)
            endif
            freebsd_zeroconf_daemon = 'avahi_daemon'
            zeroconf_provider += 'Avahi'
        else
            have_zeroconf = false
        endif
    endif
endif

if enable_zeroconf and not have_zeroconf
    warning(
        'Zeroconf (Bonjour) support requested but required libraries not found',
        'Please install Avahi or mDNS',
    )
endif

#
# Check for CUPS
#

cups = dependency('cups', required: false)
spooldir = ''
cups_config = find_program('cups-config', required: false)

if not (get_option('with-ddp') or get_option('with-cups'))
    have_cups = false
    spooldir_required = false
    cdata.set('DISABLE_SPOOL', 1)
else
    have_cups = cups.found() and cups_config.found()
    if have_cups
        spooldir_required = true
        cdata.set('HAVE_CUPS', 1)
        cups_api_version = run_command(
            cups_config,
            '--api-version',
            check: true,
        ).stdout().strip()
        cdata.set('CUPS_API_VERSION', '"' + cups_api_version + '"')
        if get_option('with-spooldir') != ''
            spooldir += get_option('with-spooldir')
        else
            spooldir += prefix / 'var/spool/netatalk'
        endif
    else
        warning('CUPS not found, you might need to specify the path to cups-config')
    endif
endif

#
# Check for quota support
#

enable_quota = get_option('with-quota')

prop = cc.find_library('prop', required: false)
quota = cc.find_library('quota', required: false)
rpcsvc = cc.find_library('rpcsvc', required: false)
tirpc = dependency('libtirpc', required: false)

quota_deps = []
quota_provider = ''

rpc_headers = [
    'rpc/rpc.h',
    'rpc/pmap_prot.h',
    'rpcsvc/rquota.h',
]

rpc_headers_ok = (
    cc.has_header('rpc/rpc.h')
    and cc.has_header('rpc/pmap_prot.h')
    and cc.has_header('rpcsvc/rquota.h')
)

if rpc_headers_ok
    foreach header : rpc_headers
        cdata.set('HAVE_' + header.underscorify().to_upper(), 1)
    endforeach
endif

if not enable_quota
    have_quota = false
    cdata.set('NO_QUOTA_SUPPORT', 1)
else
    have_quota = tirpc.found() and cc.has_header('rpcsvc/rquota.h')
    if have_quota
        cdata.set('NEED_RQUOTA', 1)
        quota_deps += tirpc
        quota_provider += 'libtirpc'
        if cc.compiles(
            '''
                    #include <rpcsvc/rquota.h>
                    int main(void) {
                        enum qr_status foo;
                        foo = Q_OK;
                        return 0;
                    }
                ''',
            dependencies: tirpc,
        )
            cdata.set('HAVE_RQUOTA_H_QR_STATUS', 1)
        endif
    else
        have_quota = rpcsvc.found() and rpc_headers_ok
        if have_quota
            quota_deps += rpcsvc
            quota_provider += 'SunRPC'
            if quota.found() and cc.has_function('getfsquota', dependencies: [quota, prop, rpcsvc])
                quota_deps += [quota, prop]
                cdata.set('HAVE_LIBQUOTA', 1)
            endif
        else
            have_quota = false
            cdata.set('NO_QUOTA_SUPPORT', 1)
        endif
    endif
endif

if enable_quota and not have_quota
    warning(
        'Quota support requested but required libtirpc or libquota libraries not found',
    )
endif

#
# Check for libgcrypt
#

libgcrypt_path = get_option('with-libgcrypt-path')

libgcrypt_link_args = []

if libgcrypt_path != ''
    libgcrypt_link_args += ['-L' + libgcrypt_path / 'lib', '-lgcrypt']
    if enable_rpath
        libgcrypt_link_args += ['-R' + libgcrypt_path / 'lib']
    endif
    libgcrypt = declare_dependency(
        link_args: libgcrypt_link_args,
        include_directories: include_directories(libgcrypt_path / 'include'),
    )
else
    libgcrypt = dependency('libgcrypt', version : '>=1.2.3', required: false)
    if not libgcrypt.found()
        libgcrypt = cc.find_library(
            'libgcrypt',
            dirs: libsearch_dirs,
            required: false,
        )
    endif
endif

have_libgcrypt = libgcrypt.found()

if have_libgcrypt
    cdata.set('HAVE_LIBGCRYPT', 1)
    cdata.set('UAM_DHX2', 1)
else
    have_libgcrypt = false
    if not libgcrypt.found()
        error(
            'Libgcrypt library required for DHX2 UAM! Please install version 1.2.3 or later',
        )
    endif
endif

#
# Check for Extended Attributes support
#

ea = '"ad'
attr = cc.find_library('attr', required: false)

if host_os == 'sunos'
    if cc.has_function('attropen')
        have_ea = true
        cdata.set('HAVE_ATTROPEN', 1)
    else
        have_ea = false
    endif

elif host_os == 'freebsd'
    have_ea = true
    ea_functions = [
        'extattr_delete_fd',
        'extattr_delete_file',
        'extattr_delete_link',
        'extattr_get_fd',
        'extattr_get_file',
        'extattr_get_link',
        'extattr_list_fd',
        'extattr_list_file',
        'extattr_list_link',
        'extattr_set_fd',
        'extattr_set_file',
        'extattr_set_link',
    ]
    foreach f : ea_functions
        if cc.has_function(f)
            define = 'HAVE_' + f.underscorify().to_upper()
            cdata.set(define, 1)
        endif
    endforeach
elif host_os == 'dragonfly'
    have_ea = false
    cdata.set('BROKEN_EXTATTR', 1)
elif host_os == 'aix'
    have_ea = true
    ea_functions = [
        'fgetea',
        'flistea',
        'fremoveea',
        'fsetea',
        'getea',
        'lgetea',
        'listea',
        'llistea',
        'lremoveea',
        'lsetea',
        'removeea',
        'setea',
    ]
    foreach f : ea_functions
        if cc.has_function(f)
            define = 'HAVE_' + f.underscorify().to_upper()
            cdata.set(define, 1)
        endif
    endforeach

elif cc.has_function('getxattr', dependencies: attr)
    have_ea = true
    ea_functions = [
        'fgetxattr',
        'flistxattr',
        'fremovexattr',
        'fsetxattr',
        'getxattr',
        'lgetxattr',
        'listxattr',
        'llistxattr',
        'lremovexattr',
        'lsetxattr',
        'removexattr',
        'setxattr',
    ]
    foreach f : ea_functions
        if cc.has_function(f)
            define = 'HAVE_' + f.underscorify().to_upper()
            cdata.set(define, 1)
        endif
    endforeach
else
    have_ea = false
endif

if have_ea
    netatalk_ea = ea + '|sys"'
else
    netatalk_ea = ea + '"'
endif

cdata.set('EA_MODULES', netatalk_ea)

# Do xattr functions take additional options like on Darwin?

if cc.compiles(
    '''
#include <sys/types.h>
#include <sys/xattr.h>
int main(void) {
  getxattr(0, 0, 0, 0, 0, 0);
  return 0;
}
''',
)
    cdata.set('XATTR_ADD_OPT', 1)
endif

#
# Check for building PGP UAM
#

enable_pgp_uam = get_option('with-pgp-uam')

if not enable_pgp_uam
    have_pgp_uam = false
else
    have_pgp_uam = have_ssl
    if have_pgp_uam
        cdata.set('UAM_PGP', 1)
    else
        have_pgp_uam = false
        warning(
            'Embedded SSL, LibreSSL or OpenSSL version 1.1 is required to build PGP UAM',
        )
    endif
endif

#
# Check for ACL support
#

enable_acls = get_option('with-acls')

acl_deps = []
acl_includes = []
acl_link_args = []

if not enable_acls
    have_acls = false
elif host_os == 'darwin'
    have_acls = false
    warning('Darwin ACLs are currently unsupported')
elif host_os.contains('sunos')
    have_acls = true
    sec = cc.find_library('sec', required: false)
    acl_deps += sec
    cdata.set('HAVE_ACLS', 1)
    cdata.set('HAVE_SOLARIS_ACLS', 1)
else
    acl = cc.find_library('acl', required: false)

    if acl.found()
        acl_link_args += '-lacl'
    endif

    if cc.has_function('acl_get_file', dependencies: acl)
        acl_deps += acl
    endif

    if host_os == 'linux'
        if cc.has_function('getxattr', dependencies: attr)
            acl_deps += attr
        endif
    endif

    # Check for POSIX ACL support

    acl_get_entry_code = '''
        #include <sys/types.h>
        #include <sys/acl.h>
        int main(void) {
            acl_t acl;
            int entry_id;
            acl_entry_t *entry_p;
            return acl_get_entry(acl, entry_id, entry_p);
        }
    '''
    have_acls = cc.links(acl_get_entry_code, args: acl_link_args)
    if have_acls
        cdata.set('HAVE_ACLS', 1)
        cdata.set('HAVE_POSIX_ACLS', 1)
        acl_get_perm_np_code = '''
            #include <sys/types.h>
            #include <sys/acl.h>
            int main(void) {
                acl_permset_t permset_d;
                acl_perm_t perm;
                return acl_get_perm_np(permset_d, perm);
            }
        '''
        if cc.links(acl_get_perm_np_code, args: acl_link_args)
            cdata.set('HAVE_ACL_GET_PERM_NP', 1)
        endif
        if cc.has_function('acl_from_mode', dependencies: acl)
            cdata.set('HAVE_ACL_FROM_MODE', 1)
        endif
    else
        have_acls = false
    endif

   if enable_acls and not have_acls
        warning('ACL support requested but not found')
    endif
endif

#
# Check for LDAP support, for client-side ACL visibility
#

enable_ldap = get_option('with-ldap')
ldap_path = get_option('with-ldap-path')

ldap_link_args = []

if ldap_path != ''
    ldap_link_args += ['-L' + ldap_path / 'lib', '-lldap', ]
    if enable_rpath
        ldap_link_args += ['-R' + ldap_path / 'lib']
    endif
    ldap = declare_dependency(
        link_args: ldap_link_args,
        include_directories: include_directories(ldap_path / 'include'),
    )
else
    ldap = cc.find_library('ldap', dirs: libsearch_dirs, required: false)
endif

if not enable_ldap
    have_ldap = false
else
    have_ldap = ldap.found() and cc.has_function('ldap_initialize', dependencies: ldap)
    if have_ldap
        cdata.set('HAVE_LDAP', 1)
    else
        have_ldap = false
    endif
endif

if enable_ldap and not have_ldap
    warning('LDAP support requested but LDAP library not found')
endif

#
# Check if realpath() takes NULL
#

if cc.compiles(
    '''
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>

void exit_on_core(int ignored) {
    exit(1);
}

int main(void) {
    char *newpath;
    signal(SIGSEGV, exit_on_core);
    newpath = realpath("/tmp", NULL);
    exit((newpath != NULL) ? 0 : 1);
}
''',
)
    cdata.set('REALPATH_TAKES_NULL', 1)
endif

#
# Check for iconv support
#

enable_iconv = get_option('with-libiconv')
iconv_path = get_option('with-libiconv-path')

iconv = dependency('iconv', required: false)
libiconv = cc.find_library('iconv', dirs: libsearch_dirs, required: false)

libiconv_link_args = []

if iconv_path != ''
    libiconv_link_args += ['-L' + iconv_path / 'lib', '-liconv']
    if enable_rpath
        libiconv_link_args += ['-R' + iconv_path / 'lib']
    endif
    iconv = declare_dependency(
        link_args: libiconv_link_args,
        include_directories: include_directories(with_libiconv / 'include'),
    )
endif

if not enable_iconv
    have_iconv = false
else
    have_iconv = (
        (iconv.found() or libiconv.found())
        and cc.compiles(
            '''
    #include <iconv.h>
    int main(void) {
        iconv_t cd = iconv_open("ASCII", "UTF-8");
        if (cd == 0 || cd == (iconv_t)-1) return -1;
    return 0;
    }
    ''',
        )
    )
    if have_iconv
        cdata.set('HAVE_USABLE_ICONV', 1)

        if cc.compiles(
            '''
        #include <stdlib.h>
        #include <iconv.h>
        extern
        #ifdef __cplusplus
            "C"
        #endif
        #if defined(__STDC__) || defined(__cplusplus)
            size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
        #else
            size_t iconv();
        #endif
        ''',
        )
            cdata.set('ICONV_CONST', '')
        else
            cdata.set('ICONV_CONST', 'const')
        endif

        if cc.run(
            '''
        #include <iconv.h>
        int main(void) {
            iconv_t cd = iconv_open("ASCII", "UCS-2-INTERNAL");
            if (cd == 0 || cd == (iconv_t)-1) return -1;
            return 0;
        }
        ''',
        ).returncode() == 0
            cdata.set('HAVE_UCS2INTERNAL', 1)
        endif
    else
        have_iconv = false
        warning(
            'iconv support requested but required libraries not found',
            'Please install libiconv',
        )
    endif
endif

#
# Check whether BDB daemon needs to be compiled
#

cnid_backends = ''

# Determine whether or not to use Database Daemon CNID backend

if not get_option('with-cnid-dbd-backend')
    use_dbd_backend = false
else
    use_dbd_backend = true
    cnid_backends += 'dbd'
    cdata.set('CNID_BACKEND_DBD', 1)
endif

# Determine whether or not to use LAST DID scheme

if not get_option('with-cnid-last-backend')
    use_last_backend = false
else
    use_last_backend = true
    cnid_backends += ' last'
    cdata.set('CNID_BACKEND_LAST', 1)
endif

compiled_backends = '"' + cnid_backends + '"'
cdata.set('compiled_backends', compiled_backends)

summary_backends = cnid_backends

# Determine default CNID backend

default_backend = get_option('with-cnid-default-backend')
cdata.set('DEFAULT_CNID_SCHEME', '"' + default_backend + '"')

if default_backend == 'dbd' and not use_dbd_backend
    error('Specified default CNID scheme dbd was not selected for compilation')
elif default_backend == 'last' and not use_last_backend
    error('Specified default CNID scheme last was not selected for compilation')
endif

#
# Check for docbook
#

docbook_path = get_option('with-docbook-path')
compile_manual = get_option('with-manual')

xsltproc = find_program('xsltproc', required: false)
xsl = []
docbook_root = ''
docbook_xsl_dirs = [
    docbook_path,
    '/usr/share/sgml/docbook/xsl-stylesheets',
    '/usr/share/xml/docbook/stylesheet/docbook-xsl',
    '/opt/local/share/xsl/docbook',
    '/usr/local/share/xsl/docbook',
    '/usr/pkg/share/xsl/docbook',
]

if host_os == 'darwin'
    docbook_xsl_dirs += macos_prefix / 'opt/docbook-xsl/docbook-xsl'
endif

if not compile_manual
    build_xml_docs = false
else
    foreach dir : docbook_xsl_dirs
        foreach format : ['html', 'manpages']
            if fs.exists(dir / format / 'docbook.xsl')
                build_xml_docs = true
                docbook_root += dir
                xsl += dir / format / 'docbook.xsl'
                if (
                    run_command(
                        [xsltproc, '--nonet', xsl],
                        check: false,
                    ).returncode() == 0
                )
                    cdata.set('DOCBOOK_ROOT', dir)
                endif
            endif
            if docbook_root != ''
                break
            else
                continue
            endif
        endforeach
    endforeach
endif

docbook_ok = xsltproc.found() and docbook_root != ''

if compile_manual and not docbook_ok
    error(
        'xsltproc and docbook-xsl stylesheets are required to compile manpages and html documentation',
    )
endif

#
# Test for PAM support
#

enable_pam = get_option('with-pam')
pam_path = get_option('with-pam-path')
pam_conf_path = get_option('with-pam-config-path')

pam_dir = ''
pam_confdir = ''
pam_includes = []
pam_link_args = []
uams_options = ''

if not enable_pam
    have_pam = false
else
    if host_os != 'sunos'
        pam_paths = [
            '/',
            '/usr',
            '/usr/local',
        ]

        foreach path : pam_paths
            if fs.is_dir(path / 'etc/pam.d')
                pam_dir += path
            endif
            break
        endforeach
    else
        warning(
            'PAM installation file = /etc/pam.conf. Please edit this file to enable PAM support',
        )
    endif

    if pam_path != '' and pam_dir != '/'
        pam_link_args += ['-L' + pam_path / 'lib', '-lpam']
        if enable_rpath
            pam_link_args += ['-R' + pam_path / 'lib']
        endif
        pam = declare_dependency(
            link_args: pam_link_args,
            include_directories: include_directories(pam_path / 'include'),
        )
    else
        pam = cc.find_library('pam', dirs: libsearch_dirs, required: false)
    endif

    have_pam = (
        cc.has_header('security/pam_appl.h')
        or cc.has_header('pam/pam_appl.h')
        or cc.has_function('pam_set_item', dependencies: pam)
    )

    if have_pam
        cdata.set('USE_PAM', 1)
        uams_options += 'PAM'
        pampath = pam_dir / 'etc/pam.d'
        # Debian/SuSE
        if fs.exists(pampath / 'common-auth')
            cdata.set('PAM_DIRECTIVE', 'include')
            cdata.set('PAM_AUTH', 'common-auth')
            cdata.set('PAM_ACCOUNT', 'common-account')
            cdata.set('PAM_PASSWORD', 'common-password')
            if fs.exists(pampath / 'common-session-noninteractive')
                cdata.set('PAM_SESSION', 'common-session-noninteractive')
            elif fs.exists(pampath / 'common-session-nonlogin')
                cdata.set('PAM_SESSION', 'common-session-nonlogin')
            else
                cdata.set('PAM_SESSION', 'common-session')
            endif
            # RHEL/FC
        elif fs.exists(pampath / 'system-auth')
            cdata.set('PAM_DIRECTIVE', 'include')
            cdata.set('PAM_AUTH', 'system-auth')
            cdata.set('PAM_ACCOUNT', 'system-auth')
            cdata.set('PAM_PASSWORD', 'system-auth')
            cdata.set('PAM_SESSION', 'system-auth')
            # FreeBSD
        elif fs.exists(pampath / 'system')
            cdata.set('PAM_DIRECTIVE', 'include')
            cdata.set('PAM_AUTH', 'system')
            cdata.set('PAM_ACCOUNT', 'system')
            cdata.set('PAM_PASSWORD', 'system')
            cdata.set('PAM_SESSION', 'system')
            # macOS
        elif fs.exists(pampath / 'chkpasswd')
            cdata.set('PAM_DIRECTIVE', 'required')
            cdata.set('PAM_AUTH', 'pam_opendirectory.so')
            cdata.set('PAM_ACCOUNT', 'pam_opendirectory.so')
            cdata.set('PAM_PASSWORD', 'pam_permit.so')
            cdata.set('PAM_SESSION', 'pam_permit.so')
            # Solaris 11+
        elif fs.exists(pampath / 'other')
            cdata.set('PAM_DIRECTIVE', 'include')
            cdata.set('PAM_AUTH', pam_dir / 'etc/pam.d/other')
            cdata.set('PAM_ACCOUNT', pam_dir / 'etc/pam.d/other')
            cdata.set('PAM_PASSWORD', pam_dir / 'etc/pam.d/other')
            cdata.set('PAM_SESSION', pam_dir / 'etc/pam.d/other')
            # Fallback
        else
            cdata.set('PAM_DIRECTIVE', 'required')
            cdata.set('PAM_AUTH', 'pam_unix.so')
            cdata.set('PAM_ACCOUNT', 'pam_unix.so')
            cdata.set('PAM_PASSWORD', '"pam_unix.so use_authtok"')
            cdata.set('PAM_SESSION', 'pam_unix.so')
        endif

        if pam_dir == ''
            warning(
                'PAM support can be compiled, but the install location for the netatalk.pamd file could not be determined. Please install this file manually. If you are running a Solaris-based host which still relies on /etc/pam.conf you will have to edit this file to get PAM working',
            )
        endif
    else
        have_pam = false
        warning('PAM support requested but required library not found')
    endif
endif

if pam_conf_path != ''
    pam_confdir += pam_conf_path
else
    pam_confdir += prefix / 'etc/pam.d'
endif

#
# Check for optional shadow password support
#

enable_shadow = get_option('with-shadow')

if not enable_shadow
    have_shadow = false
else
    have_shadow = (cc.has_header('shadow.h'))
    if have_shadow
        cdata.set('SHADOWPW', 1)
        uams_options += ' SHADOW'
    else
        have_shadow = false
        warning('Shadow password support requested but required header not found')
    endif
endif

#
# Check for largefle support
#

enable_largefile = get_option('with-largefile')

if not enable_largefile
    have_largefile = false
else
    # needed for Linux, Solaris...
    file_offset_bits_test = cc.compiles(
        '''
        #if !defined(_FILE_OFFSET_BITS) || (_FILE_OFFSET_BITS != 64)
            #error "Large-file support was not enabled"
        #endif
        ''',
    )

    # Needed for AIX, macOS
    large_files_test = cc.sizeof('off_t') == 8

    have_largefile = file_offset_bits_test or large_files_test
    if have_largefile
        cdata.set('HAVE_LARGEFILE_SUPPORT', 1)
        if file_offset_bits_test
            cdata.set('_FILE_OFFSET_BITS', 64)
        else
            if large_files_test
                cdata.set('_LARGE_FILES', 1)
            endif
        endif
    else
        have_largefile = false
        warning('Largefile support requested but not available on this platform')
    endif
endif

#
# Check for (v)snprintf functions
#

#
# Check for optional initscript install
#

init_dir = ''

sed_command = [
    find_program('sed', required: false),
    '-e', 's@:BINDIR:@' + bindir + '@',
    '-e', 's@:ETCDIR:@' + pkgconfdir + '@',
    '-e', 's@:SBINDIR:@' + sbindir + '@',
    '-e', 's@:PKGLIBEXECDIR:@' + libexecdir + '@',
    '-e', 's@:NETATALK_VERSION:@' + netatalk_version + '@',
    '@INPUT@',
]

#
# Check for cracklib support
#

enable_cracklib = get_option('with-cracklib')
cracklib_path = get_option('with-cracklib-path')

crack = cc.find_library('crack', dirs: libsearch_dirs, required: false)
cracklib_dict = ''

if fs.is_dir('/usr/share/cracklib')
    cracklib_dict += '/usr/share/cracklib/pw_dict'
elif fs.is_dir('/var/cache/cracklib')
    cracklib_dict += '/var/cache/cracklib/cracklib_dict'
elif fs.is_dir('/usr/local/libdata/cracklib')
    cracklib_dict += '/usr/local/libdata/cracklib/cracklib-words'
elif fs.is_dir('/usr/pkg/share/cracklib')
    cracklib_dict += '/usr/pkg/share/cracklib/pw_dict'
elif fs.is_dir('/usr/local/share/cracklib')
    cracklib_dict += '/usr/local/share/cracklib/cracklib-small'
endif

if not enable_cracklib
    have_cracklib = false
else
    have_cracklib = crack.found() and (cracklib_path != '' or cracklib_dict != '')
    if have_cracklib
        cdata.set('USE_CRACKLIB', 1)
        if cracklib_path != ''
            cdata.set('_PATH_CRACKLIB', '"' + cracklib_path + '"')
        elif host_os == 'darwin'
            cdata.set(
                '_PATH_CRACKLIB',
                '"' + macos_prefix / 'var/cracklib/cracklib-words' + '"',
            )
        elif cracklib_dict != ''
            cdata.set('_PATH_CRACKLIB', '"' + cracklib_dict + '"')
        endif
    else
        have_cracklib = false
        warning('Cracklib support requested but cracklib library not found')
    endif
endif

#
# Check for TCP wrappers support
#

enable_tcpwrap = get_option('with-tcp-wrappers')

wrap = cc.find_library('wrap', required: false)

if not enable_tcpwrap
    have_tcpwrap = false
else
    tcpwrap_code = '''
int allow_severity = 0;
int deny_severity = 0;

int main(void) {

    hosts_access();

    ;
    return 0;
}
'''

    have_tcpwrap = wrap.found() and cc.links(tcpwrap_code, args: '-lwrap')
    if not have_tcpwrap
        have_tcpwrap = cc.links(tcpwrap_code, args: ['-lwrap', '-lnsl'])
    endif
    if have_tcpwrap
        cdata.set('TCPWRAP', 1)
    else
        have_tcpwrap = false
        warning('TCP wrappers support requested but required library not found')
    endif
endif

#
# Check whether to enable DDP
#

enable_ddp = get_option('with-ddp')

if not enable_ddp
    have_ddp = false
    cdata.set('NO_DDP', 1)
else
    have_ddp = true
endif

#
# Check for administrative group support
#

if get_option('with-admin-group')
    cdata.set('ADMIN_GRP', 1)
    admin_group = true
else
    admin_group = false
endif

#
# Check whether system (fcntl) locking should be disabled
#

if not get_option('with-locking')
    cdata.set('DISABLE_LOCKING', 1)
endif

#
# Check whether to enable debug code
#

if get_option('with-debug')
    cdata.set('DEBUG', 1)
endif

#
# Check whether to enable debugging
#

if get_option('with-debugging')
    cdata.set('DEBUGGING', 1)
endif

#
# Check for optional valid-shell-check support
#

if not get_option('with-shell-check')
    cdata.set('DISABLE_SHELLCHECK', 1)
    valid_shellcheck = false
else
    valid_shellcheck = true
endif

#
# Check for musl C library
#

if host_os == 'linux'
    if run_command('ldd', '/bin/ls', check: true).stdout().strip().contains('musl')
        cdata.set('MUSL', 1)
    endif
endif

#
# Check for optional server location protocol support (used by Classic Mac OS and Mac OS X to 10.1)
#

enable_srvloc = get_option('with-srvloc')
srvloc_path = get_option('with-srvloc-path')

srvloc_link_args = []

if srvloc_path != ''
    srvloc_link_args += ['-L' + srvloc_path / 'lib', '-lslp']
    slp = declare_dependency(
        link_args: srvloc_link_args,
        include_directories: include_directories(srvloc_path / 'include'),
    )
else
    slp = cc.find_library( 'slp', required: false)
endif

if not enable_srvloc
    have_srvloc = false
else
    have_srvloc = cc.has_function('SLPOpen', dependencies: slp)
    if have_srvloc
        cdata.set('USE_SRVLOC', 1)
    else
        have_srvloc = false
        warning('SLP support requested but slp library not found')
    endif
endif

#
# OS-specific configuration
#

if host_os == 'freebsd'
    cdata.set('BSD4_4', 1)
    cdata.set('FREEBSD', 1)
    cdata.set('OPEN_NOFOLLOW_ERRNO', 'EMLINK')
elif host_os == 'kfreebsd-gnu'
    cdata.set('_GNU_SOURCE', 1)
elif host_os == 'linux'
    use_glibc_at_header = cc.has_header('netatalk/at.h')
    cdata.set('_GNU_SOURCE', 1)
    if cc.compiles(
        '''
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/atalk.h>

struct atalk_addr foo;

int main (void){

    ;
    return 0;
}
''',
    )
        cdata.set('HAVE_ATALK_ADDR', 1)
    endif
    cdata.set('HAVE_BROKEN_DBTOB', 1)
elif host_os == 'netbsd'
    cdata.set('BSD4_4', 1)
    cdata.set('NETBSD', 1)
    cdata.set('OPEN_NOFOLLOW_ERRNO', 'EFTYPE')
    cdata.set('UAM_DHX', 1) # NetBSD does not have crypt.h, uses unistd.h
elif host_os == 'openbsd'
    cdata.set('BSD4_4', 1)
    cdata.set('UAM_DHX', 1) # OpenBSD does not have crypt.h, uses unistd.h
    cdata.set('NO_DDP', 1)
elif host_os == 'sunos'
    cdata.set('__EXTENSIONS__', 1)
    cdata.set('__svr4__', 1)
    cdata.set('_ISOC9X_SOURCE', 1)
    cdata.set('_XOPEN_SOURCE', 600)
    cdata.set('NO_STRUCT_TM_GMTOFF', 1)
    cdata.set('SOLARIS', 1)
elif host_os == 'darwin'
    cdata.set('HAVE_BROKEN_DBTOB', 1)
endif

#
# Variable substitution
#

# Reserved for future use:
#cdata.set('BINDIR', bindir)
#cdata.set('ETCDIR', pkgconfdir)
#cdata.set('PATH_NETATALK_LOCK', lockfile_path)
#cdata.set('SBINDIR', sbindir)

cdata.set('exec_prefix', prefix)
cdata.set('includedir', includedir)
cdata.set('libdir', libdir)
cdata.set('localstatedir', localstatedir)
cdata.set('NETATALK_VERSION', netatalk_version)
cdata.set('OPEN_NOFOLLOW_ERRNO', 'ELOOP')
cdata.set('pkgconfdir', pkgconfdir)
cdata.set('prefix', prefix)
cdata.set('sbindir', sbindir)

if perl.found()
    cdata.set('PERL', perl.full_path())
endif

configure_file(
    input: 'meson_config.h',
    output: 'config.h',
    configuration: cdata,
)

subdir('libatalk')
subdir('bin')
subdir('config')
subdir('etc')
subdir('man')
subdir('contrib')
subdir('distrib')
subdir('include')
subdir('sys')

if get_option('with-manual')
    subdir('doc')
endif

if get_option('with-tests')
    subdir('test')
endif

#########################
# Configuration summary #
#########################

# Build environment

summary_info = {
    'Build directory': meson.current_build_dir(),
    'Source path': meson.current_source_dir(),
}
summary(summary_info, bool_yn: true, section: 'Build environment:')

# Directories

summary_info = {
    'Installation prefix': get_option('prefix'),
    'Executable directory': bindir,
    'Header file directory': get_option('prefix') / get_option('includedir'),
    'Library directory': libdir,
    'Manual page directory': mandir,
    'System executable directory': sbindir,
}
summary(summary_info, bool_yn: true, section: 'Directories:')

# Compilation information

summary_info = {
    'host CPU': cpu,
    'host endianness': build_machine.endian(),
    'C compiler': cc.get_id(),
}
summary(summary_info, bool_yn: true, section: 'Compilation:')

# Configuration summary

uams_using_options = '(' + uams_options + ')'

summary({'Netatalk version': netatalk_version}, section: 'Configuration Summary:')
summary_info = {
    '  Initscript style': get_option('with-init-style'),
}
summary(summary_info, bool_yn: true, section: '  Init Style:')

summary_info = {
    '  Extended Attributes': netatalk_ea,
}
summary(summary_info, bool_yn: true, section: '  AFP:')

summary_info = {
    '  Backends': summary_backends,
}
summary(summary_info, bool_yn: true, section: '  CNID:')

summary_info = {
    '  Kerberos V': enable_krbV_uam,
    '  PGP': enable_pgp_uam,
    '  Randnum': '(afppasswd)',
    '  clrtxt': uams_using_options,
    '  guest': true,
}
if have_ssl
    summary_info += {
        '  DHX': uams_using_options,
    }
endif
if have_libgcrypt
    summary_info += {
        '  DHX2': uams_using_options,
    }
endif
summary(summary_info, bool_yn: true, section: '  UAMs:')

summary_info = {
    '  admin group support': admin_group,
    '  ACL support': have_acls,
    '  CUPS support': have_cups,
    '  DDP (AppleTalk) support': have_ddp,
    '  Cracklib support': have_cracklib,
    '  GSSAPI support': have_gssapi,
    '  LDAP support': have_ldap,
    '  Quota support': have_quota,
}
if have_quota
    summary_info += {
        '  Quota provider': quota_provider,
    }
endif
summary_info += {
    '  SLP (srvloc) support': have_srvloc,
    '  SSL provider': ssl_provider,
    '  TCP wrapper support': have_tcpwrap,
    '  valid shell check': valid_shellcheck,
    '  Zeroconf support': have_zeroconf,
}
if have_zeroconf
    summary_info += {
        '  Zeroconf provider': zeroconf_provider,
    }
endif
summary(summary_info, bool_yn: true, section: '  Options:')

summary_info = {'  Docbook': build_xml_docs}
summary(summary_info, bool_yn: true, section: '  Documentation:')
