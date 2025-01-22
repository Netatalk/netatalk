subdir('dbd')
subdir('last')

libcnid_deps = [iniparser]
libcnid_libs = [libcnid_dbd]

if use_last_backend
    libcnid_libs += libcnid_last
endif

if use_mysql_backend
    subdir('mysql')
    libcnid_libs += libcnid_mysql
    libcnid_deps += mysql_deps
endif

if use_sqlite_backend
    subdir('sqlite')
    libcnid_libs += libcnid_sqlite
    libcnid_deps += sqlite_deps
endif

cnid_sources = ['cnid_init.c', 'cnid.c']

libcnid = static_library(
    'cnid',
    cnid_sources,
    include_directories: root_includes,
    dependencies: libcnid_deps,
    link_whole: libcnid_libs,
    install: false,
)
