if perl.found() and grep.found()
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
            appledump_script,
            asipstatus_script,
        ],
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
endif

custom_target(
    'lp2pap_script',
    input: 'lp2pap.tmpl',
    output: 'lp2pap',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: bindir,
    install_mode: 'rwxr-xr-x',
)
