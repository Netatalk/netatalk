man_xml = [
    'a2boot',
    'afpd',
    'atalkd',
    'cnid_dbd',
    'cnid_metad',
    'papd',
    'papstatus',
    'psf',
    'timelord',
]

foreach man : man_xml
    man_xmlfile = configure_file(
        input: man + '.8.xml',
        output: man + '.8.xml',
        configuration: cdata,
    )

    man_gen = custom_target(
        man + '.8',
        input: man_xmlfile,
        output: man + '.8',
        command: [
            manpages_command,
            '&&',
            'cp',
            '@OUTPUT@',
            meson.project_source_root() / 'man/man8' / man + '.8.in',
        ],
        build_by_default: true,
    )
endforeach
