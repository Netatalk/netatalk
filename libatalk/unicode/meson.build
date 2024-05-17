subdir('charsets')

unicode_sources = [
    'charcnv.c',
    'iconv.c',
    'utf8.c',
    'utf16_case.c',
    'util_unistr.c',
]

libunicode = static_library(
    'unicode',
    unicode_sources,
    include_directories: root_includes,
    link_with: libcharsets,
    install: false,
)
