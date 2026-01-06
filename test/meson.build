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
