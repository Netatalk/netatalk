install_data('afpstats', install_dir: bindir)

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

install_data('fce_ev_script.sh', install_dir: bindir)

install_data('make-casetable.pl', install_dir: bindir)

install_data('make-precompose.h.pl', install_dir: bindir)
