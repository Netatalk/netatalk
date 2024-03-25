libatalk_sources = []
libatalk_libs = []

subdir('acl')
subdir('adouble')
subdir('bstring')
subdir('cnid')
subdir('dsi')
subdir('util')
subdir('unicode')
subdir('vfs')

if host_os != 'darwin'
    subdir('compat')
    libatalk_libs += libcompat
endif

if get_option('enable-ddp')
    subdir('asp')
    subdir('atp')
    subdir('nbp')
    subdir('netddp')
    libatalk_libs += [
        libasp,
        libatp,
        libnbp,
        libnetddp,
    ]
endif

libatalk = both_libraries(
    'atalk',
    libatalk_sources,
    include_directories: root_includes,
    dependencies: [acl_deps, libcnid_deps],
    link_whole: [
        libacl,
        libadouble,
        libatalk_libs,
        libbstring,
        libcnid,
        libdsi,
        libunicode,
        libutil,
        libvfs,
    ],
    soversion: 18,
    install: true,
)
