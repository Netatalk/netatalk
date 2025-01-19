manfile = 'atalk'

custom_target(
    manfile + '.4',
    input: manfile + '.4.md',
    output: manfile + '.4',
    command: [
        make_man,
        cmark_man,
        '@INPUT@',
        manfile.to_upper(),
        '4',
        netatalk_version,
    ],
    capture: true,
    install: true,
    install_dir: mandir / 'man4',
    build_by_default: true,
)

if get_option('with-website')
    custom_target(
        'manual_' + page,
        input: manfile + '.4.md',
        output: manfile + '.4.html',
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
endif
