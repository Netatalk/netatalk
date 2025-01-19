if build_man_docs
    subdir('manpages')
endif

if build_html_docs
    po4a = find_program('po4a', required: false)
    if po4a.found() and get_option('with-docs-l10n')
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
    else
        warning('po4a is required for localized documentation generation')
    endif

    subdir('manual')
endif

if 'readmes' in get_option('with-docs') and cmarkgfm.found()
    readmes = [
        'DEVELOPER',
    ]

    if have_appletalk
        readmes += [
            'README.AppleTalk',
        ]
    endif

    foreach file : readmes
        install_data(
            file,
            install_dir: datadir / 'doc/netatalk',
        )
    endforeach
endif
