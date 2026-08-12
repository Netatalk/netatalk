manfiles = [
    'afp_signature.conf',
    'afp_voluuid.conf',
    'afp.conf',
    'extmap.conf',
]

if enable_appletalk
    manfiles += [
        'atalkd.conf',
        'papd.conf',
    ]

    install_data('macipgw.conf.5', install_dir: mandir / 'man5', install_tag: 'man')
endif

foreach man : manfiles
    custom_target(
        man + '.5',
        input: man + '.5.md',
        output: man + '.5',
        command: [
            make_man,
            transcoder,
            '@INPUT@',
            man.to_upper(),
            '5',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man5',
        install_tag: 'man',
        build_by_default: true,
    )
endforeach

if build_website
    foreach page : manfiles
        custom_target(
            'website_' + page,
            input: page + '.5.md',
            output: page + '.5.md',
            command: ['sed', 's/^# Name$/# ' + page + '/', '@INPUT@'],
            capture: true,
            install: true,
            install_dir: docs_install_path + '/manual/en',
        )
    endforeach
endif
