manfiles = [
    'ad.1',
    'afpldaptest.1',
    'afppasswd.1',
    'afpstats.1',
    'apple_dump.1',
    'asip-status.1',
    'dbd.1',
    'macusers.1',
    'netatalk-config.1',
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
        command: [
            manpages_command,
            '&&',
            'cp',
            '@OUTPUT@',
            man,
        ],
        install: true,
        install_dir: mandir / 'man1',
        build_by_default: true,
    )
endforeach
