macipgw_deps = []

if have_iconv
    macipgw_deps += iconv
endif

macipgw_sources = [
	'atp_input.c',
	'macip.c',
	'main.c',
	'nbp_lkup_async.c',
	'tunnel_bsd.c',
	'tunnel_linux.c',
	'util.c',
]

macipgw_includes = include_directories('../../libatalk/atp')

macipgw_c_args = ['-DSERVERTEXT="' + pkgconfdir + '/msg"', dversion]

executable(
    'macipgw',
    macipgw_sources,
    include_directories: [root_includes, macipgw_includes],
    link_with: libatalk,
    dependencies: macipgw_deps,
    c_args: macipgw_c_args,
    export_dynamic: true,
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
