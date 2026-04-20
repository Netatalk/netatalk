if fs.exists('/etc/dconf/profile/user') and not get_option('with-overwrite')
    message('will not replace existing /etc/dconf/profile/user')
else
    install_data(
        'netatalk',
        install_dir: '/etc/dconf/profile',
        rename: ['user'],
        install_tag: 'config',
    )
endif
