perl = find_program('perl', required: false)

if perl.found()
    metadata_helper = executable(
        'nad_metadata_helper',
        'metadata_helpers.c',
        include_directories: root_includes,
        dependencies: [bstring, iniparser],
        link_with: libatalk,
        install: false,
    )

    test(
        'nad metadata errors',
        perl,
        args: [
            files('test_nad_metadata.t'),
            nad.full_path(),
            metadata_helper.full_path(),
        ],
        depends: [nad, metadata_helper],
        env: {'NAD_TEST_EA': 'sys'},
        protocol: 'tap',
        suite: 'nad',
        timeout: 120,
    )

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
