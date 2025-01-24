manual_pages = [
    'Configuration',
    'Installation',
    'Upgrade',
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
endif

if get_option('with-website')
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
