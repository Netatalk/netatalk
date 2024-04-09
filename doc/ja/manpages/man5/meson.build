manxml = ['afp_signature.conf', 'afp_voluuid.conf', 'afp.conf', 'extmap.conf']

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.5.xml',
        output: man + '.5.xml',
        configuration: cdata,
    )
endforeach
