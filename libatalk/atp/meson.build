atp_sources = [
    'atp_bufs.c',
    'atp_close.c',
    'atp_open.c',
    'atp_packet.c',
    'atp_rreq.c',
    'atp_rresp.c',
    'atp_rsel.c',
    'atp_sreq.c',
    'atp_sresp.c',
]

libatp = static_library(
    'atp',
    atp_sources,
    include_directories: root_includes,
    dependencies: [bstring],
    install: false,
)
