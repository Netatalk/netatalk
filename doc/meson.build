if get_option('with-manual') == 'www'
    input_file = 'html-www.xsl.in'
else
    input_file = 'html.xsl.in'
endif

manpages_stylesheet = configure_file(
    input: 'man.xsl.in',
    output: 'man.xsl',
    configuration: cdata,
)

manual_stylesheet = configure_file(
    input: input_file,
    output: 'html.xsl',
    configuration: cdata,
)

if get_option('with-readmes')
    install_data(
        'DEVELOPER',
        install_dir: datadir / 'doc/netatalk',
    )
endif

subdir('manpages')
subdir('manual')

if get_option('with-manual') == 'www'
    subdir('ja')
endif
