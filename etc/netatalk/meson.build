netatalk_sources = ['afp_avahi.c', 'afp_mdns.c', 'afp_zeroconf.c', 'netatalk.c']

executable(
    'netatalk',
    netatalk_sources,
    include_directories: root_includes,
    link_with: libatalk,
    dependencies: [mysqlclient, dns_sd, avahi, libevent],
    c_args: [confdir, statedir, afpd, cnid_metad, dversion],
    install: true,
    install_dir: sbindir,
    install_rpath: rpath,
)
