manfile = 'atalk'

custom_target(
    manfile + '.4',
    input: manfile + '.4.md',
    output: manfile + '.4',
    command: [
        make_man,
        transcoder,
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
    install_data(manfile + '.4.md', install_dir: manual_install_path + '/en')
endif
