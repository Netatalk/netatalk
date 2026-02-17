charsets_sources = [
    'generic_cjk.c',
    'generic_mb.c',
    'mac_centraleurope.c',
    'mac_chinese_simp.c',
    'mac_chinese_trad.c',
    'mac_cyrillic.c',
    'mac_greek.c',
    'mac_hebrew.c',
    'mac_japanese.c',
    'mac_korean.c',
    'mac_roman.c',
    'mac_turkish.c',
]

libcharsets = static_library(
    'charsets',
    charsets_sources,
    include_directories: root_includes,
    dependencies: [iconv, libiconv],
    install: false,
)
