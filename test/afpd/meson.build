test_internal_deps = []
test_external_deps = []

test_sources = [
    'afpfunc_helpers.c',
    'subtests.c',
    'test.c',
    meson.project_source_root() / 'etc/afpd/afp_asp.c',
    meson.project_source_root() / 'etc/afpd/afp_avahi.c',
    meson.project_source_root() / 'etc/afpd/afp_config.c',
    meson.project_source_root() / 'etc/afpd/afp_dsi.c',
    meson.project_source_root() / 'etc/afpd/afp_mdns.c',
    meson.project_source_root() / 'etc/afpd/afp_options.c',
    meson.project_source_root() / 'etc/afpd/afp_util.c',
    meson.project_source_root() / 'etc/afpd/afp_zeroconf.c',
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
    meson.project_source_root() / 'etc/afpd/gettok.c',
    meson.project_source_root() / 'etc/afpd/hash.c',
    meson.project_source_root() / 'etc/afpd/mangle.c',
    meson.project_source_root() / 'etc/afpd/messages.c',
    meson.project_source_root() / 'etc/afpd/nfsquota.c',
    meson.project_source_root() / 'etc/afpd/ofork.c',
    meson.project_source_root() / 'etc/afpd/quota.c',
    meson.project_source_root() / 'etc/afpd/status.c',
    meson.project_source_root() / 'etc/afpd/switch.c',
    meson.project_source_root() / 'etc/afpd/uam.c',
    meson.project_source_root() / 'etc/afpd/unix.c',
    meson.project_source_root() / 'etc/afpd/volume.c',
]

if have_acls
    test_sources += meson.project_source_root() / 'etc/afpd/acls.c'
    test_external_deps += [acl_deps]
endif

afpdtest = executable(
    'afpdtest',
    test_sources,
    include_directories: root_includes,
    link_with: [test_internal_deps, libatalk],
    dependencies: [
        test_external_deps,
        avahi,
        libgcrypt,
        dns_sd,
        tirpc,
        wrap,
    ],
    c_args: [
        '-DAPPLCNAME',
        acl_ldapconf,
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
    link_args: quota_link_args,
    export_dynamic: true,
    install: false,
)

test_sh = find_program('test.sh')

test('test1', test_sh)
test('test2', afpdtest)
