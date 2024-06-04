if perl.found() and grep.found()
    afpstats_script = configure_file(
        input: 'afpstats.in',
        output: 'afpstats',
        configuration: cdata,
    )

    install_data(afpstats_script, install_dir: bindir)

    appledump_script = configure_file(
        input: 'apple_dump.in',
        output: 'apple_dump',
        configuration: cdata,
    )

    install_data(appledump_script, install_dir: bindir)

    asipstatus_script = configure_file(
        input: 'asip-status.in',
        output: 'asip-status',
        configuration: cdata,
    )

    install_data(asipstatus_script, install_dir: bindir)
endif
