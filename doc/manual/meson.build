manual_pages = [
    'ACL',
    'AppleTalk',
    'Authentication',
    'Charsets',
    'CNID',
    'Configuration',
    'Dircache',
    'FCE',
    'index',
    'Installation',
    'Legal',
    'License',
    'Search',
    'Upgrading',
]

manpage_user_commands_files = []
manpage_configuration_file_files = []
manpage_admin_commands_files = []

if build_man_docs
    make_manpage_toc = find_program('make_manpage_toc.sh')

    foreach page : manpage_user_commands
        manpage_user_commands_files += meson.project_source_root() / 'doc' / 'manpages' / 'man1' / (page + '.1.md')
    endforeach

    foreach page : manpage_configuration_files
        manpage_configuration_file_files += meson.project_source_root() / 'doc' / 'manpages' / 'man5' / (page + '.5.md')
    endforeach

    foreach page : manpage_admin_commands
        manpage_admin_commands_files += meson.project_source_root() / 'doc' / 'manpages' / 'man8' / (page + '.8.md')
    endforeach
endif

sidebar_config = configuration_data()

if build_man_docs
    sidebar_config.set(
        'USER_COMMANDS',
        run_command(
            make_manpage_toc,
            'sidebar',
            manpage_user_commands_files,
            check: true,
        ).stdout().strip(),
    )
    sidebar_config.set(
        'CONFIGURATION_FILES',
        run_command(
            make_manpage_toc,
            'sidebar',
            manpage_configuration_file_files,
            check: true,
        ).stdout().strip(),
    )
    sidebar_config.set(
        'ADMIN_COMMANDS',
        run_command(
            make_manpage_toc,
            'sidebar',
            manpage_admin_commands_files,
            check: true,
        ).stdout().strip(),
    )
else
    sidebar_config.set('USER_COMMANDS', '')
    sidebar_config.set('CONFIGURATION_FILES', '')
    sidebar_config.set('ADMIN_COMMANDS', '')
endif

manual_sidebar = configure_file(
    input: '_Sidebar.md',
    output: '_Sidebar.md',
    configuration: sidebar_config,
    install: build_website,
    install_dir: docs_install_path / 'manual' / 'en',
)

index_config = configuration_data()

if build_man_docs
    index_config.set(
        'USER_COMMANDS',
        run_command(
            make_manpage_toc,
            'toc',
            manpage_user_commands_files,
            check: true,
        ).stdout().strip(),
    )
    index_config.set(
        'CONFIGURATION_FILES',
        run_command(
            make_manpage_toc,
            'toc',
            manpage_configuration_file_files,
            check: true,
        ).stdout().strip(),
    )
    index_config.set(
        'ADMIN_COMMANDS',
        run_command(
            make_manpage_toc,
            'toc',
            manpage_admin_commands_files,
            check: true,
        ).stdout().strip(),
    )
else
    index_config.set('USER_COMMANDS', '')
    index_config.set('CONFIGURATION_FILES', '')
    index_config.set('ADMIN_COMMANDS', '')
endif

manual_index = configure_file(
    input: 'index.md',
    output: 'index.md',
    configuration: index_config,
    install: build_website,
    install_dir: docs_install_path / 'manual' / 'en',
)

# HTML man pages are deliberately not distributed.  Keep the manpage-reference
# sections in the website Markdown index, but remove them from the local HTML
# manual before passing it to pandoc or cmark.
manual_html_index = custom_target(
    'manual_html_index',
    input: manual_index,
    output: 'index-html.md',
    command: [
        'sed',
        '/^## User Commands$/,$d',
        '@INPUT@',
    ],
    capture: true,
)

if build_website
    manual_pages += [
        '_Sidebar',
    ]

    foreach page : manual_pages
        if page == 'index' or page == '_Sidebar'
            continue
        endif

        install_data(page + '.md', install_dir: docs_install_path + '/manual/en')
    endforeach
endif

foreach page : manual_pages
    page_input = page + '.md'
    if page == 'index'
        page_input = manual_html_index
    elif page == '_Sidebar'
        page_input = manual_sidebar
    endif

    if pandoc.found()
        custom_target(
            'manual_' + page,
            input: page_input,
            output: page + '.html',
            command: [
                pandoc,
                '--from', 'gfm+pipe_tables',
                '--to', 'html',
                '@INPUT@',
            ],
            capture: true,
            install: true,
            install_dir: docs_install_path / 'manual',
            install_tag: 'doc',
        )
    elif cmarkgfm.found()
        custom_target(
            'manual_' + page,
            input: page_input,
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
            install_dir: docs_install_path / 'manual',
            install_tag: 'doc',
        )
    elif cmark.found()
        custom_target(
            'manual_' + page,
            input: page_input,
            output: page + '.html',
            command: [
                'sh',
                '-c', cmark.full_path()
                + ' --to html "$1" "$2" | sed -e "s#<p>|#<pre>|#g" -e "s#|</p>#|</pre>#g"',
                'sh',
                '@INPUT@',
            ],
            capture: true,
            install: true,
            install_dir: docs_install_path / 'manual',
            install_tag: 'doc',
        )
    endif
endforeach
