netatalk_config = configure_file(
    input: 'netatalk-config.in',
    output: 'netatalk-config',
    configuration: cdata,
)

install_data(
    netatalk_config,
    install_dir: bindir,
    install_mode: 'rwxr-xr-x',
)
