atalk_xml = configure_file(
    input: 'atalk.4.xml',
    output: 'atalk.4.xml',
    configuration: cdata,
)

man_gen = custom_target(
    'atalk',
    input: atalk_xml,
    output: 'atalk.4',
    command: [
        manpages_command,
        '&&',
        'cp',
        '@OUTPUT@',
        meson.project_source_root() / 'man/man4' / 'atalk.4.in',
    ],
    build_by_default: true,
)
