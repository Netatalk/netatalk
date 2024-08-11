manpages_stylesheet = configure_file(
    input: 'man.xsl.in',
    output: 'man.xsl',
    configuration: cdata,
)

manual_stylesheet = configure_file(
    input: 'html.xsl.in',
    output: 'html.xsl',
    configuration: cdata,
)

subdir('manpages')
subdir('manual')

if 'ja' in get_option('with-manual-l10n')
    subdir('ja')
endif
