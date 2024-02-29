afppasswd_sources = [
    'afppasswd.c',
]

executable(
    'afppasswd',
    afppasswd_sources,
    include_directories: [ssl_includes, root_includes],
    dependencies: [crypto, mysqlclient],
    link_with: libatalk,
    c_args: [
        afpdpwfile,
        dversion,
    ],
    install: true,
)
