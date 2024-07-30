manfiles = [
    'afpd.8',
    'cnid_dbd.8',
    'cnid_metad.8',
    'netatalk.8',
]

foreach man : manfiles
    man_xmlfile = configure_file(
        input: man + '.xml',
        output: man + '.xml',
        configuration: cdata,
    )

    custom_target(
        man,
        input: man_xmlfile,
        output: man,
        command: manpages_command,
        install: true,
        install_dir: mandir / 'man8',
        build_by_default: true,
    )
endforeach
