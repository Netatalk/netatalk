manxml = ['afpd', 'cnid_dbd', 'cnid_metad', 'netatalk']

foreach man : manxml
    man_xmlfile = configure_file(
        input: man + '.8.xml',
        output: man + '.8.xml',
        configuration: cdata,
    )
endforeach
