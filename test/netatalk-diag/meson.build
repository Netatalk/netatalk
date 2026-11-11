if perl.found()
    test(
        'netatalk-diag helper',
        perl,
        args: [
            files('test_netatalk_diag.t'),
            files('../../contrib/bin_utils/netatalk_diag.pl'),
        ],
        protocol: 'tap',
        suite: 'perl',
    )
endif
