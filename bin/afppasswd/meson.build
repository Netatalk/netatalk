executable(
    'afppasswd',
    'afppasswd.c',
    include_directories: root_includes,
    dependencies: [crack, crypto],
    link_with: libatalk,
    c_args: [afpdpwfile, dversion],
    install: true,
    install_rpath: rpath,
)
