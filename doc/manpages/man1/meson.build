manfiles = [
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'addump',
    'asip-status',
    'dbd',
    'fce_listen',
    'nad',
]

if enable_appletalk
    manfiles += [
        'aecho',
        'getzones',
        'nbplkup',
        'nbprgstr',
        'pap',
        'rtmpqry',
    ]

    install_data(
        ['nbp.1', 'nbpunrgstr.1'],
        install_dir: mandir / 'man1',
        install_tag: 'man',
    )
endif

if build_testsuite
    manfiles += [
        'afp_lantest',
        'afp_logintest',
        'afp_spectest',
        'afp_speedtest',
        'afparg',
    ]

    install_data('afptest.1', install_dir: mandir / 'man1', install_tag: 'man')
endif

foreach man : manfiles
    custom_target(
        man + '.1',
        input: man + '.1.md',
        output: man + '.1',
        command: [
            make_man,
            transcoder,
            '@INPUT@',
            man.to_upper(),
            '1',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man1',
        install_tag: 'man',
        build_by_default: true,
    )
endforeach

if build_website
    foreach page : manfiles
        custom_target(
            'website_' + page,
            input: page + '.1.md',
            output: page + '.1.md',
            command: ['sed', 's/^# Name$/# ' + page + '/', '@INPUT@'],
            capture: true,
            install: true,
            install_dir: docs_install_path + '/manual/en',
        )
    endforeach
endif
