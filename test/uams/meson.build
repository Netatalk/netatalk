dhx_passwd_logincont_bounds_test = executable(
    'dhx_passwd_logincont_bounds_test',
    'test_dhx_logincont_bounds.c',
    include_directories: root_includes,
    dependencies: [passwd_deps, libgcrypt],
    link_with: libatalk,
    c_args: ['-DTEST_DHX_PASSWD'],
    install: false,
)

test(
    'DHX passwd login continuation bounds',
    dhx_passwd_logincont_bounds_test,
)

dhx_passwd_key_exchange_test = executable(
    'dhx_passwd_key_exchange_test',
    'test_dhx_key_exchange.c',
    include_directories: root_includes,
    dependencies: [passwd_deps, libgcrypt],
    link_with: libatalk,
    c_args: ['-DTEST_DHX_PASSWD'],
    install: false,
)

test('DHX passwd initial key exchange', dhx_passwd_key_exchange_test)

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
    dhx_pam_logincont_bounds_test = executable(
        'dhx_pam_logincont_bounds_test',
        'test_dhx_logincont_bounds.c',
        include_directories: [pam_includes, root_includes],
        dependencies: [pam, libgcrypt],
        link_with: libatalk,
        c_args: ['-DTEST_DHX_PAM'],
        install: false,
    )

    test(
        'DHX PAM login continuation bounds',
        dhx_pam_logincont_bounds_test,
    )

    dhx_pam_key_exchange_test = executable(
        'dhx_pam_key_exchange_test',
        'test_dhx_key_exchange.c',
        include_directories: [pam_includes, root_includes],
        dependencies: [pam, libgcrypt],
        link_with: libatalk,
        c_args: ['-DTEST_DHX_PAM'],
        install: false,
    )

    test('DHX PAM initial key exchange', dhx_pam_key_exchange_test)

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

if have_krb5_uam
    gss_logincont_bounds_test = executable(
        'gss_logincont_bounds_test',
        'test_gss_logincont_bounds.c',
        include_directories: [gssapi_includes, root_includes],
        dependencies: [bstring, iniparser, gss_libs],
        link_with: libatalk,
        c_args: kerberos_c_args,
        install: false,
    )

    test(
        'GSS LoginCont username and reply bounds',
        gss_logincont_bounds_test,
    )
endif
