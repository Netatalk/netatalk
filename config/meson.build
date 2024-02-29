conf_templates = ['afp.conf', 'dbus-session.conf']
conf_files = []
foreach template : conf_templates
    conf_files += configure_file(
        input: template + '.tmpl',
        output: template,
        configuration: cdata,
    )
endforeach

installed_files = [pkgconfdir / 'afp.conf', pkgconfdir / 'dbus-session.conf']
foreach file : installed_files
    if (
        not fs.exists(file)
        or (get_option('enable-overwrite')
        and fs.exists(file))
    )
        install_data(conf_files, install_dir: pkgconfdir)
    else
        message('not overwriting', file)
    endif
endforeach

install_data('extmap.conf', install_dir: pkgconfdir)

install_data('netatalk-dbus.conf', install_dir: prefix / 'etc/dbus-1/system.d')

install_data('README', install_dir: localstatedir / 'netatalk')

install_data('README', install_dir: localstatedir / 'netatalk/CNID')

if have_pam
    subdir('pam')
endif
