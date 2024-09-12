manfiles = [
    'atalk_aton.3',
    'nbp_name.3',
]

foreach man : manfiles
    man_xmlfile = configure_file(
        input: man + '.xml',
        output: man + '.xml',
        configuration: cdata,
    )
endforeach
