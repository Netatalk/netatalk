dbd_deps = [bstring, iniparser]

dbd_sources = [
    'cmd_dbd.c',
    'cmd_dbd_scanvol.c',
]

executable(
    'dbd',
    dbd_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: dbd_deps,
    c_args: dversion,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
