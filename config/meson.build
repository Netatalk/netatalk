afp_conf = configure_file(
    input: 'afp.conf.tmpl',
    output: 'afp.conf',
    configuration: cdata,
)

custom_target(
    'dbus_session',
    input: 'dbus-session.conf.tmpl',
    output: 'dbus-session.conf',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: pkgconfdir,
)

if (
    not fs.exists(pkgconfdir / 'afp.conf')
    or (get_option('enable-overwrite')
    and fs.exists(pkgconfdir / 'afp.conf'))
)
    install_data(afp_conf, install_dir: pkgconfdir)
else
    message('not overwriting', 'afp.conf')
endif

install_data('extmap.conf', install_dir: pkgconfdir)

install_data('netatalk-dbus.conf', install_dir: pkgconfdir / 'dbus-1/system.d')

install_data('README', install_dir: localstatedir / 'netatalk')

install_data('README', install_dir: localstatedir / 'netatalk/CNID')

if have_pam
    subdir('pam')
endif
