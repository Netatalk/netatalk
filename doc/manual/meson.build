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
    if pandoc.found()
        custom_target(
            'manual_' + page,
            input: [navlinks, page + '.md'],
            output: page + '.html',
            command: [
                pandoc,
                '--from', 'gfm',
                '--to', 'html',
                '@INPUT@',
            ],
            capture: true,
            install: true,
            install_dir: manual_install_path,
        )
    elif cmarkgfm.found()
        custom_target(
            'manual_' + page,
            input: [navlinks, page + '.md'],
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
    elif cmark.found()
        custom_target(
            'manual_' + page,
            input: [navlinks, page + '.md'],
            output: page + '.html',
            command: [
                'sh', '-c',
                cmark.full_path() + ' --to html "$1" "$2" | sed -e "s#<p>|#<pre>|#g" -e "s#|</p>#|</pre>#g"',
                'sh', '@INPUT@'
            ],
            capture: true,
            install: true,
            install_dir: manual_install_path,
        )
    endif
endforeach
