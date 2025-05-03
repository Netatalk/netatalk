if perl.found() and grep.found()
    afpstats_script = configure_file(
        input: 'afpstats.in',
        output: 'afpstats',
        configuration: cdata,
    )

    addump_script = configure_file(
        input: 'addump.in',
        output: 'addump',
        configuration: cdata,
    )

    asipstatus_script = configure_file(
        input: 'asip-status.in',
        output: 'asip-status',
        configuration: cdata,
    )

    macusers_script = configure_file(
        input: 'macusers.in',
        output: 'macusers',
        configuration: cdata,
    )

    install_data(
        [
            afpstats_script,
            addump_script,
            asipstatus_script,
            macusers_script,
        ],
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
endif
