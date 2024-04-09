ad_sources = [
    'ad_cp.c',
    'ad_find.c',
    'ad_ls.c',
    'ad_mv.c',
    'ad_rm.c',
    'ad_util.c',
    'ad.c',
]

executable(
    'ad',
    ad_sources,
    include_directories: root_includes,
    dependencies: acl_deps,
    link_with: [libatalk, libcnid],
    c_args: [ad, dversion],
    install: true,
    install_rpath: rpath,
)
