manfiles = [
    'ad.1',
    'afpldaptest.1',
    'afppasswd.1',
    'afpstats.1',
    'afptest.1',
    'apple_dump.1',
    'asip-status.1',
    'dbd.1',
    'macusers.1',
]

if have_appletalk
    manfiles += [
        'aecho.1',
        'getzones.1',
        'nbp.1',
        'pap.1',
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
        install_dir: mandir / 'man1',
        build_by_default: true,
    )
endforeach

afptest_staticmans = [
    'afp_lantest.1',
    'afp_logintest.1',
    'afp_spectest.1',
    'afp_speedtest.1',
    'afparg.1',
]

foreach man : afptest_staticmans
    install_data('afptest.1', rename: man, install_dir: mandir / 'man1')
endforeach

if have_appletalk
    nbp_staticmans = [
        'nbplkup.1',
        'nbprgstr.1',
        'nbpunrgstr.1',
    ]

    foreach man : nbp_staticmans
        install_data('nbp.1', rename: man, install_dir: mandir / 'man1')
    endforeach
endif
