compat_sources = [
    'misc.c',
    'rquota_xdr.c',
    'strlcpy.c',
]

libcompat_deps = []
libcompat_link_args = []

if have_quota
    if tirpc.found()
        libcompat_deps += tirpc
    elif rpcsvc.found()
        libcompat_deps += rpcsvc
        libcompat_link_args += sunrpc_link_args
    endif
    if quota.found()
        libcompat_deps += quota
        libcompat_link_args += libquota_link_args
    endif
endif

libcompat = static_library(
    'compat',
    compat_sources,
    include_directories: root_includes,
    dependencies: libcompat_deps,
    link_args: libcompat_link_args,
    install: false,
)
