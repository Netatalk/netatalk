ad_sources = [
    'ad_cp.c',
    'ad_find.c',
    'ad_ls.c',
    'ad_mv.c',
    'ad_rm.c',
    'ad_set.c',
    'ad_util.c',
    'ad.c',
    'ad.h',
]

ad_deps = []

if use_mysql_backend
    ad_deps += mysqlclient
endif

if enable_rpath
    executable(
        'ad',
        ad_sources,
        include_directories: root_includes,
        dependencies: ad_deps,
        link_with: libatalk,
        c_args: [ad, dversion],
        install: true,
        build_rpath: libdir,
        install_rpath: libdir,
    )
else
    executable(
        'ad',
        ad_sources,
        include_directories: root_includes,
        dependencies: ad_deps,
        link_with: libatalk,
        c_args: [ad, dversion],
        install: true,
    )
endif
