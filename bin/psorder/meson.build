psorder_sources = [
    'pa.c',
    'psorder.c',
]

executable(
    'psorder',
    psorder_sources,
    include_directories: root_includes,
    link_with: libatalk,
    install: true,
    install_rpath: rpath,
)
