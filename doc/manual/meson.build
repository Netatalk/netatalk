manual_pages = [
    'configuration',
    'install',
    'intro',
    'upgrade',
]

if have_appletalk
    manual_pages += [
        'appletalk',
    ]
endif

if get_option('with-website')
    manual_pages += [
        'compile',
        'gpl-2.0',
    ]
endif

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
