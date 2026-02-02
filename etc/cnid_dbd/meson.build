cnid_dbd_deps = [bstring, iniparser]

cnid_dbd_sources = [
    'comm.c',
    'db_param.c',
    'dbd_add.c',
    'dbd_dbcheck.c',
    'dbd_delete.c',
    'dbd_get.c',
    'dbd_getstamp.c',
    'dbd_lookup.c',
    'dbd_rebuild_add.c',
    'dbd_resolve.c',
    'dbd_search.c',
    'dbd_update.c',
    'dbif.c',
    'main.c',
    'pack.c',
]

cnid_metad_sources = [
    'cnid_metad.c',
    'db_param.c',
    'usockfd.c',
]

executable(
    'cnid_dbd',
    cnid_dbd_sources,
    include_directories: [bdb_includes, root_includes],
    link_with: libatalk,
    dependencies: [libdb, cnid_dbd_deps],
    c_args: [cnid_dbd, dversion],
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
executable(
    'cnid_metad',
    cnid_metad_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: cnid_dbd_deps,
    c_args: [cnid_dbd, dversion],
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
