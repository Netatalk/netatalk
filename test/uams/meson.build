dhx2_passwd_bounds_test = executable(
    'dhx2_passwd_bounds_test',
    'test_dhx2_password_bounds.c',
    include_directories: root_includes,
    dependencies: [passwd_deps, libgcrypt],
    link_with: libatalk,
    c_args: ['-DTEST_DHX2_PASSWD'],
    install: false,
)

test(
    'DHX2 passwd rejects unterminated password',
    dhx2_passwd_bounds_test,
)

if have_pam
    dhx2_pam_bounds_test = executable(
        'dhx2_pam_bounds_test',
        'test_dhx2_password_bounds.c',
        include_directories: [pam_includes, root_includes],
        dependencies: [iniparser, pam, libgcrypt],
        link_with: libatalk,
        c_args: ['-DTEST_DHX2_PAM'],
        install: false,
    )

    test(
        'DHX2 PAM rejects unterminated password',
        dhx2_pam_bounds_test,
    )
endif
