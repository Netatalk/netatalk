cnid2create_script = configure_file(
    input: 'cnid2_create.in',
    output: 'cnid2_create',
    configuration: cdata,
)

install_data(
    cnid2create_script,
    install_dir: bindir,
    install_mode: 'rwxr-xr-x',
)


