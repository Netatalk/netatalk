getzones_deps = [bstring]

if have_iconv
    getzones_deps += iconv
endif

executable(
    'rtmpqry',
    'rtmpqry.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: getzones_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
