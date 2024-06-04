executable(
    'afppasswd',
    'afppasswd.c',
    include_directories: root_includes,
    dependencies: [crack, ssl_deps],
    link_with: libatalk,
    c_args: [afpdpwfile, dversion],
    install: true,
    build_rpath: libdir,
    install_rpath: libdir,
    install_mode: 'rwsr-xr-x',
)
