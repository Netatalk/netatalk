afppasswd_sources = [
    'afppasswd.c',
]

afppasswd_deps = []
afppasswd_libs = []

if have_cracklib
    afppasswd_deps += crack
endif

if crypto.found()
    afppasswd_deps += ssl_deps
endif

if use_mysql_backend
    afppasswd_deps += mysqlclient
endif

if have_embedded_ssl
    afppasswd_libs += libatalk
endif

executable(
    'afppasswd',
    afppasswd_sources,
    include_directories: root_includes,
    dependencies: afppasswd_deps,
    link_with: afppasswd_libs,
    c_args: [
        afpdpwfile,
        dversion,
    ],
    install: true,
    install_mode: 'rwsr-xr-x',
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
