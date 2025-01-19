manual_pages = [
    'configuration.ja',
    'install.ja',
    'intro.ja',
    'upgrade.ja',
]

if get_option('with-website')
    manual_pages += [
        'ad.1.ja',
        'addump.1.ja',
        'afp.conf.5.ja',
        'afp_signature.conf.5.ja',
        'afp_voluuid.conf.5.ja',
        'afpldaptest.1.ja',
        'afpstats.1.ja',
        'afptest.1.ja',
        'asip-status.1.ja',
        'cnid_dbd.8.ja',
        'cnid_metad.8.ja',
        'compile.ja',
        'dbd.1.ja',
        'extmap.conf.5.ja',
        'gpl-2.0.ja',
        'macusers.1.ja',
        'netatalk.8.ja',
    ]
endif

if have_appletalk
    manual_pages += [
        'appletalk.ja',
    ]

    if get_option('with-website')
        manual_pages += [
            'a2boot.8.ja',
            'aecho.1.ja',
            'atalk.4.ja',
            'atalk_aton.3.ja',
            'atalkd.8.ja',
            'atalkd.conf.5.ja',
            'getzones.1.ja',
            'macipgw.8.ja',
            'nbp.1.ja',
            'nbp_name.3.ja',
            'pap.1.ja',
            'papd.8.ja',
            'papd.conf.5.ja',
            'papstatus.8.ja',
            'timelord.8.ja',
        ]
    endif
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
        install_dir: manual_install_path + '/ja',
    )
endforeach
