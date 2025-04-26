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
