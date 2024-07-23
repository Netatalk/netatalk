uams_guest_sources = ['uams_guest.c']

shared_module(
    'uams_guest',
    uams_guest_sources,
    include_directories: root_includes,
    name_prefix: '',
    name_suffix: 'so',
    install: true,
    install_dir: libdir / 'netatalk',
)

static_library(
    'uams_guest',
    uams_guest_sources,
    include_directories: root_includes,
    name_prefix: '',
    install: true,
    install_dir: libdir / 'netatalk',
)

uams_passwd_sources = ['uams_passwd.c']

shared_module(
    'uams_passwd',
    uams_passwd_sources,
    include_directories: root_includes,
    name_prefix: '',
    name_suffix: 'so',
    install: true,
    install_dir: libdir / 'netatalk',
)

static_library(
    'uams_passwd',
    uams_passwd_sources,
    include_directories: root_includes,
    name_prefix: '',
    install: true,
    install_dir: libdir / 'netatalk',
)

ssl_links = []

if have_embedded_ssl
    ssl_deps += libssl
endif

if have_ssl
    uams_dhx_passwd_sources = ['uams_dhx_passwd.c']

    shared_module(
        'uams_dhx_passwd',
        uams_dhx_passwd_sources,
        include_directories: root_includes,
        dependencies: ssl_deps,
        link_with: ssl_links,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
        build_rpath: rpath_libdir,
        install_rpath: rpath_libdir,
    )
    static_library(
        'uams_dhx_passwd',
        uams_dhx_passwd_sources,
        include_directories: root_includes,
        dependencies: ssl_deps,
        link_with: ssl_links,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
        build_rpath: rpath_libdir,
        install_rpath: rpath_libdir,
    )

    if have_pam
        uams_dhx_pam_sources = ['uams_dhx_pam.c']

        shared_module(
            'uams_dhx_pam',
            uams_dhx_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [pam, ssl_deps],
            link_with: ssl_links,
            name_prefix: '',
            name_suffix: 'so',
            install: true,
            install_dir: libdir / 'netatalk',
            build_rpath: rpath_libdir,
            install_rpath: rpath_libdir,
        )
        static_library(
            'uams_dhx_pam',
            uams_dhx_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [pam, ssl_deps],
            link_with: ssl_links,
            name_prefix: '',
            install: true,
            install_dir: libdir / 'netatalk',
            build_rpath: rpath_libdir,
            install_rpath: rpath_libdir,
        )

        install_symlink(
            'uams_dhx.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx_pam.so',
        )
    else
        install_symlink(
            'uams_dhx.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx_passwd.so',
        )
    endif
endif

if have_libgcrypt
    uams_dhx2_passwd_sources = ['uams_dhx2_passwd.c']

    shared_module(
        'uams_dhx2_passwd',
        uams_dhx2_passwd_sources,
        include_directories: root_includes,
        dependencies: libgcrypt,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    static_library(
        'uams_dhx2_passwd',
        uams_dhx2_passwd_sources,
        include_directories: root_includes,
        dependencies: libgcrypt,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
    )
    if have_pam
        uams_dhx2_pam_sources = ['uams_dhx2_pam.c']

        shared_module(
            'uams_dhx2_pam',
            uams_dhx2_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [pam, libgcrypt],
            name_prefix: '',
            name_suffix: 'so',
            install: true,
            install_dir: libdir / 'netatalk',
        )

        static_library(
            'uams_dhx2_pam',
            uams_dhx2_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [pam, libgcrypt],
            name_prefix: '',
            install: true,
            install_dir: libdir / 'netatalk',
        )

        install_symlink(
            'uams_dhx2.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx2_pam.so',
        )
    else
        install_symlink(
            'uams_dhx2.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx2_passwd.so',
        )
    endif
endif

if have_pam
    uams_pam_sources = ['uams_pam.c']

    shared_module(
        'uams_pam',
        uams_pam_sources,
        include_directories: [pam_includes, root_includes],
        dependencies: pam,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    static_library(
        'uams_pam',
        uams_pam_sources,
        include_directories: [pam_includes, root_includes],
        dependencies: pam,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    install_symlink(
        'uams_clrtxt.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_pam.so',
    )
else
    install_symlink(
        'uams_clrtxt.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_passwd.so',
    )
endif

if have_ssl
    uams_randnum_sources = ['uams_randnum.c']

    shared_module(
        'uams_randnum',
        uams_randnum_sources,
        include_directories: root_includes,
        dependencies: [crack, pam, ssl_deps, nettle],
        link_with: ssl_links,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
        build_rpath: rpath_libdir,
        install_rpath: rpath_libdir,
    )
    static_library(
        'uams_randnum',
        uams_randnum_sources,
        include_directories: root_includes,
        dependencies: [crack, pam, ssl_deps, nettle],
        link_with: ssl_links,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
        build_rpath: rpath_libdir,
        install_rpath: rpath_libdir,
    )
endif

if enable_pgp_uam
    uams_pgp_sources = ['uams_pgp.c']

    shared_module(
        'uams_pgp',
        uams_pgp_sources,
        include_directories: root_includes,
        dependencies: ssl_deps,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    static_library(
        'uams_pgp',
        uams_pgp_sources,
        include_directories: root_includes,
        dependencies: ssl_deps,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
    )
endif

if have_krb5_uam
    uams_gss_sources = ['uams_gss.c']

    shared_module(
        'uams_gss',
        uams_gss_sources,
        include_directories: [gssapi_includes, root_includes],
        dependencies: gss_libs,
        c_args: kerberos_c_args,
        name_prefix: '',
        name_suffix: 'so',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    static_library(
        'uams_gss',
        uams_gss_sources,
        include_directories: [gssapi_includes, root_includes],
        dependencies: gss_libs,
        c_args: kerberos_c_args,
        name_prefix: '',
        install: true,
        install_dir: libdir / 'netatalk',
    )
endif
