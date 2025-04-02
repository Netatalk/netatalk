headers = [
    'acl.h',
    'adouble.h',
    'aep.h',
    'afp.h',
    'asp.h',
    'atp.h',
    'bstrlib.h',
    'cnid.h',
    'ddp.h',
    'ea.h',
    'globals.h',
    'hash.h',
    'list.h',
    'logger.h',
    'netatalk_conf.h',
    'nbp.h',
    'netddp.h',
    'pap.h',
    'queue.h',
    'rtmp.h',
    'uam.h',
    'unicode.h',
    'unix.h',
    'util.h',
    'vfs.h',
    'volume.h',
    'zip.h',
]

if have_dtrace
    run_command(
        dtrace_command,
        '-o', 'afp_dtrace.h',
        '-h',
        '-s', 'afp_dtrace.d',
        check: true,
    )
endif

install_headers(headers, subdir: 'atalk')
