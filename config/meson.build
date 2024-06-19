gen_templates = ['afpd.conf', 'AppleVolumes.default']
gen_files = []

foreach template : gen_templates
    gen_files += configure_file(
        input: template + '.tmpl',
        output: template,
        configuration: cdata,
    )
endforeach

foreach file : gen_templates
    if (
        not fs.exists(pkgconfdir / file)
        or (get_option('with-overwrite') and fs.exists(pkgconfdir / file))
    )
        install_data(gen_files, install_dir: pkgconfdir)
    else
        message('not overwriting', file)
    endif
endforeach

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
        not fs.exists(pkgconfdir / file)
        or (get_option('with-overwrite') and fs.exists(file))
    )
        install_data(file, install_dir: pkgconfdir)
    else
        message('not overwriting', file)
    endif
endforeach

if have_pam
    subdir('pam')
endif
