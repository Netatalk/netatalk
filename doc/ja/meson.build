if get_option('with-manual') == 'www'
    input_file = 'html-www.xsl.in'
else
    input_file = 'html.xsl.in'
endif

manual_stylesheet_ja = configure_file(
    input: input_file,
    output: 'html.xsl',
    configuration: cdata,
)

subdir('manpages')
subdir('manual')
