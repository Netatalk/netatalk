if perl.found()
    test(
        'netacnv CLI',
        perl,
        args: [files('test_netacnv.t'), netacnv.full_path()],
        depends: netacnv,
        protocol: 'tap',
        suite: 'perl',
    )
endif
