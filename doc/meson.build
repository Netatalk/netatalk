manpage_user_commands = [
    'addump',
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'asip-status',
    'dbd',
    'fce_listen',
    'nad',
]

manpage_configuration_files = [
    'afp.conf',
    'afp_signature.conf',
    'afp_voluuid.conf',
    'extmap.conf',
]

manpage_admin_commands = [
    'afpd',
    'netatalk',
]

if build_dbd_docs
    manpage_admin_commands += [
        'cnid_dbd',
        'cnid_metad',
    ]
endif

if enable_fce
    manpage_admin_commands += [
        'fce_ev_script',
    ]
endif

if build_testsuite
    manpage_user_commands += [
        'afp_lantest',
        'afp_logintest',
        'afp_spectest',
        'afp_speedtest',
        'afparg',
    ]
endif

if enable_appletalk
    manpage_user_commands += [
        'aecho',
        'getzones',
        'nbplkup',
        'nbprgstr',
        'pap',
        'rtmpqry',
    ]
    manpage_configuration_files += [
        'atalkd.conf',
        'papd.conf',
    ]
    manpage_admin_commands += [
        'a2boot',
        'atalkd',
        'macipgw',
        'papd',
        'papstatus',
        'timelord',
    ]
endif

if build_man_docs
    subdir('manpages')
endif

if build_html_docs
    subdir('manual')

    if build_docs_l10n
        po4a = find_program('po4a', required: true)
        if po4a.found()
            run_command(
                po4a,
                '--keep', '0',
                '--option', 'markdown',
                '--package-name', 'Netatalk',
                '--package-version', netatalk_version,
                '--copyright-holder', 'Contributors to the Netatalk Project',
                'po4a.cfg',
                check: true,
            )

            subdir('translated')
        endif
    endif
endif

if build_dev_docs and doxygen.found()
    doxyfile_conf = configuration_data()
    doxyfile_conf.set('PACKAGE', meson.project_name())
    doxyfile_conf.set('VERSION', meson.project_version())
    doxyfile_conf.set('ABS_TOP_SRCDIR', meson.project_source_root())
    doxyfile_conf.set('OUTPUT_DIR', meson.current_build_dir())

    if strict_doxygen
        doxyfile_conf.set('WARN_AS_ERROR', 'YES')
    else
        doxyfile_conf.set('WARN_AS_ERROR', 'NO')
    endif

    doxyfile = configure_file(input: 'Doxyfile.in', output: 'Doxyfile', configuration: doxyfile_conf)

    run_command(doxygen, doxyfile, check: true)

    install_subdir(
        meson.current_build_dir() / 'developer',
        install_dir: datadir / 'doc' / meson.project_name(),
        strip_directory: false,
    )

    if build_website
        install_subdir(
            meson.current_build_dir() / 'developer',
            install_dir: docs_install_path / 'public',
            strip_directory: false,
        )
    endif
endif
