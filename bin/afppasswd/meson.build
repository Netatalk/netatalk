executable(
    'afppasswd',
    'afppasswd.c',
    include_directories: root_includes,
    dependencies: crypto,
    link_with: libatalk,
    c_args: [afpdpwfile, dversion],
    install: true,
)
