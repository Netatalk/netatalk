dalloc_sources = ['dalloc.c']

libdalloc = static_library(
    'dalloc',
    dalloc_sources,
    dependencies: [bstring, talloc],
    include_directories: root_includes,
    install: false,
)
