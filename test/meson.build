if build_tests or build_fuzzing
    if use_sqlite_backend
        subdir('afpd')
    else
        warning(
            'afpd tests require the sqlite CNID backend',
        )
    endif
endif

if build_tests
    subdir('afppasswd')
    subdir('libatalk')
    subdir('uams')

    if use_sqlite_backend
        subdir('nad')
    else
        warning(
            'nad tests require the sqlite CNID backend',
        )
    endif

    if enable_appletalk
        subdir('nbp')
    endif
endif

if build_testsuite
    subdir('testsuite')
endif
