util_sources = [
    'afp_util.c',
    'atalk_addr.c',
    'bprint.c',
    'constant_time.c',
    'cnid.c',
    'fault.c',
    'getiface.c',
    'gettok.c',
    'locking.c',
    'logger.c',
    'netatalk_conf.c',
    'pathconv.c',
    'queue.c',
    'server_child.c',
    'server_ipc.c',
    'server_lock.c',
    'sigpipe.c',
    'socket.c',
    'strdicasecmp.c',
    'unix.c',
]

libutil_deps = [bstring, iniparser]

# fault.c includes <libunwind.h> when HAVE_LIBUNWIND is set
if libunwind.found()
    libutil_deps += libunwind
endif

libutil = static_library(
    'util',
    util_sources,
    include_directories: root_includes,
    dependencies: libutil_deps,
    c_args: [
        confdir,
        messagedir,
        afpdpwfile,
        afpdsrppwfile,
        uamdir,
        statedir,
        dversion,
    ],
    install: false,
)
