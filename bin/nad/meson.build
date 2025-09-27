nad_sources = [
    'ftw.c',
    'nad_cp.c',
    'nad_find.c',
    'nad_ls.c',
    'nad_mv.c',
    'nad_rm.c',
    'nad_set.c',
    'nad_util.c',
    'nad.c',
]

nad_deps = [bstring, iniparser]

if have_iconv
    nad_deps += iconv
endif

if use_mysql_backend
    nad_deps += mysqlclient
endif

executable(
    'nad',
    nad_sources,
    include_directories: root_includes,
    dependencies: nad_deps,
    link_with: libatalk,
    c_args: [naddir, dversion],
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
