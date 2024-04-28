man1_files = [
    'ad.1',
    'aecho.1',
    'afpldaptest.1',
    'afppasswd.1',
    'cnid2_create.1',
    'dbd.1',
    'getzones.1',
    'megatron.1',
    'nbp.1',
    'netatalk-config.1',
    'pap.1',
    'psorder.1',
    'showppd.1',
    'uniconv.1',
]

if perl.found() and grep.found()
    man1_files += [
        'apple_dump.1',
        'asip-status.1',
        'macusers.1',
    ]
endif

foreach file : man1_files
    install_data(file + '.in', rename: file, install_dir: mandir / 'man1')
endforeach
