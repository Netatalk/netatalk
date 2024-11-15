manfiles = [
    'afp_signature.conf.5',
    'afp_voluuid.conf.5',
    'afp.conf.5',
    'extmap.conf.5',
]

if have_appletalk
    manfiles += [
        'atalkd.conf.5',
        'papd.conf.5',
    ]
endif

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
        install_dir: mandir / 'man5',
        build_by_default: true,
    )
endforeach
