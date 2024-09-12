manfiles = [
    'a2boot.8',
    'afpd.8',
    'atalkd.8',
    'cnid_dbd.8',
    'cnid_metad.8',
    'netatalk.8',
    'papd.8',
    'papstatus.8',
    'timelord.8',
]

foreach man : manfiles
    man_xmlfile = configure_file(
        input: man + '.xml',
        output: man + '.xml',
        configuration: cdata,
    )
endforeach
