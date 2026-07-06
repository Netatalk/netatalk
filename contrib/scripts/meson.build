fce_ev_script = configure_file(
    input: 'fce_ev_script.sh',
    output: 'fce_ev_script',
    copy: true,
)

install_data(
    fce_ev_script,
    install_dir: libexecdir / 'netatalk',
    install_mode: 'rwxr-xr-x',
)
