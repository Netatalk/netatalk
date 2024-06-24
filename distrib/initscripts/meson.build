init_style = get_option('with-init-style')
init_system = get_option('with-init-system')

################
# init targets #
################

systemd = custom_target(
    'systemd',
    input: 'service.systemd.tmpl',
    output: 'netatalk.service',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: init_dir,
 )

debian_sysv = custom_target(
    'debian_sysv',
    input: 'rc.debian.tmpl',
    output: 'netatalk',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: init_dir,
    install_mode: 'rwxr-xr-x',
)

redhat_sysv = custom_target(
    'redhat_sysv',
    input: 'rc.redhat.tmpl',
    output: 'netatalk',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: init_dir,
    install_mode: 'rwxr-xr-x',
)

suse_sysv = custom_target(
    'suse_sysv',
    input: 'rc.suse.tmpl',
    output: 'netatalk',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: init_dir,
    install_mode: 'rwxr-xr-x',
)

freebsd = custom_target(
    'freebsd',
    input: 'rc.freebsd.tmpl',
    output: 'netatalk',
    command: sed_command,
    capture: true,
    install: true,
    install_dir: init_dir,
    install_mode: 'rwxr-xr-x',
)

##############
# init hooks #
##############


systemd_hook = meson.add_install_script(find_program('systemctl'), 'daemon-reload')

debian_sysv_hook = meson.add_install_script(
    find_program('update-rc.d'),
    'netatalk',
    'defaults',
    '90',
    '10',
)

suse_sysv_hook = meson.add_install_script(
    find_program('insserv'),
    '/etc/init.d/netatalk',
)

redhat_sysv_hook = meson.add_install_script(
    find_program('chkconfig'),
    '--add', '/etc/rc.d/init.d/netatalk',
)

##############
# init macro #
##############

if get_option('with-init-style') == 'none'
    init_dir += 'none'
elif (
    init_style == 'debian-systemd'
    or init_style == 'gentoo-systemd'
    or init_style == 'redhat-systemd'
    or init_style == 'suse-systemd'
    or init_style == 'systemd'
)
    init_dir += '/usr/lib/systemd/system'
    systemd
    if get_option('with-init-hooks')
        systemd_hook
    endif
elif init_style == 'debian-sysv'
    init_dir += '/etc/init.d'
    debian_sysv
    if get_option('with-init-hooks')
        debian_sysv_hook
    endif
elif init_style == 'redhat-sysv'
    init_dir += '/etc/rc.d/init.d'
    redhat_sysv
    if get_option('with-init-hooks')
        redhat_sysv_hook
    endif
elif init_style == 'suse-sysv'
    init_dir += '/etc/init.d'
    suse_sysv
    if get_option('with-init-hooks')
        suse_sysv_hook
    endif
elif init_style == 'freebsd'
    init_dir += '/etc/rc.d'
    freebsd
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('sh'),
            '/etc/rc.d/netatalk',
            'enable',
        )
    endif
elif init_style == 'netbsd'
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
elif init_style == 'openbsd'
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
elif init_style == 'solaris'
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
    init_style == 'openrc'
    or init_style == 'gentoo-openrc'
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
elif init_style == 'macos-launchd'
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
elif init_style == 'auto'
    if uname_command.contains('debian')
        systemd
        debian-sysv
        if init_system == 'systemd'
            init_dir += '/usr/lib/systemd/system'
            systemd_hook
        elif init_system == 'sysv'
            init_dir += '/etc/init.d'
            debian_sysv_hook
        endif
    elif uname_command.contains('suse')
        systemd
        suse_sysv
        if init_system == 'systemd'
            init_dir += '/usr/lib/systemd/system'
            systemd_hook
        elif init_system == 'sysv'
            init_dir += '/etc/init.d'
            suse_sysv_hook
        endif
    elif uname_command.contains('redhat')
        systemd
        redhat_sysv
        if init_system == 'systemd'
            init_dir += '/usr/lib/systemd/system'
            systemd_hook
        elif init_system == 'sysv'
            init_dir += '/etc/init.d'
            redhat_sysv_hook
        endif
    endif
endif

