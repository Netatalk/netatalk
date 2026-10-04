iptoa_test = executable(
    'iptoa_test',
    'test_iptoa.c',
    '../../contrib/macipgw/util.c',
    include_directories: root_includes,
    install: false,
)

test('macipgw IPv4 address formatting', iptoa_test)
