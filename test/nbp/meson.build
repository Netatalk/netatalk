nbp_parse_test = executable(
    'nbp_parse_test',
    'test_nbp_parse.c',
    include_directories: [root_includes, '../../libatalk/nbp'],
    link_with: libatalk,
    install: false,
)

test(
    'NBP tuple parsing',
    nbp_parse_test,
)
