libcnid_sqlite_sources = ['cnid_sqlite.c']

libcnid_sqlite = static_library(
    'cnid_sqlite',
    libcnid_sqlite_sources,
    include_directories: root_includes,
    dependencies: [iniparser, sqlite_deps],
    c_args: statedir,
    install: false,
)

# FIXME: A more secure way to grant afpd access to the sqlite database
install_emptydir(localstatedir / 'netatalk/CNID/sqlite', install_mode: 'rwxrwxrwx')
