compat_sources = [
    'misc.c',
    'rquota_xdr.c',
    'strlcpy.c',
]

libcompat_deps = []

if have_quota
    if tirpc.found()
        libcompat_deps += tirpc
    elif rpcsvc.found()
        libcompat_deps += rpcsvc
    elif quota.found()
        libcompat_deps += quota
    endif
endif

libcompat = static_library(
    'compat',
    compat_sources,
    include_directories: root_includes,
    dependencies: libcompat_deps,
    link_args: quota_link_args,
    install: false,
)
