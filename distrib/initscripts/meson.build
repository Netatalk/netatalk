if get_option('with-init-style') == 'none'
    init_dir += 'none'
elif (
    get_option('with-init-style') == 'debian-systemd'
    or get_option('with-init-style') == 'gentoo-systemd'
    or get_option('with-init-style') == 'redhat-systemd'
    or get_option('with-init-style') == 'suse-systemd'
    or get_option('with-init-style') == 'systemd'
)
    init_dir += '/usr/lib/systemd/system'
    service_data = [
        'a2boot',
        'afpd',
        'atalkd',
        'cnid',
        'papd',
        'timelord',
    ]
    foreach service : service_data
        custom_target(
            service + '.service',
            input: service + '.service.tmpl',
            output: service + '.service',
            command: sed_command,
            capture: true,
            install: true,
            install_dir: init_dir,
        )
    endforeach
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('systemctl'),
            'daemon-reload',
        )
    endif
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
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('update-rc.d'),
            'netatalk',
            'defaults',
            '90',
            '10',
        )
    endif
elif get_option('with-init-style') == 'redhat-sysv'
    init_dir += '/etc/rc.d/init.d'
    custom_target(
        'redhat',
        input: 'rc.redhat.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('chkconfig'),
            '--add', '/etc/rc.d/init.d/netatalk',
        )
    endif
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
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('insserv'),
            '/etc/init.d/netatalk',
        )
    endif
elif get_option('with-init-style') == 'netbsd'
    init_dir += '/etc/rc.d'
    rc_scripts = [
        'rc.afpd',
        'rc.atalkd',
        'rc.cnid_metad',
        'rc.papd',
        'rc.timelord',
        'rc.a2boot',
    ]
    foreach script : rc_scripts
        custom_target(
            script + '.netbsd',
            input: script + '.netbsd.tmpl',
            output: script.substring(3),
            command: sed_command,
            capture: true,
            install: true,
            install_dir: init_dir,
            install_mode: 'rwxr-xr-x',
        )
    endforeach
elif get_option('with-init-style') == 'solaris'
    init_dir += '/etc/init.d'
    custom_target(
        'solaris',
        input: 'rc.solaris.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('rm'),
            '-f', '/etc/rc2.d/S90netatalk',
        )
        meson.add_install_script(
            find_program('rm'),
            '-f', '/etc/rc0.d/K04netatalk',
        )
        meson.add_install_script(
            find_program('ln'),
            '-s', '/etc/init.d/netatalk',
            '/etc/rc2.d/S90netatalk',
        )
        meson.add_install_script(
            find_program('ln'),
            '-s', '/etc/init.d/netatalk',
            '/etc/rc0.d/K04netatalk',
        )
    endif
elif (
    get_option('with-init-style') == 'openrc'
    or get_option('with-init-style') == 'gentoo-openrc'
)
    init_dir += '/etc/init.d'
    custom_target(
        'openrc',
        input: 'rc.openrc.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('rc-update'),
            'add',
            'netatalk',
            'default',
        )
    endif
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
        input: 'io.netatalk.daemon.plist.tmpl',
        output: 'io.netatalk.daemon.plist',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    if (
        not fs.exists('/Library/LaunchDaemons/io.netatalk.daemon.plist')
        and get_option('with-init-hooks')
    )
        if fs.exists(init_dir / 'com.netatalk.daemon.plist')
            meson.add_install_script(
                find_program('launchctl'),
                'load',
                '-w', '/Library/LaunchDaemons/io.netatalk.daemon.plist',
            )
        endif
        meson.add_install_script(
            find_program('launchctl'),
            'load',
            '-w', init_dir / 'io.netatalk.daemon.plist',
        )
    endif
endif
