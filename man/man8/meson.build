man8_files = [
    'a2boot.8',
    'afpd.8',
    'atalkd.8',
    'cnid_dbd.8',
    'cnid_metad.8',
    'papd.8',
    'papstatus.8',
    'psf.8',
    'timelord.8',
]
foreach file : man8_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man8')
endforeach
