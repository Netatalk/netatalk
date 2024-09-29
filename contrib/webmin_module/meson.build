webmin_files = [
    'config.info',
    '../../COPYING',
    'delete_atalk.cgi',
    'delete_sections.cgi',
    'edit_atalk.cgi',
    'edit_global_section.cgi',
    'edit_print.cgi',
    'edit_vol_section.cgi',
    'index.cgi',
    'netatalk-lib.pl',
    'README.md',
    'restart.cgi',
    'save_atalk.cgi',
    'save_global_section.cgi',
    'save_print.cgi',
    'save_vol_section.cgi',
    'server_status.cgi',
    'show_atalk.cgi',
    'show_users.cgi',
    'start.cgi',
    'stop.cgi',
]

webmin_build_dir = meson.current_build_dir() / 'netatalk'
webmin_tarball = webmin_build_dir + '-' + netatalk_version + '.wbm.gz'

install_data(webmin_files, install_dir: webmin_build_dir, install_tag: 'webmin')
install_subdir('help', install_dir: webmin_build_dir, install_tag: 'webmin')
install_subdir('images', install_dir: webmin_build_dir, install_tag: 'webmin')
install_subdir('lang', install_dir: webmin_build_dir, install_tag: 'webmin')

configure_file(
    input: 'config.in',
    output: 'config',
    configuration: cdata,
    install_dir: webmin_build_dir,
    install_tag: 'webmin',
)

configure_file(
    input: 'module.info.in',
    output: 'module.info',
    configuration: cdata,
    install_dir: webmin_build_dir,
    install_tag: 'webmin',
)

meson.add_install_script(
    tar,
    '-czf',
    webmin_tarball,
    '-C',
    meson.current_build_dir(),
    'netatalk',
    install_tag: 'webmin',
)

if install_webmin and get_option('with-install-hooks')
    meson.add_install_script(
        webmin,
        webmin_tarball,
        install_tag: 'webmin',
    )
endif
