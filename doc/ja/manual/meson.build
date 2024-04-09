manual_xml = configure_file(
    input: 'manual.xml.in',
    output: 'manual.xml',
    configuration: cdata,
)

netatalk_html = configure_file(
    input: 'netatalk.html.in',
    output: 'netatalk.html',
    configuration: cdata,
)

manual_pages = [
    'configuration.xml',
    'install.xml',
    'intro.xml',
    'upgrade.xml',
]

foreach page : manual_pages
    manual_xmlfile = configure_file(
        input: page,
        output: page,
        configuration: cdata,
    )
endforeach

manual_gen = custom_target(
    'manual',
    output: [
        'ad.1.html',
        'afp_signature.conf.5.html',
        'afp_voluuid.conf.5.html',
        'afp.conf.5.html',
        'afpd.8.html',
        'afpldaptest.1.html',
        'apple_dump.1.html',
        'asip-status.pl.1.html',
        'cnid_dbd.8.html',
        'cnid_metad.8.html',
        'configuration.html',
        'dbd.1.html',
        'example-toc.html',
        'extmap.conf.5.html',
        'index.html',
        'installation.html',
        'intro.html',
        'macusers.1.html',
        'man-pages.html',
        'manual-index.html',
        'netatalk.8.html',
        'netatalkconfig.1.html',
        'table-toc.html',
        'upgrade.html',
    ],
    input: manual_xml,
    command: [
        xsltproc,
        '--stringparam', 'generate.consistent.ids',
        '1',
        '-o', meson.current_build_dir() / 'manual',
        manual_stylesheet,
        '@INPUT@',
    ],
    install: true,
    install_dir: datadir / 'doc/netatalk/ja',
)
