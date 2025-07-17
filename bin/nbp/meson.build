npb_deps = [bstring]

if have_iconv
    npb_deps += iconv
endif

executable(
    'nbplkup',
    'nbplkup.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: npb_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
executable(
    'nbprgstr',
    'nbprgstr.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: npb_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
executable(
    'nbpunrgstr',
    'nbpunrgstr.c',
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: npb_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
