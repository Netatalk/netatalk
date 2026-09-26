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

nbplkup_test = executable(
    'nbplkup_test',
    ['test_nbplkup.c', '../../bin/nbp/nbplkup_output.c'],
    include_directories: [root_includes, '../../bin/nbp'],
    link_with: libatalk,
    install: false,
)

test(
    'nbplkup output handling',
    nbplkup_test,
)
