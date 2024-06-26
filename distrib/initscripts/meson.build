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
    custom_target(
        'systemd',
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    if get_option('with-init-hooks')
        meson.add_install_script(find_program('systemctl'), 'daemon-reload')
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
elif get_option('with-init-style') == 'freebsd'
    init_dir += '/etc/rc.d'
    custom_target(
        'freebsd',
        input: 'rc.freebsd.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('sh'),
            '/etc/rc.d/netatalk',
            'enable',
        )
    endif
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
elif get_option('with-init-style') == 'openbsd'
    init_dir += '/etc/rc.d'
    custom_target(
        'freebsd',
        input: 'rc.openbsd.tmpl',
        output: 'netatalk',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('rcctl'),
            '-d', 'enable',
            'netatalk',
        )
    endif
elif get_option('with-init-style') == 'solaris'
    init_dir += '/lib/svc/manifest/network'
    custom_target(
        'solaris',
        input: 'netatalk.xml.tmpl',
        output: 'netatalk.xml',
        command: sed_command,
        capture: true,
        install: true,
        install_dir: init_dir,
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('svccfg'),
            'import',
            '/lib/svc/manifest/network/netatalk.xml',
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
        not fs.exists(init_dir / 'io.netatalk.daemon.plist')
        and get_option('with-init-hooks')
    )
        if fs.exists(init_dir / 'com.netatalk.daemon.plist')
            meson.add_install_script(
                find_program('launchctl'),
                'disable',
                'system/com.netatalk.daemon',
            )
        endif
        meson.add_install_script(
            find_program('launchctl'),
            'load',
            '-w', init_dir / 'io.netatalk.daemon.plist',
        )
    endif
endif
