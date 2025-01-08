aecho_deps = []

if have_iconv
    aecho_deps += iconv
endif

executable(
    'aecho',
    'aecho.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: aecho_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
