dsi_sources = [
    'dsi_attn.c',
    'dsi_close.c',
    'dsi_cmdreply.c',
    'dsi_getsess.c',
    'dsi_getstat.c',
    'dsi_init.c',
    'dsi_opensess.c',
    'dsi_read.c',
    'dsi_stream.c',
    'dsi_tcp.c',
    'dsi_tickle.c',
    'dsi_write.c',
]

libdsi = static_library(
    'dsi',
    dsi_sources,
    dependencies: wrap,
    include_directories: root_includes,
    install: false,
)
