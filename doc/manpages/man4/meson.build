manfile = 'atalk.4'

man_xmlfile = configure_file(
    input: manfile + '.xml',
    output: manfile + '.xml',
    configuration: cdata,
)

custom_target(
    manfile,
    input: man_xmlfile,
    output: manfile,
    command: manpages_command,
    install: true,
    install_dir: mandir / 'man4',
    build_by_default: true,
)
