manual_pages = [
    'Configuration',
    'Installation',
    'License',
    'Upgrading',
    'index',
]

if have_appletalk
    manual_pages += [
        'AppleTalk',
    ]
endif

if get_option('with-website')
    manual_pages += [
        '_Sidebar',
    ]

    foreach page : manual_pages
        install_data(page + '.md', install_dir: manual_install_path + '/en')
    endforeach
endif

foreach page : manual_pages
    custom_target(
        'manual_' + page,
        input: ['_Navlinks.md', page + '.md'],
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
