manxml = [
    'ad',
    'afpldaptest',
    'afppasswd',
    'dbd',
    'netatalk-config',
]

if perl.found() and grep.found()
    manxml += [
        'afpstats',
        'apple_dump',
        'asip-status',
        'macusers',
    ]
endif

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.1.xml',
        output: man + '.1.xml',
        configuration: cdata,
    )

    man_gen = custom_target(
        man + '.1',
        input: man_xmlfile,
        output: man + '.1',
        command: [
            manpages_command,
            '&&',
            'cp',
            '@OUTPUT@',
            meson.project_source_root() / 'man/man1' / man + '.1.in',
        ],
        build_by_default: true,
    )
endforeach
