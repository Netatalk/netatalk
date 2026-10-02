if build_tests or build_fuzzing
    subdir('afpd')
endif

if build_tests
    subdir('afppasswd')
    subdir('libatalk')
    subdir('nad')
    subdir('uams')

    if enable_appletalk
        subdir('nbp')
    endif
endif

if build_testsuite
    subdir('testsuite')
endif
