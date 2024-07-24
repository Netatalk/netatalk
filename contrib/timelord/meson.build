if enable_rpath
    executable(
        'timelord',
        'timelord.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        install_dir: sbindir,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'timelord',
        'timelord.c',
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        install_dir: sbindir,
    )
endif
