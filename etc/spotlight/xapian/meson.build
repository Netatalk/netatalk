libspotlight_xapian = static_library(
    'spotlight_xapian',
    [
        'sl_xapian.c',
        'xapian_wrapper.cc',
    ],
    dependencies: [
        bstring,
        iniparser,
        libmagic,
        talloc,
        xapian,
    ],
    include_directories: root_includes,
    c_args: [
        '-DSPOTLIGHT_BACKEND_XAPIAN=1',
        statedir,
    ],
    cpp_args: [
        '-DSPOTLIGHT_BACKEND_XAPIAN=1',
        statedir,
    ],
    override_options: ['cpp_std=c++17'],
    install: false,
)
