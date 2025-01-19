manfiles = [
    'ad',
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'afptest',
    'addump',
    'asip-status',
    'dbd',
    'macusers',
]

if have_appletalk
    manfiles += [
        'aecho',
        'getzones',
        'nbp',
        'pap',
    ]
endif

foreach man : manfiles
    custom_target(
        man + '.1',
        input: man + '.1.md',
        output: man + '.1',
        command: [
            make_man,
            cmark_man,
            '@INPUT@',
            man.to_upper(),
            '1',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man1',
        build_by_default: true,
    )
endforeach

afptest_staticmans = [
    'afp_lantest.1',
    'afp_logintest.1',
    'afp_spectest.1',
    'afp_speedtest.1',
    'afparg.1',
]

foreach man : afptest_staticmans
    install_data('afptest.1', rename: man, install_dir: mandir / 'man1')
endforeach

if have_appletalk
    nbp_staticmans = [
        'nbplkup.1',
        'nbprgstr.1',
        'nbpunrgstr.1',
    ]

    foreach man : nbp_staticmans
        install_data('nbp.1', rename: man, install_dir: mandir / 'man1')
    endforeach
endif

if get_option('with-website')
    foreach page : manfiles
        custom_target(
            'manual_' + page,
            input: page + '.1.md',
            output: page + '.1.html',
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
