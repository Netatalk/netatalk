manfiles = [
    'afpd',
    'netatalk',
]

if enable_fce
    manfiles += [
        'fce_ev_script',
    ]
endif

if build_dbd_docs
    manfiles += [
        'cnid_dbd',
        'cnid_metad',
    ]
endif

if enable_appletalk
    manfiles += [
        'a2boot',
        'atalkd',
        'macipgw',
        'papd',
        'papstatus',
        'timelord',
    ]
endif

foreach man : manfiles
    custom_target(
        man + '.8',
        input: man + '.8.md',
        output: man + '.8',
        command: [
            make_man,
            transcoder,
            '@INPUT@',
            man.to_upper(),
            '8',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man8',
        install_tag: 'man',
        build_by_default: true,
    )
endforeach

if build_website
    foreach page : manfiles
        custom_target(
            'website_' + page,
            input: page + '.8.md',
            output: page + '.8.md',
            command: ['sed', 's/^# Name$/# ' + page + '/', '@INPUT@'],
            capture: true,
            install: true,
            install_dir: docs_install_path + '/manual/en',
        )
    endforeach
endif
