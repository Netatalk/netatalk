if perl.found() and grep.found()
    afpstats_script = configure_file(
        input: 'afpstats.in',
        output: 'afpstats',
        configuration: cdata,
    )

    appledump_script = configure_file(
        input: 'apple_dump.in',
        output: 'apple_dump',
        configuration: cdata,
    )

    asipstatus_script = configure_file(
        input: 'asip-status.in',
        output: 'asip-status',
        configuration: cdata,
    )

    install_data(
        [
            afpstats_script,
            appledump_script,
            asipstatus_script,
        ],
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
endif
