manual_pages = [
    'Configuration',
    'Installation',
    'Upgrading',
]

if have_appletalk
    manual_pages += [
        'AppleTalk',
    ]
endif

if get_option('with-website')
    manual_pages += [
        '_Sidebar',
        'Compilation',
        'index',
        'License',
    ]

    python = find_program('python3', required: false)
    if python.found()
        run_command(
            python,
            'generate_compile_docs.py',
            check: true,
            )
    endif

    foreach page : manual_pages
        install_data(page + '.md', install_dir: manual_install_path + '/en')
    endforeach
else
    foreach page : manual_pages
        custom_target(
            'manual_' + page,
            input: page + '.md',
            output: page + '.html',
            command: [
                cmarkgfm,
                '--smart',
                '--extension', 'table',
                '--to', 'html',
                '@INPUT@',
            ],
            capture: true,
            install: true,
            install_dir: manual_install_path,
        )
    endforeach
endif
