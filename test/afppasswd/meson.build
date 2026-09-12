afppasswd_migrate_test = executable(
    'afppasswd_migrate_test',
    [
        'test_migrate.c',
        '../../bin/afppasswd/afppasswd_migrate.c',
    ],
    include_directories: [root_includes, '../../bin/afppasswd'],
    link_with: libatalk,
    install: false,
)

test(
    'afppasswd legacy SRP migration',
    afppasswd_migrate_test,
)

afppasswd_test_deps = [libgcrypt]

if have_cracklib
    afppasswd_test_deps += crack
endif

afppasswd_test = executable(
    'afppasswd_test',
    [
        'test_afppasswd.c',
        '../../bin/afppasswd/afppasswd_migrate.c',
    ],
    include_directories: [root_includes, '../../bin/afppasswd'],
    dependencies: afppasswd_test_deps,
    link_with: libatalk,
    c_args: [
        afpdpwfile,
        afpdsrpverifierpath,
        confdir,
        dversion,
    ],
    install: false,
)

test(
    'afppasswd SRP path and verifier validation',
    afppasswd_test,
)
