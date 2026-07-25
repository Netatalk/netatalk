if build_afpd_tests
    if use_sqlite_backend
        subdir('afpd')
    else
        error(
            'The afpd tests require the "sqlite" backend to be built',
        )
    endif
endif

if build_testsuite
    subdir('testsuite')
endif

if not build_afpd_tests and build_fuzzing
    if use_sqlite_backend
        subdir('afpd')
    endif
endif
