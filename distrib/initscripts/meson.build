if 'none' in init_style
    init_dirs += 'none'
endif

init_dir_override = get_option('with-init-dir')

if init_dir_override != ''
    init_dir = init_dir_override
else
    init_dir = ''
endif

if 'debian-sysv' in init_style
    if init_dir_override == ''
        init_dir = '/etc/init.d'
    endif
    init_dirs += init_dir
    service_data = ['netatalk']
    if have_appletalk
        service_data += [
            'a2boot',
            'atalkd',
            'macipgw',
            'papd',
            'timelord',
        ]
    endif
    foreach service : service_data
        configure_file(
            input: 'debian.' + service + '.in',
            output: service,
            configuration: cdata,
            install: true,
            install_dir: init_dir,
            install_mode: 'rwxr-xr-x',
        )
    endforeach
    if get_option('with-init-hooks')
        foreach service : service_data
            meson.add_install_script(
                find_program('update-rc.d'),
                service,
                'defaults',
                '90',
                '10',
            )
        endforeach
    endif
endif

if 'freebsd' in init_style
    if init_dir_override == ''
        init_dir = '/usr/local/etc/rc.d'
    endif
    init_dirs += init_dir
    configure_file(
        input: 'freebsd.netatalk.in',
        output: 'netatalk',
        configuration: cdata,
        install: true,
        install_dir: init_dir,
        install_mode: 'rwxr-xr-x',
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('sh'),
            '/usr/local/etc/rc.d/netatalk',
            'enable',
        )
    endif
endif

if 'macos-launchd' in init_style
    if init_dir_override == ''
        init_dir = '/Library/LaunchDaemons'
    endif
    init_dirs += init_dir
    configure_file(
        input: 'macos.netatalk.in',
        output: 'netatalkd',
        configuration: cdata,
        install: true,
        install_dir: bindir,
        install_mode: 'rwxr-xr-x',
    )
    configure_file(
        input: 'macos.netatalk.plist.in',
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
                'bootout',
                'system/',
                init_dir / 'com.netatalk.daemon.plist',
            )
            meson.add_install_script(
                find_program('launchctl'),
                'disable',
                'system/com.netatalk.daemon',
            )
        endif
        meson.add_install_script(
            find_program('launchctl'),
            'enable',
            'system/io.netatalk.daemon',
        )
        meson.add_install_script(
            find_program('launchctl'),
            'bootstrap',
            'system/',
            init_dir / 'io.netatalk.daemon.plist',
        )
    endif
endif

if 'netbsd' in init_style
    if init_dir_override == ''
        init_dir = '/etc/rc.d'
    endif
    init_dirs += init_dir
    service_data = ['netatalk']
    if have_appletalk
        service_data += [
            'a2boot',
            'atalkd',
            'macipgw',
            'papd',
            'timelord',
        ]
    endif
    foreach service : service_data
        configure_file(
            input: 'netbsd.' + service + '.in',
            output: service,
            configuration: cdata,
            install: true,
            install_dir: init_dir,
            install_mode: 'rwxr-xr-x',
        )
    endforeach
endif

if 'openbsd' in init_style
    if init_dir_override == ''
        init_dir = '/etc/rc.d'
    endif
    init_dirs += init_dir
    configure_file(
        input: 'openbsd.netatalk.in',
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

if 'openrc' in init_style
    if init_dir_override == ''
        init_dir = '/etc/init.d'
    endif
    init_dirs += init_dir
    service_data = ['netatalk']
    if have_appletalk
        service_data += [
            'a2boot',
            'atalkd',
            'macipgw',
            'papd',
            'timelord',
        ]
    endif
    foreach service : service_data
        configure_file(
            input: 'openrc.' + service + '.in',
            output: service,
            configuration: cdata,
            install: true,
            install_dir: init_dir,
            install_mode: 'rwxr-xr-x',
        )
    endforeach
    if get_option('with-init-hooks')
        foreach service : service_data
            meson.add_install_script(
                find_program('rc-update'),
                'add',
                service,
                'default',
            )
        endforeach
    endif
endif

if 'solaris' in init_style
    if init_dir_override == ''
        init_dir = '/lib/svc/manifest/network'
    endif
    init_dirs += init_dir
    configure_file(
        input: 'solaris.netatalk.xml.in',
        output: 'netatalk.xml',
        configuration: cdata,
        install: true,
        install_dir: init_dir,
    )
    if get_option('with-init-hooks')
        meson.add_install_script(
            find_program('svcadm'),
            'restart',
            'manifest-import',
        )
    endif
endif

if 'systemd' in init_style
    if init_dir_override == ''
        init_dir = '/usr/lib/systemd/system'
    endif
    init_dirs += init_dir
    service_data = ['netatalk']
    if have_appletalk
        service_data += [
            'a2boot',
            'atalkd',
            'macipgw',
            'papd',
            'timelord',
        ]
    endif
    foreach service : service_data
        configure_file(
            input: 'systemd.' + service + '.service.in',
            output: service + '.service',
            configuration: cdata,
            install: true,
            install_dir: init_dir,
        )
    endforeach
    if get_option('with-init-hooks')
        meson.add_install_script(find_program('systemctl'), 'daemon-reload')
    endif
endif
