executable(
    'fce_listen',
    ['fce.c'],
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: [bstring, iniparser],
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
