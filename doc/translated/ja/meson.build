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
    'Interoperability',
    'Legal',
    'License',
    'Search',
    'Upgrading',
]

if get_option('with-website')
    manual_pages += [
        '_Sidebar',
        'a2boot.8',
        'addump.1',
        'aecho.1',
        'afp.conf.5',
        'afp_signature.conf.5',
        'afp_voluuid.conf.5',
        'afpd.8',
        'afp_lantest.1',
        'afp_logintest.1',
        'afp_spectest.1',
        'afp_speedtest.1',
        'afparg.1',
        'afpldaptest.1',
        'afppasswd.1',
        'afpstats.1',
        'asip-status.1',
        'atalkd.8',
        'atalkd.conf.5',
        'cnid_dbd.8',
        'cnid_metad.8',
        'dbd.1',
        'extmap.conf.5',
        'fce_listen.1',
        'getzones.1',
        'macipgw.8',
        'nad.1',
        'nbplkup.1',
        'nbprgstr.1',
        'netatalk.8',
        'pap.1',
        'papd.8',
        'papd.conf.5',
        'papstatus.8',
        'rtmpqry.1',
        'timelord.8',
    ]
endif

if get_option('with-website')
    foreach page : manual_pages
        install_data(page + '.md', install_dir: docs_install_path + '/manual/ja')
    endforeach
else
    foreach page : manual_pages
        if pandoc.found()
            custom_target(
                'manual_' + page,
                input: [navlinks, page + '.md'],
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
                install_dir: docs_install_path / 'manual' / 'ja',
            )
        elif cmark.found()
            custom_target(
                'manual_' + page,
                input: [navlinks, page + '.md'],
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
