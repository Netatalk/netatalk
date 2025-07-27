manual_pages = [
    'Configuration',
    'index',
    'Installation',
    'Legal',
    'License',
    'Upgrading',
]

if get_option('with-website')
    manual_pages += [
        '_Sidebar',
        'ad.1',
        'addump.1',
        'afp.conf.5',
        'afp_signature.conf.5',
        'afp_voluuid.conf.5',
        'afpd.8',
        'afpldaptest.1',
        'afppasswd.1',
        'afpstats.1',
        'afptest.1',
        'asip-status.1',
        'cnid_dbd.8',
        'cnid_metad.8',
        'dbd.1',
        'extmap.conf.5',
        'macusers.1',
        'netatalk.8',
    ]
endif

if have_appletalk
    manual_pages += [
        'AppleTalk',
    ]

    if get_option('with-website')
        manual_pages += [
            'a2boot.8',
            'aecho.1',
            'atalk.4',
            'atalk_aton.3',
            'atalkd.8',
            'atalkd.conf.5',
            'getzones.1',
            'macipgw.8',
            'nbp.1',
            'nbp_name.3',
            'pap.1',
            'papd.8',
            'papd.conf.5',
            'papstatus.8',
            'rtmpqry.1',
            'timelord.8',
        ]
    endif
endif

if get_option('with-website')
    foreach page : manual_pages
        install_data(page + '.md', install_dir: manual_install_path + '/ja')
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
                install_dir: manual_install_path + '/ja',
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
                install_dir: manual_install_path + '/ja',
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
                install_dir: manual_install_path + '/ja',
            )
        endif
    endforeach
endif
