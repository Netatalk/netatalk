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
        'make-casetable.pl',
        'make-precompose.h.pl',
    ],
    install_dir: bindir,
)

custom_target(
    'lp2pap_sh',
    input: 'lp2pap.sh.tmpl',
    output: 'lp2pap.sh',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: bindir,
)
