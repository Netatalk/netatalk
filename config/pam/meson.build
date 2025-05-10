netatalk_pamd = configure_file(
    input: 'netatalk.in',
    output: 'netatalk',
    configuration: cdata,
)

if (fs.exists(pam_confdir / 'netatalk') and not get_option('with-overwrite'))
    message('will not replace existing', pam_confdir / 'netatalk')
else
    install_data(netatalk_pamd, install_dir: pam_confdir)
endif
