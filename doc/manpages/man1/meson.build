manfiles = [
    'ad.1',
    'aecho.1',
    'afpldaptest.1',
    'afppasswd.1',
    'afpstats.1',
    'apple_dump.1',
    'asip-status.1',
    'dbd.1',
    'getzones.1',
    'macusers.1',
    'nbp.1',
    'pap.1',
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
        install_dir: mandir / 'man1',
        build_by_default: true,
    )
endforeach

nbp_staticmans = [
    'nbplkup.1',
    'nbprgstr.1',
    'nbpunrgstr.1',
]

foreach man : nbp_staticmans
    install_data('nbp.1', rename: man, install_dir: mandir / 'man1')
endforeach
