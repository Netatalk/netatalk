pap_deps = []

if have_iconv
    pap_deps += iconv
endif

executable(
    'pap',
    'pap.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: pap_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
executable(
    'papstatus',
    'papstatus.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: pap_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
