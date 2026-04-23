libspotlight_cnid = static_library(
    'spotlight_cnid',
    ['sl_cnid.c'],
    dependencies: [
        bstring,
        iniparser,
        talloc,
    ],
    include_directories: root_includes,
    c_args: [
        '-DSEARCH_BACKEND_CNID=1',
    ],
    install: false,
)
