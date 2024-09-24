subdir('macusers')
subdir('shell_utils')

if have_appletalk
    subdir('a2boot')
    subdir('timelord')
    if host_os in ['freebsd', 'linux', 'netbsd', 'openbsd']
        subdir('macipgw')
    endif
endif

if have_webmin
    subdir('webmin_module')
endif
