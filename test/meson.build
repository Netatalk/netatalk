if get_option('with-tests')
    if use_last_backend
        subdir('afpd')
    else
        warning(
            'The afpd tests require the "last" backend to be built. Skipping afpd tests.',
        )
    endif
endif

if get_option('with-testsuite')
    subdir('testsuite')
endif
