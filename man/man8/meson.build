man8_files = [
    'afpd.8',
    'cnid_dbd.8',
    'cnid_metad.8',
    'netatalk.8',
]
foreach file : man8_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man8')
endforeach
