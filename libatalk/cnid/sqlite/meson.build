libcnid_sqlite_sources = ['cnid_sqlite.c']

libcnid_sqlite = static_library(
    'cnid_sqlite',
    libcnid_sqlite_sources,
    include_directories: root_includes,
    dependencies: [bstring, iniparser, sqlite_deps],
    c_args: statedir,
    install: false,
)
