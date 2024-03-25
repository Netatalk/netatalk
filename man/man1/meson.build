man1_files = [
    'ad.1',
    'afpldaptest.1',
    'afppasswd.1',
    'afpstats.1',
    'apple_dump.1',
    'asip-status.1',
    'dbd.1',
    'macusers.1',
    'netatalk-config.1',
]
foreach file : man1_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man1')
endforeach
