if build_tests
    subdir('afppasswd')

    if use_sqlite_backend
        subdir('afpd')
    else
        warning(
            'The afpd unit tests require the "sqlite" backend to be built',
        )
    endif
endif

if build_testsuite
    subdir('testsuite')
endif

if not build_tests and build_fuzzing
    if use_sqlite_backend
        subdir('afpd')
    endif
endif
