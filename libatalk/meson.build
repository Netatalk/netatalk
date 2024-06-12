subdir('acl')
subdir('adouble')
subdir('bstring')
subdir('cnid')
subdir('compat')
subdir('dsi')
subdir('util')
subdir('unicode')
subdir('vfs')

libatalk_deps = []
libatalk_libs = []
libatalk_sources = []

if have_acls
    libatalk_deps += acl_deps
endif

if threads.found()
    libatalk_deps += threads
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

if have_embedded_ssl
    subdir('cast')
    subdir('ssl')
    libatalk_libs += [libcast, libssl]
endif

libatalk = both_libraries(
    'atalk',
    libatalk_sources,
    include_directories: root_includes,
    dependencies: [libatalk_deps, libcnid_deps],
    link_whole: [
        libacl,
        libadouble,
        libatalk_libs,
        libbstring,
        libcnid,
        libcompat,
        libdsi,
        libunicode,
        libutil,
        libvfs,
    ],
    soversion: 18,
    install: true,
)
