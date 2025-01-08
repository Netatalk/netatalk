timelord_deps = []

if have_iconv
    timelord_deps += iconv
endif

executable(
    'timelord',
    'timelord.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: timelord_deps,
    c_args: dversion,
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
