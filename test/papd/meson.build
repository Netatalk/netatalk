papd_lp_bounds_test = executable(
    'papd_lp_bounds_test',
    'test_lp_bounds.c',
    '../../etc/papd/file.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    c_args: ['-D_PATH_DEVPRINTER="/dev/printer"'],
    install: false,
)

test('papd authentication path and hostname bounds', papd_lp_bounds_test)

if have_cups
    cups_name_test = executable(
        'cups_name_test',
        'test_cups_name.c',
        include_directories: root_includes,
        dependencies: [bstring, iniparser, cups],
        link_with: libatalk,
        c_args: ['-DSPOOLDIR="/tmp"'],
        install: false,
    )

    test('papd CUPS printer name bounds', cups_name_test)
endif
