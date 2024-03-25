man5_files = [
    'afp_ldap.conf.5',
    'afp_signature.conf.5',
    'afp_voluuid.conf.5',
    'afpd.conf.5',
    'AppleVolumes.default.5',
    'atalkd.conf.5',
    'netatalk.conf.5',
    'papd.conf.5',
]
foreach file : man5_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man5')
endforeach
