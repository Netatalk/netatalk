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

    install_data(
        [
            afpstats_script,
            addump_script,
            asipstatus_script,
        ],
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
endif

if build_testsuite and perl.found()
    diag_script = configure_file(
        input: 'netatalk-diag.sh',
        output: 'netatalk-diag',
        configuration: cdata,
    )

    install_data(
        diag_script,
        install_dir: sbindir,
        install_mode: 'rwxr-xr-x',
    )

    install_data(
        'netatalk_diag.pl',
        install_dir: libexecdir / 'netatalk',
        install_mode: 'rw-r--r--',
    )
endif
