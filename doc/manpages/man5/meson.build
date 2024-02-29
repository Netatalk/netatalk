manxml = ['afp_signature.conf', 'afp_voluuid.conf', 'afp.conf', 'extmap.conf']

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.5.xml',
        output: man + '.5.xml',
        configuration: cdata,
    )

    man_gen = custom_target(
        man + '5',
        input: man_xmlfile,
        output: man + '.5',
        command: manpages_command,
        install: true,
        install_dir: meson.project_source_root() / 'man/man5',
    )
endforeach
