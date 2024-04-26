afpstats_script = configure_file(
    input: 'afpstats.in',
    output: 'afpstats',
    configuration: cdata,
)

install_data(afpstats_script, install_dir: bindir)

if perl.found() and grep.found()
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

install_data('fce_ev_script.sh', install_dir: bindir)

install_data('make-casetable.pl', install_dir: bindir)

install_data('make-precompose.h.pl', install_dir: bindir)
