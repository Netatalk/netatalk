afppasswd_deps = [libgcrypt]
afppasswd_libs = [libatalk]

if have_cracklib
    afppasswd_deps += crack
endif

if use_mysql_backend
    afppasswd_deps += mysqlclient
endif

if have_iconv
    afppasswd_deps += iconv
endif

executable(
    'afppasswd',
    ['afppasswd.c', 'afppasswd_migrate.c'],
    include_directories: root_includes,
    dependencies: afppasswd_deps,
    link_with: afppasswd_libs,
    c_args: [
        afpdpwfile,
        afpdsrpverifierpath,
        confdir,
        dversion,
    ],
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
