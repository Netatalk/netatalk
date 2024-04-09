manxml = [
    'ad',
    'afpldaptest',
    'afppasswd',
    'afpstats',
    'apple_dump',
    'asip-status',
    'dbd',
    'macusers',
    'netatalk-config',
]

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.1.xml',
        output: man + '.1.xml',
        configuration: cdata,
    )
endforeach
