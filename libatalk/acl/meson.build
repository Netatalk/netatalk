acl_sources = [
    'cache.c',
    'unix.c',
    'uuid.c',
]

libacl_deps = [acl_deps]

if have_ldap
    acl_sources += [
        'ldap.c',
        'ldap_config.c',
    ]
    libacl_deps += ldap
endif

libacl = static_library(
    'acl',
    acl_sources,
    include_directories: root_includes,
    dependencies: libacl_deps,
    install: false,
)
