manfiles = [
    'ad',
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'addump',
    'asip-status',
    'dbd',
    'fce_listen',
    'macusers',
]

if have_appletalk
    manfiles += [
        'aecho',
        'getzones',
        'nbplkup',
        'nbprgstr',
        'pap',
        'rtmpqry',
    ]

    install_data(['nbp.1', 'nbpunrgstr.1'], install_dir: mandir / 'man1')
endif

if get_option('with-testsuite')
    manfiles += [
        'afp_lantest',
        'afp_logintest',
        'afp_spectest',
        'afp_speedtest',
        'afparg',
    ]

    install_data('afptest.1', install_dir: mandir / 'man1')
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
        build_by_default: true,
    )
endforeach

if get_option('with-website')
    foreach page : manfiles
        install_data(page + '.1.md', install_dir: manual_install_path + '/en')
    endforeach
endif
