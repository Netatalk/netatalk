psorder_sources = [
    'pa.c',
    'psorder.c',
]

if enable_rpath
    executable(
        'psorder',
        psorder_sources,
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'psorder',
        psorder_sources,
        include_directories: root_includes,
        link_with: libatalk,
        install: true,
    )
endif
