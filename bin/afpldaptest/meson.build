afpldaptest_deps = [iniparser, ldap]

if use_mysql_backend
    afpldaptest_deps += mysql_deps
endif

executable(
    'afpldaptest',
    ['uuidtest.c'],
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: afpldaptest_deps,
    c_args: confdir,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
