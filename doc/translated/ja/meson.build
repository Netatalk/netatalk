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

ja_manpage_user_commands_files = []
ja_manpage_configuration_file_files = []
ja_manpage_admin_commands_files = []
if build_man_docs
    make_manpage_toc = find_program(
        meson.project_source_root() / 'doc' / 'manual' / 'make_manpage_toc.sh',
    )
    foreach page : manpage_user_commands
        ja_manpage_user_commands_files += meson.project_source_root() / 'doc' / 'manpages' / 'man1' / (page + '.1.md')
    endforeach

    foreach page : manpage_configuration_files
        ja_manpage_configuration_file_files += meson.project_source_root() / 'doc' / 'manpages' / 'man5' / (page + '.5.md')
    endforeach

    foreach page : manpage_admin_commands
        ja_manpage_admin_commands_files += meson.project_source_root() / 'doc' / 'manpages' / 'man8' / (page + '.8.md')
    endforeach
endif

ja_sidebar_config = configuration_data()
if build_man_docs
    ja_sidebar_config.set(
        'USER_COMMANDS',
        run_command(
            make_manpage_toc,
            'sidebar',
            ja_manpage_user_commands_files,
            check: true,
        ).stdout().strip(),
    )
    ja_sidebar_config.set(
        'CONFIGURATION_FILES',
        run_command(
            make_manpage_toc,
            'sidebar',
            ja_manpage_configuration_file_files,
            check: true,
        ).stdout().strip(),
    )
    ja_sidebar_config.set(
        'ADMIN_COMMANDS',
        run_command(
            make_manpage_toc,
            'sidebar',
            ja_manpage_admin_commands_files,
            check: true,
        ).stdout().strip(),
    )
else
    ja_sidebar_config.set('USER_COMMANDS', '')
    ja_sidebar_config.set('CONFIGURATION_FILES', '')
    ja_sidebar_config.set('ADMIN_COMMANDS', '')
endif
manual_sidebar = configure_file(
    input: '_Sidebar.md',
    output: '_Sidebar.md',
    configuration: ja_sidebar_config,
    install: build_website,
    install_dir: docs_install_path / 'manual' / 'ja',
)

ja_index_config = configuration_data()
if build_man_docs
    ja_index_config.set(
        'USER_COMMANDS',
        run_command(
            make_manpage_toc,
            'toc',
            ja_manpage_user_commands_files,
            check: true,
        ).stdout().strip(),
    )
    ja_index_config.set(
        'CONFIGURATION_FILES',
        run_command(
            make_manpage_toc,
            'toc',
            ja_manpage_configuration_file_files,
            check: true,
        ).stdout().strip(),
    )
    ja_index_config.set(
        'ADMIN_COMMANDS',
        run_command(
            make_manpage_toc,
            'toc',
            ja_manpage_admin_commands_files,
            check: true,
        ).stdout().strip(),
    )
else
    ja_index_config.set('USER_COMMANDS', '')
    ja_index_config.set('CONFIGURATION_FILES', '')
    ja_index_config.set('ADMIN_COMMANDS', '')
endif
manual_index = configure_file(
    input: 'index.md',
    output: 'index.md',
    configuration: ja_index_config,
    install: build_website,
    install_dir: docs_install_path / 'manual' / 'ja',
)

# Localized HTML manuals are built without localized HTML man pages.  Keep
# these references for the website, but omit them from local HTML output.
manual_html_index = custom_target(
    'manual_html_index',
    input: manual_index,
    output: 'index-html.md',
    command: [
        'sed',
        '/^## ユーザーコマンド$/,$d',
        '@INPUT@',
    ],
    capture: true,
)

if build_website
    manual_pages += [
        '_Sidebar',
    ]

    if build_man_docs
        foreach page : manpage_user_commands
            manual_pages += page + '.1'
        endforeach

        foreach page : manpage_configuration_files
            manual_pages += page + '.5'
        endforeach

        foreach page : manpage_admin_commands
            manual_pages += page + '.8'
        endforeach
    endif

    foreach page : manual_pages
        if page == 'index' or page == '_Sidebar'
            continue
        endif

        custom_target(
            'website_' + page,
            input: page + '.md',
            output: page + '.md',
            command: [
                'sed',
                '-e', 's/^# 名前$/# ' + page + '/',
                '-e', '1s/[.][0-9][0-9]*$//',
                '@INPUT@',
            ],
            capture: true,
            install: true,
            install_dir: docs_install_path + '/manual/ja',
        )
    endforeach
else
    foreach page : manual_pages
        page_input = page + '.md'
        if page == 'index'
            page_input = manual_html_index
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
                install_dir: docs_install_path / 'manual' / 'ja',
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
                install_dir: docs_install_path / 'manual' / 'ja',
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
                install_dir: docs_install_path / 'manual' / 'ja',
            )
        endif
    endforeach
endif
