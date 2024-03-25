acl_sources = [
    'cache.c',
    'unix.c',
    'uuid.c',
]

if have_ldap
    acl_sources += [
        'ldap.c',
        'ldap_config.c',
    ]
endif

libacl = static_library(
    'acl',
    acl_sources,
    include_directories: root_includes,
    dependencies: [acl_deps, ldap],
    install: false,
)
