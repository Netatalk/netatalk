if get_option('with-tests')
    if sqlite_deps.found()
        subdir('afpd')
    else
        warning('Not building afpd tests: SQLite library required')
    endif
endif

if get_option('with-testsuite')
    subdir('testsuite')
endif
