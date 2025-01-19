manfiles = [
    'atalk_aton',
    'nbp_name',
]

foreach man : manfiles
    custom_target(
        man + '.3',
        input: man + '.3.md',
        output: man + '.3',
        command: [
            make_man,
            cmark_man,
            '@INPUT@',
            man.to_upper(),
            '3',
            netatalk_version,
        ],
        capture: true,
        install: true,
        install_dir: mandir / 'man3',
        build_by_default: true,
    )
endforeach

if get_option('with-website')
    foreach page : manfiles
        custom_target(
            'manual_' + page,
            input: page + '.3.md',
            output: page + '.3.html',
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
