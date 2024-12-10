manfiles = [
    'ad.1',
    'aecho.1',
    'afpldaptest.1',
    'afppasswd.1',
    'afpstats.1',
    'afptest.1',
    'addump.1',
    'asip-status.1',
    'dbd.1',
    'getzones.1',
    'macusers.1',
    'nbp.1',
    'pap.1',
]

foreach man : manfiles
    configure_file(
        input: man + '.xml',
        output: man + '.xml',
        configuration: cdata,
    )
endforeach
