perl = find_program('perl', required: false)

if perl.found()
    foreach ea : ['sys', 'ad']
        suffix = ea == 'ad' ? ' AppleDouble' : ''
        test(
            'nad CLI' + suffix,
            perl,
            args: [files('test_nad.t'), nad.full_path()],
            depends: nad,
            env: {'NAD_TEST_EA': ea},
            protocol: 'tap',
            suite: 'nad',
            timeout: 120,
        )

        test(
            'nad MacBinary and BinHex' + suffix,
            perl,
            args: [files('test_nad_archive.t'), nad.full_path()],
            depends: nad,
            env: {'NAD_TEST_EA': ea},
            protocol: 'tap',
            suite: 'nad',
            timeout: 120,
        )

        if enable_stuffit
            test(
                'nad StuffIt' + suffix,
                perl,
                args: [files('test_nad_stuffit.t'), nad.full_path()],
                depends: nad,
                env: {'NAD_TEST_EA': ea},
                protocol: 'tap',
                suite: 'nad',
                timeout: 120,
            )
        endif
    endforeach
else
    warning('The nad CLI tests require Perl')
endif
