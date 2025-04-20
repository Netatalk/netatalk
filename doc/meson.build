if pandoc.found()
    transcoder = pandoc
elif cmarkgfm.found()
    transcoder = cmarkgfm
elif cmark.found()
    transcoder = cmark
else
    warning('No supported Markdown parser found')
endif

if build_man_docs
    subdir('manpages')
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

if get_option('with-website')
    python = find_program('python3', required: false)
    if python.found()
        run_command(
            python,
            'generate_compile_docs.py',
            check: true,
        )
    endif
endif
