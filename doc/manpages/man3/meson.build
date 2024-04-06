man_xml = [
    'atalk_aton',
    'nbp_name',
]

foreach man : man_xml
    man_xmlfile = configure_file(
        input: man + '.3.xml',
        output: man + '.3.xml',
        configuration: cdata,
    )

    man_gen = custom_target(
        man + '.3',
        input: man_xmlfile,
        output: man + '.3',
        command: [
            manpages_command,
            '&&',
            'cp',
            '@OUTPUT@',
            meson.project_source_root() / 'man/man3' / man + '.3.in',
        ],
        build_by_default: true,
    )
endforeach
