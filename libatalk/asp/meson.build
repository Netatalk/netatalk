asp_sources = [
    'asp_attn.c',
    'asp_close.c',
    'asp_cmdreply.c',
    'asp_getreq.c',
    'asp_getsess.c',
    'asp_init.c',
    'asp_shutdown.c',
    'asp_tickle.c',
    'asp_write.c',
]

libasp = static_library(
    'asp',
    asp_sources,
    include_directories: root_includes,
    dependencies: [bstring],
    install: false,
)
