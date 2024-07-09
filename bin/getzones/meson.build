if enable_rpath
    executable(
        'getzones',
        'getzones.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'getzones',
        'getzones.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
endif
