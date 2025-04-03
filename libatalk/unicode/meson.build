subdir('charsets')

if get_option('with-unicode-data')
    unicode_dirs = [
        meson.current_source_dir(),
        '/usr/share/unicode',
        '/usr/share/unicode/ucd',
        '/usr/pkg/share/texmf-dist/tex/generic/unicode-data',
        '/usr/local/share/texmf-dist/tex/generic/unicode-data/',
    ]

    unicode_data_path = get_option('with-unicode-data-path')

    if unicode_data_path == ''
        foreach dir : unicode_dirs
            if fs.exists(dir / 'UnicodeData.txt')
                unicode_data_path = dir
                break
            endif
        endforeach
    elif not fs.is_absolute(unicode_data_path)
        unicode_data_path = meson.current_source_dir() / unicode_data_path
    endif

    unicode_data_file = unicode_data_path / 'UnicodeData.txt'

    if fs.exists(unicode_data_file) and perl.found()
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
    else
        error('Perl and Unicode data file required to generate case tables')
    endif
endif

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
