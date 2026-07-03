afpd_test_sources = files(
    'afpfunc_helpers.c',
    'subtests.c',
) + afpd_common_sources

test_c_args = [] + afpd_common_c_args
test_external_deps = [] + afpd_common_external_deps
test_internal_deps = [] + afpd_common_internal_deps
test_link_args = [] + afpd_common_link_args
test_includes = [] + afpd_common_includes

if get_option('with-tests')

    # For dtrace probes we need to invoke dtrace on all input files, before
    # linking the final executable.
    #
    # On meson there's currently no easy way to do this. So we build
    # a static library with all the input objects, run our script to generate
    # exports, and build the final executable using that static library.

    afpdtestlib = static_library(
        'afpdtestlib',
        afpd_test_sources + files('test.c'),
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
    afpdtest_objects = []
    afpdtest_link_whole = [afpdtestlib]

    # As of 2/2024:
    # The afp_dtrace.o file is necessary for dtrace support on Solaris, and on recent
    # versions of systemtap.  (Older systemtap releases just produce an empty
    # file, but that's okay.)  However, macOS's dtrace doesn't use it and doesn't
    # even recognize the -G option.  So, build afp_dtrace.o except on macOS.
    # This might need adjustment as other platforms add dtrace support.
    #
    # On at least linux we don't actually need to pass in all the objects, but
    # at least on FreeBSD and Solaris we have to.

    if have_dtrace and host_os != 'darwin'
        afpdtest_objects = [afpdtestlib.extract_all_objects(recursive: false)]
        afpdtest_link_whole = []
    endif

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
                afpdtestlib.extract_objects(afpd_dtrace_sources),
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
        # The fault-injection control block (fi) and the unit tests that use it
        # live in the executable; export_dynamic below lets the LD_PRELOAD
        # interposer (libfaultinject.so) resolve `fi` back into this binary.
        'faultinject.c',
        'test_capabilities.c',
        'subtests_lock.c',
        include_directories: test_includes,
        objects: afpdtest_objects,
        link_whole: afpdtest_link_whole,
        link_with: [test_internal_deps, libatalk],
        dependencies: [
            test_external_deps,
            libcnid_deps,
        ],
        c_args: ['-DAPPLCNAME', dversion, messagedir, uamdir, confdir, statedir],
        link_args: test_link_args,
        link_language: use_spotlight_xapian ? 'cpp' : 'c',
        export_dynamic: true,
        install: false,
    )

    # LD_PRELOAD interposer: replaces open/close/fcntl/fchdir at dynamic-link
    # resolution, so calls made INSIDE libatalk.so (ad_open/ad_close/…) are
    # intercepted — which a link-time --wrap on afpdtest cannot reach.  Reads the
    # `fi` control block exported by afpdtest; forwards to the real libc symbol
    # via dlsym(RTLD_NEXT) unless a test has armed a failure.  Under glibc's
    # _FILE_OFFSET_BITS=64 the single open() definition is renamed by <fcntl.h>
    # to open64() (the symbol LFS callers reference), so no separate wrapper is
    # needed — see faultinject.c.
    faultinject_preload = shared_library(
        'faultinject',
        'faultinject.c',
        include_directories: test_includes,
        c_args: ['-DFAULTINJECT_PRELOAD'],
        dependencies: cc.find_library('dl', required: false),
        install: false,
        build_by_default: true,
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
        args: ['--tap'],
        protocol: 'tap',
        env: {'LD_PRELOAD': faultinject_preload.full_path()},
        depends: faultinject_preload,
        verbose: true,
        is_parallel: false,
        priority: 0,
    )

endif

if get_option('with-fuzzing')
    fuzz_engine = get_option('with-fuzzing-engine')

    if fuzz_engine != ''
        fuzz_link_args = [fuzz_engine] + test_link_args
        fuzz_c_args = []
    else
        fuzz_link_args = ['-fsanitize=fuzzer,address'] + test_link_args
        fuzz_c_args = ['-fsanitize=fuzzer,address']
    endif

    executable(
        'afpd_fuzz',
        afpd_test_sources + files('fuzz.c'),
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
            fuzz_c_args,
        ],
        link_args: fuzz_link_args,
        link_language: use_spotlight_xapian ? 'cpp' : 'c',
        install: false,
        build_by_default: false,
    )
endif
