afpd_sources = [
    'afp_asp.c',
    'afp_avahi.c',
    'afp_config.c',
    'afp_dsi.c',
    'afp_mdns.c',
    'afp_options.c',
    'afp_util.c',
    'afp_zeroconf.c',
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
    'gettok.c',
    'hash.c',
    'main.c',
    'mangle.c',
    'messages.c',
    'nfsquota.c',
    'ofork.c',
    'quota.c',
    'status.c',
    'switch.c',
    'uam.c',
    'unix.c',
    'volume.c',
]

afpd_external_deps = [libgcrypt]
afpd_internal_deps = [libcnid]
afpd_link_args = []

if have_acls
    afpd_sources += 'acls.c'
    afpd_external_deps += [acl_deps]
endif

if have_quota
    afpd_sources += [
        'nfsquota.c',
        'quota.c',
    ]
    if tirpc.found()
        afpd_external_deps += tirpc
    elif rpcsvc.found()
        afpd_external_deps += rpcsvc
        afpd_link_args += sunrpc_link_args
    endif
    if quota.found()
        afpd_external_deps += quota
        afpd_link_args += libquota_link_args
    endif
endif

if have_gssapi
    afpd_external_deps += gss
endif

if have_tcpwrap
    afpd_external_deps += wrap
endif

if threads.found()
    afpd_external_deps += threads
endif

if avahi.found()
    afpd_external_deps += avahi
endif

if have_dns
    afpd_external_deps += dns_sd
endif

executable(
    'afpd',
    afpd_sources,
    include_directories: root_includes,
    link_with: [afpd_internal_deps, libatalk],
    dependencies: afpd_external_deps,
    c_args: [
        '-DAPPLCNAME', acl_ldapconf,
        afpdconf,
        afpddefvol,
        afpdpwfile,
        afpdsigconf,
        afpdsysvol,
        afpduuidconf,
        dversion,
        messagedir,
        uamdir,
    ],
    link_args: afpd_link_args,
    export_dynamic: true,
    install: true,
    install_dir: sbindir,
    build_rpath: libdir,
    install_rpath: libdir,
)

executable(
    'fce',
    ['fce_api.c', 'fce_util.c'],
    include_directories: root_includes,
    link_with: libatalk,
    c_args: '-DFCE_TEST_MAIN',
    install: false,
)

executable(
    'hash',
    'hash.c',
    include_directories: root_includes,
    link_with: libatalk,
    c_args: '-DKAZLIB_TEST_MAIN',
    install: false,
)
