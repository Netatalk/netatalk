# The dbd scan compiled into a TAP executable with the afpd fault injector, so
# a test can fail one open() inside the scan and check the result
dbdtest = executable(
    'dbdtest',
    [
        'test_scanvol.c',
        '../afpd/faultinject.c',
        '../../bin/dbd/cmd_dbd_scanvol.c',
    ],
    include_directories: [root_includes, '../../bin/dbd', '../afpd'],
    link_with: libatalk,
    dependencies: dbd_deps,
    c_args: dversion,
    export_dynamic: true,
    install: false,
)

test(
    'dbd scan repair failures',
    dbdtest,
    protocol: 'tap',
    env: {'LD_PRELOAD': faultinject_preload.full_path()},
    depends: faultinject_preload,
)
