if get_option('with-init-style') == ''
    init_dir += 'none'
elif get_option('with-init-style') == 'redhat-sysv'
    init_dir += '/etc/rc.d/init.d'
    custom_target(
        'redhat_sysv',
        input: 'rc.redhat.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    meson.add_install_script(
        find_program('chkconfig'),
        'add',
        '/etc/rc.d/init.d/netatalk',
    )
elif (
    get_option('with-init-style') == 'service.systemd'
    or get_option('with-init-style') == 'redhat-systemd'
    or get_option('with-init-style') == 'suse-systemd'
)

    init_dir += '/usr/lib/systemd/system'
    custom_target(
        'service_systemd',
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    meson.add_install_script(find_program('systemctl'), 'daemon-reload')
elif get_option('with-init-style') == 'debian-systemd'
    init_dir += '/lib/systemd/system'
    custom_target(
        'debian_systemd',
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    meson.add_install_script(find_program('systemctl'), 'daemon-reload')
elif get_option('with-init-style') == 'suse-sysv'
    init_dir += '/etc/init.d'
    custom_target(
        'suse_sysv',
        input: 'rc.suse.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    meson.add_install_script(find_program('insserv'), '/etc/init.d/netatalk')
elif get_option('with-init-style') == 'netbsd'
    init_dir += '/etc/rc.d'
    custom_target(
        'netbsd',
        input: 'rc.netbsd.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
elif get_option('with-init-style') == 'sunos'
    init_dir += '/lib/svc/manifest/network'
    custom_target(
        'sunos',
        input: 'netatalk.xml.tmpl',
        output: 'netatalk.xml',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    meson.add_install_script(
        find_program('svccfg'),
        'import',
        '/lib/svc/manifest/network/netatalk.xml',
    )
elif (
    get_option('with-init-style') == 'openrc'
    or get_option('with-init-style') == 'gentoo-openrc'
)
    init_dir += '/etc/init.d'
    custom_target(
        'openrc',
        input: 'rc.gentoo.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
elif get_option('with-init-style') == 'debian-sysv'
    init_dir += '/etc/init.d'
    custom_target(
        'debian_sysv',
        input: 'rc.debian.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
elif get_option('with-init-style') == 'macos-launchd'
    init_dir += '/Library/LaunchDaemons'
    custom_target(
        'netatalkd',
        input: 'netatalkd.tmpl',
        output: 'netatalkd',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
    custom_target(
        'plist',
        input: 'com.netatalk.daemon.plist.tmpl',
        output: 'com.netatalk.daemon.plist',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    if not fs.exists(init_dir / 'com.netatalk.daemon.plist')
        meson.add_install_script(
            find_program('launchctl'),
            'load',
            '-w',
            init_dir / 'com.netatalk.daemon.plist',
        )
    endif
endif

