if get_option('with-init-style') == 'redhat-sysv'
    init_script = configure_file(
        input: 'rc.redhat.tmpl',
        output: 'netatalk',
        configuration: cdata,
    )
    install_data(
        init_script,
        install_dir: '/etc/rc.d/init.d',
        install_mode: 'rwxr-xr-x',
    )
    run_command('chkconfig', '--add', '/etc/rc.d/init.d/netatalk', check: true)
endif
if get_option('with-init-style') == 'service.systemd'
    init_script = configure_file(
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        configuration: cdata,
    )
    install_data(init_script, install_dir: '/usr/lib/systemd/system')
    run_command('systemctl', 'daemon-reload', check: true)
endif
if get_option('with-init-style') == 'debian-systemd'
    init_script = configure_file(
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        configuration: cdata,
    )
    install_data(init_script, install_dir: '/lib/systemd/system')
    run_command('systemctl', 'daemon-reload', check: true)
endif
if get_option('with-init-style') == 'suse-sysv'
    init_script = configure_file(
        input: 'rc.suse.tmpl',
        output: 'netatalk',
        configuration: cdata,
    )
    install_data(
        init_script,
        install_dir: '/etc/init.d',
        install_mode: 'rwxr-xr-x',
    )
    run_command('insserv', '/etc/init.d/netatalk', check: true)
endif
if get_option('with-init-style') == 'netbsd'
    init_script = configure_file(
        input: 'rc.netbsd.tmpl',
        output: 'netatalk',
        configuration: cdata,
    )
    install_data(
        init_script,
        install_dir: '/etc/rc.d',
        install_mode: 'rwxr-xr-x',
    )
endif
if get_option('with-init-style') == 'solaris'
    init_script = configure_file(
        input: 'netatalk.xml.tmpl',
        output: 'netatalk.xml',
        configuration: cdata,
    )
    install_data(init_script, install_dir: '/lib/svc/manifest/network')
    run_command(
        'svccfg',
        'import',
        '/lib/svc/manifest/network/netatalk.xml',
        check: true,
    )
endif
if get_option('with-init-style') == 'openrc'
    init_script = configure_file(
        input: 'rc.gentoo.tmpl',
        output: 'netatalk',
        configuration: cdata,
    )
    install_data(
        init_script,
        install_dir: '/etc/init.d',
        install_mode: 'rwxr-xr-x',
    )
endif
if get_option('with-init-style') == 'debian-sysv'
    init_script = configure_file(
        input: 'rc.debian.tmpl',
        output: 'netatalk',
        configuration: cdata,
    )
    install_data(
        init_script,
        install_dir: '/etc/init.d',
        install_mode: 'rwxr-xr-x',
    )
endif
if get_option('with-init-style') == 'macos-launchd'
    daemon = configure_file(
        input: 'netatalkd.tmpl',
        output: 'netatalkd',
        configuration: cdata,
    )
    install_data(daemon, install_dir: bindir, install_mode: 'rwxr-xr-x')
    plist = configure_file(
        input: 'com.netatalk.daemon.plist.in',
        output: 'com.netatalk.daemon.plist',
        configuration: cdata,
    )
    install_data(plist, install_dir: '/Library/LaunchDaemons')
    run_command(
        'launchctl',
        'load',
        '-w',
        '/Library/LaunchDaemons/com.netatalk.daemon.plist',
        check: false,
    )
endif

