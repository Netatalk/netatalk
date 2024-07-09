if enable_rpath
    executable(
        'pap',
        'pap.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
    executable(
        'papstatus',
        'papstatus.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'pap',
        'pap.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
    executable(
        'papstatus',
        'papstatus.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
endif
