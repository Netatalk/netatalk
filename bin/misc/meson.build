netacnv_sources = ['netacnv.c']

loggertest_sources = ['logger_test.c']

fce_sources = ['fce.c']

afpldaptest_sources = ['uuidtest.c']

executable(
    'netacnv',
    netacnv_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: mysqlclient,
    install: false,
)

executable(
    'logger_test',
    loggertest_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: mysqlclient,
    install: false,
)

executable(
    'fce',
    fce_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: mysqlclient,
    install: false,
)

afpldaptest_deps = []

if use_mysql_backend
    afpldaptest_deps += mysqlclient
endif

if have_ldap
    if enable_rpath
        executable(
            'afpldaptest',
            afpldaptest_sources,
            include_directories: root_includes,
            link_with: libatalk,
            dependencies: [afpldaptest_deps, ldap],
            c_args: confdir,
            install: true,
            build_rpath: libdir,
            install_rpath: libdir,
        )
    else
        executable(
            'afpldaptest',
            afpldaptest_sources,
            include_directories: root_includes,
            link_with: libatalk,
            dependencies: [afpldaptest_deps, ldap],
            c_args: confdir,
            install: true,
        )
    endif
endif
