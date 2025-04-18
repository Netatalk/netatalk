if get_option('default_library') != 'static'
    build_shared_lib = true
else
    build_shared_lib = false
endif

if host_os == 'darwin'
    lib_suffix = 'so'
else
    lib_suffix = []
endif

passwd_deps = []
if crypt.found()
    passwd_deps += crypt
endif

uams_guest_sources = ['uams_guest.c']

library(
    'uams_guest',
    uams_guest_sources,
    include_directories: root_includes,
    name_prefix: '',
    name_suffix: lib_suffix,
    override_options: 'b_lundef=false',
    install: true,
    install_dir: libdir / 'netatalk',
)

uams_passwd_sources = ['uams_passwd.c']

library(
    'uams_passwd',
    uams_passwd_sources,
    include_directories: root_includes,
    dependencies: passwd_deps,
    name_prefix: '',
    name_suffix: lib_suffix,
    override_options: 'b_lundef=false',
    install: true,
    install_dir: libdir / 'netatalk',
)

if have_libgcrypt
    uams_randnum_sources = ['uams_randnum.c']
    uams_dhx2_passwd_sources = ['uams_dhx2_passwd.c']
    uams_dhx_passwd_sources = ['uams_dhx_passwd.c']

    library(
        'uams_randnum',
        uams_randnum_sources,
        include_directories: root_includes,
        dependencies: [crack, libgcrypt],
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    library(
        'uams_dhx_passwd',
        uams_dhx_passwd_sources,
        include_directories: root_includes,
        dependencies: [passwd_deps, libgcrypt],
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    library(
        'uams_dhx2_passwd',
        uams_dhx2_passwd_sources,
        include_directories: root_includes,
        dependencies: [passwd_deps, libgcrypt],
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )
    if have_pam
        uams_dhx2_pam_sources = ['uams_dhx2_pam.c']

        library(
            'uams_dhx2_pam',
            uams_dhx2_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [iniparser, pam, libgcrypt],
            name_prefix: '',
            name_suffix: lib_suffix,
            override_options: 'b_lundef=false',
            install: true,
            install_dir: libdir / 'netatalk',
        )

        if build_shared_lib
            install_symlink(
                'uams_dhx2.so',
                install_dir: libdir / 'netatalk',
                pointing_to: 'uams_dhx2_pam.so',
            )
        endif
    elif build_shared_lib
        install_symlink(
            'uams_dhx2.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx2_passwd.so',
        )
    endif
    if have_pam
        uams_dhx_pam_sources = ['uams_dhx_pam.c']

        library(
            'uams_dhx_pam',
            uams_dhx_pam_sources,
            include_directories: [pam_includes, root_includes],
            dependencies: [pam, libgcrypt],
            name_prefix: '',
            name_suffix: lib_suffix,
            override_options: 'b_lundef=false',
            install: true,
            install_dir: libdir / 'netatalk',
        )

        if build_shared_lib
            install_symlink(
                'uams_dhx.so',
                install_dir: libdir / 'netatalk',
                pointing_to: 'uams_dhx_pam.so',
            )
        endif
    elif build_shared_lib
        install_symlink(
            'uams_dhx.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx_passwd.so',
        )
    endif

endif

if have_pam
    uams_pam_sources = ['uams_pam.c']

    library(
        'uams_pam',
        uams_pam_sources,
        include_directories: [pam_includes, root_includes],
        dependencies: pam,
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    if build_shared_lib
        install_symlink(
            'uams_clrtxt.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_pam.so',
        )
    endif
elif build_shared_lib
    install_symlink(
        'uams_clrtxt.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_passwd.so',
    )
endif

if have_krb5_uam
    uams_gss_sources = ['uams_gss.c']

    library(
        'uams_gss',
        uams_gss_sources,
        include_directories: [gssapi_includes, root_includes],
        dependencies: [iniparser, gss_libs],
        c_args: kerberos_c_args,
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )
endif
