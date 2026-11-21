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

nbp_lookup_test = executable(
    'nbp_lookup_test',
    'test_nbp_lookup.c',
    include_directories: [root_includes, '../../libatalk/nbp'],
    link_with: libatalk,
    install: false,
)

test(
    'NBP lookup timeouts',
    nbp_lookup_test,
)

nbplkup_output_test = executable(
    'nbplkup_output_test',
    ['test_nbplkup_output.c', '../../bin/nbp/nbplkup_output.c'],
    include_directories: [root_includes, '../../bin/nbp'],
    link_with: libatalk,
    install: false,
)

test(
    'nbplkup output handling',
    nbplkup_output_test,
)
