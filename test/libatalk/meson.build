cnid_registry_test = executable(
    'cnid_registry_test',
    'test_cnid_registry.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test('CNID backend registry', cnid_registry_test)

socket_helpers_test = executable(
    'socket_helpers_test',
    'test_socket_helpers.c',
    'high_fd.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk socket helpers',
    socket_helpers_test,
    protocol: 'tap',
)

sigpipe_test = executable(
    'sigpipe_test',
    'test_sigpipe.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk signal pipe',
    sigpipe_test,
    protocol: 'tap',
)

getiface_test = executable(
    'getiface_test',
    'test_getiface.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk interface list',
    getiface_test,
    protocol: 'tap',
)

shell_quote_test = executable(
    'shell_quote_test',
    'test_shell_quote.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk shell quoting',
    shell_quote_test,
    protocol: 'tap',
)

shell_hooks_test = executable(
    'shell_hooks_test',
    'test_shell_hooks.c',
    '../../etc/afpd/afprun.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test('libatalk shell hook interpolation', shell_hooks_test, protocol: 'tap')

count_open_fds_test = executable(
    'count_open_fds_test',
    'test_count_open_fds.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk count_open_fds',
    count_open_fds_test,
    protocol: 'tap',
)

logger_test = executable(
    'logger_test',
    'test_logger.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk logger filtering and routing',
    logger_test,
    protocol: 'tap',
)

logger_failure_test = executable(
    'logger_failure_test',
    'test_logger_failure.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk logger failed write',
    logger_failure_test,
    protocol: 'tap',
)

dsi_getsess_test = executable(
    'dsi_getsess_test',
    'test_dsi_getsess.c',
    'high_fd.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk dsi session start',
    dsi_getsess_test,
    protocol: 'tap',
)

dsi_stream_test = executable(
    'dsi_stream_test',
    'test_dsi_stream.c',
    'high_fd.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk dsi reply waits',
    dsi_stream_test,
    protocol: 'tap',
)

byteorder_test = executable(
    'byteorder_test',
    'test_byteorder.c',
    include_directories: root_includes,
    install: false,
)

test(
    'libatalk byteorder',
    byteorder_test,
)

hton64_test = executable(
    'hton64_test',
    'test_hton64.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    c_args: cc.has_multi_arguments(
        '-fsanitize=shift',
        '-fsanitize-undefined-trap-on-error',
    ) ? ['-fsanitize=shift', '-fsanitize-undefined-trap-on-error'] : [],
    install: false,
)

test(
    'libatalk hton64',
    hton64_test,
)

uuid_format_test = executable(
    'uuid_format_test',
    'test_uuid_format.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test('libatalk UUID formatting bounds', uuid_format_test)

unicode_test = executable(
    'unicode_test',
    'test_unicode.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk Unicode and filename conversion',
    unicode_test,
    protocol: 'tap',
)

sl_names_test = executable(
    'sl_names_test',
    'test_sl_names.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk Spotlight name comparisons',
    sl_names_test,
    protocol: 'tap',
)

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
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    c_args: adouble_convert_c_args,
    export_dynamic: true,
    install: false,
)

test(
    'libatalk adouble osx conversion bounds',
    adouble_convert_test,
    env: adouble_convert_env,
    depends: adouble_convert_depends,
)

sys_ftruncate_test = executable(
    'sys_ftruncate_test',
    'test_sys_ftruncate.c',
    include_directories: root_includes,
    dependencies: [bstring, iniparser],
    link_with: libatalk,
    install: false,
)

test(
    'libatalk ftruncate fallback',
    sys_ftruncate_test,
    protocol: 'tap',
)

if enable_appletalk
    appletalk_waits_test = executable(
        'appletalk_waits_test',
        'test_appletalk_waits.c',
        include_directories: root_includes,
        dependencies: [bstring, iniparser],
        link_with: libatalk,
        install: false,
    )

    test(
        'libatalk AppleTalk waits',
        appletalk_waits_test,
        protocol: 'tap',
    )
endif
