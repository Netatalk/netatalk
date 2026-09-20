adouble_convert_c_args = []
adouble_convert_env = {}
adouble_convert_depends = []

if host_os == 'linux'
    adouble_short_read_inject = shared_library(
        'adouble_short_read_inject',
        'short_read_inject.c',
        include_directories: [root_includes],
        dependencies: cc.find_library('dl', required: false),
        install: false,
    )
    adouble_convert_c_args += ['-DHAVE_ADOUBLE_SHORT_READ_INJECTOR']
    adouble_convert_env = {
        'LD_PRELOAD': adouble_short_read_inject.full_path(),
    }
    adouble_convert_depends = [adouble_short_read_inject]
endif

adouble_convert_test = executable(
    'adouble_convert_test',
    'test_convert.c',
    include_directories: [root_includes],
    link_with: libatalk,
    c_args: adouble_convert_c_args,
    export_dynamic: true,
    install: false,
)

test(
    'adouble osx conversion bounds',
    adouble_convert_test,
    env: adouble_convert_env,
    depends: adouble_convert_depends,
)
