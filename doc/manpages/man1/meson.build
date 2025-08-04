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
        'nbp',
        'pap',
        'rtmpqry',
    ]
endif

if get_option('with-testsuite')
    manfiles += [
        'afp_lantest',
        'afp_logintest',
        'afp_spectest',
        'afp_speedtest',
        'afparg',
    ]
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

install_data('afptest.1', install_dir: mandir / 'man1')

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
        install_data(page + '.1.md', install_dir: manual_install_path + '/en')
    endforeach
endif
