test_sources = [
    'afpfunc_helpers.c',
    'subtests.c',
    'test.c',
    meson.project_source_root() / 'etc/afpd/afp_config.c',
    meson.project_source_root() / 'etc/afpd/afp_dsi.c',
    meson.project_source_root() / 'etc/afpd/afp_options.c',
    meson.project_source_root() / 'etc/afpd/afp_util.c',
    meson.project_source_root() / 'etc/afpd/afprun.c',
    meson.project_source_root() / 'etc/afpd/appl.c',
    meson.project_source_root() / 'etc/afpd/auth.c',
    meson.project_source_root() / 'etc/afpd/catsearch.c',
    meson.project_source_root() / 'etc/afpd/desktop.c',
    meson.project_source_root() / 'etc/afpd/dircache.c',
    meson.project_source_root() / 'etc/afpd/directory.c',
    meson.project_source_root() / 'etc/afpd/enumerate.c',
    meson.project_source_root() / 'etc/afpd/extattrs.c',
    meson.project_source_root() / 'etc/afpd/fce_api.c',
    meson.project_source_root() / 'etc/afpd/fce_util.c',
    meson.project_source_root() / 'etc/afpd/file.c',
    meson.project_source_root() / 'etc/afpd/filedir.c',
    meson.project_source_root() / 'etc/afpd/fork.c',
    meson.project_source_root() / 'etc/afpd/hash.c',
    meson.project_source_root() / 'etc/afpd/mangle.c',
    meson.project_source_root() / 'etc/afpd/messages.c',
    meson.project_source_root() / 'etc/afpd/ofork.c',
    meson.project_source_root() / 'etc/afpd/status.c',
    meson.project_source_root() / 'etc/afpd/switch.c',
    meson.project_source_root() / 'etc/afpd/uam.c',
    meson.project_source_root() / 'etc/afpd/unix.c',
    meson.project_source_root() / 'etc/afpd/volume.c',
]

test_c_args = []
test_external_deps = [bstring, iniparser, libgcrypt]
test_internal_deps = []
test_link_args = []
test_includes = [root_includes]

if have_spotlight
    test_sources += [
        meson.project_source_root() / 'etc/afpd/spotlight.c',
        meson.project_source_root() / 'etc/afpd/spotlight_marshalling.c',
    ]
    test_internal_deps += libspotlight
    test_external_deps += [
        glib,
        talloc,
        sparql,
    ]
endif

if have_acls
    test_sources += meson.project_source_root() / 'etc/afpd/acls.c'
    test_external_deps += acl_deps
endif

if have_quota
    test_sources += [
        meson.project_source_root() / 'etc/afpd/quota.c',
    ]
    if not have_libquota
        test_sources += [
            meson.project_source_root() / 'etc/afpd/nfsquota.c',
        ]
    endif
    test_external_deps += quota_deps
endif

if have_krb5_uam
    test_external_deps += gss
endif

if have_kerberos
    test_external_deps += kerberos
    test_c_args += kerberos_c_args
    test_includes += kerberos_includes
endif

if have_tcpwrap
    test_external_deps += wrap
endif

if use_mysql_backend
    test_external_deps += mysqlclient
endif

if threads.found()
    test_external_deps += threads
endif

# For dtrace probes we need to invoke dtrace on all input files, before
# linking the final executable.
#
# On meson there's currently no easy way to do this. So we build
# a static library with all the input objects, run our script to generate
# exports, and build the final executable using that static library.

afpdtestlib = static_library(
    'afpdtestlib',
    test_sources,
    include_directories: test_includes,
    link_with: [test_internal_deps, libatalk],
    dependencies: [
        test_external_deps,
        libcnid_deps,
    ],
    c_args: [
        '-DAPPLCNAME', confdir,
        dversion,
        test_c_args,
        messagedir,
        statedir,
        uamdir,
    ],
    link_args: test_link_args,
    install: false,
    build_by_default: false,
)

afpdtest_dtrace_input = []
test_objs = [afpdtestlib.extract_all_objects(recursive: false)]

# As of 2/2024:
# The afp_dtrace.o file is necessary for dtrace support on Solaris, and on recent
# versions of systemtap.  (Older systemtap releases just produce an empty
# file, but that's okay.)  However, macOS's dtrace doesn't use it and doesn't
# even recognize the -G option.  So, build afp_dtrace.o except on macOS.
# This might need adjustment as other platforms add dtrace support.
#
# On at least linux we don't actually need to pass in all the objects, but
# at least on FreeBSD and Solaris we have to.

if fs.exists(meson.current_build_dir() / 'afp_dtrace.o')
    run_command(
        'rm',
        '-f', meson.current_build_dir() / 'afp_dtrace.o',
        check: true,
    )
endif

if have_dtrace and host_os != 'darwin'
    afpdtest_dtrace_input += custom_target(
        'afp_dtrace.o',
        input: [
            meson.project_source_root() / 'include/atalk/afp_dtrace.d',
            afpdtestlib.extract_objects(
                meson.project_source_root() / 'etc/afpd/afp_dsi.c',
                meson.project_source_root() / 'etc/afpd/fork.c',
                meson.project_source_root() / 'etc/afpd/appl.c',
                meson.project_source_root() / 'etc/afpd/catsearch.c',
                meson.project_source_root() / 'etc/afpd/directory.c',
                meson.project_source_root() / 'etc/afpd/enumerate.c',
                meson.project_source_root() / 'etc/afpd/file.c',
                meson.project_source_root() / 'etc/afpd/filedir.c',
            ),
        ],
        output: 'afp_dtrace.o',
        command: [
            dtrace_command,
            '-C',
            '-G',
            '-o', '@OUTPUT@',
            '-s', '@INPUT@',
        ],
        install: false,
    )
endif

afpdtest = executable(
    'afpdtest',
    afpdtest_dtrace_input,
    objects: test_objs,
    include_directories: test_includes,
    link_with: [test_internal_deps, libatalk],
    dependencies: [
        test_external_deps,
        libcnid_deps,
    ],
    c_args: ['-DAPPLCNAME', dversion, messagedir, uamdir, confdir, statedir],
    link_args: test_link_args,
    export_dynamic: true,
    install: false,
)

test_sh = find_program('test.sh')

test(
    'afpd integration tests - setup',
    test_sh,
    is_parallel: false,
    priority: 1,
)
test(
    'afpd integration tests - run',
    afpdtest,
    is_parallel: false,
    priority: 0,
)
