if build_man_docs
    subdir('manpages')
endif

if have_appletalk
    navlinks = '_Navlinks_atalk.md'
else
    navlinks = '_Navlinks.md'
endif

if build_html_docs
    subdir('manual')

    if get_option('with-docs-l10n')
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

    doxyfile = configure_file(input: 'Doxyfile.in', output: 'Doxyfile', configuration: doxyfile_conf)

    run_command(doxygen, doxyfile, check: true)

    install_subdir(
        meson.current_build_dir() / 'developer',
        install_dir: get_option('datadir') / 'doc' / meson.project_name(),
        strip_directory: false,
    )
endif
