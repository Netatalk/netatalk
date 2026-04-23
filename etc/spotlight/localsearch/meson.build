run_command(
    flex,
    '-o', 'spotlight_rawquery_lexer.c',
    'spotlight_rawquery_lexer.l',
    check: true,
)
run_command(
    bison,
    '-d',
    '-o', 'sparql_parser.c',
    'sparql_parser.y',
    check: true,
)

localsearch_srp_sources = [
    'sparql_map.c',
    'sparql_parser.c',
    'spotlight_rawquery_lexer.c',
]

localsearch_deps = [
    bstring,
    glib,
    iniparser,
    talloc,
    sparql,
]

if have_iconv
    localsearch_deps += iconv
endif

if use_mysql_backend
    localsearch_deps += mysqlclient
endif

executable(
    'srp',
    localsearch_srp_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: localsearch_deps,
    c_args: ['-DMAIN'],
    install: false,
)

libspotlight_localsearch = static_library(
    'spotlight_localsearch',
    [
        'sl_localsearch.c',
        'sparql_map.c',
        'sparql_parser.c',
        'spotlight_rawquery_lexer.c',
    ],
    dependencies: [
        bstring,
        glib,
        iniparser,
        talloc,
        sparql,
    ],
    include_directories: root_includes,
    c_args: [
        '-DDBUS_API_SUBJECT_TO_CHANGE',
        '-DSEARCH_BACKEND_LOCALSEARCH=1',
        statedir,
    ],
    install: false,
)
