afp_conf = configure_file(
    input: 'afp.conf.in',
    output: 'afp.conf',
    configuration: cdata,
)

if (
    fs.exists(pkgconfdir / 'afp.conf')
    and not get_option('with-overwrite')
)
    message('will not replace existing', pkgconfdir / 'afp.conf')
else
    install_data(afp_conf, install_dir: pkgconfdir)
endif

if have_spotlight
    dbus_session_conf = configure_file(
        input: 'dbus-session.conf.in',
        output: 'dbus-session.conf',
        configuration: cdata,
    )

    if (
        fs.exists(pkgconfdir / 'dbus-session.conf')
        and not get_option('with-overwrite')
    )
        message('will not replace existing', pkgconfdir / 'dbus-session.conf')
    else
        install_data(dbus_session_conf, install_dir: pkgconfdir)
    endif
endif

if have_afpstats
    if (
        fs.exists(dbus_sysconfpath / 'netatalk-dbus.conf')
        and not get_option('with-overwrite')
    )
        message('will not replace existing', dbus_sysconfpath / 'netatalk-dbus.conf')
    else
        install_data('netatalk-dbus.conf', install_dir: dbus_sysconfpath)
    endif
endif

static_conf_files = ['extmap.conf']

if have_appletalk
    static_conf_files += ['atalkd.conf', 'macipgw.conf', 'papd.conf']
endif

foreach file : static_conf_files
    if (
        fs.exists(pkgconfdir / file)
        and not get_option('with-overwrite')
    )
        message('will not replace existing', pkgconfdir / file)
    else
        install_data(file, install_dir: pkgconfdir)
    endif
endforeach

if (
    fs.exists('/etc/ld.so.conf.d')
    and get_option('with-ldsoconf')
)
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
