if get_option('with-tests')
    if use_sqlite_backend
        subdir('afpd')
    else
        error(
            'The afpd tests require the "sqlite" backend to be built',
        )
    endif
endif

if get_option('with-testsuite')
    subdir('testsuite')
endif

if not get_option('with-tests') and get_option('with-fuzzing')
    if use_sqlite_backend
        subdir('afpd')
    endif
endif
