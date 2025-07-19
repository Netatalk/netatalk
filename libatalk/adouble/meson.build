adouble_sources = [
    'ad_attr.c',
    'ad_conv.c',
    'ad_date.c',
    'ad_flush.c',
    'ad_lock.c',
    'ad_lock.h',
    'ad_open.c',
    'ad_read.c',
    'ad_recvfile.c',
    'ad_sendfile.c',
    'ad_size.c',
    'ad_write.c',
]

libadouble = static_library(
    'adouble',
    adouble_sources,
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    install: false,
)
