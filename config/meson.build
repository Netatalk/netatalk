afpd_conf = configure_file(
    input: 'afpd.conf.tmpl',
    output: 'afpd.conf',
    configuration: cdata,
)

avolumesdefault_conf = configure_file(
    input: 'AppleVolumes.default.tmpl',
    output: 'AppleVolumes.default',
    configuration: cdata,
)

if (
    fs.exists(pkgconfdir / 'afpd.conf')
    and not get_option('with-overwrite')
)
    message('will not replace existing', pkgconfdir / 'afpd.conf')
else
    install_data(afpd_conf, install_dir: pkgconfdir)
endif

if (
    fs.exists(pkgconfdir / 'AppleVolumes.default')
    and not get_option('with-overwrite')
)
    message('will not replace existing', pkgconfdir / 'AppleVolumes.default')
else
    install_data(avolumesdefault_conf, install_dir: pkgconfdir)
endif

conf_files = []

if get_option('with-init-style') == 'debian-sysv'
    conf_files += ['AppleVolumes.system']
else
    conf_files += ['AppleVolumes.system', 'netatalk.conf']
endif

if have_ldap
    conf_files += 'afp_ldap.conf'
endif

if have_ddp
    conf_files += ['atalkd.conf', 'papd.conf']
endif

foreach file : conf_files
    if (
        fs.exists(pkgconfdir / file)
        and not get_option('with-overwrite')
    )
        message('will not replace existing', pkgconfdir / file)
    else
        install_data(file, install_dir: pkgconfdir)
    endif
endforeach

if have_pam
    subdir('pam')
endif
