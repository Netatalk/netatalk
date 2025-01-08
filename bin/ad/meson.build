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

if have_iconv
    ad_deps += iconv
endif

if use_mysql_backend
    ad_deps += mysqlclient
endif

executable(
    'ad',
    ad_sources,
    include_directories: root_includes,
    dependencies: ad_deps,
    link_with: libatalk,
    c_args: [ad, dversion],
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
