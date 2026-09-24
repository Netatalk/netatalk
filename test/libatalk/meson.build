socket_helpers_test = executable(
    'socket_helpers_test',
    'test_socket_helpers.c',
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
