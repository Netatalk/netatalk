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

executable(
    'ad',
    ad_sources,
    include_directories: root_includes,
    dependencies: mysqlclient,
    link_with: libatalk,
    c_args: [ad, dversion],
    install: true,
)
