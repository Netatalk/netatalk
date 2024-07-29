libcnid_mysql_sources = ['cnid_mysql.c']

libcnid_mysql = static_library(
    'cnid_mysql',
    libcnid_mysql_sources,
    include_directories: root_includes,
    dependencies: mysql_deps,
    install: false,
)
