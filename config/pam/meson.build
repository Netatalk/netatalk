netatalk_pamd = configure_file(
    input: 'netatalk.tmpl',
    output: 'netatalk',
    configuration: cdata,
)

install_data(netatalk_pamd, install_dir: pampath)
