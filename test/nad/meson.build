perl = find_program('perl', required: false)

if perl.found()
    test(
        'nad CLI',
        perl,
        args: [files('test_nad.t'), nad.full_path()],
        depends: nad,
        protocol: 'tap',
        suite: 'nad',
        timeout: 120,
    )

    test(
        'nad MacBinary and BinHex',
        perl,
        args: [files('test_nad_archive.t'), nad.full_path()],
        depends: nad,
        protocol: 'tap',
        suite: 'nad',
        timeout: 120,
    )

    if enable_stuffit
        test(
            'nad StuffIt',
            perl,
            args: [files('test_nad_stuffit.t'), nad.full_path()],
            depends: nad,
            protocol: 'tap',
            suite: 'nad',
            timeout: 120,
        )
    endif
else
    warning('The nad CLI tests require Perl')
endif
