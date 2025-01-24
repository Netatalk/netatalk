manual_pages = [
    'セットアップ',
    'インストール',
    'アップグレード',
]

if get_option('with-website')
    manual_pages += [
        '_Sidebar',
        'ad',
        'addump',
        'afp.conf',
        'afp_signature.conf',
        'afp_voluuid.conf',
        'afpd',
        'afpldaptest',
        'afppasswd',
        'afpstats',
        'afptest',
        'asip-status',
        'cnid_dbd',
        'cnid_metad',
        'dbd',
        'extmap.conf',
        'index',
        'macusers',
        'netatalk',
        'コンパイル',
        'ライセンス',
    ]
endif

if have_appletalk
    manual_pages += [
        'AppleTalk',
    ]

    if get_option('with-website')
        manual_pages += [
            'a2boot',
            'aecho',
            'atalk',
            'atalk_aton',
            'atalkd',
            'atalkd.conf',
            'getzones',
            'macipgw',
            'nbp',
            'nbp_name',
            'pap',
            'papd',
            'papd.conf',
            'papstatus',
            'timelord',
        ]
    endif
endif

if get_option('with-website')
    foreach page : manual_pages
        install_data(page + '.md', install_dir: manual_install_path + '/ja')
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
            install_dir: manual_install_path + '/ja',
        )
    endforeach
endif
