subdir('sqlite')

libcnid_deps = [bstring, iniparser, sqlite_deps]
libcnid_libs = [libcnid_sqlite]

if use_mysql_backend
    subdir('mysql')
    libcnid_libs += libcnid_mysql
    libcnid_deps += mysql_deps
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
