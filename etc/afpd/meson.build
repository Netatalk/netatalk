afpd_sources = [
    'afp_config.c',
    'afp_dsi.c',
    'afp_options.c',
    'afp_util.c',
    'afprun.c',
    'appl.c',
    'auth.c',
    'catsearch.c',
    'desktop.c',
    'dircache.c',
    'directory.c',
    'enumerate.c',
    'extattrs.c',
    'fce_api.c',
    'fce_util.c',
    'file.c',
    'filedir.c',
    'fork.c',
    'hash.c',
    'mangle.c',
    'messages.c',
    'ofork.c',
    'status.c',
    'switch.c',
    'uam.c',
    'unix.c',
    'volume.c',
    'main.c',
]

afpd_c_args = []
afpd_external_deps = [libgcrypt]
afpd_internal_deps = []
afpd_link_args = []

if have_appletalk
    afpd_sources += [
        'afp_asp.c',
    ]
endif

if have_spotlight
    afpd_sources += [
        'spotlight.c',
        'spotlight_marshalling.c',
    ]
    afpd_internal_deps += libspotlight
    afpd_external_deps += [
        glib,
        talloc,
        sparql,
    ]
endif

if have_afpstats
    afpd_sources += [
        'afpstats_obj.c',
        'afpstats.c',
    ]
    afpd_external_deps += [
        dbus,
        gio,
        glib,
        gobject,
    ]
endif

if have_acls
    afpd_sources += 'acls.c'
    afpd_external_deps += [acl_deps]
endif

if have_quota
    afpd_sources += [
        'nfsquota.c',
        'quota.c',
    ]
    afpd_external_deps += quota_deps
endif

if have_krb5_uam
    afpd_external_deps += gss
endif

if have_kerberos
    afpd_external_deps += kerberos
    afpd_c_args += kerberos_c_args
endif

if have_tcpwrap
    afpd_external_deps += wrap
endif

if use_mysql_backend
    afpd_external_deps += mysqlclient
endif

if threads.found()
    afpd_external_deps += threads
endif

# For dtrace probes we need to invoke dtrace on all input files, before
# linking the final executable.
#
# On meson there's currently no easy way to do this. So we build
# a static library with all the input objects, run our script to generate
# exports, and build the final executable using that static library.

libafpd = static_library(
    'afpd',
    afpd_sources,
    include_directories: root_includes,
    link_with: [afpd_internal_deps, libatalk],
    dependencies: afpd_external_deps,
    c_args: [
        '-DAPPLCNAME', afpd_c_args,
        confdir,
        dversion,
        messagedir,
        statedir,
        uamdir,
    ],
    link_args: netatalk_common_link_args,
    install: false,
    build_by_default: false,
)

afpd_dtrace_input = []
afpd_objs = [libafpd.extract_all_objects(recursive: false)]

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
    afpd_dtrace_input += custom_target(
        'afp_dtrace.o',
        input: [
            meson.project_source_root() / 'include/atalk/afp_dtrace.d',
            libafpd.extract_objects(
                'afp_dsi.c',
                'fork.c',
                'appl.c',
                'catsearch.c',
                'directory.c',
                'enumerate.c',
                'file.c',
                'filedir.c',
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
    afpd_external_deps += dtrace_deps
endif

executable(
    'afpd',
    afpd_dtrace_input,
    objects: afpd_objs,
    include_directories: root_includes,
    link_with: [afpd_internal_deps, libatalk],
    dependencies: afpd_external_deps,
    c_args: [
        '-DAPPLCNAME', afpd_c_args,
        confdir,
        dversion,
        messagedir,
        statedir,
        uamdir,
    ],
    link_args: netatalk_common_link_args,
    export_dynamic: true,
    install: true,
    install_dir: sbindir,
    build_rpath: rpath_libdir,
    install_rpath: rpath_libdir,
)
