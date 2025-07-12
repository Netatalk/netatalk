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

library(
    'uams_guest',
    ['uams_guest.c'],
    include_directories: root_includes,
    name_prefix: '',
    name_suffix: lib_suffix,
    override_options: 'b_lundef=false',
    install: true,
    install_dir: libdir / 'netatalk',
)

library(
    'uams_passwd',
    ['uams_passwd.c'],
    include_directories: root_includes,
    dependencies: passwd_deps,
    name_prefix: '',
    name_suffix: lib_suffix,
    override_options: 'b_lundef=false',
    install: true,
    install_dir: libdir / 'netatalk',
)

library(
    'uams_randnum',
    ['uams_randnum.c'],
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
    ['uams_dhx_passwd.c'],
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
    ['uams_dhx2_passwd.c'],
    include_directories: root_includes,
    dependencies: [passwd_deps, libgcrypt],
    name_prefix: '',
    name_suffix: lib_suffix,
    override_options: 'b_lundef=false',
    install: true,
    install_dir: libdir / 'netatalk',
)

if have_pam
    library(
        'uams_dhx_pam',
        ['uams_dhx_pam.c'],
        include_directories: [pam_includes, root_includes],
        dependencies: [pam, libgcrypt],
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    library(
        'uams_dhx2_pam',
        ['uams_dhx2_pam.c'],
        include_directories: [pam_includes, root_includes],
        dependencies: [iniparser, pam, libgcrypt],
        name_prefix: '',
        name_suffix: lib_suffix,
        override_options: 'b_lundef=false',
        install: true,
        install_dir: libdir / 'netatalk',
    )

    library(
        'uams_pam',
        ['uams_pam.c'],
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
        install_symlink(
            'uams_dhx.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx_pam.so',
        )
        install_symlink(
            'uams_dhx2.so',
            install_dir: libdir / 'netatalk',
            pointing_to: 'uams_dhx2_pam.so',
        )
    endif
elif build_shared_lib
    install_symlink(
        'uams_clrtxt.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_passwd.so',
    )
    install_symlink(
        'uams_dhx.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_dhx_passwd.so',
    )
    install_symlink(
        'uams_dhx2.so',
        install_dir: libdir / 'netatalk',
        pointing_to: 'uams_dhx2_passwd.so',
    )
endif

if have_krb5_uam
    library(
        'uams_gss',
        ['uams_gss.c'],
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
