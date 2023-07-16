afppasswd_sources = [
    'afppasswd.c',
]

executable(
    'afppasswd',
    afppasswd_sources,
    include_directories: root_includes,
    dependencies: [crack, ssl_deps, mysqlclient],
    link_with: libatalk,
    c_args: [
        afpdpwfile,
        dversion,
    ],
    install: true,
    install_rpath: rpath,
)
