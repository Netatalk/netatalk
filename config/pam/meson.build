netatalk_pamd = configure_file(
    input: 'netatalk.pam.tmpl',
    output: 'netatalk',
    configuration: cdata,
)

if (
    not fs.exists(pam_confdir / 'netatalk')
    or (get_option('with-overwrite') and fs.exists(pampath / 'netatalk'))
)
    install_data(netatalk_pamd, install_dir: pam_confdir)
else
    message('not overwriting', file)
endif
