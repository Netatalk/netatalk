compat_sources = [
    'misc.c',
    'rquota_xdr.c',
    'strlcpy.c',
]

libcompat = static_library(
    'compat',
    compat_sources,
    include_directories: root_includes,
    dependencies: tirpc,
    link_args: quota_link_args,
    install: false,
)
