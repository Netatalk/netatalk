manual_xml = configure_file(
    input: 'manual.xml.in',
    output: 'manual.xml',
    configuration: cdata,
)

pageheader = configure_file(
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

manual_gen = custom_target(
    'manual',
    output: [
        'a2boot.8.html',
        'ad.1.html',
        'aecho.1.html',
        'afp_ldap.conf.5.html',
        'afp_signature.conf.5.html',
        'afp_voluuid.conf.5.html',
        'afpd.8.html',
        'afpd.conf.5.html',
        'afpldaptest.1.html',
        'afppasswd.1.html',
        'apple_dump.1.html',
        'AppleVolumes.default.5.html',
        'asip-status.1.html',
        'atalk.4.html',
        'atalkd.8.html',
        'atalk_aton.3.html',
        'atalkd.conf.5.html',
        'cnid_dbd.8.html',
        'cnid_metad.8.html',
        'cnid2_create.1.html',
        'compile.html',
        'configuration.html',
        'dbd.1.html',
        'example-toc.html',
        'getzones.1.html',
        'index.html',
        'installation.html',
        'intro.html',
        'macusers.1.html',
        'man-pages.html',
        'manual-index.html',
        'megatron.1.html',
        'nbp.1.html',
        'nbp_name.3.html',
        'netatalk.conf.5.html',
        'netatalk-config.1.html',
        'pap.1.html',
        'papd.8.html',
        'papd.conf.5.html',
        'papstatus.8.html',
        'psf.8.html',
        'psorder.1.html',
        'showppd.1.html',
        'table-toc.html',
        'timelord.8.html',
        'uniconv.1.html',
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
