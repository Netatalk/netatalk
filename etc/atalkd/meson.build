atalkd_sources = [
    'aep.c',
    'config.c',
    'main.c',
    'multicast.c',
    'nbp.c',
    'route.c',
    'rtmp.c',
    'zip.c',
]

atalkd_c_args = ['-D_PATH_ATALKDCONF="' + pkgconfdir + '/atalkd.conf"', dversion]

atalkd_deps = [bstring]

if have_iconv
    atalkd_deps += iconv
endif

executable(
    'atalkd',
    atalkd_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: atalkd_deps,
    c_args: atalkd_c_args,
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
