util_sources = [
    'atalk_addr.c',
    'bprint.c',
    'cnid.c',
    'fault.c',
    'getiface.c',
    'locking.c',
    'logger.c',
    'module.c',
    'queue.c',
    'server_child.c',
    'server_ipc.c',
    'server_lock.c',
    'socket.c',
    'strcasestr.c',
    'strdicasecmp.c',
    'strlcpy.c',
    'unix.c',
    'volinfo.c',
]

if have_atfuncs
    util_sources += 'ftw.c'
endif

libutil = static_library(
    'util',
    util_sources,
    include_directories: root_includes,
    c_args: dversion,
    install: false,
)
