compat_sources = [
    'inet_aton.c',
    'misc.c',
    'rquota_xdr.c',
]

libcompat_deps = []

if have_quota
    libcompat_deps += quota_deps
endif

libcompat = static_library(
    'compat',
    compat_sources,
    include_directories: root_includes,
    dependencies: libcompat_deps,
    install: false,
)
