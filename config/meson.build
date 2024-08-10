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
    fs.exists(pkgconfdir / 'afp.conf')
    and not get_option('with-overwrite')
)
    message('will not replace existing', pkgconfdir / 'afp.conf')
else
    install_data(afp_conf, install_dir: pkgconfdir)
endif

if (
    fs.exists(pkgconfdir / 'extmap.conf')
    and not get_option('with-overwrite')
)
    message('will not replace existing', pkgconfdir / 'extmap.conf')
else
    install_data('extmap.conf', install_dir: pkgconfdir)
endif

if (
    fs.exists(dbus_sysconfpath / 'netatalk-dbus.conf')
    and not get_option('with-overwrite')
)
    message('will not replace existing', dbus_sysconfpath / 'netatalk-dbus.conf')
else
    install_data('netatalk-dbus.conf', install_dir: dbus_sysconfpath)
endif

install_data('README', install_dir: localstatedir / 'netatalk')

if host_os == 'linux' and fs.exists('/etc/ld.so.conf.d')
    configure_file(
        input: 'libatalk.conf.in',
        output: 'libatalk.conf',
        configuration: cdata,
        install: true,
        install_dir: '/etc/ld.so.conf.d',
    )
endif

install_data('README', install_dir: localstatedir / 'netatalk/CNID')

if have_pam
    subdir('pam')
endif
