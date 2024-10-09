project(
    'netatalk',
    'c',
    version: '4.0.2dev',
    license: 'GPLv2',
    default_options: ['warning_level=3', 'c_std=c11'],
    meson_version: '>=0.61.2',
    # Reserved for future use:
    # meson_version: '>=0.62.0',
)

cc = meson.get_compiler('c')

if not cc.compiles('int func() { return 0; }')
    error('The C compiler cannot compile code. Please check the logs.')
endif

# Reserved for future use:
# cc.has_function_attribute('unused', required: false)

####################
# Global variables #
####################

cdata = configuration_data()
fs = import('fs')
netatalk_version = meson.project_version()
cpu = host_machine.cpu_family()
host_os = host_machine.system()
target_os = target_machine.system()

linux_distro = ''

if host_os == 'linux' and fs.exists('/etc/os-release')
    os_release = fs.read('/etc/os-release').strip()
    if os_release.contains('alpine')
        linux_distro = 'alpine'
    elif os_release.contains('arch')
        linux_distro = 'arch'
    elif os_release.contains('centos')
        linux_distro = 'centos'
    elif os_release.contains('gentoo')
        linux_distro = 'gentoo'
    elif os_release.contains('suse')
        linux_distro = 'suse'
    elif os_release.contains('fedora')
        linux_distro = 'fedora'
    elif os_release.contains('rhel')
        linux_distro = 'rhel'
    elif os_release.contains('ubuntu')
        linux_distro = 'ubuntu'
    elif os_release.contains('debian')
        linux_distro = 'debian'
    endif
    message('Linux distro detected: ' + linux_distro)
endif

#########
# Paths #
#########

prefix = get_option('prefix')
exec_prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
includedir = prefix / get_option('includedir')
libdir = prefix / get_option('libdir')
localstatedir = prefix / get_option('localstatedir')
mandir = prefix / get_option('mandir')
sbindir = prefix / get_option('sbindir')
sysconfdir = prefix / get_option('sysconfdir')

pkgconfdir = get_option('with-pkgconfdir-path')
if pkgconfdir == ''
    pkgconfdir = sysconfdir
endif

if host_os == 'darwin'
    homedir = '/Users'
elif host_os == 'sunos'
    homedir = '/export/home'
else
    homedir = '/home'
endif

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
afpd = '-D_PATH_AFPD="' + sbindir + '/afpd"'
afpdpwfile = '-D_PATH_AFPDPWFILE="' + pkgconfdir + '/afppasswd"'
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

macos_prefix = ''
if host_os == 'darwin'
    brew = find_program('brew', required: false)
    if cpu == 'aarch64' and brew.found()
        macos_prefix += '/opt/homebrew'
    elif cpu == 'x86_64' and brew.found()
        macos_prefix += '/usr/local'
    endif
endif

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
        'sys',
        'etc/afpd',
    )
endif

############
# Programs #
############

flex = find_program('flex', required: false)
grep = find_program('grep', required: false)
perl = find_program('perl', required: true)
uname = find_program('uname', required: false)
cmarkgfm = find_program('cmark-gfm', required: false)

if host_os == 'darwin'
    bison = find_program(macos_prefix / 'opt/bison/bin/bison', required: false)
    if not bison.found()
        bison = find_program('/opt/local/bin/bison', required: false)
    endif
else
    bison = find_program('bison', required: false)
endif

#############
# Libraries #
#############

uname_stdout = run_command(uname, '-a', check: false).stdout().strip()
libsearch_dirs = []

if host_os in ['dragonfly', 'freebsd', 'openbsd']
    libsearch_dirs += '/usr/local/lib'
endif

if host_os == 'netbsd'
    libsearch_dirs += '/usr/pkg/lib'
endif

if uname.found() and uname_stdout.to_lower().contains('omnios')
    libsearch_dirs += '/opt/local/lib'
endif

socket = cc.find_library('socket', required: false)
if socket.found()
    netatalk_common_link_args += '-lsocket'
endif

sendfile = cc.find_library('sendfile', required: false)
if sendfile.found()
    netatalk_common_link_args += '-lsendfile'
endif

nsl = cc.find_library('nsl', required: false)
if nsl.found()
    netatalk_common_link_args += '-lnsl'
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
    'dlfcn.h',
    'dns_sd.h',
    'errno.h',
    'fcntl.h',
    'inttypes.h',
    'langinfo.h',
    'linux/dqblk/xfs.h',
    'linux/xfs/fs.h',
    'linux/xqm.h',
    'locale.h',
    'mntent.h',
    'netdb.h',
    'pam/pam_appl.h',
    'poll.h',
    'security/pam_appl.h',
    'sgtty.h',
    'statfs.h',
    'stdint.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
    'strings.h',
    'sys/attr.h',
    'sys/attributes.h',
    'sys/ea.h',
    'sys/extattr.h',
    'sys/mnttab.h',
    'sys/mount.h',
    'sys/param.h',
    'sys/stat.h',
    'sys/statvfs.h',
    'sys/types.h',
    'sys/uio.h',
    'sys/vfs.h',
    'sys/xattr.h',
    'termios.h',
    'ufs/quota.h',
    'unistd.h',
    'xfs/libxfs.h',
    'xfs/xfs/fs.h',
    'xfs/xqm.h',
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
    'asprintf',
    'backtrace_symbols',
    'dirfd',
    'dlclose',
    'dlerror',
    'dlopen',
    'dlsym',
    'errno',
    'getpagesize',
    'getusershell',
    'mmap',
    'pread',
    'pselect',
    'pwrite',
    'setlinebuf',
    'shl_load',
    'splice',
    'strlcat',
    'strlcpy',
    'strnlen',
    'utime',
    'vasprintf',
]

foreach f : check_functions
    if cc.has_function(f)
        cdata.set('HAVE_' + f.underscorify().to_upper(), 1)
    endif
endforeach

if cc.has_function('mempcpy', prefix: '#include <string.h>')
    cdata.set('HAVE_MEMPCPY', 1)
endif

if cc.has_function('getifaddrs', dependencies: socket)
    cdata.set('HAVE_GETIFADDRS', 1)
endif

at_functions = [
    'openat',
    'renameat',
    'fstatat',
    'unlinkat',
]

have_atfuncs = false

foreach f : at_functions
    if cc.has_function(f)
        have_atfuncs = true
        cdata.set('HAVE_' + f.underscorify().to_upper(), 1)
        cdata.set('_ATFILE_SOURCE', 1)
        cdata.set('HAVE_ATFUNCS', 1)
    endif
endforeach

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
# Check for 64-bit libraries
#

run_command(
    cc,
    '-c', meson.project_source_root() / 'libatalk/dummy.c',
    '-o', meson.global_build_root() / 'dummy.o',
    check: false,
)
compiler_64_bit_mode = run_command(
    '/usr/bin/file',
    meson.global_build_root() / 'dummy.o',
    check: true,
).stdout().strip().contains('ELF 64')

#
# Check whether to enable rpath (the default on Solaris and NetBSD)
#

rpath_libdir = ''

if host_os == 'sunos' or host_os == 'netbsd' or get_option('with-rpath')
    rpath_libdir = libdir
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
                if target_os == 'sunos' and compiler_64_bit_mode and fs.exists(dir / 'lib/64')
                    bdb_libdir += dir / 'lib/64'
                else
                    bdb_libdir += dir / 'lib'
                endif
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
# Check for Unicode Character Database
#

unicode_dirs = [
    '/usr/share/unicode',
    '/usr/share/unicode/ucd',
    '/usr/pkg/share/texmf-dist/tex/generic/unicode-data',
    '/usr/local/share/texmf-dist/tex/generic/unicode-data/',
]

unicode_data_path = get_option('with-unicode-data-path')

if unicode_data_path == ''
    unicode_data_file = 'UnicodeData.txt'
    foreach dir : unicode_dirs
        if fs.exists(dir / 'UnicodeData.txt')
            unicode_data_file = dir / 'UnicodeData.txt'
            break
        endif
    endforeach
else
    unicode_data_file = unicode_data_path / 'UnicodeData.txt'
endif

if fs.exists(unicode_data_file)
    message('Using Unicode Character Database: ' + unicode_data_file)
else
    error('UnicodeData.txt not found. Specify path with -Dwith-unicode-data-path')
endif

#
# Check for crypt (for DHX authentication)
#

crypt = cc.find_library('crypt', has_headers: 'crypt.h', required: false)
if crypt.found()
    cdata.set('HAVE_CRYPT_H', 1)
    cdata.set('HAVE_LIBCRYPT', 1)
endif

#
# Check whether Kerberos V UAM should be compiled
#

enable_krb5_uam = get_option('with-krbV-uam')

if not enable_krb5_uam
    have_krb5_uam = false
    have_gssapi = false
    have_kerberos = false
else
    #
    # Check for GSSAPI
    #

    enable_gssapi = get_option('with-gssapi')
    gssapi_path = get_option('with-gssapi-path')

    gss_libs = []
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
            include_directories: include_directories(
                [gssapi_path / 'include', header_dir],
            ),
        )
            cdata.set('HAVE_' + header.underscorify().to_upper(), 1)
        endif
    endforeach

    if not enable_gssapi
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
        if not gss.found()
            gss = cc.find_library('gssapi', dirs: libsearch_dirs, required: false)
            if not gss.found()
                gss = cc.find_library(
                    'gssapi_krb5',
                    dirs: libsearch_dirs,
                    required: false,
                )
                if not gss.found()
                    have_gssapi = false
                endif
            endif
        endif
        have_gssapi = gss.found() and cc.has_function('gss_acquire_cred', dependencies: gss)
        if have_gssapi
            gss_libs += gss
            cdata.set('HAVE_GSSAPI', 1)
            # Heimdal/MIT compatibility fix
            if cc.has_header_symbol('gssapi.h', 'GSS_C_NT_HOSTBASED_SERVICE')
                cdata.set('HAVE_GSS_C_NT_HOSTBASED_SERVICE', 1)
            endif
        else
            have_gssapi = false
            warning('GSSAPI support requested but gssapi libraries not found')
        endif
    endif

    #
    # Check for Kerberos V
    #

    enable_kerberos = get_option('with-kerberos')

    kerberos = cc.find_library('krb5', dirs: libsearch_dirs, required: false)
    krb5_config = find_program('krb5-config', required: false)

    kerberos_c_args = []
    kerberos_headers = [
        'kerberosv5/krb5.h',
        'krb5.h',
        'krb5/krb5.h',
    ]

    foreach header : kerberos_headers
        if cc.has_header(
            header,
            include_directories: include_directories(header_dir),
        )
            cdata.set('HAVE_' + header.underscorify().to_upper(), 1)
        endif
    endforeach

    if not enable_kerberos
        have_kerberos = false
    else
        have_kerberos = kerberos.found()
        if have_kerberos
            gss_libs += kerberos
            cdata.set('HAVE_KERBEROS', 1)
            if host_os == 'sunos'
                kerberos_c_args += run_command(
                    krb5_config,
                    '--cflags',
                    check: true,
                ).stdout().strip()
            endif
            kerberos_functions = [
                'krb5_free_unparsed_name',
                'krb5_free_error_message',
                'krb5_free_keytab_entry_contents',
                'krb5_kt_free_entry',
            ]
            foreach function : kerberos_functions
                if cc.has_function(function, dependencies: kerberos)
                    cdata.set('HAVE_' + function.underscorify().to_upper(), 1)
                endif
            endforeach
        else
            have_kerberos = false
            warning('Kerberos API support requested but libkrb5 not found')
        endif
    endif
    if have_gssapi and have_kerberos
        have_krb5_uam = true
    else
        have_krb5_uam = false
    endif
    if enable_krb5_uam and not have_gssapi
        warning('Need GSSAPI support to build Kerberos V UAM')
    endif
endif

#
# Check for optional Zeroconf support
#

enable_zeroconf = get_option('with-zeroconf')

have_zeroconf = false

zeroconf_provider = ''

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

have_dns = (
    (dns_sd.found() or system.found())
    and cc.has_header('dns_sd.h')
    and cc.has_function(
        'DNSServiceRegister',
        dependencies: dns_sd_libs,
    )
)

if enable_zeroconf
    if avahi.found()
        have_zeroconf = true
        cdata.set('USE_ZEROCONF', 1)
        cdata.set('HAVE_AVAHI', 1)
        if avahi.version() >= '0.6.4'
            cdata.set('HAVE_AVAHI_THREADED_POLL', 1)
        endif
        cdata.set('freebsd_zeroconf', 'avahi_daemon')
        zeroconf_provider += 'Avahi'
    elif have_dns
        have_zeroconf = true
        cdata.set('USE_ZEROCONF', 1)
        cdata.set('HAVE_MDNS', 1)
        cdata.set('freebsd_zeroconf', 'mdnsd')
        zeroconf_provider += 'mDNS'
    endif
endif

if enable_zeroconf and not have_zeroconf
    warning(
        'Zeroconf (Bonjour) support requested but required libraries not found',
        'Please install Avahi or mDNS',
    )
endif

#
# Check for Spotlight support
#

talloc = dependency('talloc', required: false)

tracker_manager = ''
tracker_prefix = get_option('with-tracker-prefix')

if not get_option('with-spotlight')
    have_spotlight = false
else
    # Check for tracker SPARQL

    tracker_sparql_version = ['3.0', '2.0', '1.0']

    foreach version : tracker_sparql_version
        tracker_sparql = dependency('tracker-sparql-' + version, required: false)
        if tracker_sparql.found()
            break
        endif
    endforeach

    if not tracker_sparql.found()
        warning('tracker SPARQL not found (required for Spotlight support)')
    else
        # Check for tracker

        tracker = find_program(
            'tracker',
            dirs: tracker_prefix + '/bin',
            required: false,
        )
        tracker3 = find_program(
            'tracker3',
            dirs: tracker_prefix + '/bin',
            required: false,
        )
        tracker_control = find_program(
            'tracker-control',
            dirs: tracker_prefix + '/bin',
            required: false,
        )

        if tracker.found()
            cdata.set(
                'TRACKER_MANAGING_COMMAND',
                '"tracker daemon"',
            )
            cdata.set(
                'TRACKER_PREFIX',
                '"'
                + tracker_sparql.get_variable(pkgconfig: 'prefix')
                + '"',
            )
            tracker_manager += 'tracker'
        elif tracker3.found()
            cdata.set('HAVE_TRACKER3', 1)
            cdata.set(
                'TRACKER_MANAGING_COMMAND',
                '"tracker3 daemon"',
            )
            cdata.set(
                'TRACKER_PREFIX',
                '"'
                + tracker_sparql.get_variable(pkgconfig: 'prefix')
                + '"',
            )
            tracker_manager += 'tracker3'
        elif tracker_control.found()
            cdata.set(
                'TRACKER_MANAGING_COMMAND',
                '"tracker-control"',
            )
            cdata.set(
                'TRACKER_PREFIX',
                '"'
                + tracker_sparql.get_variable(pkgconfig: 'prefix')
                + '"',
            )
            tracker_manager += 'tracker_control'
        endif
        tracker_found = (tracker.found() or tracker3.found() or tracker_control.found())
        if not tracker_found
            warning('tracker not found (required for Spotlight support)')
        else
            # Check for talloc

            if talloc.found()
                cdata.set('HAVE_TALLOC', 1)
            else
                warning(
                    'talloc library not found (required for Spotlight support)',
                )
            endif
        endif
    endif
endif

have_spotlight = (
    tracker_sparql.found()
    and tracker_found
    and talloc.found()
    and flex.found()
    and bison.found()
)
if have_spotlight
    cdata.set('WITH_SPOTLIGHT', 1)
endif

#
# Check for libevent
#

libevent = dependency(
    'libevent',
    required: true,
    not_found_message: 'Libevent library required but not found! Please install the libevent devel package',
)

#
# Check for CUPS
#

cups = dependency('cups', required: false)
spooldir = ''
cups_config = find_program('cups-config', required: false)

if not (get_option('with-appletalk') or get_option('with-cups'))
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
    libgcrypt = declare_dependency(
        link_args: libgcrypt_link_args,
        include_directories: include_directories(libgcrypt_path / 'include'),
    )
else
    libgcrypt = dependency('libgcrypt', version: '>=1.2.3', required: false)
    if not libgcrypt.found()
        libgcrypt = cc.find_library(
            'libgcrypt',
            dirs: libsearch_dirs,
            required: false,
        )
    endif
endif

have_libgcrypt = libgcrypt.found()

if not have_libgcrypt
    error(
        'Libgcrypt library required for encrypted user auth! Please install version 1.2.3 or later',
    )
endif

#
# Check for Extended Attributes support
#

netatalk_ea = '"ad'
ea_summary = 'AppleDouble'
attr = cc.find_library('attr', required: false)

if host_os == 'sunos'
    # extattr API has full fledged fds for EAs

    have_ea = true
    cdata.set('HAVE_EAFD', 1)
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
    netatalk_ea += ' | sys"'
    ea_summary += ', filesystem EA'
else
    netatalk_ea += '"'
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
# Check for dbus and glib, for AFP stats
#

enable_afpstats = get_option('with-afpstats')


dbus = dependency('dbus-1', required: false)
glib = dependency('glib-2.0', required: false)
gobject = dependency('gobject-2.0', required: false)
gio = dependency('gio-2.0', required: false)

if not enable_afpstats
    have_afpstats = false
else
    have_afpstats = dbus.found() and glib.found() and gobject.found() and gio.found()
    if have_afpstats
        cdata.set('HAVE_DBUS_GLIB', 1)
    else
        warning(
            'afpstats support requested but required D-Bus and GLib libraries not found',
        )
    endif
endif

#
# Set dbus-daemon path
#

dbus_daemon_path = get_option('with-dbus-daemon-path')
dbus_daemonpath = ''

if dbus_daemon_path != ''
    dbus_daemonpath += dbus_daemon_path
    cdata.set('DBUS_DAEMON_PATH', '"' + dbus_daemon_path + '"')
elif host_os == 'darwin'
    dbus_daemonpath += bindir / 'dbus-daemon'
    cdata.set('DBUS_DAEMON_PATH', '"' + bindir / 'dbus-daemon' + '"')
else
    dbus_daemonpath += '/bin/dbus-daemon'
    cdata.set('DBUS_DAEMON_PATH', '"/bin/dbus-daemon"')
endif

#
# Set dbus sysconf directory
#

dbus_sysconf_path = get_option('with-dbus-sysconf-path')
dbus_sysconfpath = ''

if dbus_sysconf_path != ''
    dbus_sysconfpath += dbus_sysconf_path
    cdata.set('DBUS_SYS_DIR', '"' + dbus_sysconf_path + '"')
else
    dbus_sysconfpath += datadir / 'dbus-1/system.d'
    cdata.set('DBUS_SYS_DIR', '"' + datadir + '/dbus-1/system.d' + '"')
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
    cdata.set('HAVE_NFSV4_ACLS', 1)
elif host_os == 'freebsd'
    sunacl = cc.find_library(
        'sunacl',
        dirs: libsearch_dirs,
        required: false,
    )
    have_acls = cc.has_function('acl', dependencies: sunacl)
    if have_acls
        acl_deps += sunacl
        cdata.set('HAVE_ACLS', 1)
        cdata.set('HAVE_FREEBSD_SUNACL', 1)
        cdata.set('HAVE_LIBSUNACL', 1)
        cdata.set('HAVE_NFSV4_ACLS', 1)
    else
        have_acls = false
        warning('libsunacl not found, disabling ZFS ACL support')
    endif
else
    # Runtime checks for POSIX ACLs

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
        warning('ACL support requested but required libraries not found')
    endif
endif

#
# Check for LDAP support, for client-side ACL visibility
#

enable_ldap = get_option('with-ldap')
ldap_path = get_option('with-ldap-path')

ldap_link_args = []

if ldap_path != ''
    ldap_link_args += ['-L' + ldap_path / 'lib', '-lldap']
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
    iconv = declare_dependency(
        link_args: libiconv_link_args,
        include_directories: include_directories(iconv_path / 'include'),
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

# Check for mysql CNID backend

mysqlclient = dependency('mysqlclient', required: false)
mariadb = dependency('libmariadb', required: false)

use_mysql_backend = false

if get_option('with-cnid-mysql-backend')
    if mysqlclient.found()
        mysql_deps = mysqlclient
    else
        mysql_deps = mariadb
    endif
    if mysql_deps.found()
        use_mysql_backend = (
            fs.exists(
                mysql_deps.get_variable(pkgconfig: 'includedir') / 'mysql.h',
            )
        )
    endif
    if use_mysql_backend
        cnid_backends += ' mysql'
        cdata.set('CNID_BACKEND_MYSQL', 1)
    endif
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
elif default_backend == 'mysql' and not use_mysql_backend
    error(
        'Specified default CNID scheme mysql was not selected for compilation',
    )
endif

#
# Check for docbook
#

docbook_path = get_option('with-docbook-path')
compile_manual = get_option('with-manual') != 'none'
build_xml_docs = false

xsltproc = find_program('xsltproc', required: false)
xsl = []
docbook_root = ''
docbook_xsl_dirs = [
    docbook_path,
    '/usr/share/sgml/docbook/xsl-stylesheets',
    '/usr/share/xml/docbook/stylesheet/docbook-xsl',
    '/usr/share/xml/docbook/xsl-stylesheets',
    '/opt/local/share/xsl/docbook',
    '/usr/local/share/xsl/docbook',
    '/usr/pkg/share/xsl/docbook',
]

if host_os == 'darwin'
    docbook_xsl_dirs += macos_prefix / 'opt/docbook-xsl/docbook-xsl'
endif

if compile_manual
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
    warning(
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
        uams_options += 'PAM '
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
        uams_options += 'shadow'
    else
        have_shadow = false
        warning('Shadow password support requested but required header not found')
    endif
endif

#
# Netatalk lockfile path
#

lockfile_path = get_option('with-lockfile-path')
lockfile = ''

if lockfile_path != ''
    lockfile += lockfile_path
elif host_os == 'freebsd'
    lockfile += '/var/spool/lock/netatalk'
elif host_os in ['netbsd', 'openbsd', 'darwin']
    lockfile += '/var/run/netatalk.pid'
elif host_os == 'linux'
    lockfile += '/var/lock/netatalk'
else
    lockfile += '/var/spool/locks/netatalk'
endif

cdata.set('PATH_NETATALK_LOCK', '"' + lockfile + '"')

#
# Check for largefle support
#

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
    if file_offset_bits_test
        cdata.set('_FILE_OFFSET_BITS', 64)
    else
        if large_files_test
            cdata.set('_LARGE_FILES', 1)
        endif
    endif
else
    error('Largefile support required but not available on this platform')
endif

#
# Check for sendfile
#

enable_sendfile = get_option('with-sendfile')

if not enable_sendfile
    have_sendfile = false
elif host_os == 'linux'
    cdata.set('SENDFILE_FLAVOR_LINUX', 1)
    have_sendfile = cc.has_function('sendfile')
    if have_sendfile
        cdata.set('WITH_SENDFILE', 1)
    endif
elif host_os == 'sunos'
    cdata.set('SENDFILE_FLAVOR_SOLARIS', 1)
    have_sendfile = cc.has_function('sendfile', dependencies: sendfile)
    if have_sendfile
        cdata.set('WITH_SENDFILE', 1)
    endif
    if cc.has_function('sendfilev', dependencies: sendfile)
        cdata.set('HAVE_SENDFILEV', 1)
    endif
elif host_os == 'freebsd'
    cdata.set('SENDFILE_FLAVOR_FREEBSD', 1)
    have_sendfile = cc.has_function('sendfile')
    if have_sendfile
        cdata.set('WITH_SENDFILE', 1)
    endif
endif

#
# Check for recvfile
#

if host_os == 'linux' and cc.has_function('splice')
    cdata.set('WITH_RECVFILE', 1)
    cdata.set('HAVE_SPLICE', 1)
endif

#
# Check for dtrace
#

enable_dtrace = get_option('with-dtrace')

dtrace = find_program('dtrace', required: false)
libelf = cc.find_library('elf', required: false)
dtrace_command = [
    'env',
    'CC=' + ' '.join(cc.cmd_array()),
    dtrace,
]
dtrace_deps = []

if not enable_dtrace
    have_dtrace = false
elif host_os == 'freebsd' and cpu == 'aarch64'

    # See https://wiki.freebsd.org/arm64#Known_Limitations

    have_dtrace = false
    warning('The version of dtrace in aarch64 FreeBSD is currently unsupported')
else
    have_dtrace = dtrace.found()
    if have_dtrace
        cdata.set('WITH_DTRACE', 1)
        if host_os == 'freebsd'
            dtrace_deps += libelf
        endif
    else
        have_dtrace = false
        warning('Dtrace probe support requested but dtrace not found')
    endif
endif

#
# Check for POSIX threads
#

threads = dependency('threads', required: true)
if threads.found()
    have_threads = true
    cdata.set('HAVE_PTHREAD', 1)

    # Check for PTHREAD_PRIO_INHERIT

    if cc.compiles(
        '''
    #include <pthread.h>
    int main(void) {
    int i = PTHREAD_PRIO_INHERIT;
      return i;
    }
    ''',
    )
        cdata.set('HAVE_PTHREAD_PRIO_INHERIT', 1)
    endif
endif

#
# Check for optional initscript install
#

init_style = get_option('with-init-style')
init_dirs = []

if 'auto' in init_style
    if host_os == 'linux'
        if linux_distro in ['arch', 'centos', 'debian', 'fedora', 'rhel', 'suse', 'ubuntu']
            init_style = ['systemd']
        elif linux_distro in ['alpine', 'gentoo']
            init_style = ['openrc']
        endif
    elif host_os == 'freebsd'
        init_style = ['freebsd']
    elif host_os == 'netbsd'
        init_style = ['netbsd']
    elif host_os == 'openbsd'
        init_style = ['openbsd']
    elif host_os == 'darwin'
        init_style = ['macos-launchd']
    elif host_os == 'sunos'
        init_style = ['solaris']
    else
        init_style = ['none']
    endif
endif

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
        if not crack.found()
            warning('Cracklib support requested but cracklib library not found')
        endif
        if not (cracklib_path != '' or cracklib_dict != '')
            warning('Cracklib support requested but cracklib dictionary not found')
        endif
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
#include <tcpd.h>
int main(void) {
    int allow_severity = 0;
    int deny_severity = 0;
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
# Check whether to enable DDP (AppleTalk)
#

enable_appletalk = get_option('with-appletalk')

if not enable_appletalk
    have_appletalk = false
    cdata.set('NO_DDP', 1)
else
    have_appletalk = true
endif

#
# Check whether to install the Netatalk Webmin Module
#

with_webmin = get_option('with-webmin')
have_webmin = false
install_webmin = false

webmin_dirs = [
    '/usr/share/webmin',
    '/usr/libexec/webmin',
    '/opt/webmin',
]

if with_webmin
    tar = find_program('tar', required: true)
    webmin_install_script = 'install-module.pl'
    have_webmin = true
    foreach dir : webmin_dirs
        if fs.exists(dir / webmin_install_script)
            webmin = find_program(dir / webmin_install_script, required: true)
            install_webmin = true
            break
        endif
    endforeach
    if not install_webmin
        warning('Webmin module will be generated but not installed, because Webmin could not be found')
    endif
endif

#
# OS-specific configuration
#

cdata.set('OPEN_NOFOLLOW_ERRNO', 'ELOOP')
use_glibc_at_header = false

if host_os.contains('freebsd')
    cdata.set('BSD4_4', 1)
    cdata.set('FREEBSD', 1)
    cdata.set('OPEN_NOFOLLOW_ERRNO', 'EMLINK')
elif host_os.contains('linux')
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
    cdata.set('LINUX', 1)
elif host_os.contains('netbsd')
    cdata.set('BSD4_4', 1)
    cdata.set('NETBSD', 1)
    cdata.set('OPEN_NOFOLLOW_ERRNO', 'EFTYPE')
elif host_os.contains('openbsd')
    cdata.set('BSD4_4', 1)
elif host_os.contains('sunos')
    cdata.set('__EXTENSIONS__', 1)
    cdata.set('__svr4__', 1)
    cdata.set('_ISOC9X_SOURCE', 1)
    cdata.set('_XOPEN_SOURCE', 600)
    cdata.set('NO_STRUCT_TM_GMTOFF', 1)
    cdata.set('SOLARIS', 1)
elif host_os.contains('darwin')
    cdata.set('HAVE_BROKEN_DBTOB', 1)
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
endif

#
# Variable substitution
#

cdata.set('bindir', bindir)
cdata.set('etcdir', pkgconfdir)
cdata.set('exec_prefix', exec_prefix)
cdata.set('homedir', homedir)
cdata.set('libdir', libdir)
cdata.set('includedir', includedir)
cdata.set('localstatedir', localstatedir)
cdata.set('lockfile_path', lockfile_path)
cdata.set('netatalk_version', netatalk_version)
cdata.set('pkgconfdir', pkgconfdir)
cdata.set('prefix', prefix)
cdata.set('sbindir', sbindir)

if perl.found()
    cdata.set('PERL', perl.full_path())
endif

if have_webmin
    init_netatalk_start = ''
    init_netatalk_stop = ''
    init_netatalk_restart = ''
    init_atalkd_start = ''
    init_atalkd_stop = ''
    init_atalkd_restart = ''
    init_papd_start = ''
    init_papd_stop = ''
    init_papd_restart = ''
    init_timelord_start = ''
    init_timelord_stop = ''
    init_timelord_restart = ''
    init_a2boot_start = ''
    init_a2boot_stop = ''
    init_a2boot_restart = ''

    if 'systemd' in init_style
        init_netatalk_start = 'systemmctl start netatalk'
        init_netatalk_stop = 'systemctl stop netatalk'
        init_netatalk_restart = 'systemctl restart netatalk'
        init_atalkd_start = 'systemctl start atalkd'
        init_atalkd_stop = 'systemctl stop atalkd'
        init_atalkd_restart = 'systemctl restart atalkd'
        init_papd_start = 'systemctl start papd'
        init_papd_stop = 'systemctl stop papd'
        init_papd_restart = 'systemctl restart papd'
        init_timelord_start = 'systemctl start timelord'
        init_timelord_stop = 'systemctl stop timelord'
        init_timelord_restart = 'systemctl restart timelord'
        init_a2boot_start = 'systemctl start a2boot'
        init_a2boot_stop = 'systemctl stop a2boot'
        init_a2boot_restart = 'systemctl restart a2boot'
    elif 'openrc' in init_style
        init_netatalk_start = 'rc-service netatalk start'
        init_netatalk_stop = 'rc-service netatalk stop'
        init_netatalk_restart = 'rc-service netatalk restart'
        init_atalkd_start = 'rc-service atalkd start'
        init_atalkd_stop = 'rc-service atalkd stop'
        init_atalkd_restart = 'rc-service atalkd restart'
        init_papd_start = 'rc-service papd start'
        init_papd_stop = 'rc-service papd stop'
        init_papd_restart = 'rc-service papd restart'
        init_timelord_start = 'rc-service timelord start'
        init_timelord_stop = 'rc-service timelord stop'
        init_timelord_restart = 'rc-service timelord restart'
        init_a2boot_start = 'rc-service a2boot start'
        init_a2boot_stop = 'rc-service a2boot stop'
        init_a2boot_restart = 'rc-service a2boot restart'
    elif 'debian-sysv' in init_style
        init_netatalk_start = 'service netatalk start'
        init_netatalk_stop = 'service netatalk stop'
        init_netatalk_restart = 'service netatalk restart'
        init_atalkd_start = 'service atalkd start'
        init_atalkd_stop = 'service atalkd stop'
        init_atalkd_restart = 'service atalkd restart'
        init_papd_start = 'service papd start'
        init_papd_stop = 'service papd stop'
        init_papd_restart = 'service papd restart'
        init_timelord_start = 'service timelord start'
        init_timelord_stop = 'service timelord stop'
        init_timelord_restart = 'service timelord restart'
        init_a2boot_start = 'service a2boot start'
        init_a2boot_stop = 'service a2boot stop'
        init_a2boot_restart = 'service a2boot restart'
    elif 'freebsd' in init_style
        init_dir_override = get_option('with-init-dir')
        if init_dir_override == ''
            init_dir = '/usr/local/etc/rc.d'
        endif
        init_netatalk_start = init_dir + '/netatalk start'
        init_netatalk_stop = init_dir + '/netatalk stop'
        init_netatalk_restart = init_dir + '/netatalk restart'
    elif 'macos-launchd' in init_style
        init_netatalk_start = 'netadalkd start'
        init_netatalk_stop = 'netatalkd stop'
        init_netatalk_restart = 'netatalkd restart'
    elif 'netbsd' in init_style
        init_netatalk_start = 'service netatalk onestart'
        init_netatalk_stop = 'service netatalk onestop'
        init_netatalk_restart = 'service netatalk onerestart'
        init_atalkd_start = 'service atalkd onestart'
        init_atalkd_stop = 'service atalkd onestop'
        init_atalkd_restart = 'service atalkd onerestart'
        init_papd_start = 'service papd onestart'
        init_papd_stop = 'service papd onestop'
        init_papd_restart = 'service papd onerestart'
        init_timelord_start = 'service timelord onestart'
        init_timelord_stop = 'service timelord onestop'
        init_timelord_restart = 'service timelord onerestart'
        init_a2boot_start = 'service a2boot onestart'
        init_a2boot_stop = 'service a2boot onestop'
        init_a2boot_restart = 'service a2boot onerestart'
    elif 'openbsd' in init_style
        init_netatalk_start = 'rcctl -d start netatalk'
        init_netatalk_stop = 'rcctl -d stop netatalk'
        init_netatalk_restart = 'rcctl -d reload netatalk'
    elif 'solaris' in init_style
        init_netatalk_start = 'svcadm enable svc:/network/netatalk:default'
        init_netatalk_stop = 'svcadm disable svc:/network/netatalk:default'
        init_netatalk_restart = 'svcadm restart svc:/network/netatalk:default'
    endif

    cdata.set('init_netatalk_start', init_netatalk_start)
    cdata.set('init_netatalk_stop', init_netatalk_stop)
    cdata.set('init_netatalk_restart', init_netatalk_restart)
    cdata.set('init_atalkd_start', init_atalkd_start)
    cdata.set('init_atalkd_stop', init_atalkd_stop)
    cdata.set('init_atalkd_restart', init_atalkd_restart)
    cdata.set('init_papd_start', init_papd_start)
    cdata.set('init_papd_stop', init_papd_stop)
    cdata.set('init_papd_restart', init_papd_restart)
    cdata.set('init_timelord_start', init_timelord_start)
    cdata.set('init_timelord_stop', init_timelord_stop)
    cdata.set('init_timelord_restart', init_timelord_restart)
    cdata.set('init_a2boot_start', init_a2boot_start)
    cdata.set('init_a2boot_stop', init_a2boot_stop)
    cdata.set('init_a2boot_restart', init_a2boot_restart)
endif

configure_file(
    input: 'meson_config.h',
    output: 'config.h',
    configuration: cdata,
)

#########################
# Prepare documentation #
#########################


if get_option('with-readmes')
    readmes = [
        'CONTRIBUTORS',
        'NEWS',
    ]

    foreach file : readmes
        install_data(
            file,
            install_dir: datadir / 'doc/netatalk',
        )
    endforeach

    if cmarkgfm.found()
        readmes_md = [
            'DOCKER',
            'INSTALL',
            'README',
            'SECURITY',
        ]

        foreach file : readmes_md
            custom_target(
                'readmes_' + file,
                input: file + '.md',
                output: file + '.txt',
                command: [
                    'cmark-gfm',
                    '--width', '80',
                    '--to', 'plaintext',
                    '@INPUT@',
                ],
                capture: true,
                install: true,
                install_dir: datadir / 'doc/netatalk',
            )
        endforeach
    else
        warning('cmark-gfm is required for markdown to text conversion')
    endif
endif

subdir('include')
subdir('libatalk')
subdir('bin')
subdir('config')
subdir('etc')
subdir('contrib')
subdir('distrib')
subdir('sys')

if get_option('with-manual') != 'none'
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
    'Download dependencies': get_option('wrap_mode') != 'nodownload',
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
    'Package conf directory': pkgconfdir,
}
summary(summary_info, bool_yn: true, section: 'Directories:')

# Compilation information

summary_info = {
    'host CPU': cpu,
    'host endianness': build_machine.endian(),
    'C compiler': cc.get_id(),
    'Shared or static libraries': get_option('default_library'),
}
summary(summary_info, bool_yn: true, section: 'Compilation:')

# Configuration summary

summary({'Netatalk version': netatalk_version}, section: 'Configuration Summary:')

summary_info = {
    '  Access control': uams_options,
    '  CNID backends': summary_backends,
    '  Extended Attributes': ea_summary,
    '  Init script style': init_style,
}
summary(summary_info, bool_yn: true, section: '  Options:')

summary_info = {
    '  ClearTxt': true,
    '  DHX': have_libgcrypt,
    '  DHX2': have_libgcrypt,
    '  Guest': true,
    '  Kerberos V': have_krb5_uam,
    '  RandNum': have_libgcrypt,
}
summary(summary_info, bool_yn: true, section: '  User Authentication Methods:')

summary_info = {
    '  ACL': have_acls,
    '  AFP stats via D-Bus': have_afpstats,
    '  AppleTalk': have_appletalk,
    '  Cracklib': have_cracklib,
    '  CUPS': have_cups,
    '  dtrace probes': have_dtrace,
    '  GSSAPI': have_gssapi,
    '  Kerberos': have_kerberos,
    '  LDAP': have_ldap,
    '  Quota': have_quota,
}
if have_quota
    summary_info += {
        '  Quota provider': quota_provider,
    }
endif
summary_info += {
    '  Spotlight': have_spotlight,
    '  TCP wrapper': have_tcpwrap,
    '  Zeroconf': have_zeroconf,
}
if enable_zeroconf
    summary_info += {
        '  Zeroconf provider': zeroconf_provider,
    }
endif
summary(summary_info, bool_yn: true, section: '  Features:')

summary_info = {
    '  dbus daemon path': dbus_daemonpath,
    '  dbus system directory': dbus_sysconfpath,
    '  init directory': init_dirs,
    '  Netatalk lockfile': lockfile,
    '  PAM config directory': pam_confdir,
    '  CUPS print spool directory': spooldir,
    '  User home basedir': homedir,
}
if have_spotlight
    summary_info += {
        '  tracker prefix': tracker_sparql.get_variable(pkgconfig: 'prefix'),
        '  tracker install prefix': tracker_sparql.get_variable(
            pkgconfig: 'prefix',
        ),
        '  tracker manager': tracker_sparql.get_variable(pkgconfig: 'prefix') / 'bin' / tracker_manager
        + ' daemon',
    }
endif
summary(summary_info, bool_yn: true, section: '  Paths:')

summary_info = {
    '  Docbook': build_xml_docs,
    '  Manual style': get_option('with-manual'),
}
summary(summary_info, bool_yn: true, section: '  Documentation:')

summary_info = {
    '  Webmin module': have_webmin,
}
summary(summary_info, bool_yn: true, section: '  Add-ons:')
