manfiles = [
    'afp_signature.conf',
    'afp_voluuid.conf',
    'afp.conf',
    'extmap.conf',
]

if have_appletalk
    manfiles += [
        'atalkd.conf',
        'papd.conf',
    ]
endif

foreach man : manfiles
    custom_target(
        man + '.5',
        input: man + '.5.md',
        output: man + '.5',
        command: [
            make_man,
            cmark_man,
            '@INPUT@',
            man.to_upper(),
            '5',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man5',
        build_by_default: true,
    )
endforeach

install_data('macipgw.conf.5', install_dir: mandir / 'man5')

if get_option('with-website')
    foreach page : manfiles
        custom_target(
            'manual_' + page,
            input: page + '.5.md',
            output: page + '.5.html',
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
