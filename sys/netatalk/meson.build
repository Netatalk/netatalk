netatalk_headers = [
    'aarp.h',
    'at_var.h',
    'ddp.h',
    'ddp_var.h',
    'endian.h',
    'phase2.h',
]

if host_os != 'linux'
    netatalk_headers += 'at.h'
endif

install_headers(netatalk_headers, subdir: 'netatalk')
