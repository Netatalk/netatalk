libcnid_sqlite = static_library(
    'cnid_sqlite',
    ['cnid_sqlite.c'],
    include_directories: root_includes,
    dependencies: [bstring, iniparser, sqlite_deps],
    c_args: statedir,
    install: false,
)

libcnid_deps = [bstring, iniparser, sqlite_deps]
libcnid_libs = [libcnid_sqlite]

if use_mysql_backend
    libcnid_mysql = static_library(
        'cnid_mysql',
        ['cnid_mysql.c'],
        include_directories: root_includes,
        dependencies: [bstring, iniparser, mysql_deps],
        c_args: statedir,
        install: false,
    )

    libcnid_libs += libcnid_mysql
    libcnid_deps += mysql_deps
endif

libcnid = static_library(
    'cnid',
    ['cnid_init.c', 'cnid.c'],
    include_directories: root_includes,
    dependencies: libcnid_deps,
    link_whole: libcnid_libs,
    install: false,
)
