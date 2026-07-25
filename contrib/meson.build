subdir('bin_utils')

if enable_fce
    subdir('scripts')
endif

if enable_appletalk
    subdir('a2boot')
    subdir('timelord')
    if host_os in ['freebsd', 'linux', 'netbsd', 'openbsd']
        subdir('macipgw')
    endif
endif

if build_webmin
    subdir('webmin_module')
endif
