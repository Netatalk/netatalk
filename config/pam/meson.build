netatalk_pamd = configure_file(
    input: 'netatalk.pam.tmpl',
    output: 'netatalk',
    configuration: cdata,
)

if (
    not fs.exists(pampath / 'netatalk')
    or (get_option('enable-overwrite')
    and fs.exists(pampath / 'netatalk'))
)
    install_data(netatalk_pamd, install_dir: pampath)
else
    message('not overwriting', file)
endif

