man5_files = [
    'afp_signature.conf.5',
    'afp_voluuid.conf.5',
    'afp.conf.5',
    'extmap.conf.5',
]
foreach file : man5_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man5')
endforeach
