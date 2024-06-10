ad_sources = [
    'ad_cp.c',
    'ad_find.c',
    'ad_ls.c',
    'ad_mv.c',
    'ad_rm.c',
    'ad_util.c',
    'ad.c',
]

ad_deps = []

if have_acls
    ad_deps += acl_deps
endif

executable(
    'ad',
    ad_sources,
    include_directories: root_includes,
    dependencies: ad_deps,
    link_with: [libatalk, libcnid],
    c_args: [ad, dversion],
    install: true,
    build_rpath: libdir,
    install_rpath: libdir,
)
