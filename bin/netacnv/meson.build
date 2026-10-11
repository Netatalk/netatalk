netacnv_deps = []
if iconv.found()
    netacnv_deps += iconv
elif libiconv.found()
    netacnv_deps += libiconv
endif

netacnv = executable(
    'netacnv',
    ['netacnv.c'],
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: netacnv_deps,
    install: true,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
