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

if enable_rpath
    executable(
        'atalkd',
        atalkd_sources,
        include_directories: root_includes,
        link_with: libatalk,
        c_args: atalkd_c_args,
        install: true,
        install_dir: sbindir,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'atalkd',
        atalkd_sources,
        include_directories: root_includes,
        link_with: libatalk,
        c_args: atalkd_c_args,
        install: true,
        install_dir: sbindir,
    )
endif
