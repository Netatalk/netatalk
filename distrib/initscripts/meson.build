if init_style == 'none'
    init_dirs += 'none'
endif

if (
    init_style == 'debian'
    or init_style == 'debian-systemd'
    or init_style == 'gentoo-systemd'
    or init_style == 'redhat-systemd'
    or init_style == 'suse-systemd'
    or init_style == 'systemd'
)
    init_dir = '/usr/lib/systemd/system'
    init_dirs += init_dir
    configure_file(
        input: 'service.systemd.tmpl',
        output: 'netatalk.service',
        configuration: cdata,
        install: true,
        install_dir: init_dir,
    )
    if get_option('with-init-hooks')
        meson.add_install_script(find_program('systemctl'), 'daemon-reload')
    endif
endif

if (
  init_style == 'debian'
  or init_style == 'debian-sysv'
)
    init_dir = '/etc/init.d'
    init_dirs += init_dir
    configure_file(
        input: 'rc.debian.tmpl',
        output: 'netatalk',
        configuration: cdata,
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
endif

if init_style == 'freebsd'
    init_dir = '/etc/rc.d'
    init_dirs += init_dir
    configure_file(
        input: 'rc.freebsd.tmpl',
        output: 'netatalk',
        configuration: cdata,
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
endif

if init_style == 'netbsd'
    init_dir = '/etc/rc.d'
    init_dirs += init_dir
    configure_file(
        input: 'rc.netbsd.tmpl',
        output: 'netatalk',
        configuration: cdata,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
endif

if init_style == 'openbsd'
    init_dir = '/etc/rc.d'
    init_dirs += init_dir
    configure_file(
        input: 'rc.openbsd.tmpl',
        output: 'netatalk',
        configuration: cdata,
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
endif

if init_style == 'solaris'
    init_dir = '/lib/svc/manifest/network'
    init_dirs += init_dir
    configure_file(
        input: 'netatalk.xml.tmpl',
        output: 'netatalk.xml',
        configuration: cdata,
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
endif

if (
    init_style == 'openrc'
    or init_style == 'gentoo-openrc'
)
    init_dir = '/etc/init.d'
    init_dirs += init_dir
    configure_file(
        input: 'rc.openrc.tmpl',
        output: 'netatalk',
        configuration: cdata,
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
endif

if init_style == 'macos-launchd'
    init_dir = '/Library/LaunchDaemons'
    init_dirs += init_dir
    configure_file(
        input: 'netatalkd.tmpl',
        output: 'netatalkd',
        configuration: cdata,
        install: true,
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
    configure_file(
        input: 'io.netatalk.daemon.plist.tmpl',
        output: 'io.netatalk.daemon.plist',
        configuration: cdata,
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
