headers = [
    'acl.h',
    'adouble.h',
    'afp.h',
    'bstrlib.h',
    'cnid.h',
    'dictionary.h',
    'ea.h',
    'globals.h',
    'hash.h',
    'iniparser.h',
    'list.h',
    'logger.h',
    'netatalk_conf.h',
    'paths.h',
    'standards.h',
    'uam.h',
    'unicode.h',
    'unix.h',
    'util.h',
    'vfs.h',
    'volume.h',
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
