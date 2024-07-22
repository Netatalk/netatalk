subdir('charsets')

unicode_sources = [
    'charcnv.c',
    'iconv.c',
    'utf8.c',
    'utf16_case.c',
    'util_unistr.c',
]

if unicode_data_file != '' and perl.found()
    run_command(
        'make-casetable.pl',
        unicode_data_file,
        'utf16_casetable.h',
        'utf16_case.c',
        check: true,
    )
    run_command(
        'make-precompose.h.pl',
        unicode_data_file,
        'precompose.h',
        check: true,
    )
endif

libunicode = static_library(
    'unicode',
    unicode_sources,
    include_directories: root_includes,
    link_with: libcharsets,
    install: false,
)
