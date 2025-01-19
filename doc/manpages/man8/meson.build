manfiles = [
    'afpd',
    'cnid_dbd',
    'cnid_metad',
    'netatalk',
]

if have_appletalk
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
            cmark_man,
            '@INPUT@',
            man.to_upper(),
            '8',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man8',
        build_by_default: true,
    )
endforeach

if get_option('with-website')
    foreach page : manfiles
        custom_target(
            'manual_' + page,
            input: page + '.8.md',
            output: page + '.8.html',
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
