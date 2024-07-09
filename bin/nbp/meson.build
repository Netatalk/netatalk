if enable_rpath
    executable(
        'nbplkup',
        'nbplkup.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
    executable(
        'nbprgstr',
        'nbprgstr.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
    executable(
        'nbpunrgstr',
        'nbpunrgstr.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'nbplkup',
        'nbplkup.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
    executable(
        'nbprgstr',
        'nbprgstr.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
    executable(
        'nbpunrgstr',
        'nbpunrgstr.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
endif
