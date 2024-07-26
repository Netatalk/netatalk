manual_xml = configure_file(
    input: 'manual.xml.in',
    output: 'manual.xml',
    configuration: cdata,
)

configure_file(
    input: 'pageheader.txt.in',
    output: 'pageheader.txt',
    configuration: cdata,
)

manual_pages = [
    'compile.xml',
    'configuration.xml',
    'gpl.xml',
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

custom_target(
    'manual',
    output: [
        'ad.1.html',
        'afp_signature.conf.5.html',
        'afp_voluuid.conf.5.html',
        'afp.conf.5.html',
        'afpd.8.html',
        'afpldaptest.1.html',
        'afppasswd.1.html',
        'apple_dump.1.html',
        'asip-status.1.html',
        'cnid_dbd.8.html',
        'cnid_metad.8.html',
        'compile.html',
        'configuration.html',
        'dbd.1.html',
        'example-toc.html',
        'extmap.conf.5.html',
        'gpl.html',
        'index.html',
        'installation.html',
        'intro.html',
        'macusers.1.html',
        'man-pages.html',
        'manual-index.html',
        'netatalk.8.html',
        'netatalk-config.1.html',
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
    install_dir: datadir / 'doc/netatalk/htmldocs',
)
