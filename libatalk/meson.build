libatalk_sources = []
libatalk_libs = []

if have_embedded_ssl
    subdir('ssl')
    libatalk_libs += libssl
endif

subdir('acl')
subdir('adouble')
subdir('bstring')
subdir('cnid')
subdir('dsi')
subdir('iniparser')
subdir('util')
subdir('unicode')
subdir('vfs')

if host_os != 'darwin'
    subdir('compat')
    libatalk_libs += libcompat
endif

if have_spotlight
    subdir('talloc')
    libatalk_libs += libtalloc
endif

libatalk = both_libraries(
    'atalk',
    libatalk_sources,
    include_directories: root_includes,
    dependencies: [acl_deps, libcnid_deps, threads],
    link_whole: [
        libacl,
        libadouble,
        libatalk_libs,
        libbstring,
        libcnid,
        libdsi,
        libiniparser,
        libunicode,
        libutil,
        libvfs,
    ],
    soversion: 18,
    install: true,
)
