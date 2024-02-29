manxml = [
    'ad',
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'apple_dump',
    'asip-status',
    'dbd',
    'macusers',
    'netatalk-config',
]

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.1.xml',
        output: man + '.1.xml',
        configuration: cdata,
    )

    man_gen = custom_target(
        man + '1',
        input: man_xmlfile,
        output: man + '.1',
        command: manpages_command,
        install: true,
        install_dir: meson.project_source_root() / 'man/man1',
    )
endforeach
