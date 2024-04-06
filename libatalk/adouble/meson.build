adouble_sources = [
    'ad_attr.c',
    'ad_date.c',
    'ad_flush.c',
    'ad_lock.c',
    'ad_mmap.c',
    'ad_open.c',
    'ad_read.c',
    'ad_sendfile.c',
    'ad_size.c',
    'ad_write.c',
]

libadouble = static_library(
    'adouble',
    adouble_sources,
    include_directories: root_includes,
    install: false,
)
