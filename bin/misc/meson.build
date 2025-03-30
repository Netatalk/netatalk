if have_iconv
    executable(
        'netacnv',
        ['netacnv.c'],
        include_directories: root_includes,
        link_with: libatalk,
        dependencies: [iconv],
        install: false,
    )
endif

executable(
    'logger_test',
    ['logger_test.c'],
    include_directories: root_includes,
    link_with: libatalk,
    install: false,
)

executable(
    'fce_listen',
    ['fce.c'],
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: [iniparser],
    install: true,
)

afpldaptest_deps = [iniparser, ldap]

if use_mysql_backend
    afpldaptest_deps += mysqlclient
endif

if have_ldap
    executable(
        'afpldaptest',
        ['uuidtest.c'],
        include_directories: root_includes,
        link_with: libatalk,
        dependencies: [afpldaptest_deps],
        c_args: confdir,
        install: true,
        build_rpath: rpath_libdir,
        install_rpath: rpath_libdir,
    )
endif
