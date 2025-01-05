papd_sources = [
    'auth.c',
    'comment.c',
    'file.c',
    'headers.c',
    'lp.c',
    'magics.c',
    'main.c',
    'ppd.c',
    'print_cups.c',
    'printcap.c',
    'queries.c',
    'session.c',
    'uam.c',
]

devprinter_path = ''

if host_os == 'netbsd'
    devprinter_path = '/var/run/printer'
else
    devprinter_path = '/dev/printer'
endif

papd_c_args = [
    '-D_PATH_DEVPRINTER="' + devprinter_path + '"',
    '-D_PATH_PAPDCONF="' + pkgconfdir + '/papd.conf"',
    '-D_PATH_PAPDPRINTCAP="/etc/printcap"',
    '-D_PATH_PAPDSPOOLDIR="/var/spool/lpd"',
    '-D_PATH_PAPDUAMPATH="' + libdir + '/netatalk/"',
    '-DSPOOLDIR="' + spooldir + '/"',
    dversion,
]

executable(
    'papd',
    papd_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: cups,
    c_args: papd_c_args,
    export_dynamic: true,
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)

if have_cups and spooldir_required
    install_emptydir(spooldir, install_mode: 'rwxrwxrwx')
endif
