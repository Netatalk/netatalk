util_sources = [
    'cnid.c',
    'fault.c',
    'ftw.c',
    'getiface.c',
    'gettok.c',
    'locking.c',
    'logger.c',
    'netatalk_conf.c',
    'queue.c',
    'server_child.c',
    'server_ipc.c',
    'server_lock.c',
    'socket.c',
    'strdicasecmp.c',
    'unix.c',
]

libutil = static_library(
    'util',
    util_sources,
    include_directories: root_includes,
    c_args: [
        confdir,
        messagedir,
        afpdpwfile,
        uamdir,
        statedir,
        dversion,
    ],
    install: false,
)
