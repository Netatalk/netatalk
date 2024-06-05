netatalk_sources = ['afp_avahi.c', 'afp_mdns.c', 'afp_zeroconf.c', 'netatalk.c']
netatalk_deps = [libevent]

if use_mysql_backend
    netatalk_deps += mysqlclient
endif

if avahi.found()
    netatalk_deps += avahi
elif have_dns
    netatalk_deps += dns_sd
endif

executable(
    'netatalk',
    netatalk_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: netatalk_deps,
    c_args: [confdir, statedir, afpd, cnid_metad, dversion],
    install: true,
    install_dir: sbindir,
    build_rpath: libdir,
    install_rpath: libdir,
)
