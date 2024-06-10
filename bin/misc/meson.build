executable(
    'netacnv',
    'netacnv.c',
    include_directories: root_includes,
    link_with: libatalk,
    install: false,
)

executable(
    'logger_test',
    'logger_test.c',
    include_directories: root_includes,
    link_with: libatalk,
    install: false,
)

executable(
    'fce',
    'fce.c',
    include_directories: root_includes,
    link_with: libatalk,
    install: false,
)

if have_ldap
    executable(
        'afpldaptest',
        'uuidtest.c',
        include_directories: root_includes,
       dependencies: ldap,
        link_with: libatalk,
        c_args: acl_ldapconf,
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
endif
