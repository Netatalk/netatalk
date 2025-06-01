if perl.found() and grep.found()
    afpstats_script = configure_file(
        input: 'afpstats.pl',
        output: 'afpstats',
        configuration: cdata,
    )

    addump_script = configure_file(
        input: 'addump.pl',
        output: 'addump',
        configuration: cdata,
    )

    asipstatus_script = configure_file(
        input: 'asip-status.pl',
        output: 'asip-status',
        configuration: cdata,
    )

    macusers_script = configure_file(
        input: 'macusers.pl',
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
