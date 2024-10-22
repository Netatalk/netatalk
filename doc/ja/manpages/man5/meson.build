manfiles = [
    'afp_signature.conf.5',
    'afp_voluuid.conf.5',
    'afp.conf.5',
    'atalkd.conf.5',
    'extmap.conf.5',
    'papd.conf.5',
]

foreach man : manfiles
    configure_file(
        input: man + '.xml',
        output: man + '.xml',
        configuration: cdata,
    )
endforeach
