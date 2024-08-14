subdir('acl')
subdir('adouble')
subdir('bstring')
subdir('cnid')
subdir('compat')
subdir('dsi')
subdir('util')
subdir('unicode')
subdir('vfs')

libatalk_deps = []
libatalk_libs = []
libatalk_sources = []

if have_acls
    libatalk_deps += acl_deps
endif

if have_threads
    libatalk_deps += threads
endif

if have_ddp
    subdir('asp')
    subdir('atp')
    subdir('nbp')
    subdir('netddp')
    libatalk_libs += [
        libasp,
        libatp,
        libnbp,
        libnetddp,
    ]
endif

if have_embedded_ssl
    subdir('ssl')
    libatalk_libs += libssl
endif

libatalk = library(
    'atalk',
    libatalk_sources,
    include_directories: root_includes,
    dependencies: [libatalk_deps, libcnid_deps],
    link_whole: [
        libacl,
        libadouble,
        libatalk_libs,
        libbstring,
        libcnid,
        libcompat,
        libdsi,
        libunicode,
        libutil,
        libvfs,
    ],
    version: '0.0.0',
    soversion: '0',
    install: true,
)

if host_os == 'linux' and get_option('with-install-hooks')
    ldconfig = find_program('ldconfig', required: false)
    if ldconfig.found()
        if run_command(ldconfig, '-N', '-X', check: false).returncode() == 0
            meson.add_install_script(ldconfig, '-v', skip_if_destdir: true)
        else
            warning('You may have to run ldconfig manually for netatalk to find the libatalk shared library.')
        endif
    endif
 endif
